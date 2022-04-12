package main

import (
	"github.com/general252/gst/gosrt"
	"log"
)

func init() {
	log.SetFlags(log.LstdFlags | log.Lshortfile)
}

func main() {
	log.Println("hello")

	r := gosrt.Startup()
	log.Println("Startup: ", r)

	defer gosrt.Clean()

	r = gosrt.Listen(8855)
	log.Println("Listen: ", r)

	select {}

}
