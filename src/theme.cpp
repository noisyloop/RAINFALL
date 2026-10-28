#include "theme.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

using Colors = std::map<std::string, RGB>;

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\"'"), b = s.find_last_not_of(" \t\r\"'");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

// Accepts "#rrggbb", "0xrrggbb" or bare "rrggbb" (ghostty).
static bool parse_hex(std::string v, RGB& out) {
    size_t p = v.find('#');
    if (p != std::string::npos) v = v.substr(p + 1);
    else if (v.rfind("0x", 0) == 0) v = v.substr(2);
    if (v.size() < 6) return false;
    for (int i = 0; i < 6; ++i)
        if (!std::isxdigit(static_cast<unsigned char>(v[i]))) return false;
    long n = std::strtol(v.substr(0, 6).c_str(), nullptr, 16);
    out = {int(n >> 16 & 255), int(n >> 8 & 255), int(n & 255)};
    return true;
}

// Lenient reader for alacritty.toml, kitty.conf and ghostty.conf style files.
static void parse_file(const std::string& path, Colors& c) {
    static const std::map<std::string, std::string> alias = {
        {"red", "color1"}, {"green", "color2"}, {"yellow", "color3"},
        {"blue", "color4"}, {"magenta", "color5"}, {"cyan", "color6"}};
    std::ifstream f(path);
    std::string line, section;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[') { section = line; continue; }
        // Only [colors.primary] / [colors.normal] in alacritty; skip bright, cursor, ...
        if (!section.empty() && section.find("primary") == std::string::npos &&
            section.find("normal") == std::string::npos) continue;
        size_t p = line.find_first_of("= \t");
        if (p == std::string::npos) continue;
        std::string key = trim(line.substr(0, p)), val = trim(line.substr(p));
        if (!val.empty() && val[0] == '=') val = trim(val.substr(1));
        if (key == "palette") {  // ghostty: palette = 2=#9ece6a
            size_t e = val.find('=');
            if (e == std::string::npos) continue;
            key = "color" + trim(val.substr(0, e));
            val = val.substr(e + 1);
        }
        auto a = alias.find(key);
        if (a != alias.end()) key = a->second;
        RGB rgb;
        if (parse_hex(val, rgb)) c.emplace(key, rgb);  // first occurrence wins
    }
}

Theme load_theme(bool classic) {
    Theme t{{0, 255, 70}, {235, 255, 235}, {255, 40, 40}, {120, 255, 160}, {150, 230, 150},
            false, 16.f};
    const char* home = std::getenv("HOME");
    if (classic || !home) return t;

    std::string dir = std::string(home) + "/.config/omarchy/current/theme/";
    Colors c;
    for (const char* f : {"alacritty.toml", "colors.toml", "ghostty.conf", "kitty.conf"})
        parse_file(dir + f, c);

    auto get = [&](const char* k, RGB& o) {
        auto it = c.find(k);
        if (it == c.end()) return false;
        o = it->second;
        return true;
    };
    RGB fg;
    if (!get("foreground", fg) || !(get("accent", t.rain) || get("color2", t.rain))) return t;
    t.hot = {(fg.r + 255 * 3) / 4, (fg.g + 255 * 3) / 4, (fg.b + 255 * 3) / 4};
    t.hud = fg;
    get("color1", t.bad);
    if (!get("color6", t.ship)) t.ship = fg;
    t.default_bg = true;
    t.hue_spread = 55.f;
    return t;
}
