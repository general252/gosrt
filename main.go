package main

import (
	"log"
	"os"

	"github.com/general252/gosrt/gosrt"
)

func init() {
	log.SetFlags(log.LstdFlags | log.Lshortfile)
}

func main() {
	lis, err := gosrt.Listen(8855)
	if err != nil {
		log.Println(err)
		return
	}

	for {
		conn, err := lis.AcceptSRT()
		if err != nil {
			log.Println(err)
			continue
		}

		log.Println("accepted ", conn.SID())

		go func() {
			defer conn.Close()

			fp, err := os.Create("mpegts.ts")
			if err != nil {
				log.Println(err)
				return
			}

			buff := make([]byte, 2048)
			for {
				if n, err := conn.Read(buff); err != nil {
					log.Println(err)
					return
				} else {
					_, _ = fp.Write(buff[:n])
				}
			}
		}()
	}

}
