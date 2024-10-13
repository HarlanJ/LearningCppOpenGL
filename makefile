.PHONY: test build clean rebuild

linkLibs := glfw GL
incDirs  := include

srcFiles := src/*

linkLine := $(foreach lib,$(linkLibs),-l$(lib))
incLine  := $(foreach dir,$(incDirs),-I$(dir)/)

build:
	mkdir -p output
	gcc -o output/helloWorld $(srcFiles) $(linkLine) $(incLine)

test: build
	./output/helloWorld

clean:
	rm -rf output

rebuild: clean build