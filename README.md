# RAINFALL

**Matrix rain meets trippy arcade.** A tiny terminal game for [Omarchy](https://omarchy.org)
(Arch Linux + Hyprland): katakana streams cascade down your screen, hue-drifting
across the columns in your current Omarchy theme colours. Catch the white-hot
glyphs, dodge the red corrupt ones, and chain combos to set off a screen-wide
colour pulse.

- One small C++17 binary; the only dependency is `ncursesw`
- 30 FPS at a few % CPU. No network, no login, no telemetry
- Only writes its high score, to `~/.local/share/rainfall/highscore`

## Install

```sh
git clone https://github.com/noisyloop/rainfall && cd rainfall && make && make install
```

This puts `rainfall` in `~/.local/bin` and `rainfall.desktop` in
`~/.local/share/applications`, so it appears in the Omarchy app launcher
(`SUPER + SPACE`, type "Rainfall"). You don't need sudo.

`./install.sh` does the same thing and checks build dependencies first
(`sudo pacman -S --needed base-devel ncurses` if anything is missing).

Uninstall with `make uninstall` (or `./install.sh uninstall`). Your high score
stays behind; run `rm -rf ~/.local/share/rainfall` to wipe it.

A `PKGBUILD` is included for building a pacman package: `makepkg -si`.

## Controls

| Key            | Action                                           |
|----------------|--------------------------------------------------|
| `←` `→` / `h` `l` | Move the ship                                 |
| `Space`        | Phase: pass through corrupt rain for ~1 s (4 s cooldown) |
| `p`            | Pause / resume                                   |
| `r`            | Restart after game over                          |
| `q` / `Esc`    | Quit (Ctrl+C also exits cleanly)                 |

**Rules:** white-hot glyphs give points, and every 5-catch combo raises the
multiplier and fires a colour pulse. Your combo lapses if you go 2.5 s without
a catch. Red `✖` glyphs cost a life, and you have 3. Speed and density keep
ramping up.

## Flags

| Flag        | Effect                                                    |
|-------------|-----------------------------------------------------------|
| `--classic` | Force classic green-on-black and ignore the Omarchy theme |
| `--zen`     | Rain only, no game. Any key exits, so it works as a screensaver |
| `--help`    | Usage                                                     |

## Theming

On launch RAINFALL reads `~/.config/omarchy/current/theme/` (`alacritty.toml`,
falling back to `colors.toml`, `ghostty.conf` or `kitty.conf`):

- rain = theme green (or `accent`)
- corrupt = theme red
- ship = theme cyan
- HUD / white-hot = theme foreground

The terminal's own themed background shows through. When no theme is found,
you get classic green-on-black. Switch themes with Omarchy and relaunch to
pick up the new colours.

## Hyprland keybind (optional)

Add to `~/.config/hypr/bindings.conf`:

```
bind = SUPER SHIFT, R, exec, $terminal -e rainfall
```

Or as a screensaver-style toy:

```
bind = SUPER SHIFT, Z, exec, $terminal -e rainfall --zen
```

## Code layout

```
src/main.cpp     args, frame loop
src/theme.cpp    Omarchy theme parsing
src/render.cpp   terminal lifecycle, colour table, glyphs, rain
src/input.cpp    key mapping, Ctrl+C handling
src/game.cpp     ship, drops, scoring, HUD, high score
```

Build flags are `-std=c++17 -Wall -Wextra`, and it compiles with zero warnings.

## License

MIT, see [LICENSE](LICENSE).
