#ifndef __M_GO_SRT__
#define __M_GO_SRT__


#ifdef _MSC_VER

#ifdef GOSRT_EXPORTS
#define GOSRT_API __declspec(dllexport)
#else
#define GOSRT_API __declspec(dllimport)

#endif

#else
#define GOSRT_API
#endif

#include <stdint.h>


#ifdef  __cplusplus
extern "C" {
#endif
    // new_connection_cb, data_cb, error_cb, close_cb

    typedef void(*func_new_connection_callback)(int32_t listen_fd, int64_t epoll_fd, int64_t fd_cli, uint8_t* sid, int32_t sid_size);
    typedef void(*func_data_read_callback)(int32_t listen_fd, int64_t epoll_fd, int64_t fd_cli, uint8_t* sid, int32_t sid_size, uint8_t* data, int32_t data_size);

    typedef void(*func_close_callback)(int32_t listen_fd, int64_t epoll_fd, int64_t fd_cli);
    typedef void(*func_error_callback)(int32_t listen_fd, int64_t epoll_fd, int64_t fd_cli, uint8_t* error, int32_t error_size);

    GOSRT_API int32_t gosrt_startup(func_new_connection_callback conn_cb, func_data_read_callback data_cb, func_close_callback close_cb);
    GOSRT_API int32_t gosrt_cleanup();

    GOSRT_API int32_t gosrt_listen(int32_t port);
    GOSRT_API int32_t gosrt_close(int32_t listen_fd);
    GOSRT_API int32_t gosrt_send(int32_t fd_cli, uint8_t* buf, int32_t buf_size);

    GOSRT_API int32_t gosrt_epoll_create(int32_t listen_fd);
    GOSRT_API int32_t gosrt_epoll_uwait(int32_t listen_fd, int32_t epoll_fd);
    GOSRT_API int32_t gosrt_epoll_release(int32_t epoll_fd);


#ifdef  __cplusplus
}
#endif

#endif
