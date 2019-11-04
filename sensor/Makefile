EXE = main
SRCS := src/main.cpp src/camera.cpp \
	src/encoder_config.cpp \
	lib/cpp-logging/logging.cpp \
	proto/picam.pb.cpp \


OBJS := $(SRCS:%.cpp=%.o)
DEPS := $(SRCS:%.cpp=%.d)

INCLUDES := \
	include \
	proto \
	lib/cpp-logging \


INCDIRS := $(addprefix -I,$(INCLUDES))
CFLAGS += -Wall -MD -std=gnu99 -g $(INCDIRS)
CXXFLAGS += -Wall -MD -std=gnu++17 -g $(INCDIRS)

LIBS := -lprotobuf
LDFLAGS += -Wall -g $(LIBS)

ifdef WERROR
	CFLAGS += -Werror
	CXXFLAGS += -Werror
endif

# mmal
CFLAGS += $(shell pkg-config --cflags mmal)
CXXFLAGS += $(shell pkg-config --cflags mmal)
LDFLAGS += $(shell pkg-config --libs mmal)

all: $(EXE)

.PHONY: proto_defs
proto_defs:
	$(MAKE) -C ../proto sensor

proto/picam.pb.cpp: proto_defs

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(EXE) $(OBJS) $(DEPS) tags
	make -C ../proto sensor_clean


.PHONY: ctags
ctags: $(SRCS)
	ctags $(SRCS)
	ctags --append -R $(INCLUDES)
	ctags --append -R lib
	ctags --append -R /opt/vc/include/interface/mmal

-include $(DEPS)
