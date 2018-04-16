.POSIX:
.PHONY: all
CFLAGS := -Wall -Werror
LDFLAGS := -lvterm -lcairo
all: termimg
