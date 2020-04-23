

LIB_DIR := ../lib

INC_DIR := include
INC_DIRS := $(INC_DIR) \
  $(LIB_DIR)/zmqpp/src

INC_DIRS := $(LIB_DIR)/zmqpp

SRCS := src/server.cpp

all: server

server: src/server.cpp
	$(CXX) -o $@ $<

.PHONY: tags
tags:
	ctags -R $(LIB_DIR)
	ctags --append $(SRCS)

