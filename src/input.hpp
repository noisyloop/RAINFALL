#pragma once

enum class Act { None, Left, Right, Phase, Pause, Quit, Restart, Resize, Other };

void install_quit_handlers();  // SIGINT/SIGTERM/SIGHUP -> clean shutdown
bool quit_requested();
Act read_key();                // non-blocking; Act::None when the queue is empty
