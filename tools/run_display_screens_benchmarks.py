#!/usr/bin/env python3
import argparse
import datetime as dt
import json
import os
import platform
import statistics
import subprocess
import sys
from pathlib import Path


SCHEMA = "cthughanix.display-screens-benchmarks.v1"
DEFAULT_REPETITIONS = 20
DEFAULT_MIN_TIME = "0.02s"
DEFAULT_BUFFER_SIZE = "1600x1200"
DEFAULT_PATTERNS = "zero,gradient,noise"
VISIBLE_PIXELS_NOTE = "one item is one source indexed pixel"


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


def benchmark_binary(build_dir):
    candidates = [
        build_dir / "tests" / "benchmarks" / "display_screens_bench",
        build_dir / "display_screens_bench",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit(f"display_screens_bench was not found under {build_dir}")


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
            "screen": name,
            "pattern": "",
        }
    return {
        "stage": parts[0],
        "screen": "/".join(parts[1:-1]),
        "pattern": parts[-1],
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
            "screen": parsed["screen"],
            "pattern": parsed["pattern"],
            "total_samples": len(samples),
            "real_time": spread(real_times),
            "cpu_time": spread(cpu_times),
            "items_per_second_mean": statistics.fmean(item_rates) if item_rates else None,
            "bytes_per_second_mean": statistics.fmean(byte_rates) if byte_rates else None,
        }
        rows.append(row)

    rows.sort(key=lambda row: (row["screen"], row["pattern"]))
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


def format_rate(value, scale):
    if value is None:
        return ""
    return f"{value / scale:.2f}"


def write_slowest_table(f, rows):
    interesting = sorted(
        rows,
        key=lambda row: row["real_time"]["mean_us"],
        reverse=True,
    )[:10]
    f.write("## Slowest Cases\n\n")
    f.write("| Screen | Pattern | Mean us | CV % | MPix/s |\n")
    f.write("| --- | --- | ---: | ---: | ---: |\n")
    for row in interesting:
        timing = row["real_time"]
        f.write(
            f"| `{row['screen']}` | `{row['pattern']}` | "
            f"{timing['mean_us']:.3f} | {timing['cv_percent']:.2f} | "
            f"{format_rate(row['items_per_second_mean'], 1000000.0)} |\n"
        )
    f.write("\n")


