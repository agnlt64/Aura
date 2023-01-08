UNAME_S = $(shell uname -s)

CC := g++-12
CFLAGS := -std=c++17 -g -Wno-deprecated -I./include/

ifeq ($(UNAME_S), Darwin)
	LDFLAGS = /opt/homebrew/lib/libX11.dylib
else
	LDFLAGS = -lX11
endif

SRC := $(shell find src -name "*.cpp")
OBJS := $(SRC:.cpp=.o)

EXEC := aura-wm

$(EXEC): $(OBJS)
	mkdir -p bin/
	mkdir -p logs/
	$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) -o bin/$(EXEC)

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm bin/*
	rm -rf bin/
	rm src/*.o

cleanlogs:
	rm logs/*
	rm -rf logs/
