# CthughaNix - An Oscilloscope on Acid

## Relicensing and the Future

This is a relicensing, renaming, and modernization of the Cthugha-L
distribution. Much remains to be done, though if you are using Linux/x86 you
should be able to enjoy Cthugha immediately.

Please see the `TODO` file for more information.

For details on the new licence, see the `COPYING` file.

## How to Install

See `INSTALL.md`.

## Some Tips

- `xcthugha` does not react to `-geometry` in the normal way. Use the `-D` and
  `--position` options.

- Take a look at the documentation. I know it is quite long and not very well
  written, but it is still useful.

- Example of nice full-screen options for X:

  ```sh
  ./xcthugha -D 5 --full-screen --play /export/home/brandon/Shared/Beach\ Boys\ -\ Good\ Vibrations.mp3
  ```

## Bugs

See the `TODO.md` file. Known bugs and problems are usually listed there.


If you have problems with sound, such as sound reading errors, use the
`--snd-method` option. Setting a value of `3` should work.