def write_screen_table(f, rows):
    f.write("## Screen Renderers\n\n")
    if not rows:
        f.write("_No benchmark rows._\n\n")
        return

    f.write("| Screen | Pattern | Samples | Mean us | SD us | CV % | Min us | Max us | Median us | P99 us | MPix/s | GiB/s |\n")
    f.write("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
    for row in rows:
        timing = row["real_time"]
        f.write(
            f"| `{row['screen']}` | `{row['pattern']}` | {timing['samples']} | "
            f"{timing['mean_us']:.3f} | {timing['sd_us']:.3f} | "
            f"{timing['cv_percent']:.2f} | {timing['min_us']:.3f} | "
            f"{timing['max_us']:.3f} | {timing['median_us']:.3f} | "
            f"{timing['p99_us']:.3f} | "
            f"{format_rate(row['items_per_second_mean'], 1000000.0)} | "
            f"{format_rate(row['bytes_per_second_mean'], 1024.0 * 1024.0 * 1024.0)} |\n"
        )
    f.write("\n")


def write_summary(path, manifest, rows):
    with path.open("w", encoding="utf-8") as f:
        f.write(f"# Display Screens Benchmark Summary ({manifest['buffer_size']})\n\n")
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
        f.write(f"- Patterns: `{manifest['patterns']}`\n")
        f.write("- Timing unit: `us`\n")
        f.write(f"- Items: `{VISIBLE_PIXELS_NOTE}`\n")
        f.write(f"- Build dir: `{manifest['build_dir']}`\n")
        f.write(f"- Benchmark binary: `{manifest['benchmark_binary']}`\n\n")
        f.write("Source fixture generation and destination allocation are excluded from measured timing.\n\n")

        write_slowest_table(f, rows)
        write_screen_table(f, rows)

        f.write("CPU-time spread metrics are available in `spread-metrics.json`.\n")
        f.write("Raw Google Benchmark output is available in `google-benchmark.json`.\n")
        f.write("Console output is available in `trace.log`.\n")


def main():
    parser = argparse.ArgumentParser(
        description="Run and archive CthughaNix display/screen benchmarks."
    )
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--source-dir", default=None)
    parser.add_argument("--out-dir", default=None)
    parser.add_argument("--benchmark-arg", action="append", default=[])
    parser.add_argument("--repetitions", type=int, default=DEFAULT_REPETITIONS)
    parser.add_argument("--min-time", default=DEFAULT_MIN_TIME)
    parser.add_argument("--buffer-size", default=DEFAULT_BUFFER_SIZE)
    parser.add_argument("--patterns", default=DEFAULT_PATTERNS)
    args = parser.parse_args()

    build_dir = Path(args.build_dir).resolve()
    source_dir = Path(args.source_dir).resolve() if args.source_dir else Path.cwd()
    binary = benchmark_binary(build_dir)
    timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H%M%SZ")
    git_commit = git_value(["rev-parse", "--short=12", "HEAD"], source_dir) or "unknown"
    dirty = bool(git_value(["status", "--porcelain"], source_dir))
    run_id = f"{timestamp}-{git_commit}{'-dirty' if dirty else ''}"
    out_root = Path(args.out_dir).resolve() if args.out_dir else (
        build_dir / "test-results" / "display-screens"
    )
    out_dir = out_root / run_id
    out_dir.mkdir(parents=True, exist_ok=True)

    benchmark_json = out_dir / "google-benchmark.json"
    trace_log = out_dir / "trace.log"

    benchmark_args = list(args.benchmark_arg)
    if not benchmark_arg_present(benchmark_args, "--benchmark_repetitions"):
        benchmark_args.append(f"--benchmark_repetitions={args.repetitions}")
    if not benchmark_arg_present(benchmark_args, "--benchmark_report_aggregates_only"):
        benchmark_args.append("--benchmark_report_aggregates_only=false")
    if not benchmark_arg_present(benchmark_args, "--benchmark_time_unit"):
        benchmark_args.append("--benchmark_time_unit=us")
    if not benchmark_arg_present(benchmark_args, "--benchmark_min_time"):
        benchmark_args.append(f"--benchmark_min_time={args.min_time}")

    display_args = [
        f"--cth-buffer-size={args.buffer_size}",
        f"--cth-patterns={args.patterns}",
    ]

    cmd = [
        str(binary),
        "--benchmark_format=console",
        f"--benchmark_out={benchmark_json}",
        "--benchmark_out_format=json",
    ] + display_args + benchmark_args

    env = os.environ.copy()
    env.setdefault("LC_ALL", "C")
    result = subprocess.run(cmd, cwd=source_dir, text=True, capture_output=True, env=env)
    trace_log.write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode != 0:
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)

    manifest = {
        "schema": SCHEMA,
        "timestamp_utc": timestamp,
        "git_commit": git_commit,
        "git_dirty": dirty,
        "project_version": "1.5",
        "benchmark_repetitions": args.repetitions,
        "benchmark_min_time": args.min_time,
        "buffer_size": args.buffer_size,
        "patterns": args.patterns,
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
        "trace_log": trace_log.name,
    }

    rows = load_benchmark_metrics(benchmark_json)
    spread_metrics = {
        "schema": SCHEMA,
        "manifest": manifest,
        "benchmarks": rows,
    }
    (out_dir / "spread-metrics.json").write_text(
        json.dumps(spread_metrics, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    write_summary(out_dir / "summary.md", manifest, rows)

    print(f"Display screens benchmark report: {out_dir}")


if __name__ == "__main__":
    main()
