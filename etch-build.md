# Building `xcthugha` on Debian Etch

This document describes the verified historical build path for `xcthugha`, the X11 build of Cthugha, on Debian Etch.

The goal of this setup is preservation and reproducibility. It is not intended to describe a modern Linux build.

## Status

Verified:

- Debian Etch build environment
- `xcthugha`
- X11 display
- OSS-style audio device access through an emulated sound card

Not yet verified:

- Modern Linux distributions
- Native ALSA, PulseAudio, or PipeWire audio paths
- Packaging or installation outside the historical build environment

## Build target

The currently verified target is:

```sh
xcthugha
```

This is the X11 version of Cthugha.

## Expected environment

Tested in a Debian Etch virtual machine.

The build expects an old-style Linux userspace and toolchain. In particular, the project still carries assumptions from the autotools-era Linux ecosystem and from OSS-era sound devices.

A suitable environment includes:

- Debian Etch
- GCC/G++
- GNU Make
- Autotools-era build tools
- X11 development headers/libraries
- OSS-compatible sound device support, or an emulated sound card exposing OSS device nodes

## Required packages

The exact package set may vary depending on the base install, but the following categories are expected:

- C/C++ build tools
- `make`
- autotools support
- X11 development files
- sound headers providing `sys/soundcard.h`


## Source layout

Build from the repository root, not from `src/`.

The historical build system expects to be run from the project root. Some generated paths and make rules assume that layout.

## Configure

From the repository root:

```sh
./configure
```

If `configure` repeatedly re-runs during `make`, check file timestamps. This can happen when files are copied into the VM in a way that disturbs modification times.

When syncing source from a host machine into the Etch VM, preserve timestamps where possible.

For example, when using `rsync`, use archive mode:

```sh
rsync -av ./ user@etch-vm:~/projects/cthughanix/
```

Avoid copying host-generated build artefacts into the VM unless you intend to preserve them. In particular, stale or host-created autotools outputs can cause confusing rebuild behaviour.

## Build

From the repository root:

```sh
make
```

The verified binary is the X11 build:

```sh
src/xcthugha
```

Depending on the repository layout and makefile state, the generated binary path may differ. If unsure, search for it after the build:

```sh
find . -name 'xcthugha' -type f
```

## Common build issues

### `aclocal-1.9` not found

Some versions of the historical build system may try to call a specific autotools version such as:

```text
aclocal-1.9
```

If the configure script has already been generated and only the program needs to be built, the simplest fix is often to avoid regenerating autotools files at all.

Check whether timestamp churn is causing unnecessary rebuilds of generated files.

### `missing: Unknown --run option`

This usually indicates a mismatch between generated autotools files and the `missing` helper script in the source tree.

The practical preservation-oriented fix is to avoid unnecessary autotools regeneration and build from a clean, timestamp-consistent tree.

### `newly created file is older than distributed files`

This usually indicates clock skew or timestamp problems inside the VM or between the host and VM.

Check:

```sh
date
```

Then make sure copied files preserve timestamps, or refresh the tree consistently inside the VM.

### `makeinfo` errors

The documentation build may try to regenerate Texinfo output. For a preservation build, this is often accidental timestamp-driven rebuild behaviour rather than a necessary part of building `xcthugha`.

If this blocks the build, check whether `version.texi`, `cthugha.texi`, or generated `.info` files have inconsistent timestamps.

## Verified runtime command

A known-good runtime path may require disabling MIT-SHM:

```sh
./src/xcthugha --no-mit-shm
```

See [`runtime-notes.md`](runtime-notes.md) for details.

## Release interpretation

A successful build on Debian Etch should be treated as a historical compatibility baseline.

It does not imply that the project currently supports modern Linux distributions.
