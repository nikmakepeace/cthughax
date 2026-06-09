# Runtime notes for `xcthugha`

This document describes the currently known runtime behaviour of `xcthugha`, the X11 build of Cthugha, in the verified Debian Etch environment.

The goal is to record what is known to work, what is untested, and what common runtime failures mean.

## Status

Verified:

- `xcthugha` starts on Debian Etch
- X11 output works
- Keyboard/input handling is usable
- Audio input works through the emulated sound card
- The program responds visually to input from the emulated sound device

Not yet verified:

- Runtime behaviour on modern Linux distributions
- Runtime behaviour with ALSA, PulseAudio, or PipeWire-native devices
- Runtime behaviour without OSS compatibility

## Recommended command

In the verified VM/X11 setup, use:

```sh
./src/xcthugha --no-mit-shm
```

The exact binary path may differ depending on the build layout.

## X11 and MIT-SHM

`xcthugha` will try to use the MIT-SHM X11 extension.

MIT-SHM allows an X11 client and server to share memory for image transfer, avoiding slower socket-based image copies. This can improve performance when the X client and X server are on the same machine and share a compatible memory model.

In virtualised, forwarded, nested, or unusual X11 environments, MIT-SHM can fail.

A typical failure looks like:

```text
Initializing X11 display...
X Error of failed request:  BadRequest (invalid request code or no such operation)
  Major opcode of failed request:  133 (MIT-SHM)
  Minor opcode of failed request:  1 (X_ShmAttach)
```

If this happens, run:

```sh
./src/xcthugha --no-mit-shm
```

Although this option may not appear in the visible help text, it has been observed to work.

## Display performance

Observed performance in the verified environment:

- 320x200: approximately 90 FPS
- 640x480: approximately 25 FPS

But! This was a virtualised 

## Random palettes

Palette keys distinguish existing catalog palettes from generated random
palettes:

- `p`/`P` moves through the loaded palette catalog.
- `R` creates a new generated random palette at `max(random.N) + 1`, saves it
  as `resources/map/random.N.map`, and selects it.
- `r` re-rolls the currently selected `random.N` palette and saves that same
  map. If the current selection is not `random.N`, it re-rolls the most recent
  generated palette from the current process; if none exists yet, it behaves
  like `R`.

Because the palette search path includes `./resources/map/`, saved
`random.N.map` files are loaded as ordinary catalog palettes on the next run.
New generated random palettes continue from the highest existing `random.N`
instead of overwriting earlier saved random palettes.

## Audio expectations

For file playback, the current code decodes into the modern audio runtime and
plays through PulseAudio/PipeWire-Pulse when available. Use `--pulse-server` or
`cthugha.pulse-server` to point the client at a specific Pulse server.

For live input and mixer control, the remaining Unix device paths are commonly:

```text
/dev/dsp
/dev/mixer
```

If these devices are missing, startup may print messages like:

```text
Initializing the sound device...
    audio input strategy: OSS /dev/dsp input
Can't open `/dev/dsp' for reading. (2 - No such file or directory)
Can not use requested sound input. Using random noise.
Initializing CD player...
Initializing Mixer device...
Can not open `/dev/mixer'. (2 - No such file or directory)
```

This means the program could not open the requested live-input device. It may
fall back to random noise or degraded behaviour. File playback does not depend
on the live-input device path.

In the verified setup, audio input works when the VM exposes an emulated sound card in a way that provides usable OSS-compatible device access.

## Host microphone prompt in UTM

When running inside UTM on macOS, UTM may request microphone access from the host.

This can happen because the VM sound device is being backed by host audio input.
If granted, the emulated sound card may provide input that `xcthugha` can read
through the OSS-compatible line-input path.

## Mixer device

The program may also attempt to open:

```text
/dev/mixer
```

Failure to open `/dev/mixer` does not necessarily prevent the program from running, but it indicates that mixer control through the old OSS interface is unavailable.

## Terminal settings

Some old Debian Etch tools may not recognise modern terminal names such as:

```text
xterm-256color
```

For better compatibility, use:

```sh
export TERM=xterm
```

If colour support causes issues, fall back further:

```sh
export TERM=vt100
```

For normal interactive work in Etch, `xterm` is usually the best balance of compatibility and usability.

## Known caveats

- `--no-mit-shm` may be required even though it is not advertised in the help text.
- Live input and mixer control still depend on OSS-compatible device paths.
- `/dev/dsp` and `/dev/mixer` may not exist on modern Linux systems.
- File playback is better exercised through PulseAudio/PipeWire-Pulse output.
- The X11 path is the only graphics path in the current tree.

## Troubleshooting checklist

If `xcthugha` does not start:

1. Confirm that X11 is running.
2. Confirm that the `DISPLAY` environment variable is set.
3. Try running with MIT-SHM disabled:

   ```sh
   ./src/xcthugha --no-mit-shm
   ```

4. Confirm that the binary was built inside the Etch environment.
5. Confirm that the VM has a configured sound card if audio input is desired.
6. Check whether `/dev/dsp` exists:

   ```sh
   ls -l /dev/dsp
   ```

7. Check whether `/dev/mixer` exists:

   ```sh
   ls -l /dev/mixer
   ```

If the program runs but does not react to audio, the most likely cause is that the expected OSS audio input path is not connected to a live input source.

## Current interpretation

The current runtime state should be described as:

```text
xcthugha is verified to run on Debian Etch under X11, with audio input working through an emulated OSS-compatible sound device.
```

It should not yet be described as broadly portable or modern-Linux-ready.
