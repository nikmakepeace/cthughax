#!/usr/bin/env python3
import argparse
import datetime as dt
import hashlib
import json
import math
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
from pathlib import Path


SCHEMA = "cthughanix.scene-script-benchmark.v1"
DEFAULT_SCENE_SCRIPT_DIR = "tests/fixtures/ini/perf"
DEFAULT_SCENE_SCRIPT = "perf.scenescript"
DEFAULT_AUDIO_FILE = "tests/fixtures/audio/sine-100-1600-doubling-10s.wav"
DEFAULT_BUFFER_SIZE = "1600x1200"
DEFAULT_TIMEOUT_SECONDS = 30.0
DEFAULT_SOURCE_DIR = Path(__file__).resolve().parents[1]


FILTER_RE = re.compile(
    r"frame filterchain: filter-ms=(?P<ms>[0-9.]+) "
    r"stage=(?P<stage>[0-9]+) filter=(?P<filter>\S+) .* mode=(?P<mode>-?[0-9]+)"
)
FILTER_RUN_RE = re.compile(r"frame filterchain: run-ms=(?P<ms>[0-9.]+)")
FRAME_TIMING_RE = re.compile(r"frame timing: (?P<body>.*)")
DISPLAY_TIMING_RE = re.compile(r"(?:sdl3|x11) (?P<body>frame-ms=.*)")
MAINLOOP_RE = re.compile(r"display timing: mainloop-ms=(?P<body>.*)")
SCENE_EVENT_RE = re.compile(
    r"scene script: step=(?P<step>[0-9]+) file=(?P<file>\S+) "
    r"elapsed-ms=(?P<elapsed>[0-9]+)"
)
SCENE_STOP_RE = re.compile(
    r"scene script: stop step=(?P<step>[0-9]+) elapsed-ms=(?P<elapsed>[0-9]+)"
)


def run(cmd, cwd=None, check=True):
    result = subprocess.run(
        cmd,
        cwd=cwd,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if check and result.returncode != 0:
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)
    return result


def git_value(args, cwd):
    result = run(["git"] + args, cwd=cwd, check=False)
    if result.returncode != 0:
        return ""
    return result.stdout.strip()


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def file_metadata(path, source_dir):
    try:
        display_path = str(path.relative_to(source_dir))
    except ValueError:
        display_path = str(path)
    return {
        "file": display_path,
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
    }


