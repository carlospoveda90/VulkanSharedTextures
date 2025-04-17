#include "window/glfw_window.hpp"

namespace vst
{
    GLFWWindowWrapper::GLFWWindowWrapper(int width, int height, const char *title)
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    }

    GLFWWindowWrapper::~GLFWWindowWrapper()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void GLFWWindowWrapper::pollEvents()
    {
        glfwPollEvents();
    }

    bool GLFWWindowWrapper::shouldClose() const
    {
        return glfwWindowShouldClose(window);
    }

    void *GLFWWindowWrapper::getNativeHandle() const
    {
        return static_cast<void *>(window);
    }
}
