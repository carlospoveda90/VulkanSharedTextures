#include <iostream>
#include <signal.h>
#include <atomic>
#include <thread>
#include <chrono>
#include "utils/file_utils.hpp"
#include "app/producer_app.hpp"
#include "window/glfw_window.hpp"
#include "window/sdl_window.hpp"
#include "utils/mode_probe.hpp"
#include "utils/file_utils.hpp"
#include "media/video_loader.hpp"

// Global variables for signal handling
std::atomic<bool> g_running(true);
vst::ProducerApp *g_app = nullptr;

// Signal handler for Ctrl+C and termination signals
void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    g_running = false;

    // Give some time for threads to clean up
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

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

    // In case of SIGTERM or SIGINT, exit immediately after cleanup
    if (signum == SIGTERM || signum == SIGINT)
    {
        exit(0);
    }
}

void print_usage()
{
    std::cerr << "Usage: ./vst_producer [-i <image_path> | -v <video_path>] [--mode=shm|dma | -s | -d]\n";
    std::cerr << "  -i <image_path>   Path to image file\n";
    std::cerr << "  -v <video_path>   Path to video file\n";
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

    // Must have either -i or -v
    if (filePath.empty())
    {
        std::cerr << "Error: You must provide either an image (-i) or video (-v) path.\n";
        print_usage();
        return EXIT_FAILURE;
    }

    // Print selected options
    std::cout << "Producer Mode: " << mode << (modeSetExplicitly ? "" : " (default)") << "\n";
    std::cout << (isVideo ? "Video Path: " : "Image Path: ") << filePath << "\n";

    // Register signal handler for graceful shutdown
    signal(SIGINT, signalHandler);  // Ctrl+C
    signal(SIGTERM, signalHandler); // Termination request

    // Create the app and store in global variable for signal handler
    g_app = new vst::ProducerApp();

    // Use a try-finally style approach to ensure cleanup
    int result = EXIT_SUCCESS;

    try
    {
        // Check mode
        if (mode == "dma")
        {
            // DMA-BUF mode (Vulkan with window)
            // Initialize GLFW
            std::shared_ptr<vst::IAppWindow> window;
            if (!glfwInit())
            {
                std::cerr << "Failed to initialize GLFW.\n";
                result = EXIT_FAILURE;
                throw std::runtime_error("GLFW initialization failed");
            }

            int windowWidth = 0;
            int windowHeight = 0;

            // Create a window without OpenGL context
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            std::string titleWindow;

            if (isVideo)
            {
                vst::VideoInfo videoInfo = vst::VideoLoader::getVideoResolution(filePath);
                if (!videoInfo.valid)
                {
                    std::cerr << "Failed to load video resolution!\n";
                    result = EXIT_FAILURE;
                    throw std::runtime_error("Failed to load video resolution");
                }
                windowWidth = videoInfo.width;
                windowHeight = videoInfo.height;
                titleWindow = "Producer DMA-BUF - Video (" + std::to_string(windowWidth) + "x" + std::to_string(windowHeight) + ")";
                std::cout << "Video resolution: " << windowWidth << "x" << windowHeight << "\n";
            }
            else
            {
                // Get image size
                vst::utils::ImageSize imageData = vst::utils::getImageSize(filePath);
                windowWidth = imageData.width;
                windowHeight = imageData.height;
                titleWindow = "Producer DMA-BUF - Image (" + std::to_string(windowWidth) + "x" + std::to_string(windowHeight) + ")";
                std::cout << "Image resolution: " << windowWidth << "x" << windowHeight << "\n";
            }

            window = std::make_shared<vst::GLFWWindowWrapper>(windowWidth, windowHeight, titleWindow.c_str());
            GLFWwindow *glfwWindow = static_cast<GLFWwindow *>(window->getNativeHandle());
            if (!glfwWindow)
            {
                std::cerr << "Failed to create window.\n";
                glfwTerminate();
                result = EXIT_FAILURE;
                throw std::runtime_error("Failed to create GLFW window");
            }

            // Start the producer app
            g_app->ProducerDMA(glfwWindow, filePath, mode, isVideo);

            // Main loop
            while (!glfwWindowShouldClose(glfwWindow) && g_running)
            {
                glfwPollEvents();
                g_app->update();
                g_app->runFrame();
            }

            glfwDestroyWindow(glfwWindow);
            glfwTerminate();
        }
        else if (mode == "shm")
        {
            // Initialize OpenCV window system before starting
            if (isVideo)
            {
                cv::namedWindow("Init", cv::WINDOW_AUTOSIZE);
                cv::destroyWindow("Init");
                cv::waitKey(1);
            }

            g_app->ProducerSHM(filePath, mode, isVideo);

            if (isVideo)
            {
                // For video in SHM mode, we'll handle the playback in the main thread
                std::cout << "Video streaming started in SHM mode. Press ESC or 'q' in the video window or Ctrl+C to stop.\n";

                // Get video properties
                cv::VideoCapture &cap = g_app->getVideoLoader()->getCapture();
                double fps = cap.get(cv::CAP_PROP_FPS);
                int frameDelay = std::max(1, static_cast<int>(1000.0 / fps));

                // Main video playback loop
                cv::Mat frame;
                int frameCount = 0;
                auto startTime = std::chrono::steady_clock::now();

                // While the app is running and the user hasn't pressed quit
                while (g_running)
                {
                    // Measure the start time of this frame processing
                    auto frameStart = std::chrono::steady_clock::now();

                    // Try to grab a frame
                    if (!g_app->getVideoLoader()->grabFrame(frame))
                    {
                        // End of video reached, loop back
                        std::cout << "End of video reached, restarting..." << std::endl;
                        g_app->getVideoLoader()->getCapture().set(cv::CAP_PROP_POS_FRAMES, 0);
                        frameCount = 0;
                        startTime = std::chrono::steady_clock::now();
                        continue;
                    }

                    // Display the frame
                    cv::imshow(g_app->getWindowTitle(), frame);

                    // Convert to RGBA for shared memory
                    cv::Mat rgbaFrame;
                    cv::cvtColor(frame, rgbaFrame, cv::COLOR_BGR2RGBA);

                    // Calculate timestamp
                    auto now = std::chrono::steady_clock::now();
                    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                             now - startTime)
                                             .count();

                    // Write frame to shared memory
                    g_app->getSharedMemoryHandler()->writeFrame(
                        rgbaFrame,
                        frameCount,
                        static_cast<uint32_t>(cap.get(cv::CAP_PROP_FRAME_COUNT)),
                        fps,
                        timestamp);

                    // Process window events and check for key press
                    int key = cv::waitKey(1);
                    if (key == 27 || key == 'q')
                    { // ESC or 'q' key
                        std::cout << "User pressed exit key" << std::endl;
                        g_running = false;
                        break;
                    }

                    // Measure how long this frame took to process
                    auto frameEnd = std::chrono::steady_clock::now();
                    auto processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                              frameEnd - frameStart)
                                              .count();

                    // Sleep for the remaining time to maintain frame rate
                    int remainingTime = frameDelay - static_cast<int>(processingTime);
                    if (remainingTime > 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(remainingTime));
                    }

                    // Log progress every 100 frames
                    frameCount++;
                    if (frameCount % 100 == 0)
                    {
                        std::cout << "Processed " << frameCount << " frames" << std::endl;
                    }
                }

                // Clean up
                cv::destroyAllWindows();
            }
            else
            {
                // For images, run the viewer
                g_app->runFrame(filePath);
            }
        }
        else
        {
            std::cerr << "Invalid mode: " << mode << "\n";
            result = EXIT_FAILURE;
            throw std::runtime_error("Invalid mode");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        result = EXIT_FAILURE;
    }

    // Always clean up
    try
    {
        g_app->cleanup();
        //delete g_app;
        g_app = nullptr;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] During cleanup: " << e.what() << std::endl;
        result = EXIT_FAILURE;
    }

    return result;
}