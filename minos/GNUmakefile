build := release
include ../nall/GNUmakefile
include ../hiro/GNUmakefile

flags += -I. -I..
link +=

ifeq ($(platform),windows)
  ifeq ($(console),true)
    link += -mconsole
  else
    link += -mwindows
  endif
endif

objects := obj/hiro.o
objects += obj/game.o obj/minos.o
objects += $(if $(call streq,$(platform),windows),obj/resource.o)

all: $(objects)
	$(compiler) -o out/minos $(objects) $(link) $(hirolink)

obj/hiro.o: ../hiro/hiro.cpp
	$(compiler) $(hiroflags) -o obj/hiro.o -c ../hiro/hiro.cpp

obj/game.o: game/game.cpp $(call rwildcard,game/)
	$(compiler) $(cppflags) $(flags) -o obj/game.o -c game/game.cpp

obj/minos.o: minos.cpp
	$(compiler) $(cppflags) $(flags) -o obj/minos.o -c minos.cpp

obj/resource.o:
	windres ../hiro/windows/hiro.rc obj/resource.o

clean:
	-@$(call delete,obj/*.o)
	-@$(call delete,out/*.exe)
