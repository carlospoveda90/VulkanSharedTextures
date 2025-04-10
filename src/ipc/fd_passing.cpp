#include "ipc/fd_passing.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace vst::ipc
{

    int setup_unix_server_socket(const std::string &path)
    {
        unlink(path.c_str());

        int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            perror("socket");
            return -1;
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

        if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("bind");
            close(server_fd);
            return -1;
        }

        if (listen(server_fd, 1) < 0)
        {
            perror("listen");
            close(server_fd);
            return -1;
        }

        return server_fd;
    }

    int connect_unix_client_socket(const std::string &path)
    {
        int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_fd < 0)
        {
            perror("socket");
            return -1;
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(client_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("connect");
            close(client_fd);
            return -1;
        }

        return client_fd;
    }

    int send_fd_with_info(int socket_fd, int fd_to_send, uint32_t width, uint32_t height)
    {
        char buffer[8];
        memcpy(buffer, &width, 4);
        memcpy(buffer + 4, &height, 4);

        struct iovec iov;
        iov.iov_base = buffer;
        iov.iov_len = sizeof(buffer);

        char cmsg_buf[CMSG_SPACE(sizeof(int))] = {};
        msghdr msg{};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsg_buf;
        msg.msg_controllen = sizeof(cmsg_buf);

        cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));

        memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));

        int sent = sendmsg(socket_fd, &msg, 0);
        return (sent >= 0) ? 0 : -1;
    }

    int receive_fd_with_info(int socket_fd, int &received_fd, uint32_t &width, uint32_t &height)
    {
        char buffer[8];
        struct iovec iov;
        iov.iov_base = buffer;
        iov.iov_len = sizeof(buffer);

        char cmsg_buf[CMSG_SPACE(sizeof(int))] = {};
        msghdr msg{};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsg_buf;
        msg.msg_controllen = sizeof(cmsg_buf);

        int received = recvmsg(socket_fd, &msg, 0);
        if (received < 0)
        {
            perror("recvmsg");
            return -1;
        }

        memcpy(&width, buffer, 4);
        memcpy(&height, buffer + 4, 4);

        cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
        {
            received_fd = *(int *)CMSG_DATA(cmsg);
            return 0;
        }

        return -1;
    }

    // int send_fd_with_info(int sock_fd, int fd_to_send, uint32_t width, uint32_t height)
    // {
    //     struct
    //     {
    //         uint32_t width;
    //         uint32_t height;
    //     } dims = {width, height};

    //     struct iovec iov = {
    //         .iov_base = &dims,
    //         .iov_len = sizeof(dims)};

    //     char buf[CMSG_SPACE(sizeof(int))];
    //     memset(buf, 0, sizeof(buf));

    //     struct msghdr msg{};
    //     msg.msg_iov = &iov;
    //     msg.msg_iovlen = 1;
    //     msg.msg_control = buf;
    //     msg.msg_controllen = sizeof(buf);

    //     struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    //     cmsg->cmsg_level = SOL_SOCKET;
    //     cmsg->cmsg_type = SCM_RIGHTS;
    //     cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    //     memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));

    //     msg.msg_controllen = cmsg->cmsg_len;

    //     return sendmsg(sock_fd, &msg, 0);
    // }

    // int receive_fd_with_info(int sock_fd, int &out_fd, uint32_t &width, uint32_t &height)
    // {
    //     struct
    //     {
    //         uint32_t width;
    //         uint32_t height;
    //     } dims;

    //     struct iovec iov = {
    //         .iov_base = &dims,
    //         .iov_len = sizeof(dims)};

    //     char buf[CMSG_SPACE(sizeof(int))];
    //     memset(buf, 0, sizeof(buf));

    //     struct msghdr msg{};
    //     msg.msg_iov = &iov;
    //     msg.msg_iovlen = 1;
    //     msg.msg_control = buf;
    //     msg.msg_controllen = sizeof(buf);

    //     ssize_t len = recvmsg(sock_fd, &msg, 0);
    //     if (len < 0)
    //         return -1;

    //     struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    //     if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
    //     {
    //         memcpy(&out_fd, CMSG_DATA(cmsg), sizeof(int));
    //     }
    //     else
    //     {
    //         return -1;
    //     }

    //     width = dims.width;
    //     height = dims.height;

    //     return 0;
    // }

} // namespace vst::ipc
