#pragma once
#include "input.hpp"

#include <vector>

struct Drop {
    float x, y, speed;
    bool hot, done;  // hot = white-hot (collect), else corrupt
    wchar_t g;
};

struct Game {
    int w = 0, h = 0, lives = 3, score = 0, best = 0, combo = 0;
    float px = 0, t = 0, phase = 0, cooldown = 0, invuln = 0, pulse = 0, combo_t = 0, spawn = 0;
    bool paused = false, over = false, new_best = false;
    std::vector<Drop> drops;

    void reset();
    void resize(int w, int h);
    bool handle(Act a);  // false -> quit
    void update(float dt);
    void draw() const;
    float speed() const { return 1.f + t / 40.f; }
    float density() const { return t / 120.f > 0.6f ? 0.95f : 0.35f + t / 120.f; }
    void load_best();
    void save_best() const;
};
