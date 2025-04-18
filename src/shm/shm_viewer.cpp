#include "shm/shm_viewer.hpp"
#include "utils/file_utils.hpp"
#include <SDL3/SDL.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>

namespace vst::shm
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;

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

        window = SDL_CreateWindow(("SHM Viewer " + owner).c_str(), imageData.width, imageData.height, SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(window, nullptr);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, imageData.width, imageData.height);

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
    }

    void destroy_viewer(const std::string &shmName)
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (renderer)
        {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        SDL_Quit();

        if (!shmName.empty())
        {
            shm_unlink(shmName.c_str());
            std::cout << "[INFO] Unlinked SHM file: " << shmName << std::endl;
        }
    }

} // namespace vst::shm
