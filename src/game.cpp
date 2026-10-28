#include "game.hpp"
#include "render.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <curses.h>
#include <fstream>
#include <string>
#include <sys/stat.h>

constexpr float PHASE_TIME = 0.9f, PHASE_COOLDOWN = 4.f, COMBO_WINDOW = 2.5f;

// High score lives in ~/.local/share/rainfall/highscore and nowhere else.
static std::string score_dir() {
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + "/.local/share/rainfall" : "";
}

void Game::load_best() {
    std::ifstream f(score_dir() + "/highscore");
    if (!(f >> best)) best = 0;
}

void Game::save_best() const {
    std::string dir = score_dir();
    if (dir.empty()) return;
    for (size_t p = dir.find('/', 1); ; p = dir.find('/', p + 1)) {  // mkdir -p
        mkdir(dir.substr(0, p).c_str(), 0755);
        if (p == std::string::npos) break;
    }
    std::ofstream(dir + "/highscore") << best << '\n';
}

void Game::reset() {
    lives = 3, score = combo = 0;
    t = phase = cooldown = invuln = combo_t = spawn = 0;
    paused = over = new_best = false;
    drops.clear();
    px = w / 2;
}

void Game::resize(int W, int H) {
    w = W, h = H;
    px = std::clamp(px, 1.f, std::max(1.f, w - 2.f));
    drops.erase(std::remove_if(drops.begin(), drops.end(), [&](const Drop& d) { return d.x >= w; }),
                drops.end());
}

bool Game::handle(Act a) {
    switch (a) {
    case Act::Quit: return false;
    case Act::Pause: if (!over) paused = !paused; break;
    case Act::Restart: if (over) reset(); break;
    default:
        if (paused || over) break;
        if (a == Act::Left) px = std::max(1.f, px - 2);
        if (a == Act::Right) px = std::min(w - 2.f, px + 2);
        if (a == Act::Phase && cooldown <= 0) phase = PHASE_TIME, cooldown = PHASE_COOLDOWN;
    }
    return true;
}

void Game::update(float dt) {
    pulse = std::max(0.f, pulse - dt * 1.1f);
    if (paused || over) return;
    t += dt;
    phase = std::max(0.f, phase - dt);
    cooldown = std::max(0.f, cooldown - dt);
    invuln = std::max(0.f, invuln - dt);
    if ((combo_t -= dt) <= 0) combo = 0;

    for (spawn += dt * (3.f + t / 12.f) * w / 80.f; spawn >= 1; spawn -= 1) {
        bool hot = frand(0, 1) > std::min(0.55f, 0.25f + t / 150.f);
        drops.push_back({float(irand(w)), -1, frand(7, 12) * speed(), hot, false,
                         hot ? rand_glyph() : glyph(L'✖', 'X')});
    }

    const int row = h - 2;
    for (auto& d : drops) {
        d.y += d.speed * dt;
        if (d.done || int(d.y) < row) continue;
        d.done = true;
        if (std::abs(int(d.x) - int(px)) > 1) continue;
        d.y = float(h + 10);  // consumed by the ship
        if (d.hot) {
            ++combo, combo_t = COMBO_WINDOW;
            score += 10 * (1 + combo / 5);
            if (combo % 5 == 0) pulse = 1;
        } else if (phase <= 0 && invuln <= 0) {
            combo = 0, invuln = 1.2f;
            if (--lives <= 0) {
                over = true;
                if (score > best) best = score, new_best = true, save_best();
            }
        }
    }
    drops.erase(std::remove_if(drops.begin(), drops.end(), [&](const Drop& d) { return d.y > h + 2; }),
                drops.end());
}

void Game::draw() const {
    for (const auto& d : drops) {
        int x = int(d.x), y = int(d.y), p = d.hot ? P_HOT : P_BAD;
        put(y - 1, x, rand_glyph(), d.hot ? P_TRAIL : P_BAD, A_DIM);
        put(y - 2, x, rand_glyph(), d.hot ? P_TRAIL : P_BAD, A_DIM);
        put(y, x, d.g, p, A_BOLD);
    }

    if (!(invuln > 0 && int(invuln * 12) % 2)) {
        int p = phase > 0 ? P_PHASE : P_SHIP, sx = int(px);
        unsigned a = phase > 0 ? A_DIM : A_BOLD;
        put(h - 2, sx - 1, glyph(L'▟', '/'), p, a);
        put(h - 2, sx, glyph(L'█', 'A'), p, a);
        put(h - 2, sx + 1, glyph(L'▙', '\\'), p, a);
    }

    for (int x = 0; x < w; ++x) put(h - 1, x, ' ', P_HUD);
    char buf[128];
    std::snprintf(buf, sizeof buf, " SCORE %-6d BEST %-6d COMBO x%-3d PHASE ", score, best, combo);
    text(h - 1, 0, buf, P_HUD, A_BOLD);
    int x = int(std::strlen(buf));
    if (cooldown <= 0) text(h - 1, x, "READY", P_PHASE, A_BOLD);
    else for (int i = 0; i < 5; ++i) put(h - 1, x + i, glyph(i < 5 - cooldown / PHASE_COOLDOWN * 5 ? L'■' : L'□', '#'), P_PHASE);
    for (int i = 0; i < 3; ++i) put(h - 1, w - 7 + 2 * i, glyph(i < lives ? L'♥' : L'♡', i < lives ? '@' : '.'), P_BAD, A_BOLD);

    int cy = h / 2;
    if (paused) {
        center(cy, "                         ", P_HUD, A_REVERSE);
        center(cy, "  PAUSED  -  p to resume  ", P_HUD, A_REVERSE | A_BOLD);
    }
    if (over) {
        std::snprintf(buf, sizeof buf, "   score %d   best %d%s   ", score, best, new_best ? "  NEW BEST!" : "");
        center(cy - 2, "   G A M E   O V E R   ", P_BAD, A_REVERSE | A_BOLD);
        center(cy, buf, P_HOT, A_BOLD);
        center(cy + 2, "   r: again    q: quit   ", P_HUD, A_REVERSE);
    }
}
