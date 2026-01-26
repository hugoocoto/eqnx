WAYLAND_XDGSHELL_PATH = /usr/share/wayland-protocols/stable/xdg-shell
WAYLAND_OUT_PATH = wayland-protocol
OBJ_DIR = obj
OUT = eqnx

SRC = $(wildcard src/*.c) $(WAYLAND_OUT_PATH)/xdg-shell-protocol.c
HEADERS = $(wildcard src/*.h) $(WAYLAND_OUT_PATH)/xdg-shell-client-protocol.h \
thirdparty/minicoro.h thirdparty/stb_image_write.h thirdparty/stb_truetype.h config.h

OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))

CC = gcc
FLAGS = -Wall -Wextra \
  -Wno-unused-parameter -Wno-unused-function -Wno-override-init \
  -ggdb -rdynamic
LIBS = -lrt -lwayland-client -lxkbcommon -lm -lfontconfig

.PHONY: all compile install uninstall clean

all: compile

compile: \
	$(WAYLAND_OUT_PATH)/xdg-shell-protocol.c \
	$(WAYLAND_OUT_PATH)/xdg-shell-client-protocol.h \
	$(OUT) \
	plugins/plugin.so plugins/plugin2.so

$(OUT): $(OBJ) wc.txt
	$(CC) $(FLAGS) $(OBJ) -o $(OUT) $(LIBS)

$(OBJ_DIR)/%.o: %.c $(HEADERS) makefile
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) -c $< -o $@

plugins/plugin.so: plugins/plugin.c $(HEADERS) $(OBJ)
	$(CC) $(FLAGS) -fPIC -shared $< -o $@

plugins/plugin2.so: plugins/plugin2.c $(HEADERS) $(OBJ)
	$(CC) $(FLAGS) -fPIC -shared $< -o $@

$(WAYLAND_OUT_PATH)/xdg-shell-protocol.c: \
	$(WAYLAND_XDGSHELL_PATH)/xdg-shell.xml
	@mkdir -p $(WAYLAND_OUT_PATH)
	wayland-scanner private-code $< $@

$(WAYLAND_OUT_PATH)/xdg-shell-client-protocol.h: \
	$(WAYLAND_XDGSHELL_PATH)/xdg-shell.xml
	@mkdir -p $(WAYLAND_OUT_PATH)
	wayland-scanner client-header $< $@

install: /usr/local/man/man1/equinox.1

/usr/local/man/man1/equinox.1: equinox.1
	sudo install -g 0 -o 0 -m 0644 $< /usr/local/man/man1/
	sudo gzip -f /usr/local/man/man1/equinox.1

uninstall:
	rm -rf /usr/local/man/man1/equinox*

clean:
	rm -rf \
		$(WAYLAND_OUT_PATH) \
		$(OBJ_DIR) \
		$(OUT) \
		plugins/plug.so

wc.txt: $(SRC) $(HEADERS)
	@ cloc `find src plugins -name "*.c" -o -name "*.h"` > wc.txt 2>/dev/null || \
	wc `find src plugins -name "*.c" -o -name "*.h"` > wc.txt

thirdparty/minicoro.h:
	@ mkdir -p thirdparty
	wget https://raw.githubusercontent.com/edubart/minicoro/refs/heads/main/minicoro.h -O thirdparty/minicoro.h

thirdparty/stb_image_write.h:
	@ mkdir -p thirdparty
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h -O thirdparty/stb_image_write.h

thirdparty/stb_truetype.h: 
	@ mkdir -p thirdparty
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_truetype.h -O thirdparty/stb_truetype.h

