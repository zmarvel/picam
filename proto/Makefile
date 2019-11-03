
SENSOR_DEST = ../sensor/proto
RECEIVER_DEST = ../receiver/src/picam

all: sensor receiver

sensor: picam.proto
	mkdir -p $(SENSOR_DEST)
	protoc --cpp_out=$(SENSOR_DEST) $<

receiver: picam.proto
	mkdir -p $(RECEIVER_DEST)
	protoc --go_out=$(RECEIVER_DEST) $<
