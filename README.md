# Corne Wireless View ZMK Config

ZMK configuration for my Corne Wireless keyboard. Hardware use for the current build:

- Corne choc PCB with hotswap sockets from [typeractive](https://typeractive.xyz/)
- Milled aluminum case from [typeractive](https://typeractive.xyz/)
- NiceNano Controller
- NiceView displays

## Keymap reference

Visual reference from `config/corne.keymap`. Keycodes follow [ZMK keycodes](https://zmk.dev/docs/keymaps/list-of-keycodes).

### Layer: Base

```
┌───────┬───────┬───────┬───────┬───────┬───────┐ ┌───────┬───────┬───────┬───────┬───────┬───────┐
│  TAB  │   Q   │   W   │   E   │   R   │   T   │ │   Y   │   U   │   I   │   O   │   P   │  BSPC │
├───────┼───────┼───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┼───────┼───────┤
│Ctl+Esc│   A   │ Alt+S │ Cmd+D │ Shf+F │   G   │ │   H   │ Shf+J │ Cmd+K │ Alt+L │  SEMI │   '   │
├───────┼───────┼───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┼───────┼───────┤
│Shf+Cap│   Z   │   X   │   C   │   V   │   B   │ │   N   │   M   │   ,   │   .   │   /   │Shf+Ret│
└───────┴───────┴───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┴───────┴───────┘
                        │  TD0  │Layer 1│   Sp  │ │ L3+Sp │Layer 2│   RA  │
                        └───────┴───────┴───────┘ └───────┴───────┴───────┘
```

### Layer: Lower

```
┌───────┬───────┬───────┬───────┬───────┬───────┐ ┌───────┬───────┬───────┬───────┬───────┬───────┐
│   ~   │   !   │   @   │   #   │   $   │   %   │ │   ^   │   &   │   *   │   (   │   )   │       │
├───────┼───────┼───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┼───────┼───────┤
│       │       │       │       │       │       │ │   |   │       │   +   │   {   │   }   │   `   │
├───────┼───────┼───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┼───────┼───────┤
│       │       │       │       │       │       │ │  BSLH │   _   │   =   │   [   │   ]   │   /   │
└───────┴───────┴───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┴───────┴───────┘
                        │       │       │       │ │       │       │       │
                        └───────┴───────┴───────┘ └───────┴───────┴───────┘
```

### Layer: Raise

```
┌───────┬───────┬───────┬───────┬───────┬───────┐ ┌───────┬───────┬───────┬───────┬───────┬───────┐
│       │   1   │   2   │   3   │   4   │   5   │ │   6   │   7   │   8   │   9   │   0   │       │
├───────┼───────┼───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┼───────┼───────┤
│       │  BT0  │  BT1  │  BT2  │  BT3  │  BT4  │ │       │   4   │   5   │   6   │       │  Clr  │
├───────┼───────┼───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┼───────┼───────┤
│       │       │       │       │       │       │ │       │   1   │   2   │   3   │       │       │
└───────┴───────┴───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┴───────┴───────┘
                        │       │       │       │ │       │       │       │
                        └───────┴───────┴───────┘ └───────┴───────┴───────┘
```

### Layer: Fun

```
┌───────┬───────┬───────┬───────┬───────┬───────┐ ┌───────┬───────┬───────┬───────┬───────┬───────┐
│       │       │  Prev │  Play │  Next │       │ │       │       │       │       │       │       │
├───────┼───────┼───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┼───────┼───────┤
│       │       │  Mute │ VolDn │ VolUp │       │ │   ←   │   ↓   │   ↑   │   →   │       │       │
├───────┼───────┼───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┼───────┼───────┤
│       │       │       │       │       │       │ │       │       │       │       │       │       │
└───────┴───────┴───────┼───────┼───────┼───────┤ ├───────┼───────┼───────┼───────┴───────┴───────┘
                        │       │       │       │ │       │       │       │
                        └───────┴───────┴───────┘ └───────┴───────┴───────┘
```

Legend: empty cell = transparent (pass-through); `Layer 1`/`Layer 2` = momentary layer; `L3+Sp` = layer-tap (hold = layer 3, tap = Space); `Alt+S`, `Cmd+D`, `Shf+J` = mod-tap (hold modifier, tap key); `BT0`–`BT4` = Bluetooth profile; `Clr` = clear BT.

## How to update the layout

1. Use the [nickcoutsos.github.io/keymap-editor](https://nickcoutsos.github.io/keymap-editor/) to load this repository,
2. Edit the layout.
3. Use the option to `View keymap data` and copy the output.
4. Modify the local file and commit.
5. To refresh the keymap diagrams in this README, run `python3 scripts/keymap-to-ascii.py` and paste the output into the Keymap reference section above. Edit `scripts/keycode-labels.json` to change how keycodes appear (e.g. new ones).
