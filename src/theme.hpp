#pragma once

struct RGB { int r, g, b; };

struct Theme {
    RGB rain, hot, bad, ship, hud;
    bool default_bg;   // let the terminal's own (themed) background show through
    float hue_spread;  // degrees of hue drift across columns
};

// Reads ~/.config/omarchy/current/theme/; falls back to green-on-black.
Theme load_theme(bool classic);
