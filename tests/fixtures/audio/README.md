Audio fixture notes
===================

`prism-2s-48000-stereo-s16le.raw` is a two-second, 48 kHz stereo signed
16-bit little-endian PCM extract from `prism.mp3`. It exists so FFT benchmarks
can use non-sine-wave music-like input without decoding MP3 during benchmark
runs.

The raw fixture is intentionally headerless. Benchmarks that read it must set
the PCM format explicitly before building `AudioFrame` data.
