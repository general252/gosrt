#define  _CRT_SECURE_NO_WARNINGS

#include "gosrt.h"
#include "srt/srt.h"

#ifdef _MSC_VER
#pragma comment(lib, "srt.lib")
#pragma comment(lib, "ws2_32.lib")
#endif


int handle_events(int epoll_fd, SRT_EPOLL_EVENT* events, int num, int listen_fd);
int handle_accpet(int epoll_fd, int listen_fd);

int do_read(int listen_fd, int epoll_fd, int fd);

int add_event(int epoll_fd, int fd, int events);
int mod_event(int epoll_fd, int fd, int events);
int del_event(int epoll_fd, int fd);


const int EPOLL_EVENTS = 100;
const int MAX_SIZE = 2048;


func_new_connection_callback m_conn_cb;
func_data_read_callback m_data_cb;
func_close_callback m_close_cb;
func_error_callback m_err_cb;

GOSRT_API int32_t gosrt_startup(
    func_new_connection_callback conn_cb,
    func_data_read_callback data_cb,
    func_close_callback close_cb)
{
    m_conn_cb = conn_cb;
    m_data_cb = data_cb;
    m_close_cb = close_cb;

    srt_setloglevel(LOG_INFO);

    return srt_startup();
}

GOSRT_API int32_t gosrt_cleanup()
{
    return srt_cleanup();
}

