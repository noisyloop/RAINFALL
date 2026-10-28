#include "render.hpp"

#include <algorithm>
#include <clocale>
#include <cmath>
#include <csignal>
#include <initializer_list>
#include <cstdlib>
#include <cstring>
#include <curses.h>
#include <random>
#include <termios.h>
#include <unistd.h>

static std::mt19937 g_rng{std::random_device{}()};
static bool g_utf8 = false;
static float g_base_hue = 120.f, g_spread = 16.f;
static termios g_saved;
static bool g_have_saved = false;

float frand(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(g_rng); }
int irand(int n) { return n > 0 ? int(g_rng() % unsigned(n)) : 0; }

wchar_t glyph(wchar_t uni, char ascii) { return g_utf8 ? uni : wchar_t(ascii); }

wchar_t rand_glyph() {
    static const char sym[] = "+-*=<>:|\"^~";
    int r = irand(100);
    if (r < 20) return L'0' + irand(10);
    if (r < 35) return wchar_t(sym[irand(sizeof sym - 1)]);
    if (!g_utf8) return L'!' + irand(94);
    return 0xFF66 + irand(56);  // half-width katakana, one cell wide
}

// --- colour ---------------------------------------------------------------

static RGB hsv(float h, float s, float v) {
    h = std::fmod(std::fmod(h, 360.f) + 360.f, 360.f) / 60.f;
    float c = v * s, x = c * (1 - std::fabs(std::fmod(h, 2.f) - 1)), m = v - c;
    float r = 0, g = 0, b = 0;
    switch (int(h)) {
    case 0: r = c, g = x; break;
    case 1: r = x, g = c; break;
    case 2: g = c, b = x; break;
    case 3: g = x, b = c; break;
    case 4: r = x, b = c; break;
    default: r = c, b = x; break;
    }
    return {int((r + m) * 255), int((g + m) * 255), int((b + m) * 255)};
}

static void to_hsv(RGB c, float& h, float& s, float& v) {
    float r = c.r / 255.f, g = c.g / 255.f, b = c.b / 255.f;
    float mx = std::max({r, g, b}), d = mx - std::min({r, g, b});
    v = mx;
    s = mx > 0 ? d / mx : 0;
    if (d == 0) h = 0;
    else if (mx == r) h = 60 * std::fmod((g - b) / d + 6, 6.f);
    else if (mx == g) h = 60 * ((b - r) / d + 2);
    else h = 60 * ((r - g) / d + 4);
}

// Nearest xterm-256 cube entry, or nearest of the 8 ANSI colours on small terminals.
static short color_index(RGB c) {
    if (COLORS >= 256) {
        auto q = [](int v) { return v < 48 ? 0 : v < 115 ? 1 : (v - 35) / 40; };
        return short(16 + 36 * q(c.r) + 6 * q(c.g) + q(c.b));
    }
    int mx = std::max({c.r, c.g, c.b, 1}), best = 7, bd = 1 << 30;
    for (int i = 1; i < 8; ++i) {  // compare by hue: normalise to full brightness
        int dr = c.r * 255 / mx - (i & 1) * 255, dg = c.g * 255 / mx - (i >> 1 & 1) * 255,
            db = c.b * 255 / mx - (i >> 2 & 1) * 255, d = dr * dr + dg * dg + db * db;
        if (d < bd) bd = d, best = i;
    }
    return short(best);
}

static int rain_pair(float hue, int level) {
    int h = int(std::lround(std::fmod(std::fmod(hue, 360.f) + 360.f, 360.f) / (360.f / HUES)));
    return 1 + (h % HUES) * LEVELS + level;
}

// --- terminal lifecycle ---------------------------------------------------

