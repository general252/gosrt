package main

import (
	"github.com/pion/transport/packetio"
	"log"
	"os"
	"os/signal"
	"syscall"

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

	log.Printf("Listening on %s\n", lis.Addr())

	go func() {
		var (
			pusherConnList = map[string]*gosrt.SRTConn{}
		)

		for {
			conn, err := lis.AcceptSRT()
			if err != nil {
				log.Println(err)
				continue
			}

			log.Printf("accepted sid: %v, streamID: %v, isPusher: %v", conn.SID(), conn.GetStreamID(), conn.IsPusher())

			if conn.IsPusher() {
				pusherConnList[conn.SID()] = conn
			} else {
				for _, srtConn := range pusherConnList {
					if srtConn.GetStreamID() == conn.GetStreamID() {

						go func(connPusher *gosrt.SRTConn) {
							reader := packetio.NewBuffer()
							connPusher.AddReader(conn.GetStreamID(), reader)

							defer func() {
								log.Println("拉流关闭")
								connPusher.DelReader(conn.GetStreamID())
							}()

							buff := make([]byte, 2048)
							for {
								if n, err := reader.Read(buff); err != nil {
									log.Println(err)
									return
								} else {
									if _, err = conn.Write(buff[:n]); err != nil {
										return
									}
								}
							}
						}(srtConn)

						break
					}
				}
			}

			var handlePusher = func() {
				defer func() {
					log.Println("推流关闭")

					delete(pusherConnList, conn.SID())
					_ = conn.Close()
				}()

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
			}

			if conn.IsPusher() {
				go handlePusher()
			} else {

			}
		}
	}()

	quitChan := make(chan os.Signal, 2)
	signal.Notify(quitChan,
		syscall.SIGINT,
		syscall.SIGTERM,
		syscall.SIGHUP,
	)
	<-quitChan

	_ = lis.Close()
}
