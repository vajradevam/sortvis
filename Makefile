CC = gcc
CFLAGS = -Wall -Wextra -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf -lm
SRCS = src/main.c src/sorter.c src/renderer.c
OBJS = main.o sorter.o renderer.o
TARGET = sortvis

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: src/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

sorter.o: src/sorter.c
	$(CC) $(CFLAGS) -c -o $@ $<

renderer.o: src/renderer.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
