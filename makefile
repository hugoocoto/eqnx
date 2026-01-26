#
# eqnx makefile
#

MAJOR = 0
MINOR = 0
PATCH = 1 
INFO = dev (pre-release)
VERSION = "\"$(MAJOR).$(MINOR).$(PATCH)-$(INFO)\""

OUT = eqnx

CC = gcc
FLAGS = -Wall -Wextra -Wno-unused-parameter -ggdb -rdynamic -fPIC
LIBS = -lrt -lwayland-client -lxkbcommon -lm -lfontconfig

# Hardcoded path: fix this if it's possible
WAYLAND_XDGSHELL_PATH = /usr/share/wayland-protocols/stable/xdg-shell

SRC = $(wildcard src/*.c) $(WAYLAND_OUT_PATH)/xdg-shell-protocol.c
OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
HEADERS = $(WAYLAND_OUT_PATH)/xdg-shell-client-protocol.h \
  $(wildcard src/*.h thirdparty/*.h) config.h

OBJ_DIR = obj
WAYLAND_OUT_PATH = wayland-protocol

.PHONY: all compile install uninstall clean

all: compile
compile: \
	$(WAYLAND_OUT_PATH)/xdg-shell-protocol.c \
	$(WAYLAND_OUT_PATH)/xdg-shell-client-protocol.h \
	$(OUT) \
	plugins/plugin.so plugins/plugin2.so

$(OUT): $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o $(OUT) $(LIBS) 

$(OBJ_DIR)/%.o: %.c $(HEADERS) makefile
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) -c $< -o $@ -DVERSION=$(VERSION)

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

install: /usr/local/man/man1/eqnx.1
	@ # Todo: install eqnx

/usr/local/man/man1/eqnx.1: eqnx.1
	sudo install -g 0 -o 0 -m 0644 $< /usr/local/man/man1/
	sudo gzip -f /usr/local/man/man1/eqnx.1

uninstall:
	rm -rf /usr/local/man/man1/eqnx*

clean:
	rm -rf \
		$(WAYLAND_OUT_PATH) \
		$(OBJ_DIR) \
		$(OUT) \
		plugins/plug.so

# You can update deps by removing them from ./thirdparty/ and building all again

thirdparty/minicoro.h:
	@ mkdir -p thirdparty
	wget https://raw.githubusercontent.com/edubart/minicoro/refs/heads/main/minicoro.h -O thirdparty/minicoro.h

thirdparty/stb_image_write.h:
	@ mkdir -p thirdparty
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h -O thirdparty/stb_image_write.h

thirdparty/stb_truetype.h: 
	@ mkdir -p thirdparty
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_truetype.h -O thirdparty/stb_truetype.h
