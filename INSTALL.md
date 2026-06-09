# Installation

## Building From Source

CthughaNix is built with CMake.

```sh
cmake -S . -B build
cmake --build build
```

The main executable is:

```text
build/src/xcthugha
```

To install it and the runtime resources:

```sh
cmake --install build
```

Use a custom install prefix in the configure step:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/opt/cthughanix
cmake --build build
cmake --install build
```

## Useful Build Options

- `CTH_BUILD_X11`: build the X11 frontend. Default: `ON`.
- `CTH_ENABLE_PULSE`: enable PulseAudio/PipeWire-Pulse output when available.
  Default: `ON`.
- `CTH_ENABLE_DSP`: enable OSS `/dev/dsp` support when soundcard headers are
  available. Default: `ON`.
- `CTH_ENABLE_MIXER`: enable OSS mixer controls when soundcard headers are
  available. Default: `ON`.
- `CTH_ENABLE_MINIMP3`: enable embedded MP3 decoding. Default: `ON`.
- `CTH_DATA_DIR`: installed runtime data directory.

Example:

```sh
cmake -S . -B build -DCTH_ENABLE_DSP=OFF -DCTH_ENABLE_MIXER=OFF
```

## Runtime Resources

Installed resources live under `CTH_DATA_DIR`, which defaults to the platform
data directory chosen by CMake, usually `/usr/local/share/cthughanix` for a
default local install.

From a build tree, `xcthugha` can also find assets in the repository's
`resources/` directories.

## Problems

- The X11 frontend requires X11, Xt, Xaw, Xmu, Xext, and Zlib development
  libraries. Xpm support is optional.
- PulseAudio-compatible output requires `libpulse-simple`.
- OSS DSP and mixer support depend on soundcard headers and usable device
  nodes.
- Indexed-image assets are loaded directly from uncompressed `.pcx` and indexed
  `.png` files.
