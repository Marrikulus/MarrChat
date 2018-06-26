
ifeq ($(OS),Windows_NT)
	$(error This is not ready for windows)
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S), Linux)
		PRE := sudo apt install libglew-dev libsdl2-dev libglm-dev libsdl2-ttf-dev -y
	else
		PRE := brew install sdl2 sdl2_ttf glew glm
	endif
endif


linkerDebugFlags= -Wl,-V

linkerFlags = -lRakNetLibStatic -lpthread -lSDL2 -lGLEW -lGL -lSDL2_ttf

compilerFlags = -Wall -std=c++11

default: client server

client:
	g++ Source/Client.cpp -g $(compilerFlags) -IInclude -LLibraries -D_REENTRANT -L/usr/lib/x86_64-linux-gnu $(linkerFlags) -o bin/client

server:
	g++ Source/Server.cpp -IInclude -LLibraries $(linkerFlags) -o bin/server

pre:
	$(PRE)
