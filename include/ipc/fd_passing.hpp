
#pragma once

#include <string>
#include <cstdint>

namespace vst::ipc
{

    int setup_unix_server_socket(const std::string &path);
    int connect_unix_client_socket(const std::string &path);

    int send_fd_with_info(int socket_fd, int image_fd, uint32_t width, uint32_t height);
    int receive_fd_with_info(int socket_fd, int &image_fd, uint32_t &width, uint32_t &height);

    void cleanup_unix_socket(const std::string &path);

} // namespace vst::ipc
