package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"github.com/BurntSushi/toml"
	"github.com/golang/protobuf/proto"
	"encoding/json"
	"log"
	"net"
	"os"
	"time"
	"path"
	"picam"
	"strconv"
	"strings"
	"unicode"
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
		MaxSize string
	}
}

// Translate the max_size field in the config into a size in bytes
func (config *Config) ParseMaxSize() (uint64, error) {
	spacesRemoved := strings.Replace(config.Log.MaxSize, " ", "", 0)
	var sizeBuilder strings.Builder
	var unitBuilder strings.Builder
	for _, r := range spacesRemoved {
		if unitBuilder.Len() == 0 && unicode.IsNumber(r) {
			_, err := sizeBuilder.WriteRune(r)
			if err != nil {
				return 0, fmt.Errorf("ParseMaxSize: %v", err)
			}
		} else if unitBuilder.Len() > 0 && unicode.IsLetter(r) {
			_, err := unitBuilder.WriteRune(r)
			if err != nil {
				return 0, fmt.Errorf("ParseMaxSize: %v", err)
			}
		} else {
			return 0, fmt.Errorf("ParseMaxSize: Unrecognized rune %v", r)
		}
	}

	if unitBuilder.Len() == 0 {
		_, err := unitBuilder.WriteString("k")
		if err != nil {
			return 0, fmt.Errorf("ParseMaxSize: %v", err)
		}
	}

	sizeStr := sizeBuilder.String()
	unitStr := strings.ToLower(unitBuilder.String())

	sizeNum, err := strconv.Atoi(sizeStr)
	if err != nil {
		return 0, fmt.Errorf("ParseMaxSize: Unable to convert size to number: %v",
			err)
	}

	if unitStr == "k" {
		sizeNum *= 1024
	} else if unitStr == "m" {
		sizeNum *= 1024 * 1024
	} else if unitStr == "g" {
		sizeNum *= 1024 * 1024 * 1024
	} else {
		return 0, fmt.Errorf("ParseMaxSize: Unrecognized unit %s", unitStr)
	}

	return uint64(sizeNum), nil
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

		imagePath, err := logImage(logDir, &imageMessage)
		if err != nil {
			log.Printf("Failed to log image: %v", err)
		} else {
			log.Printf("Logged image to %s", imagePath)
		}
	}
	log.Printf("Connection with %v closed\n", conn.RemoteAddr())
}

func logImage(logDir string, image *picam.Image) (string, error) {
	meta := image.Metadata
	if (meta == nil || image.Data == nil) {
		return "", fmt.Errorf("logImage: message contains nil")
	}

	timestamp := time.Unix(meta.TimeS, 1000 * meta.TimeUs)
	subdir := path.Join(formatDayDir(timestamp))

	// Check if the day's directory already exists. If it does exist, make sure
	// it's a directory. Otherwise, if it doesn't exist, create it.
	subdirInfo, err := os.Stat(subdir)
	if err == nil && !subdirInfo.IsDir() {
		// The file exists but isn't a directory--we can't proceed
		return "", fmt.Errorf("logImage: %s exists but is not a directory", subdir)
	} else if (os.IsExist(err)) {
		// The directory doesn't exist--create it!
		err := os.Mkdir(subdir, 0755)
		if err != nil {
			return "", fmt.Errorf("logImage: failed to create directory at %s", subdir)
		}
	}

	// Log the image and its metadata in the day's directory.
	imgFilename := path.Join(subdir, formatImageFilename(timestamp))
	imgFile, err := os.Create(imgFilename)
	defer imgFile.Close()
	if err != nil {
		return "", fmt.Errorf("logImage: failed to create image file at %s",
			imgFilename)
	}
	metaFilename := path.Join(subdir, formatMetaFilename(timestamp))
	metaFile, err := os.Create(metaFilename)
	defer metaFile.Close()
	if err != nil {
		return "", fmt.Errorf("logImage: failed to create metadata file at %s",
			metaFilename)
	}

	metaJson, err := json.Marshal(meta)
	if err != nil {
		return "", fmt.Errorf("logImage: failed to serialize metadata")
	}

	_, err = metaFile.Write(metaJson)
	if err != nil {
		return "", fmt.Errorf("logImage: failed to write to metadata file")
	}

	_, err = imgFile.Write(image.Data)
	if err != nil {
		return "", fmt.Errorf("logImage: failed to write image data to file")
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

// Recursively calculate the space occupied by a directory
func DiskUsage(dirPath string) (int64, error) {
	dir, err := os.Open(dirPath)
	if err != nil {
		return 0, fmt.Errorf("DiskUsage: Failed to open dir %s: %v", dirPath, err)
	}

	infos, err := dir.Readdir(0)
	if err == io.EOF {
		dirInfo, err := dir.Stat()
		if err != nil {
			return 0, fmt.Errorf("DiskUsage: Failed to stat directory: %v", err)
		}

		var sum int64 = dirInfo.Size()
		for _, info := range infos {
			sum += info.Size()
			// If the file is a subdirectory, traverse it recursively
			if info.IsDir() {
				childPath := path.Join(dirPath, info.Name())
				childSum, err := DiskUsage(childPath)
				if (err != nil) {
					return 0, fmt.Errorf("DiskUsage: Failed to get size of child %s: %v",
						childPath, err)
				}
				sum += childSum
			}
		}
		return sum, nil
	} else if err != nil {
		return 0, fmt.Errorf("DiskUsage: Failed to scan directory: %v", err)
	} else {
		return 0, fmt.Errorf("DiskUsage: EOF expected!")
	}
}
