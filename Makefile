SOURCES = main.cpp src/chip8.cpp
TARGET = build/chip8
LFLAGS = -lSDL2

all: $(SOURCES)
	@mkdir -p build
	g++ $(SOURCES) $(LFLAGS) -o $(TARGET)

clean: 
	rm -rf build

run-pong: $(TARGET)
					$(TARGET) ROMs/PONG
	
run-tetris: $(TARGET)
					$(TARGET) ROMs/TETRIS
.PHONY: all clean run-pong run-tetris
