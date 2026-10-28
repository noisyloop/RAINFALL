// RAINFALL - Matrix rain meets trippy arcade. MIT licensed.
#include "game.hpp"
#include "input.hpp"
#include "render.hpp"
#include "theme.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <curses.h>
#include <thread>

static const char* USAGE =
    "usage: rainfall [--classic] [--zen]\n"
    "  --classic   force green-on-black (ignore the Omarchy theme)\n"
    "  --zen       rain only, no game; any key exits (screensaver mode)\n"
    "controls: <-/-> or h/l move, space phase, p pause, q quit\n";

int main(int argc, char** argv) {
    bool classic = false, zen = false;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--classic")) classic = true;
        else if (!std::strcmp(argv[i], "--zen")) zen = true;
        else if (!std::strcmp(argv[i], "-h") || !std::strcmp(argv[i], "--help")) return std::fputs(USAGE, stdout), 0;
        else if (!std::strcmp(argv[i], "-v") || !std::strcmp(argv[i], "--version")) return std::puts("rainfall 1.0.0"), 0;
        else return std::fprintf(stderr, "rainfall: unknown option '%s'\n%s", argv[i], USAGE), 2;
    }

    install_quit_handlers();
    screen_init(load_theme(classic));
    Rain rain;
    Game game;
    game.load_best();
    rain.resize(COLS, LINES);
    game.resize(COLS, LINES);
    game.reset();

    using clock = std::chrono::steady_clock;
    const auto frame = std::chrono::microseconds(1000000 / 30);
    auto last = clock::now(), next = last;
    float t = 0, zen_pulse = 0;
    bool running = true;

    while (running && !quit_requested()) {
        for (Act a; running && (a = read_key()) != Act::None;) {
            if (a == Act::Resize) rain.resize(COLS, LINES), game.resize(COLS, LINES);
            else running = zen ? false : game.handle(a);
        }
        auto now = clock::now();
        float dt = std::min(0.1f, std::chrono::duration<float>(now - last).count());
        last = now;

        bool too_small = COLS < 30 || LINES < 10;
        if (!game.paused && !too_small) {
            t += dt;
            rain.density = zen ? 0.6f : game.density();
            rain.update(dt, zen ? 1.f : game.speed());
            if (zen && frand(0, 1) < dt / 20.f) zen_pulse = 1;  // occasional dream-flash
            zen_pulse = std::max(0.f, zen_pulse - dt * 0.5f);
        }
        if (!zen && !too_small) game.update(dt);

        erase();
        if (too_small) center(LINES / 2, "enlarge the terminal", P_HUD, A_BOLD);
        else {
            rain.draw(t, zen ? zen_pulse : game.pulse);
            if (!zen) game.draw();
        }
        refresh();

        next += frame;
        if (next < now) next = now;  // don't try to catch up after a stall
        std::this_thread::sleep_until(next);
    }
    screen_end();
    if (!zen && game.score > game.best) game.best = game.score, game.save_best();
    return 0;
}
