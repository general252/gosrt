package gosrt

import (
	"github.com/pion/transport/packetio"
	"log"
)

type SRTConn struct {
	fd  int
	sid string

	buf *packetio.Buffer
}

func NewSRTConn(fd int, sid string) *SRTConn {
	return &SRTConn{
		fd:  fd,
		sid: sid,
		buf: packetio.NewBuffer(),
	}
}

func (c *SRTConn) SID() string {
	return c.sid
}

func (c *SRTConn) Read(b []byte) (n int, err error) {
	return c.buf.Read(b)
}

func (c *SRTConn) Write(b []byte) (n int, err error) {
	return c.buf.Write(b)
}

func (c *SRTConn) Close() error {
	return c.buf.Close()
}

func (c *SRTConn) onDataRead(data []byte) {
	if _, err := c.buf.Write(data); err != nil {
		log.Println(err)
	}
}
