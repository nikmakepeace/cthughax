# Retired Debian Etch Build Note

This file used to describe a preservation build of `xcthugha` on Debian Etch
through the old configure-era build system.

That path is no longer part of the active project. CMake is now the only
supported build system:

```sh
cmake -S . -B build
cmake --build build
```

The Debian Etch notes were useful for historical archaeology, but they should
not be treated as current build instructions.
