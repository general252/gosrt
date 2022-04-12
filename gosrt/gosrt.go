package gosrt

/*
#cgo CFLAGS: -I../library
#cgo LDFLAGS: -L../bin -lgosrt

#include "gosrt.h"

void OnNewConnectionCallback(int64_t epoll_fd, int64_t fd_cli, uint8_t* sid, int32_t sid_size);
void OnDataReadCallback(int64_t epoll_fd, int64_t fd_cli, uint8_t* sid, int32_t sid_size, uint8_t* data, int32_t data_size);

*/
import "C"
import (
	"log"
	"os"
	"unsafe"
)

//export OnNewConnectionCallback
func OnNewConnectionCallback(epollFd C.int64_t, cliFd C.int64_t, sid *C.uint8_t, sidSize C.int32_t) {
	var strSID []byte = C.GoBytes(unsafe.Pointer(sid), sidSize)

	log.Printf("new connection: epollFd: %v cliFd: %v, sid: %v", epollFd, cliFd, string(strSID))
}

//export OnDataReadCallback
func OnDataReadCallback(epollFd C.int64_t, cliFd C.int64_t, sid *C.uint8_t, sidSize C.int32_t, data *C.uint8_t, dataSize C.int32_t) {
	if fp == nil {
		if f, err := os.Create("out.ts"); err == nil {
			fp = f
		}
	}

	var mpegTsData []byte = C.GoBytes(unsafe.Pointer(data), dataSize)
	// fmt.Printf("%v", hex.Dump(mpegTsData))

	n, err := fp.Write(mpegTsData)
	if err != nil {
		log.Println(err)
		return
	}

	var strSID []byte = C.GoBytes(unsafe.Pointer(sid), sidSize)

	log.Printf("dataSize: %v n: %v sid: %v", dataSize, n, string(strSID))
}

var fp *os.File

func Startup() int {
	var rc = C.gosrt_init(
		C.func_new_connection_callback(C.OnNewConnectionCallback),
		C.func_data_read_callback(C.OnDataReadCallback),
	)

	return int(rc)
}

func Clean() {
	C.gosrt_deinit()
}

func Listen(port int) int {
	var rc = C.gosrt_listen(C.int32_t(port))

	return int(rc)
}
