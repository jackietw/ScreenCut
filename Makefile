# ScreenCut Root Makefile for convenience
.PHONY: all clean clean-all distclean rebuild capture editor

all:
	@cmake --build build --config Release

capture:
	@cmake --build build --config Release --target ScreenCut

editor:
	@cmake --build build --config Release --target SCEditor

clean:
	@cmake --build build --config Release --target clean

clean-all:
	@cmake -E remove_directory build
	@cmake -E make_directory build
	@cmake -S . -B build -A x64

distclean: clean-all

rebuild: clean all
