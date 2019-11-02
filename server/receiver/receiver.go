package main

import (
    "fmt"
    "net"
    "log"
)

const PORT = 9000

func main() {
    ln, err := net.Listen("tcp", fmt.Sprintf(":%v", PORT))
    if err != nil {
        log.Fatalf("Failed to bind to port %v\n", PORT)
    }
    for {
        conn, err := ln.Accept()
        if err != nil {
            log.Printf("Failed to accept connection\n")
        } else {
            go handleConnection(conn)
        }
    }
}

func handleConnection(conn net.Conn) {

}
