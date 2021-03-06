## Copyright (c) 2015 Ahmad Fatoum
CROSS_COMPILE ?= arm-linux-gnueabi-
PREFIX ?= $(CROSS_COMPILE)
CC = $(PREFIX)gcc
AR = $(PREFIX)ar
SRCS = $(wildcard src/*.c) $(wildcard src/**/*.c) $(wildcard firmware_headers/**/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
CFLAGS += -fno-strict-aliasing -fwrapv
CFLAGS += -Wall -Wextra -Wpointer-sign -Wno-unused-parameter

.DEFAULT: libev3api.a
libev3api.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) -Os $(CFLAGS) -c $< -o $@ 

example:
	echo 'int main(void) { return EV3IsInitialized() == 1; }' | $(CC) -xc $(CFLAGS) - -L. -lev3api -I. -oexample -include ev3.h

#ifeq ($(OS),Windows_NT)
#RM = del /Q
#endif

.PHONY: clean
clean:
	$(RM) src/*.o *.a *.d src/ev3_sensors/*.o src/ev3_inputs/*.o copied/bluetooth/*.o example
