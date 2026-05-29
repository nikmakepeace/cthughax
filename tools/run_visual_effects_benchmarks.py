#!/usr/bin/env python3
import argparse
import datetime as dt
import hashlib
import json
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
from pathlib import Path


SCHEMA = "cthughanix.visual-effects-benchmarks.v1"
DEFAULT_REPETITIONS = 20
DEFAULT_MIN_TIME = "0.02s"
DEFAULT_BUFFER_SIZE = "160x100"
VISIBLE_PIXELS_NOTE = "one item is one visible indexed pixel"


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


def benchmark_binary(build_dir):
    candidates = [
        build_dir / "tests" / "benchmarks" / "visual_effects_bench",
        build_dir / "visual_effects_bench",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit(f"visual_effects_bench was not found under {build_dir}")


def benchmark_arg_present(args, name):
    prefix = name + "="
    return any(arg == name or arg.startswith(prefix) for arg in args)


def benchmark_name(name):
    return name.split("/repeats:", 1)[0]


def time_to_us(value, unit):
    if unit == "ns":
        return value / 1000.0
    if unit == "us":
        return value
    if unit == "ms":
        return value * 1000.0
    if unit == "s":
        return value * 1000000.0
    return value


def nearest_rank_percentile(values, percentile):
    import math

    if not values:
        return 0.0
    rank = int(math.ceil((percentile / 100.0) * len(values)))
    rank = min(max(rank, 1), len(values))
    return sorted(values)[rank - 1]


def spread(values):
    if not values:
        return {
            "samples": 0,
            "mean_us": 0.0,
            "sd_us": 0.0,
            "cv_percent": 0.0,
            "min_us": 0.0,
            "max_us": 0.0,
            "median_us": 0.0,
            "p99_us": 0.0,
        }
    mean = statistics.fmean(values)
    sd = statistics.stdev(values) if len(values) > 1 else 0.0
    return {
        "samples": len(values),
        "mean_us": mean,
        "sd_us": sd,
        "cv_percent": (sd / mean * 100.0) if mean else 0.0,
        "min_us": min(values),
        "max_us": max(values),
        "median_us": statistics.median(values),
        "p99_us": nearest_rank_percentile(values, 99.0),
    }


def parse_case_name(name):
    parts = name.split("/")
    if len(parts) < 3:
        return {
            "stage": "Other",
            "entry": name,
            "fixture": "",
        }
    return {
        "stage": parts[0],
        "entry": "/".join(parts[1:-1]),
        "fixture": parts[-1],
    }


def load_benchmark_metrics(path):
    with path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    grouped = {}
    for entry in data.get("benchmarks", []):
        if entry.get("run_type") == "aggregate":
            continue
        name = benchmark_name(entry.get("name", ""))
        unit = entry.get("time_unit", "")
        grouped.setdefault(name, []).append(
            {
                "real_time_us": time_to_us(entry.get("real_time", 0.0), unit),
                "cpu_time_us": time_to_us(entry.get("cpu_time", 0.0), unit),
                "items_per_second": entry.get("items_per_second"),
                "bytes_per_second": entry.get("bytes_per_second"),
            }
        )

    rows = []
    for name, samples in grouped.items():
        parsed = parse_case_name(name)
        real_times = [sample["real_time_us"] for sample in samples]
        cpu_times = [sample["cpu_time_us"] for sample in samples]
        item_rates = [
            sample["items_per_second"]
            for sample in samples
            if sample.get("items_per_second") is not None
        ]
        byte_rates = [
            sample["bytes_per_second"]
            for sample in samples
            if sample.get("bytes_per_second") is not None
        ]
        row = {
            "name": name,
            "stage": parsed["stage"],
            "entry": parsed["entry"],
            "fixture": parsed["fixture"],
            "total_samples": len(samples),
            "real_time": spread(real_times),
            "cpu_time": spread(cpu_times),
            "items_per_second_mean": statistics.fmean(item_rates) if item_rates else None,
            "bytes_per_second_mean": statistics.fmean(byte_rates) if byte_rates else None,
        }
        rows.append(row)

    rows.sort(key=lambda row: (row["stage"], row["entry"], row["fixture"]))
    return rows


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


def parse_buffer_size(text):
    match = re.fullmatch(r"([0-9]+)x([0-9]+)", text or "")
    if not match:
        return None
    return int(match.group(1)), int(match.group(2))


def pgm_token(f):
    token = bytearray()

    while True:
        c = f.read(1)
        if not c:
            return None
        if c.isspace():
            continue
        if c == b"#":
            f.readline()
            continue
        token.extend(c)
        break

    while True:
        c = f.read(1)
        if not c:
            break
        if c.isspace():
            break
        if c == b"#":
            f.readline()
            break
        token.extend(c)

    return token.decode("ascii")


def read_pgm_header(path):
    with path.open("rb") as f:
        magic = pgm_token(f)
        width = int(pgm_token(f) or 0)
        height = int(pgm_token(f) or 0)
        max_value = int(pgm_token(f) or 0)
        data_offset = f.tell()
    return {
        "format": magic or "",
        "width": width,
        "height": height,
        "max_value": max_value,
        "payload_bytes": path.stat().st_size - data_offset,
    }


def buffer_match_for_dimensions(width, height, buffer_size):
    parsed = parse_buffer_size(buffer_size)
    if not parsed:
        return ""
    expected_width, expected_height = parsed
    if width == expected_width and height == expected_height:
        return "visible"
    if width == expected_width and height == expected_height + 6:
        return "visible+hidden-rows"
    return "mismatch"


def buffer_match_for_raw_size(byte_count, buffer_size):
    parsed = parse_buffer_size(buffer_size)
    if not parsed:
        return ""
    width, height = parsed
    if byte_count == width * height:
        return "visible"
    if byte_count == width * (height + 6):
        return "visible+hidden-rows"
    return "mismatch"


def selected_fixture_paths(fixture_dir, fixture_selection):
    if not fixture_dir.exists():
        return []
    if not fixture_selection:
        return sorted(list(fixture_dir.glob("*.pgm")) + list(fixture_dir.glob("*.raw")))

    paths = []
    names = [item.strip() for item in fixture_selection.split(",") if item.strip()]
    for name in names:
        for stem in (name, name + "-active", name + "-passive"):
            for extension in (".pgm", ".raw"):
                path = fixture_dir / (stem + extension)
                if path.exists():
                    paths.append(path)

    unique = []
    seen = set()
    for path in sorted(paths):
        if path not in seen:
            unique.append(path)
            seen.add(path)
    return unique


def image_metadata(path, source_dir, buffer_size):
    metadata = file_metadata(path, source_dir)
    suffix = path.suffix.lower()
    if suffix == ".pgm":
        header = read_pgm_header(path)
        metadata.update(header)
        metadata["image_size"] = f"{header['width']}x{header['height']}"
        metadata["buffer_match"] = buffer_match_for_dimensions(
            header["width"], header["height"], buffer_size
        )
        metadata["payload_matches_header"] = (
            header["payload_bytes"] == header["width"] * header["height"]
        )
    elif suffix == ".raw":
        metadata["format"] = "raw"
        metadata["image_size"] = ""
        metadata["max_value"] = ""
        metadata["payload_bytes"] = metadata["bytes"]
        metadata["buffer_match"] = buffer_match_for_raw_size(metadata["bytes"], buffer_size)
        metadata["payload_matches_header"] = ""
    else:
        metadata["format"] = suffix.lstrip(".")
        metadata["image_size"] = ""
        metadata["max_value"] = ""
        metadata["payload_bytes"] = metadata["bytes"]
        metadata["buffer_match"] = ""
        metadata["payload_matches_header"] = ""
    return metadata


def fixture_metadata(source_dir, fixture_dir, active, passive, fixture_selection, buffer_size):
    fixtures = []
    for fixture in selected_fixture_paths(fixture_dir, fixture_selection):
        fixtures.append(image_metadata(fixture, source_dir, buffer_size))

    explicit = []
    for label, path_text in (("active", active), ("passive", passive)):
        if not path_text:
            continue
        path = Path(path_text).resolve()
        if path.exists():
            metadata = image_metadata(path, source_dir, buffer_size)
            metadata["role"] = label
            explicit.append(metadata)

    return {
        "fixture_dir": str(fixture_dir),
        "fixture_files": fixtures,
        "explicit_images": explicit,
    }


def format_rate(value, scale):
    if value is None:
        return ""
    return f"{value / scale:.2f}"


def write_stage_table(f, title, rows):
    f.write(f"## {title}\n\n")
    if not rows:
        f.write("_No benchmark rows._\n\n")
        return

    f.write("| Entry | Fixture | Samples | Mean us | SD us | CV % | Min us | Max us | Median us | P99 us | MPix/s | GiB/s |\n")
    f.write("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
    for row in rows:
        timing = row["real_time"]
        f.write(
            f"| `{row['entry']}` | `{row['fixture']}` | {timing['samples']} | "
            f"{timing['mean_us']:.3f} | {timing['sd_us']:.3f} | "
            f"{timing['cv_percent']:.2f} | {timing['min_us']:.3f} | "
            f"{timing['max_us']:.3f} | {timing['median_us']:.3f} | "
            f"{timing['p99_us']:.3f} | "
            f"{format_rate(row['items_per_second_mean'], 1000000.0)} | "
            f"{format_rate(row['bytes_per_second_mean'], 1024.0 * 1024.0 * 1024.0)} |\n"
        )
    f.write("\n")


def write_slowest_table(f, rows):
    interesting = sorted(
        rows,
        key=lambda row: row["real_time"]["mean_us"],
        reverse=True,
    )[:10]
    f.write("## Slowest Cases\n\n")
    f.write("| Stage | Entry | Fixture | Mean us | CV % | MPix/s |\n")
    f.write("| --- | --- | --- | ---: | ---: | ---: |\n")
    for row in interesting:
        timing = row["real_time"]
        f.write(
            f"| `{row['stage']}` | `{row['entry']}` | `{row['fixture']}` | "
            f"{timing['mean_us']:.3f} | {timing['cv_percent']:.2f} | "
            f"{format_rate(row['items_per_second_mean'], 1000000.0)} |\n"
        )
    f.write("\n")


def write_fixture_table(f, manifest):
    rows = []
    for entry in manifest.get("fixture_files", []):
        item = dict(entry)
        item["role"] = "fixture"
        rows.append(item)
    rows.extend(manifest.get("explicit_images", []))

    if not rows:
        return

    f.write("## Fixture Files\n\n")
    f.write("| Role | File | Format | Image Size | Max | Payload Bytes | Buffer Match |\n")
    f.write("| --- | --- | --- | ---: | ---: | ---: | --- |\n")
    for row in rows:
        f.write(
            f"| `{row.get('role', 'fixture')}` | `{row.get('file', '')}` | "
            f"`{row.get('format', '')}` | `{row.get('image_size', '')}` | "
            f"`{row.get('max_value', '')}` | {row.get('payload_bytes', '')} | "
            f"`{row.get('buffer_match', '')}` |\n"
        )
    f.write("\n")


def write_summary(path, manifest, rows):
    stages = sorted({row["stage"] for row in rows})
    stage_rows = {stage: [row for row in rows if row["stage"] == stage] for stage in stages}

    with path.open("w", encoding="utf-8") as f:
        f.write(f"# Visual Effects Benchmark Summary ({manifest['buffer_size']})\n\n")
        f.write(f"- Schema: `{manifest['schema']}`\n")
        f.write(f"- Timestamp: `{manifest['timestamp_utc']}`\n")
        f.write(f"- Git: `{manifest['git_commit']}`")
        if manifest["git_dirty"]:
            f.write(" dirty")
        f.write("\n")
        f.write(f"- Build type: `{manifest['build_type']}`\n")
        f.write(f"- C++ compiler: `{manifest['cxx_compiler']}`\n")
        f.write(f"- Repetitions: `{manifest['benchmark_repetitions']}`\n")
        f.write(f"- Minimum time per repetition: `{manifest['benchmark_min_time']}`\n")
        f.write(f"- Buffer size: `{manifest['buffer_size']}`\n")
        if manifest["fixture_selection"]:
            f.write(f"- Fixture selection: `{manifest['fixture_selection']}`\n")
        elif manifest["explicit_images"]:
            f.write("- Fixture selection: explicit active/passive images\n")
        else:
            f.write("- Fixture selection: built-in `zero`, `impulse`, `gradient`, `noise`\n")
        f.write(f"- Timing unit: `us`\n")
        f.write(f"- Items: `{VISIBLE_PIXELS_NOTE}`\n")
        f.write(f"- Build dir: `{manifest['build_dir']}`\n")
        f.write(f"- Benchmark binary: `{manifest['benchmark_binary']}`\n\n")
        f.write("Fixture copying and buffer reset are excluded from measured timing.\n\n")

        write_fixture_table(f, manifest)
        write_slowest_table(f, rows)

        for stage in ("Flame", "Translate"):
            write_stage_table(f, stage, stage_rows.get(stage, []))
        for stage in stages:
            if stage not in ("Flame", "Translate"):
                write_stage_table(f, stage, stage_rows[stage])

        f.write("CPU-time spread metrics are available in `spread-metrics.json`.\n")
        f.write("Raw Google Benchmark output is available in `google-benchmark.json`.\n")


def main():
    parser = argparse.ArgumentParser(
        description="Run and archive CthughaNix visual effects benchmarks."
    )
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--source-dir", default=None)
    parser.add_argument("--out-dir", default=None)
    parser.add_argument("--benchmark-arg", action="append", default=[])
    parser.add_argument("--repetitions", type=int, default=DEFAULT_REPETITIONS)
    parser.add_argument("--min-time", default=DEFAULT_MIN_TIME)
    parser.add_argument("--buffer-size", default=DEFAULT_BUFFER_SIZE)
    parser.add_argument("--fixture-dir", default=None)
    parser.add_argument("--fixtures", default=None)
    parser.add_argument("--active", default=None)
    parser.add_argument("--passive", default=None)
    args = parser.parse_args()

    if bool(args.active) != bool(args.passive):
        raise SystemExit("--active and --passive must be specified together")

    build_dir = Path(args.build_dir).resolve()
    source_dir = Path(args.source_dir).resolve() if args.source_dir else Path.cwd()
    binary = benchmark_binary(build_dir)
    timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H%M%SZ")
    git_commit = git_value(["rev-parse", "--short=12", "HEAD"], source_dir) or "unknown"
    dirty = bool(git_value(["status", "--porcelain"], source_dir))
    run_id = f"{timestamp}-{git_commit}{'-dirty' if dirty else ''}"
    out_root = Path(args.out_dir).resolve() if args.out_dir else (
        build_dir / "test-results" / "visual-effects"
    )
    out_dir = out_root / run_id
    out_dir.mkdir(parents=True, exist_ok=True)

    benchmark_json = out_dir / "google-benchmark.json"
    console_log = out_dir / "console.log"
    default_fixture_dir = source_dir / "tests" / "fixtures" / "visual"
    fixture_dir = Path(args.fixture_dir).resolve() if args.fixture_dir else default_fixture_dir

    benchmark_args = list(args.benchmark_arg)
    if not benchmark_arg_present(benchmark_args, "--benchmark_repetitions"):
        benchmark_args.append(f"--benchmark_repetitions={args.repetitions}")
    if not benchmark_arg_present(benchmark_args, "--benchmark_report_aggregates_only"):
        benchmark_args.append("--benchmark_report_aggregates_only=false")
    if not benchmark_arg_present(benchmark_args, "--benchmark_time_unit"):
        benchmark_args.append("--benchmark_time_unit=us")
    if not benchmark_arg_present(benchmark_args, "--benchmark_min_time"):
        benchmark_args.append(f"--benchmark_min_time={args.min_time}")

    visual_args = []
    if args.buffer_size:
        visual_args.append(f"--cth-buffer-size={args.buffer_size}")
    if args.fixture_dir:
        visual_args.append(f"--cth-fixture-dir={fixture_dir}")
    if args.fixtures:
        visual_args.append(f"--cth-fixtures={args.fixtures}")
    if args.active and args.passive:
        visual_args.append(f"--cth-active={Path(args.active).resolve()}")
        visual_args.append(f"--cth-passive={Path(args.passive).resolve()}")

    cmd = [
        str(binary),
        "--benchmark_format=console",
        f"--benchmark_out={benchmark_json}",
        "--benchmark_out_format=json",
    ] + visual_args + benchmark_args

    env = os.environ.copy()
    env.setdefault("LC_ALL", "C")
    result = subprocess.run(cmd, cwd=source_dir, text=True, capture_output=True, env=env)
    console_log.write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode != 0:
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)

    fixture_info = fixture_metadata(
        source_dir, fixture_dir, args.active, args.passive, args.fixtures, args.buffer_size
    )
    manifest = {
        "schema": SCHEMA,
        "timestamp_utc": timestamp,
        "git_commit": git_commit,
        "git_dirty": dirty,
        "project_version": "1.5",
        "benchmark_repetitions": args.repetitions,
        "benchmark_min_time": args.min_time,
        "buffer_size": args.buffer_size,
        "fixture_selection": args.fixtures or "",
        "fixture_dir": fixture_info["fixture_dir"],
        "fixture_files": fixture_info["fixture_files"],
        "explicit_images": fixture_info["explicit_images"],
        "build_type": cmake_cache_value(build_dir, "CMAKE_BUILD_TYPE"),
        "c_compiler": cmake_cache_value(build_dir, "CMAKE_C_COMPILER"),
        "cxx_compiler": cmake_cache_value(build_dir, "CMAKE_CXX_COMPILER"),
        "source_dir": str(source_dir),
        "build_dir": str(build_dir),
        "benchmark_binary": str(binary),
        "python": sys.version.split()[0],
        "platform": platform.platform(),
        "processor": platform.processor(),
        "command": cmd,
        "benchmark_json": benchmark_json.name,
        "console_log": console_log.name,
    }

    rows = load_benchmark_metrics(benchmark_json)
    (out_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (out_dir / "spread-metrics.json").write_text(
        json.dumps(rows, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    write_summary(out_dir / "summary.md", manifest, rows)

    shutil.copy2(benchmark_json, out_dir / "latest-google-benchmark.json")
    print(f"Visual effects benchmark report: {out_dir}")


if __name__ == "__main__":
    main()
