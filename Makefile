# Application settings
APP_NAME = Sandbox
APP_VER_MAJOR = 0
APP_VER_MINOR = 12
APP_VER_BUILD = 2742

DEFINES = -DAPP_NAME=\"$(APP_NAME)\" -DAPP_VER_MAJOR=$(APP_VER_MAJOR) -DAPP_VER_MINOR=$(APP_VER_MINOR) -DAPP_VER_BUILD=$(APP_VER_BUILD)

# Directories
SRCDIR = src
DEPDIR = dependencies/src
BUILDDIR = build

# Compiler settings
CC = clang

# Compiler flags etc.
CFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-parameter
LFLAGS =  -lSDL2 -lSDL2_ttf -lm -lSDL2_image # -lgdi32 


# Uncomment the following block to enable a full set of Clang sanitizers
ifeq ($(OS),Windows_NT)
# On Windows, no sanitizer
else
# On other platforms, enable a full set of sanitizers
# Choose one of these combinations:
	# SANITIZERS = address,undefined,leak  # For memory errors and leaks
	# # SANITIZERS = thread,undefined       # For threading issues
	# # SANITIZERS = memory,undefined -fsanitize-memory-track-origins=2      # For uninitialized reads
	# CFLAGS += -fsanitize=$(SANITIZERS) 
	# LFLAGS += -fsanitize=$(SANITIZERS)
	
	# # Sanitizer runtime options - these will be passed to the program when running
	# SANITIZER_OPTIONS = ASAN_OPTIONS=halt_on_error=0
	# UBSAN_OPTIONS = halt_on_error=0:print_stacktrace=1
endif

# Uncomment for Debugging
# LFLAGS += -Wl,--warn-common -Wl,--demangle
# CFLAGS += -g3 -D_FORTIFY_SOURCE=2 -fstack-clash-protection -fcf-protection=full -fno-omit-frame-pointer -fstack-protector-all

# Uncomment for all optimizations
CFLAGS += -flto -O3 -ffast-math -funroll-loops -fno-stack-protector -fno-exceptions -g3
LFLAGS += -Wl,-O3,--strip-debug,--as-needed -flto -fuse-linker-plugin

EMSFLAGS = -sEXPORTED_RUNTIME_METHODS=cwrap -sTOTAL_MEMORY=536870912 -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 \
            -sUSE_SDL_TTF=2 -sUSE_WEBGL2=1 -sFULL_ES3=1 -sMAX_WEBGL_VERSION=2 -sASSERTIONS -sGL_ASSERTIONS \
            -sSTACK_SIZE=1048576 --emrun

# Source files for native build (update these lists to add new files)
NATIVE_SRCS = application.c window.c simulation.c terrainGeneration.c camera.c globals.c render.c
NATIVE_OBJS = $(patsubst %.c,$(BUILDDIR)/%.o,$(NATIVE_SRCS))

# Source files for Emscripten build
EM_SRCS = application.c window.c simulation.c terrainGeneration.c camera.c globals.c render.c
EM_OBJS = $(patsubst %.c,$(BUILDDIR)/%_em.o,$(EM_SRCS))

# Glad object files (common for both builds)
GLAD_SRC = glad.c
GLAD_OBJ  = $(BUILDDIR)/glad.o
GLAD_EM_OBJ = $(BUILDDIR)/glad_em.o

# Increment APP_VER_BUILD number by 1
increment_version:
	$(eval APP_VER_BUILD := $(shell echo $$(($(APP_VER_BUILD) + 1))))
	@sed -i 's/^\(APP_VER_BUILD = \).*/\1$(APP_VER_BUILD)/' Makefile
	$(shell touch src/application.c)



###############################
# Pattern rules for native build
###############################

# clang -target x86_64-pc-windows-gnu

# For application.c – applying extra defines
$(BUILDDIR)/application.o: $(SRCDIR)/application.c
	$(CC) $(CFLAGS) -c $< -o $@ $(DEFINES)

# For window.c – add extra flags (e.g. -mavx)
$(BUILDDIR)/window.o: $(SRCDIR)/window.c
	$(CC) $(CFLAGS) -mavx -c $< -o $@

# For simulation.c – default compile
$(BUILDDIR)/simulation.o: $(SRCDIR)/simulation.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/terrainGeneration.o: $(SRCDIR)/terrainGeneration.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/camera.o: $(SRCDIR)/camera.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/globals.o: $(SRCDIR)/globals.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/render.o: $(SRCDIR)/render.c
	$(CC) $(CFLAGS) -c $< -o $@

# Glad
$(BUILDDIR)/glad.o: $(DEPDIR)/$(GLAD_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

###############################
# Pattern rules for Emscripten build
###############################

# For application
$(BUILDDIR)/application_em.o: $(SRCDIR)/application.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -c $< -o $@ $(DEFINES)

# For window – add extra SIMD flags
$(BUILDDIR)/window_em.o: $(SRCDIR)/window.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -mavx -msimd128 -c $< -o $@

# For simulation – add extra SIMD flags
$(BUILDDIR)/simulation_em.o: $(SRCDIR)/simulation.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -mavx -msimd128 -c $< -o $@

$(BUILDDIR)/terrainGeneration_em.o: $(SRCDIR)/terrainGeneration.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -c $< -o $@

$(BUILDDIR)/camera_em.o: $(SRCDIR)/camera.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -c $< -o $@

$(BUILDDIR)/globals_em.o: $(SRCDIR)/globals.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -c $< -o $@

$(BUILDDIR)/render_em.o: $(SRCDIR)/render.c
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -c $< -o $@

# Glad for emscripten
$(BUILDDIR)/glad_em.o: $(DEPDIR)/$(GLAD_SRC)
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -c $< -o $@

###############################
# Main targets
###############################

# Default target: build native executable and emscripten version
all: application.exe emscripten

# Native linking
application.exe: increment_version $(NATIVE_OBJS) $(GLAD_OBJ)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/$(APP_NAME).exe $(NATIVE_OBJS) $(GLAD_OBJ) $(LFLAGS)

# Emscripten linking & packaging
emscripten: $(EM_OBJS) $(GLAD_EM_OBJ)
	C:\Users\dwtys\emsdk\upstream\emscripten\emcc $(CFLAGS) -o application.js $(GLAD_EM_OBJ) $(EM_OBJS) $(EMSFLAGS) --preload-file Resources --preload-file src/shaders

# Shortcut to run native application
run: application.exe
	$(SANITIZER_OPTIONS) $(UBSAN_OPTIONS) $(BUILDDIR)/$(APP_NAME).exe

# Clean build artifacts
clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/*.exe gmon.out

###############################
# Valgrind testing targets
###############################
memcheck: application.exe
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=definite,indirect,possible --track-origins=yes --suppressions=./valgrind-suppresions.txt $(BUILDDIR)/$(APP_NAME).exe 

callgrind: application.exe
	valgrind --tool=callgrind --callgrind-out-file=callgrind.out.%p $(BUILDDIR)/$(APP_NAME).exe
	@echo "To view results: kcachegrind callgrind.out.*"

cachegrind: application.exe
	valgrind --tool=cachegrind --cachegrind-out-file=cachegrind.out.%p $(BUILDDIR)/$(APP_NAME).exe
	@echo "To view results: kcachegrind cachegrind.out.*"

.PHONY: memcheck callgrind cachegrind