# Nestruts

An NES undulat-or.

## You need

* Meson
* A recent-ish C++ compiler
* SDL2

## Build and run

```
meson setup build
cd build
ninja
```

Run with `./nestruts <path_to_rom>`.

## Playing

Use the arrow keys for the D-pad. The A and B buttons are mapped to `x` and `z`. Start and select are assigned
`enter` and `shift`.

`Spacebar` enables debug mode.

## Supported games

Only game that is known to work is Donkey Kong.
