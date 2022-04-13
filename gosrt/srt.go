package gosrt

import (
	"fmt"
	"log"
	"sync"
)

var (
	srtEventHandler = new(mySRTEventHandleImpl)
)

func init() {
	_ = SrtStartUp(srtEventHandler)
}

func Listen(port int) (*SRTListener, error) {
	listenFd := SrtListen(port)
	if listenFd < 0 {
		return nil, fmt.Errorf("srt listen failed: %v", listenFd)
	}

	epollFd := SrtEpollCreate(listenFd)
	if epollFd < 0 {
		SrtClose(listenFd)
		return nil, fmt.Errorf("srt epoll create failed: %v", epollFd)
	}

	c := NewSRTListener(listenFd, epollFd)

	srtEventHandler.addSRTListener(c)

	go c.run()

	return c, nil
}

type mySRTEventHandleImpl struct {
	EpollEventEmptyImpl

	srtListenerList sync.Map
}

func (c *mySRTEventHandleImpl) addSRTListener(listener *SRTListener) {
	c.srtListenerList.Store(listener.listenFd, listener)
}

func (c *mySRTEventHandleImpl) OnConnectionAccept(listenFd int, epollFd int, cliFd int, sid string) {
	log.Println("OnConnectionAccept", epollFd, cliFd, sid)

	o, ok := c.srtListenerList.Load(listenFd)
	if ok {
		o.(*SRTListener).onConnectionAccept(cliFd, sid)
	}
}
func (c *mySRTEventHandleImpl) OnConnectionClose(listenFd int, epollFd int, cliFd int) {
	log.Println("OnConnectionClose", epollFd, cliFd)

	o, ok := c.srtListenerList.Load(listenFd)
	if ok {
		o.(*SRTListener).onConnectionClose(cliFd)
	}
}
func (c *mySRTEventHandleImpl) OnDataRead(listenFd int, epollFd int, cliFd int, sid string, data []byte) {
	log.Println("on data read ", sid, len(data))

	o, ok := c.srtListenerList.Load(listenFd)
	if ok {
		o.(*SRTListener).onDataRead(cliFd, sid, data)
	}
}
