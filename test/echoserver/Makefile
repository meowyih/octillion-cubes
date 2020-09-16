
CPP = g++
CPPFLAGS = -O3 -ansi -std=c++11 -pthread -I../../include -Iinclude -L/usr/local/lib -lssl -lcrypto
VPATH = ../../include \
        ../../src/error \
        ../../src/server \
        ../../src/memory \
		include \
		src

OBJDIR = obj
OBJS = $(addprefix $(OBJDIR)/, \
       echoserver.o \
       ocerror.o \
       sslserver.o \
       sslclient.o \
       memleak.o \
       )

OBJSERVER = $(addprefix $(OBJDIR)/, server.o)
OBJCLIENT = $(addprefix $(OBJDIR)/, client.o)

TARGETSERVER = server
TARGETCLIENT = client

all: ${TARGETSERVER} ${TARGETCLIENT}

# clear suffix list and set new one
.SUFFIXES:
.SUFFIXES: .cpp .o

# $@ is the target, i.e. ${TARGET}
${TARGETSERVER} : resources ${OBJS} ${OBJSERVER}
	${CPP} ${OBJSERVER} ${OBJS} ${CPPFLAGS} ${INC} -o $@
    
${TARGETCLIENT} : resources ${OBJS} ${OBJCLIENT}
	${CPP} ${OBJCLIENT} ${OBJS} ${CPPFLAGS} ${INC} -o $@    

# create folder if not exist
resources :
	@mkdir -p $(OBJDIR)

# <$ is the first dependency, i.e. xxx.cpp
$(OBJDIR)/%.o : %.cpp
	${CPP} $< ${CPPFLAGS} -c -o $@

$(OBJSERVER)/%.o : %.cpp
	${CPP} $< ${CPPFLAGS} -c -o $@

$(OBJCLIENT)/%.o : %.cpp
	${CPP} $< ${CPPFLAGS} -c -o $@

# prevent there is a file named clean.cpp
.PHONY: clean

# prefix '@' is not to print the command to console
clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(TARGETSERVER)
	@rm -rf $(TARGETCLIENT)
