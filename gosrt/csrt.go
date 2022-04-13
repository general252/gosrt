package gosrt

/*
#cgo CFLAGS: -I../library
#cgo LDFLAGS: -L../bin -lgosrt

#include "gosrt.h"

void OnNewConnectionCallback(int64_t listen_fd, int64_t epoll_fd, int64_t fd_cli, uint8_t* sid, int32_t sid_size);
void OnConnectionCloseCallback(int64_t listen_fd, int64_t epoll_fd, int64_t fd_cli);
void OnDataReadCallback(int64_t listen_fd, int64_t epoll_fd, int64_t fd_cli, uint8_t* sid, int32_t sid_size, uint8_t* data, int32_t data_size);

*/
import "C"
import (
	"unsafe"
)

//export OnNewConnectionCallback
func OnNewConnectionCallback(listenFd C.int64_t, epollFd C.int64_t, cliFd C.int64_t, sid *C.uint8_t, sidSize C.int32_t) {
	var strSID []byte = C.GoBytes(unsafe.Pointer(sid), sidSize)

	cb.OnConnectionAccept(int(listenFd), int(epollFd), int(cliFd), string(strSID))
}

//export OnDataReadCallback
func OnDataReadCallback(listenFd C.int64_t, epollFd C.int64_t, cliFd C.int64_t, sid *C.uint8_t, sidSize C.int32_t, data *C.uint8_t, dataSize C.int32_t) {
	var strSID []byte = C.GoBytes(unsafe.Pointer(sid), sidSize)
	var mpegTsData []byte = C.GoBytes(unsafe.Pointer(data), dataSize)

	cb.OnDataRead(int(listenFd), int(epollFd), int(cliFd), string(strSID), mpegTsData)
}

//export OnConnectionCloseCallback
func OnConnectionCloseCallback(listenFd C.int64_t, epollFd C.int64_t, cliFd C.int64_t) {
	cb.OnConnectionClose(int(listenFd), int(epollFd), int(cliFd))
}

var (
	cb EpollEvent = new(EpollEventEmptyImpl)
)

type EpollEvent interface {
	OnConnectionAccept(listenFd int, epollFd int, cliFd int, sid string)
	OnConnectionClose(listenFd int, epollFd int, cliFd int)
	OnDataRead(listenFd int, epollFd int, cliFd int, sid string, data []byte)
}

type EpollEventEmptyImpl struct{}

func (c *EpollEventEmptyImpl) OnConnectionAccept(listenFd int, epollFd int, cliFd int, sid string) {}
func (c *EpollEventEmptyImpl) OnConnectionClose(listenFd int, epollFd int, cliFd int)              {}
func (c *EpollEventEmptyImpl) OnDataRead(listenFd int, epollFd int, cliFd int, sid string, data []byte) {
}

func SrtStartUp(c EpollEvent) int {
	var rc = C.gosrt_startup(
		C.func_new_connection_callback(C.OnNewConnectionCallback),
		C.func_data_read_callback(C.OnDataReadCallback),
		C.func_close_callback(C.OnConnectionCloseCallback),
	)

	if c != nil {
		cb = c
	}

	return int(rc)
}

func SrtCleanUp() {
	C.gosrt_cleanup()
}

func SrtListen(port int) int {
	var listenFd = C.gosrt_listen(C.int32_t(port))

	return int(listenFd)
}

func SrtClose(listenFd int) {
	C.gosrt_close(C.int32_t(listenFd))
}

func SrtEpollCreate(listenFd int) int {
	var epollFd = C.gosrt_epoll_create(C.int32_t(listenFd))

	return int(epollFd)
}

func SrtEpollRelease(epollFd int) {
	C.gosrt_epoll_release(C.int32_t(epollFd))
}

func SrtEpollWait(listenFd int, epollFd int) int {
	var num = C.gosrt_epoll_uwait(C.int32_t(listenFd), C.int32_t(epollFd))

	return int(num)
}
