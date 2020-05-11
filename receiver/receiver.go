package main

import (
	"encoding/binary"
	"fmt"
	"github.com/BurntSushi/toml"
	"github.com/golang/protobuf/proto"
	"log"
	"net"
	"os"
	"time"
	"path"
	"picam"
)

const (
	PORT        = 9000
	CONFIG_PATH = "../server.toml"
)

type Config struct {
	Server struct {
		Port int
	}

	DB struct {
		Database string
		User     string
		Password string
		Host     string
	}

	Log struct {
		Path string
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

	info, err := os.Stat(conf.Log.Path)
	if err != nil || !info.IsDir() {
		log.Fatalf("Log directory does not exist: %v\n", conf.Log.Path)
	}

	// Bind to the socket
	sock, err := net.Listen("tcp", fmt.Sprintf(":%v", conf.Server.Port))
	if err != nil {
		log.Fatalf("Failed to bind to port %v\n", PORT)
	}
	for {
		conn, err := sock.Accept()
		if err != nil {
			log.Printf("Failed to accept connection\n")
		} else {
			go handleConnection(conf.Log.Path, conn)
		}
	}
}

func handleConnection(logDir string, conn net.Conn) {
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
			if err != nil {
				logFile, err := os.Create(path.Join(logDir, fmt.Sprintf("%d.%06d.fits",
						meta.TimeS, meta.TimeUs)))
				defer logFile.Close()
				if err != nil {
				}
			}

		}
	}
	log.Printf("Connection with %v closed\n", conn.RemoteAddr())
}

func logImage(logDir string, image *picam.Image) (string, error) {
	meta := image.Metadata
	timestamp := time.Unix(meta.TimeS, 1000 * meta.TimeUs)
	subdir := path.Join(formatDayDir(timestamp))

	// Check if the day's directory already exists. If it does exist, make sure
	// it's a directory. Otherwise, if it doesn't exist, create it.
	subdirInfo, err := os.Stat(subdir)
	if (err == nil && !subdirInfo.IsDir()) {
		// The file exists but isn't a directory--we can't proceed
		return "", fmt.Errorf("logImage: %s exists but is not a directory", subdir)
	} else if (os.IsExist(err)) {
		// The directory doesn't exist--create it!
		err := os.Mkdir(subdir, 0755)
		if (err != nil) {
			return "", fmt.Errorf("logImage: failed to create directory at %s", subdir)
		}
	}

	// Log the image and its metadata in the day's directory.
	imgFilename := path.Join(subdir, formatImageFilename(timestamp))
	// imgFile, err := os.Create(imgFilename)
	_, err = os.Create(imgFilename)
	if (err != nil) {
		return "", fmt.Errorf("logImage: failed to create image file at %s",
			imgFilename)
	}
	metaFilename := path.Join(subdir, formatMetaFilename(timestamp))
	// metaFile, err := os.Create(metaFilename)
	_, err = os.Create(metaFilename)
	if (err != nil) {
		return "", fmt.Errorf("logImage: failed to create metadata file at %s",
			metaFilename)
	}

	

	return imgFilename, nil
}

func formatDayDir(tm time.Time) string {
	return tm.Format("2006-01-02")
}

func formatImageFilename(tm time.Time) string {
	return tm.Format("150405.000.png")
}

func formatMetaFilename(tm time.Time) string {
	return tm.Format("150405.000.json")
}
