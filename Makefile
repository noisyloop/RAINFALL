# RAINFALL - make && make install  (installs to ~/.local, no sudo)
PREFIX  ?= $(HOME)/.local
BINDIR  ?= $(PREFIX)/bin
APPDIR  ?= $(PREFIX)/share/applications
DESTDIR ?=

CXX      ?= g++
CXXFLAGS ?= -O2
CXXFLAGS += -std=c++17 -Wall -Wextra -DNCURSES_WIDECHAR=1 -D_XOPEN_SOURCE_EXTENDED \
            $(shell pkg-config --cflags ncursesw 2>/dev/null)
LDLIBS   += $(shell pkg-config --libs ncursesw 2>/dev/null || echo -lncursesw)

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
HDR := $(wildcard src/*.hpp)

# Launch through xdg-terminal-exec (Omarchy's default) when present.
TERMEXEC := $(shell command -v xdg-terminal-exec 2>/dev/null)

.PHONY: all clean install uninstall

all: rainfall

rainfall: $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.cpp $(HDR)
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

install: rainfall
	install -Dm755 rainfall "$(DESTDIR)$(BINDIR)/rainfall"
	@mkdir -p "$(DESTDIR)$(APPDIR)"
	@{ echo "[Desktop Entry]"; \
	   echo "Type=Application"; \
	   echo "Name=Rainfall"; \
	   echo "Comment=Matrix rain meets trippy arcade"; \
	   echo "Icon=utilities-terminal"; \
	   echo "Categories=Game;ArcadeGame;"; \
	   if [ -n "$(TERMEXEC)" ]; then \
	     echo "Exec=xdg-terminal-exec $(BINDIR)/rainfall"; echo "Terminal=false"; \
	   else \
	     echo "Exec=$(BINDIR)/rainfall"; echo "Terminal=true"; \
	   fi; } > "$(DESTDIR)$(APPDIR)/rainfall.desktop"
	@chmod 644 "$(DESTDIR)$(APPDIR)/rainfall.desktop"
	-@command -v update-desktop-database >/dev/null && update-desktop-database "$(DESTDIR)$(APPDIR)" 2>/dev/null || true
	@echo "installed $(BINDIR)/rainfall and $(APPDIR)/rainfall.desktop"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/rainfall" "$(DESTDIR)$(APPDIR)/rainfall.desktop"
	-@command -v update-desktop-database >/dev/null && update-desktop-database "$(DESTDIR)$(APPDIR)" 2>/dev/null || true
	@echo "removed. high score kept in ~/.local/share/rainfall (rm -rf it to wipe)"

clean:
	rm -rf build rainfall
