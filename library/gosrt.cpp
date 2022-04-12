#define  _CRT_SECURE_NO_WARNINGS

#include "gosrt.h"
#include "srt/srt.h"
#include <thread>

#ifdef _MSC_VER
#pragma comment(lib, "srt.lib")
#pragma comment(lib, "ws2_32.lib")
#endif


int do_epoll(int epoll_fd, int listen_fd);
int handle_events(int epoll_fd, SRT_EPOLL_EVENT* events, int num, int listen_fd);
int handle_accpet(int epoll_fd, int listen_fd);

int do_read(int epoll_fd, int fd);

int add_event(int epoll_fd, int fd, int events);
int mod_event(int epoll_fd, int fd, int events);
int del_event(int epoll_fd, int fd);


func_new_connection_callback m_conn_cb;
func_data_read_callback m_data_cb;
func_close_callback m_close_cb;
func_error_callback m_err_cb;

GOSRT_API int32_t gosrt_init(
    func_new_connection_callback conn_cb,
    func_data_read_callback data_cb)
{
    m_conn_cb = conn_cb;
    m_data_cb = data_cb;

    return srt_startup();
}

GOSRT_API int32_t gosrt_deinit()
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
        return -2;
    }

    // listen
    srt_listen(listen_fd, 50);


    // 创建一个文件描述符
    int epoll_fd = srt_epoll_create();

    std::thread writeThread([epoll_fd, listen_fd]()
        {
            // poll
            do_epoll(epoll_fd, listen_fd);

            srt_epoll_release(epoll_fd);
        });

    writeThread.detach();

    return epoll_fd;
}





const char* IPADDRESS = "0.0.0.0";
const int PORT = 8787;
const int MAX_SIZE = 2048;
const int LISTEN_Q = 5;
const int FD_SIZE = 1000;
const int EPOLL_EVENTS = 100;



// 1. poll
int do_epoll(int epoll_fd, int listen_fd)
{
    SRT_EPOLL_EVENT events[EPOLL_EVENTS];
    int num = EPOLL_EVENTS;

    int ret = -1;

    // 添加监听描述符事件
    add_event(epoll_fd, listen_fd, SRT_EPOLL_IN | SRT_EPOLL_ERR | SRT_EPOLL_ACCEPT);

    while (true)
    {
        ret = srt_epoll_uwait(epoll_fd, &events[0], num, 5);
        if (ret > 0) {
            handle_events(epoll_fd, events, ret, listen_fd);
        }
        else if (SRT_ERROR == ret) {
            break;
        }
    }


    return 0;
}

// 2. event
int handle_events(int epoll_fd, SRT_EPOLL_EVENT* events, int num, int listen_fd)
{
    int i;
    for (i = 0; i < num; i++)
    {
        SRT_EPOLL_EVENT s = events[i];

        if ((s.fd == listen_fd) && (s.events & SRT_EPOLL_IN))
        {
            printf("[event] accept fd: %d\n", s.fd);
            handle_accpet(epoll_fd, listen_fd);
        }
        else if (s.events & SRT_EPOLL_IN)
        {
            do_read(epoll_fd, s.fd);
        }
        else if (s.events & SRT_EPOLL_OUT)
        {
            // printf("[event] write fd: %d\n", s.fd);
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

// 3. accept
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
            m_conn_cb(epoll_fd, cli_fd, (uint8_t*)sid, (int32_t)strlen(sid));
        }

#if 0
        std::thread writeThread([epoll_fd, cli_fd]()
            {
                char buffer[64];
                for (int i = 0; i < 10000; i++)
                {
                    sprintf(buffer, "hello world %d", i);
                    int rc = srt_sendmsg(cli_fd, buffer, strlen(buffer) + 1, -1, false);
                    printf("srt_sendmsg %d. %s\n", rc, buffer);
                    if (SRT_ERROR == rc) {
                        printf("send fail. %s\n", srt_getlasterror_str());
                        del_event(epoll_fd, cli_fd);
                        srt_close(cli_fd);
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            });

        writeThread.detach();
#endif
    }

    return 0;
}

// 4. read
int do_read(int epoll_fd, int fd)
{
    char buf[MAX_SIZE] = { 0 };
    int nread = srt_recvmsg(fd, buf, MAX_SIZE);

    if (SRT_ERROR == nread)
    {
        const char* err = srt_getlasterror_str();
        printf("read error. %s\n", err); //	(-1) when an error occurs

        if (m_err_cb) {
            m_err_cb(epoll_fd, fd, (uint8_t*)err, (int32_t)strlen(err));
        }
        if (m_close_cb) {
            m_close_cb(epoll_fd, fd);
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
            m_data_cb(epoll_fd, fd, (uint8_t*)sid, (int32_t)strlen(sid), (uint8_t*)buf, nread);
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

