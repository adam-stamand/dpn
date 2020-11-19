all: config.linux


distclean: clean


config.linux:
	meson setup build/linux      \
		--werror

build.linux: 
	meson compile -C build/linux

test.linux: 
	meson test -C build/linux --print-errorlogs

clean:
	rm -rf build

.PHONY:
	distclean
	clean
	all
	config.linux
	build.linux
	# config.sirius configu-sirius-obc config-sirius-tcm

# config.sirius: config-sirius-obc config-sirius-tcm

# config.sirius-obc:
# 	meson setup build/sirius-obc                           \
# 		# --debug --optimization=g                           \
# 		# --warnlevel=1 --werror                             \
# 		--cross-file "${PWD}/meson/cross/sirius/cross.ini" \
# 		--cross-file "${PWD}/meson/cross/sirius/tcm.ini"   \
# 		-Ddefault_library=static

# config.sirius-tcm:
# 	meson setup build/sirius-tcm                           \
# 		# --debug --optimization=g                           \
# 		# --warnlevel=1  --werror                            \
# 		--cross-file "${PWD}/meson/cross/sirius/cross.ini" \
# 		--cross-file "${PWD}/meson/cross/sirius/tcm.ini"   \
# 		-Ddefault_library=static
