build := release
include nall/GNUmakefile
include hiro/GNUmakefile

flags += -I.

ifneq ($(platform),windows)
  $(error "kaijuu is only for Windows")
endif

ifeq ($(console),true)
  link += -mconsole
else
  link += -mwindows
endif

dllobjects := obj/kaijuu.o

exeobjects := obj/interface.o obj/resource.o obj/hiro.o

all: $(dllobjects) $(exeobjects)
	$(compiler) -shared -static-libgcc -Wl,kaijuu.def -Wl,-enable-stdcall-fixup -s -o out/kaijuu64.dll $(dllobjects) -lm $(link) -luuid -lshlwapi
	$(compiler) -o out/kaijuu64.exe $(exeobjects) $(link) $(hirolink) -s

obj/kaijuu.o:
	$(compiler) $(cppflags) $(flags) -c kaijuu.cpp -o obj/kaijuu.o

obj/interface.o:
	$(compiler) $(cppflags) $(flags) -o obj/interface.o -c interface.cpp

obj/resource.o:
	windres data/resource.rc obj/resource.o

obj/hiro.o:
	$(compiler) $(hiroflags) -o obj/hiro.o -c hiro/hiro.cpp

resource: force
	sourcery resource/resource.bml resource/resource.cpp resource/resource.hpp

sync:
	if [ -d ./nall ]; then rm -r ./nall; fi
	if [ -d ./hiro ]; then rm -r ./hiro; fi
	cp -r ../nall ./nall
	cp -r ../hiro ./hiro
	rm -r nall/test
	rm -r hiro/nall
	rm -r hiro/test

force:
