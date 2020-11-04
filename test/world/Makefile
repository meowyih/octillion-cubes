
CPP = g++
CPPFLAGS = -O3 -ansi -std=c++17 -pthread -I../../include -Iinclude -L/usr/local/lib -lssl -lcrypto
VPATH = ../../include \
        ../../src/error \
        ../../src/error \
        ../../src/server \
        ../../src/auth \
        ../../src/world

OBJDIR = obj
OBJS = $(addprefix $(OBJDIR)/, \
       loginserver.o \
       gameserver.o \
       dataqueue.o \
       ocerror.o \
       sslserver.o \
       sslclient.o \
       server.o \
       blowfish.o \
       event.o \
       cube.o \
       worldmap.o \
       world.o \
       )

OBJLSERVER = $(addprefix $(OBJDIR)/, lserver.o)
OBJGSERVER = $(addprefix $(OBJDIR)/, gserver.o)
OBJCLIENT = $(addprefix $(OBJDIR)/, client.o)

TARGETLSERVER = lserver
TARGETGSERVER = gserver
TARGETCLIENT = client

all: ${TARGETLSERVER} ${TARGETGSERVER} ${TARGETCLIENT}

# clear suffix list and set new one
.SUFFIXES:
.SUFFIXES: .cpp .o

# $@ is the target, i.e. ${TARGET}
${TARGETLSERVER} : resources ${OBJS} ${OBJLSERVER}
	${CPP} ${OBJLSERVER} ${OBJS} ${CPPFLAGS} ${INC} -o $@

${TARGETGSERVER} : resources ${OBJS} ${OBJGSERVER}
	${CPP} ${OBJGSERVER} ${OBJS} ${CPPFLAGS} ${INC} -o $@

${TARGETCLIENT} : resources ${OBJS} ${OBJCLIENT}
	${CPP} ${OBJCLIENT} ${OBJS} ${CPPFLAGS} ${INC} -o $@    

# create folder if not exist
resources :
	@mkdir -p $(OBJDIR)

# <$ is the first dependency, i.e. xxx.cpp
$(OBJDIR)/%.o : %.cpp
	${CPP} $< ${CPPFLAGS} -c -o $@

$(OBJLSERVER)/%.o : %.cpp
	${CPP} $< ${CPPFLAGS} -c -o $@

$(OBJGSERVER)/%.o : %.cpp
	${CPP} $< ${CPPFLAGS} -c -o $@

$(OBJCLIENT)/%.o : %.cpp
	${CPP} $< ${CPPFLAGS} -c -o $@

# prevent there is a file named clean.cpp
.PHONY: clean

# prefix '@' is not to print the command to console
clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(TARGETLSERVER)
	@rm -rf $(TARGETGSERVER)
	@rm -rf $(TARGETCLIENT)