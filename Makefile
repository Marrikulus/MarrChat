
linkerDebugFlags= -Wl,-V

linkerFlags = -lRakNetLibStatic -lpthread -lSDL2 -lGLEW -lGL -lSDL2_ttf

compilerFlags = -Wall -std=c++11

default: client server

client:
	g++ Source/Client.cpp -g $(compilerFlags) -IInclude -LLibraries -D_REENTRANT -L/usr/lib/x86_64-linux-gnu $(linkerFlags) -o bin/client

server:
	g++ Source/Server.cpp -IInclude -LLibraries $(linkerFlags) -o bin/server