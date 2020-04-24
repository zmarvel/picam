

LIB_DIR := ../lib

INC_DIR := include
INC_DIRS := $(INC_DIR) \
  $(LIB_DIR)/zmqpp/src

LIBS := $(LIB_DIR)/zmqpp/build/max-g++/libzmqpp.a

SRCS := src/server.cpp \
    src/client.cpp

OBJS := $(SRCS:%.cpp=%.o)

EXES := server \
    client

EXES := $(addprefix build/,$(EXES))

CXXFLAGS += -std=c++17 $(patsubst %,-I%,$(INC_DIRS))

LDFLAGS += -std=c++17 -lpng16 -lzmq

all: buildout $(EXES)

.PHONY: buildout
buildout:
	@mkdir -p build

build/%.o: src/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

build/%: build/%.o $(LIBS)
	$(CXX) $(LDFLAGS) -o $@ $^

.PHONY: tags
tags:
	ctags -R $(LIB_DIR)
	ctags --append $(SRCS)
	ctags --append /usr/include/png++

.PHONY: clean
clean:
	rm -rf build
