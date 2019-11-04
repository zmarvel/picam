SENSOR_DEST = ../sensor/proto
RECEIVER_DEST = ../receiver/src/picam

SENSOR_PROTO = $(SENSOR_DEST)/picam.pb.cpp \
							 $(SENSOR_DEST)/picam.pb.h \

RECEIVER_PROTO = $(RECEIVER_DEST)/picam.pb.go

.PHONY: sensor receiver

all: sensor receiver

sensor: $(SENSOR_PROTO)

$(SENSOR_PROTO): picam.proto
	mkdir -p $(SENSOR_DEST)
	protoc --cpp_out=$(SENSOR_DEST) $<
	mv $(SENSOR_DEST)/picam.pb.cc $(SENSOR_DEST)/picam.pb.cpp

receiver: $(RECEIVER_PROTO)

$(RECEIVER_PROTO): picam.proto
	mkdir -p $(RECEIVER_DEST)
	protoc --go_out=$(RECEIVER_DEST) $<

.PHONY: clean sensor_clean receiver_clean
clean: sensor_clean receiver_clean

sensor_clean:
	rm -f $(SENSOR_DEST)/*.pb.*

receiver_clean:
	rm -f $(RECEIVER_DEST)/*.pb.go
