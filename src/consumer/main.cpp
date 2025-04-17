#include "app/consumer_app.hpp"
#include "utils/logger.hpp"
#include "utils/mode_probe.hpp"
#include "utils/file_utils.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if (!glfwInit())
    {
        LOG_ERR("Failed to initialize GLFW.");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // vst::utils::ImageSize imageSize = vst::utils::getImageSize("/dev/shm/vst_shared_texture");

    GLFWwindow *window = glfwCreateWindow(800, 600, "Consumer DMA-BUF", nullptr, nullptr);
    if (!window)
    {
        LOG_ERR("Failed to create GLFW window.");
        glfwTerminate();
        return -1;
    }
    std::string mode = vst::utils::detect_producer_mode();
    vst::ConsumerApp app(window, mode);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        app.runFrame();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
