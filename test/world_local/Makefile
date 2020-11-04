
CPP = g++
CPPFLAGS = -O3 -ansi -std=c++11 -DTEST_WORLD_WITH_NO_GAMESERVER -pthread -I../../include -Iinclude -L/usr/local/lib -lssl -lcrypto
VPATH = ../../include \
        ../../src/error \
        ../../src/world \
		include \
		src

OBJDIR = obj
OBJS = $(addprefix $(OBJDIR)/, \
       cube.o \
       worldmap.o \
       ocerror.o \
       event.o \
       player.o \
       world.o \
       main.o \
       )

TARGET = exe

# clear suffix list and set new one
.SUFFIXES:
.SUFFIXES: .cpp .o

# $@ is the target, i.e. ${TARGET}
${TARGET} : resources ${OBJS}
	${CPP} ${OBJS} ${CPPFLAGS} ${INC} -o $@

# create folder if not exist
resources :
	@mkdir -p $(OBJDIR)
        
# <$ is the first dependency, i.e. xxx.cpp
$(OBJDIR)/%.o : %.cpp
	${CPP} $< ${CPPFLAGS} -c -o $@
	
# prevent there is a file named clean.cpp
.PHONY: clean
	
# prefix '@' is not to print the command to console
clean:
	@rm -rf $(OBJDIR)
	@rm -rf ${TARGET}
	@rm -rf save
    
