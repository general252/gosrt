package gosrt

import (
	"net"
	"sync"
	"time"
)

type SRTListener struct {
	listenFd int
	epollFd  int

	chAccept chan *SRTConn
	connList sync.Map
}

func NewSRTListener(listenFd int, epollFd int) *SRTListener {
	c := &SRTListener{
		listenFd: listenFd,
		epollFd:  epollFd,
		chAccept: make(chan *SRTConn, 10),
	}

	return c
}

func (c *SRTListener) run() {
	time.Sleep(time.Second)

	for {
		if rc := SrtEpollWait(c.listenFd, c.epollFd); rc < 0 {
			break
		}
		time.Sleep(time.Millisecond)
	}

	_ = c.Close()
}

func (c *SRTListener) AcceptSRT() (*SRTConn, error) {
	conn := <-c.chAccept

	c.connList.Store(conn.fd, conn)

	return conn, nil
}

// Close closes the listener.
// Any blocked Accept operations will be unblocked and return errors.
func (c *SRTListener) Close() error {
	SrtEpollRelease(c.epollFd)
	SrtClose(c.listenFd)

	return nil
}

// Addr returns the listener's network address.
func (c *SRTListener) Addr() net.Addr {
	return &net.UDPAddr{}
}

func (c *SRTListener) onConnectionAccept(fd int, sid string) {
	c.chAccept <- NewSRTConn(fd, sid)
}

func (c *SRTListener) onConnectionClose(fd int) {
	conn, ok := c.connList.Load(fd)
	if ok {
		_ = conn.(*SRTConn).Close()

		c.connList.Delete(fd)
	}
}

func (c *SRTListener) onDataRead(fd int, sid string, data []byte) {
	conn, ok := c.connList.Load(fd)
	if ok {
		conn.(*SRTConn).onDataRead(data)
	}
}
