#include "app/consumer_app.hpp"
#include "utils/logger.hpp"
#include "utils/file_utils.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <regex>
#include <signal.h>

// Global variables for signal handling
std::atomic<bool> g_running(true);
vst::ConsumerApp *g_app = nullptr;

// Signal handler for termination signals
void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    g_running = false;

    // If app exists, call cleanup explicitly
    if (g_app)
    {
        try
        {
            g_app->cleanup();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error during cleanup: " << e.what() << std::endl;
        }
    }

    // Exit gracefully
    exit(0);
}

int main(int argc, char **argv)
{
    // Register signal handler for Ctrl+C
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Auto-detect shared resources
    auto sharedResource = vst::utils::findSharedResource();
    if (!sharedResource)
    {
        LOG_ERR("No shared resources found. Is the producer running?");
        return EXIT_FAILURE;
    }

    // Handle based on the detected resource
    std::string mode = sharedResource->mode;
    std::string type = sharedResource->type;
    int width = sharedResource->dimensions.width;
    int height = sharedResource->dimensions.height;

    LOG_INFO("Found shared resource: " + sharedResource->path);
    LOG_INFO("Mode: " + mode + ", Type: " + type + ", Dimensions: " +
             std::to_string(width) + "x" + std::to_string(height));

    // Handle based on resource type and mode
    if (mode == "shm")
    {
        if (type == "video")
        {
            // For video content (only SHM supported currently)
            LOG_INFO("Creating consumer for video content");
            g_app = new vst::ConsumerApp();

            // Extract the filename portion for consumeShmVideo
            std::string filename = sharedResource->path.substr(
                sharedResource->path.find_last_of("/\\") + 1);

            if (g_app->consumeShmVideo(filename))
            {
                LOG_INFO("Starting video consumption...");
                g_app->runVideoLoop();
            }
            else
            {
                LOG_ERR("Failed to initialize video consumer");
            }

            // Clean up
            delete g_app;
            return EXIT_SUCCESS;
        }
        else
        {
            // For image content in SHM mode
            LOG_INFO("Creating consumer for SHM image");
            g_app = new vst::ConsumerApp(mode);

            // The constructor already handles the viewing

            // Clean up
            delete g_app;
            return EXIT_SUCCESS;
        }
    }
    else if (mode == "dma")
    {
        if (!glfwInit())
        {
            LOG_ERR("Failed to initialize GLFW.");
            return EXIT_FAILURE;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Set up the window with the detected dimensions
        int width = sharedResource->dimensions.width;
        int height = sharedResource->dimensions.height;

        // Add a title that shows content type (video or image)
        std::string typeFile = sharedResource->type == "video" ? "Video" : "Image";
        std::string windowTitle = "Consumer DMA-BUF - " + typeFile + " (" +
                                  std::to_string(width) + "x" + std::to_string(height) + ")";
                                  

        // Create the window
        GLFWwindow *window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
        if (!window)
        {
            LOG_ERR("Failed to create GLFW window.");
            glfwTerminate();
            return EXIT_FAILURE;
        }

        // Create consumer app
        g_app = new vst::ConsumerApp(window, mode);

        // Main loop - will automatically display updated content
        // whether it's a static image or continuously updated video frames
        while (!glfwWindowShouldClose(window) && g_running)
        {
            glfwPollEvents();
            g_app->runFrame();
        }

        // Clean up
        delete g_app;

        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_SUCCESS;
    }
}