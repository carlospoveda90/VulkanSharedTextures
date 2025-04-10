#include "app/consumer_app.hpp"
#include "utils/logger.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    // if (argc < 4)
    // {
    //     std::cerr << "Usage: ./vst_consumer <dma_buf_fd> <width> <height>\n";
    //     return 1;
    // }

    // int fd = std::stoi(argv[1]);
    // uint32_t width = static_cast<uint32_t>(std::stoi(argv[2]));
    // uint32_t height = static_cast<uint32_t>(std::stoi(argv[3]));

    // LOG_INFO("Launching consumer with DMA-BUF FD: " + std::to_string(fd));
    // LOG_INFO("Image dimensions: " + std::to_string(width) + "x" + std::to_string(height));

    if (!glfwInit())
    {
        LOG_ERR("Failed to initialize GLFW.");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan Consumer", nullptr, nullptr);
    if (!window)
    {
        LOG_ERR("Failed to create GLFW window.");
        glfwTerminate();
        return -1;
    }
    // vst::ConsumerApp app(window, fd, width, height);
    vst::ConsumerApp app(window);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        app.runFrame();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
