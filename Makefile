

EXE = main
SRCS := src/main.cpp \
  src/camera.cpp \
  src/encoder_config.cpp \
  lib/cpp-logging/logging.cpp \

OBJS := $(SRCS:%.cpp=%.o)
DEPS := $(SRCS:%.cpp=%.d)

INCDIRS := -Iinclude \
    -Ilib/cpp-logging \

CFLAGS += -Wall -MD -std=gnu99 -g $(INCDIRS)
CXXFLAGS += -Wall -MD -std=gnu++11 -g $(INCDIRS)
LDFLAGS += -Wall -g

ifdef WERROR
    CFLAGS += -Werror
    CXXFLAGS += -Werror
endif

# mmal
CFLAGS += $(shell pkg-config --cflags mmal)
CXXFLAGS += $(shell pkg-config --cflags mmal)
LDFLAGS += $(shell pkg-config --libs mmal)

all: $(EXE)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(EXE) $(OBJS) $(DEPS) tags

.PHONY: ctags
ctags: $(SRCS)
	ctags $(SRCS)
	ctags --append -R lib
	ctags --append -R /opt/vc/include/interface/mmal

-include $(DEPS)
