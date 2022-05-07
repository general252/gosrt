package gosrt

import (
	"log"
	"os"
	"strings"
	"sync"

	"github.com/pion/transport/packetio"
)

type SRTConn struct {
	fd       int
	sid      string
	isPusher bool
	streamId string

	buf *packetio.Buffer

	readers    map[string]*packetio.Buffer
	readersMux sync.RWMutex
}

func NewSRTConn(fd int, sid string) *SRTConn {
	c := &SRTConn{
		fd:       fd,
		sid:      sid,
		readers:  map[string]*packetio.Buffer{},
		isPusher: strings.Contains(sid, "m=publish"),
		buf:      packetio.NewBuffer(),
	}

	// srt://127.0.0.1:8855?streamid=uplive/live/test,m=publish
	// srt://127.0.0.1:10080?streamid=#!::h=live/livestream,m=publish
	// srt://127.0.0.1:10080?streamid=#!::h=live/livestream,m=request
	// srt://127.0.0.1:10080?streamid=#!::h=srs.srt.com.cn/live/livestream,m=publish
	// srt://127.0.0.1:10080?streamid=#!::h=srs.srt.com.cn/live/livestream,m=request
	tmpSID := strings.ReplaceAll(sid, "#!::", "")
	partList := strings.Split(tmpSID, ",")

	streamID := ""
	for _, part := range partList {
		if !strings.Contains(part, "=") {
			streamID = part
			break
		}
	}

	if len(streamID) == 0 {
		for _, part := range partList {
			if strings.Contains(part, "h=") {
				kv := strings.Split(part, "=")
				if len(kv) > 1 {
					streamID = kv[1]
				}
				break
			}
		}
	}
	c.streamId = streamID

	return c
}

func (c *SRTConn) SID() string {
	return c.sid
}

func (c *SRTConn) IsPusher() bool {
	return c.isPusher
}

func (c *SRTConn) GetStreamID() string {
	return c.streamId
}

func (c *SRTConn) Read(b []byte) (n int, err error) {
	return c.buf.Read(b)
}

func (c *SRTConn) Write(b []byte) (n int, err error) {
	if c.fd == 0 {
		return 0, os.ErrClosed
	}

	n = SrtSend(c.fd, b)
	return n, nil
}

func (c *SRTConn) Close() error {
	_ = c.buf.Close()

	c.readersMux.Lock()
	defer c.readersMux.Unlock()

	for _, reader := range c.readers {
		_ = reader.Close()
	}

	c.fd = 0

	return nil
}

func (c *SRTConn) onDataRead(data []byte) {
	if _, err := c.buf.Write(data); err != nil {
		log.Println(err)
	}

	if len(c.readers) > 0 {
		c.readersMux.RLock()
		c.readersMux.RUnlock()

		for _, reader := range c.readers {
			_, _ = reader.Write(data)
		}
	}
}

func (c *SRTConn) AddReader(streamId string, reader *packetio.Buffer) {
	c.readersMux.Lock()
	defer c.readersMux.Unlock()

	c.readers[streamId] = reader
}

func (c *SRTConn) DelReader(streamId string) {
	c.readersMux.Lock()
	defer c.readersMux.Unlock()

	delete(c.readers, streamId)
}
