#include <iostream>
#include "utils/file_utils.hpp"
#include "app/producer_app.hpp"
#include "window/glfw_window.hpp"
#include "window/sdl_window.hpp"
#include "utils/mode_probe.hpp"
#include "utils/file_utils.hpp"
#include "media/video_loader.hpp"

void print_usage()
{
    std::cerr << "Usage: ./vst_producer [-i <image_path> | -v <video_path>] [--mode=shm|dma | -s | -d]\n";
    std::cerr << "  -i <image_path>   Path to image file\n";
    std::cerr << "  -v <video_path>   Path to video file (future support)\n";
    std::cerr << "  --mode=shm        Use shared memory mode\n";
    std::cerr << "  --mode=dma        Use DMA-BUF mode (default)\n";
    std::cerr << "  -s                Shortcut for --mode=shm\n";
    std::cerr << "  -d                Shortcut for --mode=dma\n";
}

int main(int argc, char *argv[])
{
    std::string mode = "dma"; // Default mode
    std::string filePath;
    bool isVideo = false;
    bool modeSetExplicitly = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-i" && i + 1 < argc)
        {
            filePath = argv[++i];
            isVideo = false;
        }
        else if (arg == "-v" && i + 1 < argc)
        {
            filePath = argv[++i];
            isVideo = true;
        }
        else if (arg == "--mode=shm" || arg == "-s")
        {
            mode = "shm";
            modeSetExplicitly = true;
        }
        else if (arg == "--mode=dma" || arg == "-d")
        {
            mode = "dma";
            modeSetExplicitly = true;
        }
        else if (arg.rfind("--mode=", 0) == 0)
        {
            std::string parsedMode = arg.substr(7);
            if (parsedMode != "shm" && parsedMode != "dma")
            {
                std::cerr << "Invalid mode: " << parsedMode << "\n";
                print_usage();
                return EXIT_FAILURE;
            }
            mode = parsedMode;
            modeSetExplicitly = true;
        }
        else
        {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n";
            print_usage();
            return EXIT_FAILURE;
        }
    }

    // Must have either -i or -v (currently only imagePath is used)
    if (filePath.empty())
    {
        std::cerr << "Error: You must provide either an image (-i) or video (-v) path.\n";
        print_usage();
        return EXIT_FAILURE;
    }
    
    // Print selected options
    std::cout << "Producer Mode: " << mode << (modeSetExplicitly ? "" : " (default)") << "\n";
    std::cout << (isVideo ? "Video Path: " : "Image Path: ") << filePath << "\n";
    
    vst::ProducerApp app;

    // Check mode
    if (mode == "dma")
    {
        // Initialize GLFW
        std::shared_ptr<vst::IAppWindow> window;
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW.\n";
            return EXIT_FAILURE;
        }
        int windowWidth = 0;
        int windowHeight = 0;
        // Create a window without OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        if (isVideo)
        {
            vst::VideoInfo videoInfo = vst::VideoLoader::getVideoResolution(filePath);
            if (!videoInfo.valid)
            {
                std::cerr << "Failed to load video resolution!\n";
                return EXIT_FAILURE;
            }
            windowWidth = videoInfo.width;
            windowHeight = videoInfo.height;
            std::cout << "Video resolution: " << windowWidth << "x" << windowHeight << "\n";
        }
        else
        {
            // Get image size
            vst::utils::ImageSize imageData = vst::utils::getImageSize(filePath);
            windowWidth = imageData.width;
            windowHeight = imageData.height;
        }

        window = std::make_shared<vst::GLFWWindowWrapper>(windowWidth, windowHeight, "Producer DMA-BUF");
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
            // vst::ProducerApp app(glfwWindow, filePath, mode, isVideo);
            app.ProducerDMA(glfwWindow, filePath, mode, isVideo);
            while (!glfwWindowShouldClose(glfwWindow))
            {
                glfwPollEvents();
                // app.updateFrame();
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
        try
        {
            //vst::ProducerApp app(filePath, mode, isVideo);
            app.ProducerSHM(filePath, mode, isVideo);
            app.runFrame(filePath);
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
