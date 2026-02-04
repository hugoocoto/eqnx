MAJOR = 0
MINOR = 0
PATCH = 1
INFO = dev (pre-release)
VERSION = "\"$(MAJOR).$(MINOR).$(PATCH)-$(INFO)\""

OUT = eqnx
CC = gcc
OBJ_DIR = obj
PLUGINS_DIR = plugins
WAYLAND_OUT_PATH = wayland-protocol

WAYLAND_PROTOCOLS_DIR = $(shell pkg-config --variable=pkgdatadir wayland-protocols)
XDG_SHELL_XML = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml
XDG_DECOR_XML = $(WAYLAND_PROTOCOLS_DIR)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml

FLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-override-init -ggdb -rdynamic -fPIC 
LIBS = -lwayland-client -lwayland-cursor -lxkbcommon -lm -lfontconfig -ldl
INCLUDES = -I. -Isrc -I$(WAYLAND_OUT_PATH) -Ithirdparty

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,$(OBJ_DIR)/src/%.o,$(SRC))
HEADERS = $(wildcard src/*.h) $(WAYLAND_OUT_PATH)/xdg-shell-client-protocol.h $(WAYLAND_OUT_PATH)/xdg-decoration-unstable-v1.h $(DEPS) config.h
DEPS = thirdparty/minicoro.h thirdparty/stb_image_write.h thirdparty/stb_truetype.h thirdparty/stb_c_lexer.h

# Protocolos Wayland (Se compilan como objetos separados para el binario final)
WAYLAND_SOURCES = $(WAYLAND_OUT_PATH)/xdg-shell-protocol.c \
                  $(WAYLAND_OUT_PATH)/xdg-decoration-unstable-v1.c
WAYLAND_OBJ = $(patsubst $(WAYLAND_OUT_PATH)/%.c,$(OBJ_DIR)/protocol/%.o,$(WAYLAND_SOURCES))

# Plugins
PLUGINS_SO = $(patsubst plugins_src/%.c,$(PLUGINS_DIR)/%.so,$(wildcard plugins_src/*.c))

.PHONY: all compile install uninstall clean

all: compile

compile: $(WAYLAND_SOURCES) $(OUT) $(PLUGINS_SO)

$(OUT): $(OBJ) $(WAYLAND_OBJ) 
	$(CC) $(FLAGS) $^ -o $@ $(LIBS)

$(OBJ_DIR)/src/%.o: src/%.c makefile $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) $(INCLUDES) -c $< -o $@ -DVERSION=$(VERSION)

$(OBJ_DIR)/protocol/%.o: $(WAYLAND_OUT_PATH)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) $(INCLUDES) -c $< -o $@

$(WAYLAND_OUT_PATH)/xdg-shell-protocol.c: $(XDG_SHELL_XML)
	@mkdir -p $(WAYLAND_OUT_PATH)
	wayland-scanner private-code $< $@

$(WAYLAND_OUT_PATH)/xdg-shell-client-protocol.h: $(XDG_SHELL_XML)
	@mkdir -p $(WAYLAND_OUT_PATH)
	wayland-scanner client-header $< $@

$(WAYLAND_OUT_PATH)/xdg-decoration-unstable-v1.c: $(XDG_DECOR_XML)
	@mkdir -p $(WAYLAND_OUT_PATH)
	wayland-scanner private-code $< $@

$(WAYLAND_OUT_PATH)/xdg-decoration-unstable-v1.h: $(XDG_DECOR_XML)
	@mkdir -p $(WAYLAND_OUT_PATH)
	wayland-scanner client-header $< $@

plugins/%.so: plugins_src/%.c $(OBJ) $(wildchar plugins_src/*.h)
	@ mkdir -p plugins
	$(CC) $(FLAGS) $(INCLUDES) -shared $< -o $@

install: /usr/local/man/man1/eqnx.1
	# install -m 0755 $(OUT) /usr/local/bin/

/usr/local/man/man1/eqnx.1: eqnx.1
	sudo install -g 0 -o 0 -m 0644 $< /usr/local/man/man1/
	sudo gzip -f /usr/local/man/man1/eqnx.1

uninstall:
	rm -f /usr/local/man/man1/eqnx.1.gz
	rm -f /usr/local/bin/$(OUT)

clean:
	rm -rf $(WAYLAND_OUT_PATH) $(OBJ_DIR) $(OUT) $(PLUGINS_DIR)

-include $(OBJ:.o=.d)
-include $(WAYLAND_OBJ:.o=.d)

thirdparty/minicoro.h:
	@mkdir -p thirdparty
	wget https://raw.githubusercontent.com/edubart/minicoro/refs/heads/main/minicoro.h -O $@

thirdparty/stb_image_write.h:
	@mkdir -p thirdparty
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h -O $@

thirdparty/stb_truetype.h:
	@mkdir -p thirdparty
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_truetype.h -O $@

thirdparty/stb_c_lexer.h:
	@mkdir -p thirdparty
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_c_lexer.h -O $@

debug: compile
	gdb $(OUT) -ex "r -p ./esx/example.esx" 
