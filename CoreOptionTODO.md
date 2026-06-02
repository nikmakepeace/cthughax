# CoreOption TODO

Work from highest severity to lowest. After each issue is fixed, verify, strike it through here, and commit the fix.

- [ ] **High:** `CoreOption` is doing too much: option value, entry catalog, global registry, random-change target, history/hotkey store, ini participant, and asset loader. The constructor self-registers every instance globally, and autochange randomly picks from that hidden registry. Adding a new `CoreOption` can silently change runtime behavior.
- [ ] **High:** Settings parsing is bidirectionally coupled. `IniFiles.cc` reads `long_options`, calls `do_param`, then separately calls `CoreOption::getIniInitials()`. Meanwhile `CoreOption` calls `getini`/`putini` directly. CLI, ini, runtime defaults, and option catalogs are not cleanly separable.
- [ ] **Medium:** `CoreOption` owns filesystem/catalog loading. The gzip/path/directory scan logic lives in `CoreOption`, and image/object/palette loaders call into it. That makes a setting responsible for external asset discovery.
- [ ] **Medium:** Value changes have display side effects. Any `CoreOption::change()` can reset display FPS through global `cthughaDisplay`. That is a runtime lifecycle dependency hiding inside the option primitive.
- [ ] **Medium:** Initial values are staged as fixed text buffers. `setInitialEntry()` uses `strncpy` without guaranteed termination, then startup resolves all of those strings later. This is both a coupling issue and a small correctness hazard.
- [ ] **Low/Medium:** `changeRandom()` can modulo by zero before `change()` gets its empty-entry guard. Probably unreachable in normal startup, but fragile.
