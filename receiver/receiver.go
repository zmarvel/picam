package main

import (
	"fmt"
	"github.com/BurntSushi/toml"
	"log"
	"net"
	"picam"
	"encoding/binary"
	"github.com/golang/protobuf/proto"
)

const (
	PORT        = 9000
	CONFIG_PATH = "../server.toml"
)

type Config struct {
	DB struct {
		Database string
		User     string
		Password string
		Host     string
	}
}

func main() {
	// Load the config
	var conf Config
	_, err := toml.DecodeFile(CONFIG_PATH, &conf)
	if err != nil {
		log.Fatalf("Failed to read config from %v\n", CONFIG_PATH)
	}

	// Bind to the socket
	sock, err := net.Listen("tcp", fmt.Sprintf(":%v", PORT))
	if err != nil {
		log.Fatalf("Failed to bind to port %v\n", PORT)
	}
	for {
		conn, err := sock.Accept()
		if err != nil {
			log.Printf("Failed to accept connection\n")
		} else {
			go handleConnection(conn)
		}
	}
}

func handleConnection(conn net.Conn) {
	log.Printf("Connection with %v established\n", conn.RemoteAddr())
	done := false
	for !done {
		// Read the size first so we know how big the rest of the message is.
		// This is not a member of the protobuf--it gets sent just before.
		var size int32 = 0
		err := binary.Read(conn, binary.BigEndian, &size)
		if err != nil {
			log.Printf("Failed to read size\n")
			done = true
			break
		}

		// Now we know how big of a buffer we need
		var imageMessage picam.Image
		buffer := make([]uint8, size)
		for read := 0; read < int(size); {
			chunkSize, err := conn.Read(buffer[read:])
			if err != nil {
				// We encountered an error--exit the goroutine
				log.Printf("Connection with %v dropped\n", conn.RemoteAddr())
				done = true
				break
			}
			read += chunkSize
		}
		proto.Unmarshal(buffer, &imageMessage)
		fmt.Printf("Read Image (%v bytes)\n", size)
		// Image is only valid with metadata
		if meta := imageMessage.Metadata; meta != nil {
			fmt.Printf("Image time (%v, %v)\n", meta.TimeS, meta.TimeUs)
		}
	}
}