GOSRT_API int32_t gosrt_listen(int32_t port)
{
    const char* ip = "0.0.0.0";
    int listen_fd = 0;
    struct sockaddr_in server_addr;
    listen_fd = srt_create_socket();
    if (SRT_ERROR == listen_fd) {
        printf("socket error\n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(sockaddr_in));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    // bind
    if (SRT_ERROR == srt_bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(sockaddr_in))) {
        printf("bind error\n");
        srt_close(listen_fd);
        return -2;
    }

    // listen
    if (SRT_ERROR == srt_listen(listen_fd, 50)) {
        printf("listen error\n");
        srt_close(listen_fd);
        return -3;
    }

    return listen_fd;
}

GOSRT_API int32_t gosrt_close(int32_t listen_fd)
{
    return srt_close(listen_fd);
}

GOSRT_API int32_t gosrt_send(int32_t fd_cli, uint8_t* buf, int32_t buf_size)
{
    return srt_send(fd_cli, (const char*)buf, buf_size);
}

GOSRT_API int32_t gosrt_epoll_create(int32_t listen_fd)
{
    // 创建一个文件描述符
    int epoll_fd = srt_epoll_create();


    // 添加监听描述符事件
    add_event(epoll_fd, listen_fd, SRT_EPOLL_IN | SRT_EPOLL_ERR | SRT_EPOLL_ACCEPT);


    return epoll_fd;
}

GOSRT_API int32_t gosrt_epoll_uwait(int32_t listen_fd, int32_t epoll_fd)
{
    SRT_EPOLL_EVENT events[EPOLL_EVENTS];
    int num = EPOLL_EVENTS;

    int ret = srt_epoll_uwait(epoll_fd, &events[0], num, 0);
    if (ret > 0) {
        handle_events(epoll_fd, events, ret, listen_fd);
    }

    return ret;
}

GOSRT_API int32_t gosrt_epoll_release(int32_t epoll_fd)
{
    return srt_epoll_release(epoll_fd);
}




int handle_events(int epoll_fd, SRT_EPOLL_EVENT* events, int num, int listen_fd)
{
    for (int i = 0; i < num; i++)
    {
        SRT_EPOLL_EVENT s = events[i];

        if ((s.fd == listen_fd) && (s.events & SRT_EPOLL_IN))
        {
            printf("[event] accept fd: %d\n", s.fd);
            handle_accpet(epoll_fd, listen_fd);
        }
        else if (s.events & SRT_EPOLL_IN)
        {
            do_read(listen_fd, epoll_fd, s.fd);
        }
        else if (s.events & SRT_EPOLL_OUT)
        {
            //do_write(epoll_fd, s.fd, buf);

            //mod_event(epoll_fd, s.fd, SRT_EPOLL_IN | SRT_EPOLL_ERR);
        }
        else if (s.events & SRT_EPOLL_ERR)
        {
            printf("[event] error fd: %d. error: %s\n", s.fd, srt_getlasterror_str());
        }
        else
        {
            printf("[event] other\n");
        }
    }

    return 0;
}

int handle_accpet(int epoll_fd, int listen_fd)
{
    SRTSOCKET cli_fd;
    struct sockaddr_in cli_addr;
    int addrlen = sizeof(cli_addr);

    cli_fd = srt_accept(listen_fd, (sockaddr*)&cli_addr, &addrlen);
    if (SRT_INVALID_SOCK == cli_fd)
    {
        printf("accept fail. %s\n", srt_getlasterror_str());
        return -1;
    }
    else
    {
        char ip[46] = { 0 };
        uint16_t port = 0;

        inet_ntop(AF_INET, &cli_addr.sin_addr, ip, sizeof(ip));
        port = ntohs(cli_addr.sin_port);


        char sid[256] = { 0 };
        int  sid_size = sizeof(sid);
        srt_getsockopt(cli_fd, 0, SRTO_STREAMID, &sid, &sid_size);

        // 新连接
        printf("accept new client %s:%d stream id: %s\n", ip, port, sid);


        // 添加一个客户端描述符事件
        mod_event(epoll_fd, cli_fd, SRT_EPOLL_IN | SRT_EPOLL_OUT | SRT_EPOLL_ERR);

        // 通知
        if (m_conn_cb) {
            m_conn_cb(listen_fd, epoll_fd, cli_fd, (uint8_t*)sid, (int32_t)strlen(sid));
        }


        if (false) {
            char buffer[64];
            sprintf(buffer, "hello world %d", 123);
            if (SRT_ERROR == srt_sendmsg(cli_fd, buffer, strlen(buffer) + 1, -1, false)) {
                printf("send fail. %s\n", srt_getlasterror_str());
                del_event(epoll_fd, cli_fd);
                srt_close(cli_fd);
            }
        }

    }

    return 0;
}

int do_read(int listen_fd, int epoll_fd, int fd)
{
    char buf[MAX_SIZE] = { 0 };
    int nread = srt_recvmsg(fd, buf, MAX_SIZE);

    if (SRT_ERROR == nread)
    {
        const char* err = srt_getlasterror_str();
        printf("read error. %s\n", err); //	(-1) when an error occurs

        if (m_err_cb) {
            m_err_cb(listen_fd, epoll_fd, fd, (uint8_t*)err, (int32_t)strlen(err));
        }
        if (m_close_cb) {
            m_close_cb(listen_fd, epoll_fd, fd);
        }

        del_event(epoll_fd, fd);
        srt_close(fd);
    }
    else if (0 == nread)
    {
        printf("client close\n"); // If the connection has been closed
        del_event(epoll_fd, fd);
        srt_close(fd);
    }
    else
    {
        // 有数据来了, streamId
        char sid[256] = { 0 };
        int  sid_size = sizeof(sid);
        srt_getsockopt(fd, 0, SRTO_STREAMID, &sid, &sid_size);

#if 0
        static FILE* fp = fopen("out_cpp.ts", "wb+");
        if (fp) {
            fwrite(buf, 1, nread, fp);
            fflush(fp);
        }
#endif

        if (m_data_cb) {
            m_data_cb(listen_fd, epoll_fd, fd, (uint8_t*)sid, (int32_t)strlen(sid), (uint8_t*)buf, nread);
            //printf("ss %d %d\n", nread, (int32_t)strlen(sid));
        }
    }

    return 0;
}



int add_event(int epoll_fd, int fd, int events)
{
    srt_epoll_add_usock(epoll_fd, fd, &events);

    return 0;
}

int mod_event(int epoll_fd, int fd, int events)
{
    srt_epoll_update_usock(epoll_fd, fd, &events);
    // srt_epoll_update_usock/srt_epoll_add_usock的底层调用一样

    return 0;
}

int del_event(int epoll_fd, int fd)
{
    srt_epoll_remove_usock(epoll_fd, fd);

    return 0;
}

