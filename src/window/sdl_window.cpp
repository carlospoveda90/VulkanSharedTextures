#include "window/sdl_window.hpp"

namespace vst
{
    SDLWindowWrapper::SDLWindowWrapper(int width, int height, const char *title)
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);
    }

    SDLWindowWrapper::~SDLWindowWrapper()
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void SDLWindowWrapper::pollEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                closed = true;
        }
    }

    bool SDLWindowWrapper::shouldClose() const
    {
        return closed;
    }

    void *SDLWindowWrapper::getNativeHandle() const
    {
        return static_cast<void *>(window);
    }
}
