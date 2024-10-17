.PHONY: test build clean rebuild

linkLibs := m glfw GL
incDirs  := include

srcFiles := src/*

linkLine := $(foreach lib,$(linkLibs),-l$(lib))
incLine  := $(foreach dir,$(incDirs),-I$(dir)/)

build:
	mkdir -p output
	cp -r assets output/assets
	g++ -o output/helloWorld $(srcFiles) $(linkLine) $(incLine)

test: rebuild
	./output/helloWorld

clean:
	rm -rf output

rebuild: clean build