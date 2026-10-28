#pragma once
#include "theme.hpp"

#include <vector>

constexpr int HUES = 24, LEVELS = 4;  // rain colour table: 24 hues x 4 brightness levels
enum { P_HOT = 100, P_TRAIL, P_BAD, P_SHIP, P_PHASE, P_HUD };

void screen_init(const Theme& t);
void screen_end();

float frand(float lo, float hi);
int irand(int n);
wchar_t rand_glyph();
wchar_t glyph(wchar_t uni, char ascii);  // ascii fallback for non-UTF-8 locales

void put(int y, int x, wchar_t ch, int pair, unsigned attr = 0);
void text(int y, int x, const char* s, int pair, unsigned attr = 0);
void center(int y, const char* s, int pair, unsigned attr = 0);

struct Rain {
    struct Stream { float y, speed; int len; };
    int w = 0, h = 0;
    float density = 0.5f;  // 0..1, how quickly empty columns get a new stream
    std::vector<Stream> streams;
    std::vector<wchar_t> grid;  // glyph under every cell, slowly mutating

    void resize(int w, int h);
    void update(float dt, float speed);
    void draw(float t, float pulse) const;

private:
    void spawn(Stream& s, bool anywhere);
};
