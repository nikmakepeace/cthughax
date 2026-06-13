#!/usr/bin/env python3
"""Package CthughaNix binaries into a relocatable macOS .app bundle."""

import argparse
import os
import plistlib
import shutil
import stat
import subprocess
import sys
from pathlib import Path


SYSTEM_DEPENDENCY_PREFIXES = (
    "/usr/lib/",
    "/System/Library/",
)


def run(command):
    subprocess.run(command, check=True)


def command_output(command):
    return subprocess.check_output(command, text=True)


def require_tool(name):
    if shutil.which(name) is None:
        raise RuntimeError(f"required macOS packaging tool not found: {name}")


def make_executable(path):
    mode = path.stat().st_mode
    path.chmod(mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def make_user_writable(path):
    mode = path.stat().st_mode
    path.chmod(mode | stat.S_IWUSR)


def otool_dependencies(path, skip_install_name=False):
    lines = command_output(["otool", "-L", str(path)]).splitlines()
    dependencies = []
    for line in lines[1:]:
        stripped = line.strip()
        if not stripped:
            continue
        dependencies.append(stripped.split(" ", 1)[0])

    if skip_install_name and dependencies:
        return dependencies[1:]
    return dependencies


def otool_rpaths(path):
    lines = command_output(["otool", "-l", str(path)]).splitlines()
    rpaths = []
    saw_rpath = False
    for line in lines:
        stripped = line.strip()
        if stripped == "cmd LC_RPATH":
            saw_rpath = True
            continue
        if saw_rpath and stripped.startswith("path "):
            rpaths.append(stripped.split(" ", 2)[1])
            saw_rpath = False
    return rpaths


def is_library_file(path):
    return path.suffix in (".dylib", ".so")


def expanded_loader_path(token, loader_path, executable_dir):
    if token.startswith("@loader_path"):
        return str(loader_path.parent) + token[len("@loader_path"):]
    if token.startswith("@executable_path"):
        return str(executable_dir) + token[len("@executable_path"):]
    return token


def resolve_dependency(dependency, loader_path, executable_dir):
    if dependency.startswith("@rpath/"):
        suffix = dependency[len("@rpath/"):]
        for rpath in otool_rpaths(loader_path):
            candidate = Path(expanded_loader_path(
                rpath, loader_path, executable_dir)) / suffix
            if candidate.exists():
                return candidate.resolve()
        return None

    expanded = expanded_loader_path(dependency, loader_path, executable_dir)
    path = Path(expanded)
    if path.exists():
        return path.resolve()
    return None


def is_system_dependency(dependency):
    return dependency.startswith(SYSTEM_DEPENDENCY_PREFIXES)


def should_bundle_dependency(dependency, resolved):
    if resolved is None:
        return False
    if is_system_dependency(str(resolved)):
        return False
    if dependency.startswith(SYSTEM_DEPENDENCY_PREFIXES):
        return False
    return resolved.suffix in (".dylib", ".so")


def copy_binary(source, destination):
    if not source.exists():
        raise RuntimeError(f"missing binary: {source}")
    shutil.copy2(source, destination)
    make_user_writable(destination)
    make_executable(destination)


def copy_resources(source, destination):
    if not source.exists():
        raise RuntimeError(f"missing resources directory: {source}")
    shutil.copytree(source, destination)


def write_launcher(path):
    path.write_text(
        "#!/bin/sh\n"
        "APP_CONTENTS=$(CDPATH= cd -- \"$(dirname -- \"$0\")/..\" && pwd) "
        "|| exit 1\n"
        "cd \"$APP_CONTENTS/Resources\" || exit 1\n"
        "exec \"$APP_CONTENTS/MacOS/cthugha\" \"$@\"\n",
        encoding="utf-8",
    )
    make_executable(path)


def write_info_plist(path, app_name, bundle_id, version, executable_name):
    plist = {
        "CFBundleDevelopmentRegion": "en",
        "CFBundleDisplayName": app_name,
        "CFBundleExecutable": executable_name,
        "CFBundleIdentifier": bundle_id,
        "CFBundleInfoDictionaryVersion": "6.0",
        "CFBundleName": app_name,
        "CFBundlePackageType": "APPL",
        "CFBundleShortVersionString": version,
        "CFBundleVersion": version,
        "LSMinimumSystemVersion": "11.0",
        "NSHighResolutionCapable": True,
    }
    with path.open("wb") as file:
        plistlib.dump(plist, file)


def framework_reference(name):
    return f"@executable_path/../Frameworks/{name}"


def collect_dependencies(linked_files, frameworks_dir, executable_dir):
    copied_by_source = {}
    copied_by_name = {}
    scan_queue = list(linked_files)

    while scan_queue:
        linked_file = scan_queue.pop(0)
        for dependency in otool_dependencies(
                linked_file, skip_install_name=is_library_file(linked_file)):
            resolved = resolve_dependency(dependency, linked_file, executable_dir)
            if not should_bundle_dependency(dependency, resolved):
                continue

            name = resolved.name
            existing_source = copied_by_name.get(name)
            if existing_source is not None and existing_source != resolved:
                raise RuntimeError(
                    f"dependency basename collision for {name}: "
                    f"{existing_source} and {resolved}")

            if resolved in copied_by_source:
                continue

            destination = frameworks_dir / name
            shutil.copy2(resolved, destination)
            make_user_writable(destination)
            make_executable(destination)
            copied_by_source[resolved] = destination
            copied_by_name[name] = resolved
            scan_queue.append(destination)

    return copied_by_source


def rewrite_dependencies(linked_files, frameworks_dir, executable_dir):
    for linked_file in linked_files:
        skip_install_name = is_library_file(linked_file)
        for dependency in otool_dependencies(linked_file, skip_install_name):
            resolved = resolve_dependency(dependency, linked_file, executable_dir)
            if not should_bundle_dependency(dependency, resolved):
                continue

            bundled = frameworks_dir / resolved.name
            if not bundled.exists():
                raise RuntimeError(
                    f"dependency was not copied before rewrite: {resolved}")
            run([
                "install_name_tool",
                "-change",
                dependency,
                framework_reference(bundled.name),
                str(linked_file),
            ])

        if is_library_file(linked_file):
            run([
                "install_name_tool",
                "-id",
                framework_reference(linked_file.name),
                str(linked_file),
            ])


def validate_bundle_dependencies(linked_files):
    bad_dependencies = []
    for linked_file in linked_files:
        for dependency in otool_dependencies(
                linked_file, skip_install_name=is_library_file(linked_file)):
            if dependency.startswith("@executable_path/../Frameworks/"):
                continue
            if is_system_dependency(dependency):
                continue
            if dependency.startswith("/"):
                bad_dependencies.append((linked_file, dependency))

    if bad_dependencies:
        formatted = "\n".join(
            f"  {path}: {dependency}" for path, dependency in bad_dependencies)
        raise RuntimeError(
            "bundle still has absolute non-system dependencies:\n"
            + formatted)


def sign_bundle(app_path, identity):
    if not identity:
        return
    run(["codesign", "--force", "--deep", "--sign", identity, str(app_path)])


def build_app(args):
    require_tool("otool")
    require_tool("install_name_tool")
    if args.codesign_identity:
        require_tool("codesign")

    output_dir = args.output_dir.resolve()
    app_path = output_dir / f"{args.app_name}.app"
    contents_dir = app_path / "Contents"
    macos_dir = contents_dir / "MacOS"
    resources_dir = contents_dir / "Resources"
    frameworks_dir = contents_dir / "Frameworks"

    if app_path.exists():
        shutil.rmtree(app_path)

    macos_dir.mkdir(parents=True)
    resources_dir.mkdir(parents=True)
    frameworks_dir.mkdir(parents=True)

    launcher_path = macos_dir / args.app_name
    app_binary_path = macos_dir / "cthugha"
    panel_binary_path = macos_dir / "cthugha-panel"

    write_launcher(launcher_path)
    copy_binary(args.app_binary.resolve(), app_binary_path)
    copy_binary(args.panel_binary.resolve(), panel_binary_path)
    copy_resources(args.resources_dir.resolve(), resources_dir / "resources")
    write_info_plist(
        contents_dir / "Info.plist",
        args.app_name,
        args.bundle_id,
        args.version,
        args.app_name,
    )
    (contents_dir / "PkgInfo").write_text("APPL????", encoding="ascii")

    initial_linked_files = [app_binary_path, panel_binary_path]
    copied_dependencies = collect_dependencies(
        initial_linked_files, frameworks_dir, macos_dir)
    linked_files = initial_linked_files + sorted(copied_dependencies.values())
    rewrite_dependencies(linked_files, frameworks_dir, macos_dir)
    validate_bundle_dependencies(linked_files)
    sign_bundle(app_path, args.codesign_identity)

    print(f"Created {app_path}")
    if copied_dependencies:
        print("Bundled shared libraries:")
        for source, destination in sorted(copied_dependencies.items()):
            print(f"  {source} -> {destination.relative_to(app_path)}")
    else:
        print("No non-system shared libraries needed bundling.")


def parse_args(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app-name", required=True)
    parser.add_argument("--bundle-id", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--app-binary", type=Path, required=True)
    parser.add_argument("--panel-binary", type=Path, required=True)
    parser.add_argument("--resources-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--codesign-identity", default="")
    return parser.parse_args(argv)


def main(argv):
    try:
        build_app(parse_args(argv))
    except Exception as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
