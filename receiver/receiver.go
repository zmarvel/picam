package main

import (
	"database/sql"
	"encoding/base64"
	"encoding/binary"
	"fmt"
	"github.com/BurntSushi/toml"
	"github.com/golang/protobuf/proto"
	_ "github.com/lib/pq"
	"log"
	"net"
	"picam"
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

func (config *Config) DBString() string {
	return fmt.Sprintf("dbname=%v user=%v password=%v host=%v sslmode=disable",
		config.DB.Database, config.DB.User, config.DB.Password, config.DB.Host)
}

func main() {
	// Load the config
	var conf Config
	_, err := toml.DecodeFile(CONFIG_PATH, &conf)
	if err != nil {
		log.Fatalf("Failed to read config from %v\n", CONFIG_PATH)
	}

	// Establish database connection
	db, err := sql.Open("postgres", conf.DBString())
	if err != nil {
		log.Fatalf("Failed to connect to postgres %v/%v\n", conf.DB.Host,
			conf.DB.Database)
	}

	if db.Ping() != nil {
		log.Fatalf("Failed to ping postgres %v/%v\n", conf.DB.Host, conf.DB.Database)
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
			go handleConnection(db, conn)
		}
	}
}

func handleConnection(db *sql.DB, conn net.Conn) {
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
		meta := imageMessage.Metadata
		if meta != nil {
			fmt.Printf("Image time (%v, %v)\n", meta.TimeS, meta.TimeUs)
		}

		if imageData := imageMessage.Data; imageData != nil {
			var id int
			err := db.QueryRow(`INSERT INTO images (image)
			  VALUES (decode($1, 'base64'))
			  RETURNING id;`,
			  base64.StdEncoding.EncodeToString(imageMessage.Data)).Scan(&id)
			if err != nil {
				log.Printf("Image dropped: %v\n", err)
			} else {
				log.Printf("Image logged (%v B)\n", len(imageMessage.Data))
			}
			_, err = db.Exec(
				`INSERT INTO image_metadata (image_id, time, width, height)
				 VALUES ($1, to_timestamp($2::double precision / 1000000), $3, $4);`,
				 id, int64(meta.TimeS) * 1000000 + int64(meta.TimeUs),
				 meta.Width, meta.Height)
			if err != nil {
				log.Printf("Failed to log metadata: %v\n", err)
			}
		}
	}
	log.Printf("Connection with %v closed\n", conn.RemoteAddr())
}
