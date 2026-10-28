#include "input.hpp"

#include <csignal>
#include <initializer_list>
#include <curses.h>

static volatile std::sig_atomic_t g_quit = 0;

static void on_quit(int) { g_quit = 1; }

void install_quit_handlers() {
    for (int s : {SIGINT, SIGTERM, SIGHUP}) std::signal(s, on_quit);
}

bool quit_requested() { return g_quit != 0; }

Act read_key() {
    wint_t ch;
    int r = get_wch(&ch);
    if (r == ERR) return Act::None;
    if (r == KEY_CODE_YES) {
        switch (ch) {
        case KEY_LEFT: return Act::Left;
        case KEY_RIGHT: return Act::Right;
        case KEY_RESIZE: return Act::Resize;
        default: return Act::Other;
        }
    }
    switch (ch) {
    case 'h': case 'a': return Act::Left;
    case 'l': case 'd': return Act::Right;
    case ' ': return Act::Phase;
    case 'p': case 'P': return Act::Pause;
    case 'r': case 'R': return Act::Restart;
    case 'q': case 'Q': case 27: return Act::Quit;
    default: return Act::Other;
    }
}
