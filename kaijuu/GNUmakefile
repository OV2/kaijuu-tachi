build := release
include ../nall/GNUmakefile
include ../hiro/GNUmakefile

flags += -I..
name := kaijuu64

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
	$(compiler) -shared -static-libgcc -Wl,kaijuu.def -Wl,-enable-stdcall-fixup -s -o out/$(name).dll $(dllobjects) -lm $(link) -luuid -lshlwapi
	$(compiler) -o out/$(name).exe $(exeobjects) $(link) $(hirolink) -s

obj/kaijuu.o: kaijuu.cpp extension.cpp factory.cpp kaijuu.hpp guid.hpp settings.hpp extension.hpp factory.hpp
	$(compiler) $(cppflags) $(flags) -c kaijuu.cpp -o obj/kaijuu.o

obj/interface.o: interface.cpp interface.hpp guid.hpp settings.hpp
	$(compiler) $(cppflags) $(flags) -o obj/interface.o -c interface.cpp

obj/resource.o:
	windres data/resource.rc obj/resource.o

obj/hiro.o:
	$(compiler) $(hiroflags) -o obj/hiro.o -c ../hiro/hiro.cpp
