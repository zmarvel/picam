package main

import (
	"fmt"
	"github.com/BurntSushi/toml"
	"log"
	"net"
	"picam"
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
		var imageMessage picam.Image
		buffer := make([]uint8, 4096)
		size := 0
		for read := 0; read < size; {
			nRead, err := conn.Read(buffer[read:])
			if err != nil {
				// We encountered an error--exit the goroutine
				log.Printf("Connection with %v dropped\n", conn.RemoteAddr())
				done = true
				break
			}
			if read == 0 {
				// This is the start of the packet. The size is at the beginning, so
				// figure out how much there is to read.
				proto.Unmarshal(buffer, &imageMessage)
				size = int(imageMessage.Size)
				read = 0
			}
			read += nRead
		}
		fmt.Printf("Read Image (%v bytes)\n", imageMessage.Size)
	}
}
