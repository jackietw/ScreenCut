# ScreenCut Root Makefile for convenience
.PHONY: all clean clean-all distclean rebuild capture editor

all:
	@test -f build/CMakeCache.txt || cmake -S . -B build
	@MAKEFLAGS= cmake --build build --config Release --parallel

capture:
	@test -f build/CMakeCache.txt || cmake -S . -B build
	@MAKEFLAGS= cmake --build build --config Release --target ScreenCut --parallel

editor:
	@test -f build/CMakeCache.txt || cmake -S . -B build
	@MAKEFLAGS= cmake --build build --config Release --target SCEditor --parallel

clean:
	@test -f build/CMakeCache.txt || cmake -S . -B build
	@MAKEFLAGS= cmake --build build --config Release --target clean

clean-all:
	@cmake -E remove_directory build
	@cmake -S . -B build
	@cmake -E create_symlink build/compile_commands.json compile_commands.json 2>/dev/null || true

distclean: clean-all

rebuild: clean all
