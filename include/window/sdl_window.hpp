#pragma once
#include "window/iapp_window.hpp"
#include <SDL3/SDL.h>

namespace vst
{
    class SDLWindowWrapper : public IAppWindow
    {
    public:
        SDLWindowWrapper(int width, int height, const char *title);
        ~SDLWindowWrapper() override;

        void pollEvents() override;
        bool shouldClose() const override;
        void *getNativeHandle() const override;

    private:
        SDL_Window *window;
        bool closed = false;
    };
}
