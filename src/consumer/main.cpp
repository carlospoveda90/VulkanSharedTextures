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

    std::optional<std::string> mediaPath;
    int width = 0, height = 0;
    std::string mode = vst::utils::detect_producer_mode();
    if (mode == "shm")
    {
        mediaPath = vst::utils::findLatestShmFile();
        if (mediaPath)
        {
            auto dims = vst::utils::parseImageDimensions(*mediaPath);
            width = dims.width;
            height = dims.height;
            LOG_INFO("[SHM] Detected image dimensions: " + std::to_string(width) + "x" + std::to_string(height));
        }

        vst::ConsumerApp app(mode);
    }
    else if (mode == "dma")
    {
        mediaPath = vst::utils::findLatestDmaSocket();
        if (mediaPath)
        {
            auto dims = vst::utils::parseImageDimensions(*mediaPath);
            width = dims.width;
            height = dims.height;
            LOG_INFO("[DMA] Detected image dimensions: " + std::to_string(width) + "x" + std::to_string(height));
        }

        GLFWwindow *window = glfwCreateWindow(width, height, "Consumer DMA-BUF", nullptr, nullptr);
        if (!window)
        {
            LOG_ERR("Failed to create GLFW window.");
            glfwTerminate();
            return -1;
        }

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
    else
    {
        LOG_ERR("Unknown mode: " + mode);
        return -1;
    }
}
