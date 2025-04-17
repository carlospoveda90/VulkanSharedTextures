// #include <GLFW/glfw3.h>
// #include <algorithm>
// #include <iostream>
// #include "utils/logger.hpp"
// #include "app/producer_app.hpp"

// int main(int argc, char **argv)
// {
//     if (argc < 2)
//     {
//         std::cerr << "Usage: ./vst_producer <image_path> [--mode=shm|dma]\n";
//         return -1;
//     }

//     std::string mediaPath;
//     std::string mode = "dma"; // Default

//     for (int i = 1; i < argc; ++i)
//     {
//         std::string arg = argv[i];
//         if (arg.rfind("--mode=", 0) == 0)
//         {
//             mode = arg.substr(7);
//         }
//         else if (arg.rfind("--", 0) != 0 && mediaPath.empty())
//         {
//             mediaPath = arg;
//         }
//     }

//     if (mediaPath.empty())
//     {
//         std::cerr << "Error: image path is required.\n";
//         return -1;
//     }

//     if (mode != "shm" && mode != "dma")
//     {
//         std::cerr << "Invalid mode: use --mode=shm or --mode=dma\n";
//         return -1;
//     }

//     LOG_INFO("Running in mode: " + mode);
//     LOG_INFO("Image path: " + mediaPath);

//     if (!glfwInit())
//     {
//         LOG_ERR("Failed to initialize GLFW.");
//         return -1;
//     }

//     glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//     GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan Producer", nullptr, nullptr);
//     if (!window)
//     {
//         LOG_ERR("Failed to create GLFW window.");
//         glfwTerminate();
//         return -1;
//     }

//     vst::ProducerApp app(window, mediaPath, mode);

//     while (!glfwWindowShouldClose(window))
//     {
//         glfwPollEvents();
//         app.runFrame(mode);
//     }

//     glfwDestroyWindow(window);
//     glfwTerminate();

//     LOG_INFO("Producer exited cleanly.");
//     return 0;
// }
// main.cpp (producer)
#include <iostream>
// #include <GLFW/glfw3.h>
#include "app/producer_app.hpp"
#include "window/glfw_window.hpp"
#include "window/sdl_window.hpp"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./vst_producer <image_path> [--mode=shm|dma]\n";
        return EXIT_FAILURE;
    }

    std::string mediaPath = argv[1];
    std::string mode = "dma";
    std::shared_ptr<vst::IAppWindow> window;

    // Parse optional mode argument
    if (argc >= 3)
    {
        std::string modeArg = argv[2];
        if (modeArg.rfind("--mode=", 0) == 0)
        {
            mode = modeArg.substr(7);
            if (mode != "dma" && mode != "shm")
            {
                std::cerr << "Invalid mode: " << mode << "\n";
                std::cerr << "Supported modes: dma (default), shm\n";
                return EXIT_FAILURE;
            }
        }
    }
    // Check mode
    if (mode == "dma")
    {
        // Initialize GLFW
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW.\n";
            return EXIT_FAILURE;
        }
        // Create a window without OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = std::make_shared<vst::GLFWWindowWrapper>(800, 600, "Producer DMA-BUF");
        GLFWwindow *glfwWindow = static_cast<GLFWwindow *>(window->getNativeHandle());
        if (!glfwWindow)
        {
            std::cerr << "Failed to create window.\n";
            glfwTerminate();
            return EXIT_FAILURE;
        }
        // Start the producer app
        try
        {
            vst::ProducerApp app(glfwWindow, mediaPath, mode);
            while (!glfwWindowShouldClose(glfwWindow))
            {
                glfwPollEvents();
                app.runFrame();
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] " << e.what() << std::endl;
            glfwDestroyWindow(glfwWindow);
            glfwTerminate();
            return EXIT_FAILURE;
        }

        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
        return EXIT_SUCCESS;
    }
    else if (mode == "shm")
    {
        // window = std::make_unique<vst::SDLWindowWrapper>(1280, 720, "Producer SHM");
        // SDL_Window *sdlWindow = static_cast<SDL_Window *>(window->getNativeHandle());
        try
        {
            vst::ProducerApp app(mediaPath, mode);
            app.runFrame(mediaPath);

        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        // Run the shared memory viewer
        return EXIT_SUCCESS;
    }
    else
    {
        std::cerr << "Invalid mode: " << mode << "\n";
        return EXIT_FAILURE;
    }
}
