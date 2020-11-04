
CPP = g++
CPPFLAGS = -O3 -ansi -std=c++17 -pthread -I../../include -Iinclude -L/usr/local/lib -lssl -lcrypto
VPATH = ../../src/auth

OBJDIR = obj
OBJS = $(addprefix $(OBJDIR)/, \
       blowfish.o \
       main.o \
       )

TARGET = exe

all: ${TARGET}

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
	@rm -rf $(TARGET)