static void on_crash(int sig) {  // async-signal-safe best effort
    if (g_have_saved) tcsetattr(STDIN_FILENO, TCSANOW, &g_saved);
    static const char seq[] = "\033[0m\033[?25h\033[?1049l\r\n";
    if (write(STDOUT_FILENO, seq, sizeof seq - 1) < 0) {}
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void screen_init(const Theme& t) {
    std::setlocale(LC_ALL, "");
    g_utf8 = MB_CUR_MAX > 1;
    g_have_saved = tcgetattr(STDIN_FILENO, &g_saved) == 0;
    for (int s : {SIGSEGV, SIGABRT, SIGFPE, SIGBUS, SIGILL}) std::signal(s, on_crash);

    initscr();
    if (!std::getenv("ESCDELAY")) set_escdelay(25);
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);
    if (!has_colors()) return;
    start_color();

    short bg = COLORS >= 256 ? 16 : COLOR_BLACK;  // 16 = true black in the 256 cube
    if (t.default_bg && use_default_colors() == OK) bg = -1;
    float s, v;
    to_hsv(t.rain, g_base_hue, s, v);
    v = std::max(v, 0.8f);
    g_spread = t.hue_spread;
    static const float sat[LEVELS] = {0.3f, 1.f, 1.f, 1.f}, val[LEVELS] = {1.f, 1.f, .62f, .36f};
    for (int h = 0; h < HUES; ++h)
        for (int l = 0; l < LEVELS; ++l)
            init_pair(short(1 + h * LEVELS + l), color_index(hsv(h * 360.f / HUES, s * sat[l], v * val[l])), bg);
    init_pair(P_HOT, color_index(t.hot), bg);
    init_pair(P_TRAIL, color_index({t.hot.r * 3 / 5, t.hot.g * 3 / 5, t.hot.b * 3 / 5}), bg);
    init_pair(P_BAD, color_index(t.bad), bg);
    init_pair(P_SHIP, color_index(t.ship), bg);
    init_pair(P_PHASE, color_index(hsv(g_base_hue + 180, 0.6f, 1)), bg);
    init_pair(P_HUD, color_index(t.hud), bg);
    bkgd(' ' | COLOR_PAIR(P_HUD));
}

void screen_end() {
    if (!isendwin()) endwin();
}

// --- drawing --------------------------------------------------------------

void put(int y, int x, wchar_t ch, int pair, unsigned attr) {
    if (y < 0 || x < 0 || y >= LINES || x >= COLS) return;
    wchar_t s[2] = {ch, 0};
    cchar_t cc;
    setcchar(&cc, s, attr, short(pair), nullptr);
    mvadd_wch(y, x, &cc);
}

void text(int y, int x, const char* s, int pair, unsigned attr) {
    for (; *s; ++s, ++x) put(y, x, wchar_t(*s), pair, attr);
}

void center(int y, const char* s, int pair, unsigned attr) {
    text(y, (COLS - int(std::strlen(s))) / 2, s, pair, attr);
}

// --- rain -----------------------------------------------------------------

void Rain::spawn(Stream& s, bool anywhere) {
    s.len = 4 + irand(std::max(4, h * 2 / 3));
    s.speed = frand(7.f, 20.f);
    s.y = anywhere ? frand(-float(h), float(h)) : -frand(0.f, h * (1.6f - density));
}

void Rain::resize(int W, int H) {
    w = std::max(W, 1), h = std::max(H, 1);
    streams.assign(w, Stream{});
    for (auto& s : streams) spawn(s, true);
    grid.resize(size_t(w) * h);
    for (auto& g : grid) g = rand_glyph();
}

void Rain::update(float dt, float speed) {
    for (auto& s : streams) {
        s.y += s.speed * speed * dt;
        if (s.y - s.len > h) spawn(s, false);
    }
    for (int i = int(grid.size()) / 40 + 1; i > 0; --i) grid[irand(int(grid.size()))] = rand_glyph();
}

void Rain::draw(float t, float pulse) const {
    for (int x = 0; x < w; ++x) {
        const Stream& s = streams[x];
        int head = int(s.y);
        float hue = g_base_hue + g_spread * std::sin(x * 0.07f + t * 0.3f) + pulse * (x * 9.f + t * 300.f);
        for (int y = std::max(0, head - s.len); y <= std::min(h - 1, head); ++y) {
            int d = head - y;
            int lvl = d == 0 ? 0 : d <= s.len / 4 ? 1 : d <= s.len * 3 / 5 ? 2 : 3;
            if (pulse > 0.25f && lvl > 1) --lvl;
            unsigned a = lvl == 0 ? A_BOLD : lvl == 3 ? A_DIM : A_NORMAL;
            put(y, x, grid[size_t(y) * w + x], rain_pair(hue, lvl), a);
        }
    }
}
