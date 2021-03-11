CXX=g++

all: config.linux


distclean: clean


config.linux:
	CXX=${CXX} meson setup build/linux/${CXX}      \
		--werror

build.linux: 
	meson compile -C build/linux/${CXX}

test.linux: 
	meson test -C build/linux/${CXX} --print-errorlogs

clean:
	rm -rf build

.PHONY:
	distclean
	clean
	all
	config.linux
	build.linux
