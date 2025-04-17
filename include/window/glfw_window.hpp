#pragma once
#include "window/iapp_window.hpp"
#include <GLFW/glfw3.h>

namespace vst
{
    class GLFWWindowWrapper : public IAppWindow
    {
    public:
        GLFWWindowWrapper(int width, int height, const char *title);
        ~GLFWWindowWrapper() override;

        void pollEvents() override;
        bool shouldClose() const override;
        void *getNativeHandle() const override;

    private:
        GLFWwindow *window;
    };
}