def cthugha_binary(build_dir):
    candidates = [
        build_dir / "src" / "cthugha",
        build_dir / "cthugha",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit(f"cthugha was not found under {build_dir}")


def cmake_cache_value(build_dir, key):
    cache = build_dir / "CMakeCache.txt"
    if not cache.exists():
        return ""
    prefix = key + ":"
    with cache.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith(prefix):
                return line.split("=", 1)[1].strip()
    return ""


def nearest_rank_percentile(values, percentile):
    if not values:
        return 0.0
    rank = int(math.ceil((percentile / 100.0) * len(values)))
    rank = min(max(rank, 1), len(values))
    return sorted(values)[rank - 1]


def spread(values):
    if not values:
        return {
            "samples": 0,
            "mean_ms": 0.0,
            "sd_ms": 0.0,
            "cv_percent": 0.0,
            "min_ms": 0.0,
            "max_ms": 0.0,
            "median_ms": 0.0,
            "p95_ms": 0.0,
            "p99_ms": 0.0,
        }
    mean = statistics.fmean(values)
    sd = statistics.stdev(values) if len(values) > 1 else 0.0
    return {
        "samples": len(values),
        "mean_ms": mean,
        "sd_ms": sd,
        "cv_percent": (sd / mean * 100.0) if mean else 0.0,
        "min_ms": min(values),
        "max_ms": max(values),
        "median_ms": statistics.median(values),
        "p95_ms": nearest_rank_percentile(values, 95.0),
        "p99_ms": nearest_rank_percentile(values, 99.0),
    }


def parse_key_value_body(body):
    values = {}
    for token in body.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        try:
            values[key] = float(value)
        except ValueError:
            values[key] = value
    return values


def append_timing_samples(grouped, current_scene, values, suffix="-ms"):
    scene = dict(current_scene)
    for key, value in values.items():
        if not key.endswith(suffix) or not isinstance(value, float):
            continue
        grouped.setdefault(key[:-len(suffix)], []).append(
            {
                "scene_step": scene.get("step"),
                "scene_file": scene.get("file"),
                "ms": value,
            }
        )


def summarize_timing_groups(groups):
    rows = []
    for name, samples in sorted(groups.items()):
        rows.append(
            {
                "name": name,
                "timing": spread([sample["ms"] for sample in samples]),
            }
        )
    return rows


def summarize_filter_samples(samples, keys):
    grouped = {}
    for sample in samples:
        group_key = tuple(sample.get(key) for key in keys)
        grouped.setdefault(group_key, []).append(sample["ms"])

    rows = []
    for group_key, values in grouped.items():
        row = {key: value for key, value in zip(keys, group_key)}
        row["timing"] = spread(values)
        rows.append(row)

    rows.sort(
        key=lambda row: (
            str(row.get("filter", "")),
            row.get("scene_step") if row.get("scene_step") is not None else -1,
            str(row.get("scene_file", "")),
            row.get("stage") if row.get("stage") is not None else -1,
        )
    )
    return rows


def parse_trace(text):
    current_scene = {"step": None, "file": ""}
    scene_events = []
    filter_samples = []
    frame_groups = {}
    display_groups = {}
    mainloop_groups = {}
    filter_run_samples = []

    for raw_line in text.splitlines():
        line = raw_line.strip().replace("\r", "")
        if not line:
            continue

        match = SCENE_EVENT_RE.search(line)
        if match:
            current_scene = {
                "step": int(match.group("step")),
                "file": match.group("file"),
                "elapsed_ms": int(match.group("elapsed")),
            }
            scene_events.append(dict(current_scene))
            continue

        match = SCENE_STOP_RE.search(line)
        if match:
            scene_events.append(
                {
                    "step": int(match.group("step")),
                    "file": "",
                    "elapsed_ms": int(match.group("elapsed")),
                    "stop": True,
                }
            )
            continue

        match = FILTER_RE.search(line)
        if match:
            filter_samples.append(
                {
                    "scene_step": current_scene.get("step"),
                    "scene_file": current_scene.get("file", ""),
                    "stage": int(match.group("stage")),
                    "filter": match.group("filter"),
                    "mode": int(match.group("mode")),
                    "ms": float(match.group("ms")),
                }
            )
            continue

        match = FILTER_RUN_RE.search(line)
        if match:
            filter_run_samples.append(
                {
                    "scene_step": current_scene.get("step"),
                    "scene_file": current_scene.get("file", ""),
                    "ms": float(match.group("ms")),
                }
            )
            continue

        match = FRAME_TIMING_RE.search(line)
        if match:
            append_timing_samples(
                frame_groups, current_scene, parse_key_value_body(match.group("body"))
            )
            continue

        match = DISPLAY_TIMING_RE.search(line)
        if match:
            append_timing_samples(
                display_groups, current_scene, parse_key_value_body(match.group("body"))
            )
            continue

        match = MAINLOOP_RE.search(line)
        if match:
            append_timing_samples(
                mainloop_groups,
                current_scene,
                parse_key_value_body("mainloop-ms=" + match.group("body")),
            )

    filter_run_groups = {"filterchain-run": filter_run_samples}
    return {
        "scene_events": scene_events,
        "filter_samples": filter_samples,
        "filter_by_name": summarize_filter_samples(filter_samples, ["filter"]),
        "filter_by_scene": summarize_filter_samples(
            filter_samples, ["scene_step", "scene_file", "stage", "filter"]
        ),
        "filterchain_run": summarize_timing_groups(filter_run_groups),
        "frame_timing": summarize_timing_groups(frame_groups),
        "display_timing": summarize_timing_groups(display_groups),
        "mainloop_timing": summarize_timing_groups(mainloop_groups),
        "sample_counts": {
            "filter_samples": len(filter_samples),
            "filterchain_runs": len(filter_run_samples),
            "scene_events": len(scene_events),
            "frame_timing_samples": sum(len(values) for values in frame_groups.values()),
            "display_timing_samples": sum(len(values) for values in display_groups.values()),
            "mainloop_timing_samples": sum(len(values) for values in mainloop_groups.values()),
        },
    }


def format_timing(timing):
    return (
        f"{timing['mean_ms']:.3f} | {timing['median_ms']:.3f} | "
        f"{timing['p95_ms']:.3f} | {timing['max_ms']:.3f} | "
        f"{timing['samples']}"
    )


def write_timing_table(f, title, rows):
    f.write(f"## {title}\n\n")
    if not rows:
        f.write("_No samples._\n\n")
        return

    f.write("| Name | Mean ms | Median ms | P95 ms | Max ms | Samples |\n")
    f.write("| --- | ---: | ---: | ---: | ---: | ---: |\n")
    for row in rows:
        f.write(f"| `{row['name']}` | {format_timing(row['timing'])} |\n")
    f.write("\n")


def write_filter_table(f, title, rows, include_scene):
    f.write(f"## {title}\n\n")
    if not rows:
        f.write("_No samples._\n\n")
        return

    if include_scene:
        f.write("| Step | File | Stage | Filter | Mean ms | Median ms | P95 ms | Max ms | Samples |\n")
        f.write("| ---: | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |\n")
        for row in rows:
            f.write(
                f"| {row.get('scene_step', '')} | `{row.get('scene_file', '')}` | "
                f"{row.get('stage', '')} | `{row.get('filter', '')}` | "
                f"{format_timing(row['timing'])} |\n"
            )
    else:
        f.write("| Filter | Mean ms | Median ms | P95 ms | Max ms | Samples |\n")
        f.write("| --- | ---: | ---: | ---: | ---: | ---: |\n")
        for row in rows:
            f.write(f"| `{row.get('filter', '')}` | {format_timing(row['timing'])} |\n")
    f.write("\n")


def write_summary(path, manifest, metrics):
    slowest = sorted(
        metrics["filter_by_scene"],
        key=lambda row: row["timing"]["mean_ms"],
        reverse=True,
    )[:12]

    with path.open("w", encoding="utf-8") as f:
        f.write("# Scene Script Benchmark Summary\n\n")
        f.write(f"- Schema: `{manifest['schema']}`\n")
        f.write(f"- Timestamp: `{manifest['timestamp_utc']}`\n")
        f.write(f"- Git: `{manifest['git_commit']}`")
        if manifest["git_dirty"]:
            f.write(" dirty")
        f.write("\n")
        f.write(f"- Build type: `{manifest['build_type']}`\n")
        f.write(f"- C++ compiler: `{manifest['cxx_compiler']}`\n")
        f.write(f"- Scene script: `{manifest['scene_script_dir']}/{manifest['scene_script']}`\n")
        f.write(f"- Audio file: `{manifest['audio_file']}`\n")
        f.write(f"- Buffer size: `{manifest['buffer_size']}`\n")
        f.write(f"- Autochange locked by runner: `{manifest['autochange_locked']}`\n")
        f.write(f"- Build dir: `{manifest['build_dir']}`\n")
        f.write(f"- Binary: `{manifest['binary']}`\n")
        f.write(f"- Trace: `{manifest['trace_log']}`\n\n")

        f.write("## Scene Events\n\n")
        f.write("| Step | File | Elapsed ms | Stop |\n")
        f.write("| ---: | --- | ---: | --- |\n")
        for event in metrics["scene_events"]:
            f.write(
                f"| {event.get('step', '')} | `{event.get('file', '')}` | "
                f"{event.get('elapsed_ms', '')} | "
                f"{'yes' if event.get('stop') else ''} |\n"
            )
        f.write("\n")

        write_filter_table(f, "Slowest Filters By Scene", slowest, True)
        write_filter_table(f, "Filters By Name", metrics["filter_by_name"], False)
        write_timing_table(f, "Frame Timing", metrics["frame_timing"])
        write_timing_table(f, "Display Timing", metrics["display_timing"])
        write_timing_table(f, "Main Loop Timing", metrics["mainloop_timing"])
        write_timing_table(f, "Filterchain Run Timing", metrics["filterchain_run"])

        f.write("Detailed metrics are available in `metrics.json`.\n")
        f.write("Raw combined stdout/stderr trace is available in `trace.log`.\n")


def parse_env_assignment(text):
    if "=" not in text:
        raise SystemExit(f"--env expects NAME=VALUE, got {text!r}")
    key, value = text.split("=", 1)
    if not key:
        raise SystemExit(f"--env expects NAME=VALUE, got {text!r}")
    return key, value


def build_command(args, binary, source_dir):
    scene_script_dir = Path(args.scene_script_dir)
    if not scene_script_dir.is_absolute():
        scene_script_dir = source_dir / scene_script_dir

    command = [
        str(binary),
        "--verbose=10",
        "--display-driver=sdl3",
        "--scene-script-dir",
        str(scene_script_dir),
        "--scene-script",
        args.scene_script,
        "--play",
        str(Path(args.audio_file).resolve()),
        "--silent",
        "--no-loop",
        "--max-fps=0",
    ]
    if args.lock_autochange:
        command.append("--lock")
    if args.buffer_size:
        command.append(f"--buff-size={args.buffer_size}")
    command.extend(args.app_arg)
    return command


def main():
    parser = argparse.ArgumentParser(
        description="Run and archive a deterministic Cthugha scene-script benchmark."
    )
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--source-dir", default=None)
    parser.add_argument("--out-dir", default=None)
    parser.add_argument("--scene-script-dir", default=DEFAULT_SCENE_SCRIPT_DIR)
    parser.add_argument("--scene-script", default=DEFAULT_SCENE_SCRIPT)
    parser.add_argument("--audio-file", default=DEFAULT_AUDIO_FILE)
    parser.add_argument("--buffer-size", default=DEFAULT_BUFFER_SIZE)
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT_SECONDS)
    parser.add_argument("--app-arg", action="append", default=[])
    parser.add_argument("--env", action="append", default=[])
    parser.add_argument("--allow-autochange", action="store_true")
    args = parser.parse_args()
    args.lock_autochange = not args.allow_autochange

    build_dir = Path(args.build_dir).resolve()
    source_dir = Path(args.source_dir).resolve() if args.source_dir else DEFAULT_SOURCE_DIR
    binary = cthugha_binary(build_dir)
    audio_file = Path(args.audio_file)
    if not audio_file.is_absolute():
        audio_file = source_dir / audio_file
    if not audio_file.exists():
        raise SystemExit(f"audio fixture was not found: {audio_file}")
    args.audio_file = str(audio_file)

    timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H%M%SZ")
    git_commit = git_value(["rev-parse", "--short=12", "HEAD"], source_dir) or "unknown"
    dirty = bool(git_value(["status", "--porcelain"], source_dir))
    run_id = f"{timestamp}-{git_commit}{'-dirty' if dirty else ''}"
    out_root = Path(args.out_dir).resolve() if args.out_dir else (
        build_dir / "test-results" / "scene-script"
    )
    out_dir = out_root / run_id
    out_dir.mkdir(parents=True, exist_ok=True)

    command = build_command(args, binary, source_dir)
    env = os.environ.copy()
    env.setdefault("LC_ALL", "C")
    for assignment in args.env:
        key, value = parse_env_assignment(assignment)
        env[key] = value

    trace_log = out_dir / "trace.log"
    try:
        result = subprocess.run(
            command,
            cwd=source_dir,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
            timeout=args.timeout,
            check=False,
        )
        trace_text = result.stdout + result.stderr
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout or ""
        stderr = exc.stderr or ""
        if isinstance(stdout, bytes):
            stdout = stdout.decode("utf-8", errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode("utf-8", errors="replace")
        trace_text = stdout + stderr
        trace_log.write_text(trace_text, encoding="utf-8")
        raise SystemExit(f"cthugha timed out after {args.timeout:.1f}s; trace: {trace_log}")

    trace_log.write_text(trace_text, encoding="utf-8")
    if result.returncode != 0:
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)

    metrics = parse_trace(trace_text)
    scene_script_dir = Path(args.scene_script_dir)
    if not scene_script_dir.is_absolute():
        scene_script_dir = source_dir / scene_script_dir
    script_path = scene_script_dir / args.scene_script
    fixture_files = []
    if script_path.exists():
        fixture_files.append(file_metadata(script_path, source_dir))
    for path in sorted(scene_script_dir.glob("*.ini")):
        fixture_files.append(file_metadata(path, source_dir))

    manifest = {
        "schema": SCHEMA,
        "timestamp_utc": timestamp,
        "git_commit": git_commit,
        "git_dirty": dirty,
        "build_type": cmake_cache_value(build_dir, "CMAKE_BUILD_TYPE"),
        "c_compiler": cmake_cache_value(build_dir, "CMAKE_C_COMPILER"),
        "cxx_compiler": cmake_cache_value(build_dir, "CMAKE_CXX_COMPILER"),
        "source_dir": str(source_dir),
        "build_dir": str(build_dir),
        "binary": str(binary),
        "scene_script_dir": str(scene_script_dir),
        "scene_script": args.scene_script,
        "audio_file": str(audio_file),
        "audio_metadata": file_metadata(audio_file, source_dir),
        "fixture_files": fixture_files,
        "buffer_size": args.buffer_size,
        "autochange_locked": args.lock_autochange,
        "timeout_seconds": args.timeout,
        "command": command,
        "environment_overrides": args.env,
        "python": sys.version.split()[0],
        "platform": platform.platform(),
        "processor": platform.processor(),
        "trace_log": trace_log.name,
        "metrics_json": "metrics.json",
    }

    (out_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (out_dir / "metrics.json").write_text(
        json.dumps(metrics, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    write_summary(out_dir / "summary.md", manifest, metrics)

    latest = out_root / "latest"
    if latest.exists() or latest.is_symlink():
        if latest.is_dir() and not latest.is_symlink():
            shutil.rmtree(latest)
        else:
            latest.unlink()
    try:
        latest.symlink_to(out_dir, target_is_directory=True)
    except OSError:
        shutil.copytree(out_dir, latest)

    print(f"Scene script benchmark report: {out_dir}")


if __name__ == "__main__":
    main()
