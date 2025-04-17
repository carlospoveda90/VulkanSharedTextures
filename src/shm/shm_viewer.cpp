#include "shm/shm_viewer.hpp"
#include "utils/file_utils.hpp"
#include <SDL3/SDL.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>

namespace vst::shm
{

    // void run_viewer(const char *shmName, int width, int height)
    void run_viewer(const char *shmName, const std::string &owner)
    {
        vst::utils::ImageSize imageData = vst::utils::parseImageDimensions(shmName);
        std::cout << "[INFO] Image dimensions : " << imageData.width << "x" << imageData.height << std::endl;

        const size_t imageSize = imageData.width * imageData.height * 4;
        int shm_fd = shm_open(shmName, O_RDONLY, 0666);
        if (shm_fd < 0)
        {
            perror("shm_open");
            return;
        }

        void *data = mmap(nullptr, imageSize, PROT_READ, MAP_SHARED, shm_fd, 0);
        if (data == MAP_FAILED)
        {
            perror("mmap");
            close(shm_fd);
            return;
        }

        // Initialize SDL for preview window
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return;
        }

        SDL_Window *window = SDL_CreateWindow(("SHM Viewer " + owner).c_str(), imageData.width, imageData.height, SDL_WINDOW_RESIZABLE);
        // SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr, 0);
        SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
        SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, imageData.width, imageData.height);

        bool running = true;
        SDL_Event e;
        while (running)
        {
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_EVENT_QUIT)
                    running = false;
            }

            SDL_UpdateTexture(texture, nullptr, data, imageData.width * 4);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }

        munmap(data, imageSize);
        close(shm_fd);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

} // namespace vst::shm
