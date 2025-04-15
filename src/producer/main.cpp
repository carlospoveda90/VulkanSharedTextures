#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include "utils/logger.hpp"
#include "app/producer_app.hpp"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./vst_producer <image_path> [--mode=shm|dma]\n";
        return -1;
    }

    std::string imagePath;
    std::string mode = "dma"; // Default

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg.rfind("--mode=", 0) == 0)
        {
            mode = arg.substr(7);
        }
        else if (arg.rfind("--", 0) != 0 && imagePath.empty())
        {
            imagePath = arg;
        }
    }

    if (imagePath.empty())
    {
        std::cerr << "Error: image path is required.\n";
        return -1;
    }

    if (mode != "shm" && mode != "dma")
    {
        std::cerr << "Invalid mode: use --mode=shm or --mode=dma\n";
        return -1;
    }

    LOG_INFO("Running in mode: " + mode);
    LOG_INFO("Image path: " + imagePath);

    if (!glfwInit())
    {
        LOG_ERR("Failed to initialize GLFW.");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan Producer", nullptr, nullptr);
    if (!window)
    {
        LOG_ERR("Failed to create GLFW window.");
        glfwTerminate();
        return -1;
    }

    vst::ProducerApp app(window, imagePath, mode);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        app.runFrame(mode);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    LOG_INFO("Producer exited cleanly.");
    return 0;
}
