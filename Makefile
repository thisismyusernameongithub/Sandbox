
CFLAGS = -O3  -Wall -Wextra -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -mavx2




all: application.exe emscripten

emscripten: build\application_em.o build\window_em.o build\simulation_em.o
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -sASSERTIONS -sSTACK_SIZE=1048576 --emrun -Wextra build\window_em.o build\simulation_em.o build\application_em.o -o application.js -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_TTF=2 --preload-file Resources -sEXPORTED_RUNTIME_METHODS=cwrap -sTOTAL_MEMORY=536870912 -pthread 

build\application_em.o: src\application.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -c src\application.c -o build\application_em.o -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_TTF=2 -pthread 

build\window_em.o: src\window.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -c src\window.c -o build\window_em.o -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_TTF=2 -pthread 

build\simulation_em.o: src\simulation.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -c src\simulation.c -o build\simulation_em.o -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_TTF=2 -pthread 

run: application.exe
	build\application.exe

application.exe: build\application.o build\window.o build\simulation.o
	gcc $(CFLAGS) -o build\application.exe build\window.o build\simulation.o build\application.o -lgdi32 -lSDL2 -lSDL2_ttf -lm -lSDL2_image

build\application.o: src\application.c
	gcc $(CFLAGS) -c src\application.c -o build\application.o

build\window.o: src\window.c
	gcc $(CFLAGS) -c src\window.c -o build\window.o

build\simulation.o: src\simulation.c
	gcc $(CFLAGS) -c src\simulation.c -o build\simulation.o


clean: 
	cmd //C del build\application.exe build\application.o build\simulation.o build\window.o build\application_em.o build\window_em.o build\simulation_em.o
