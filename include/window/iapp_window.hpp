// include/window/iapp_window.hpp
#pragma once

namespace vst
{
    class IAppWindow
    {
    public:
        virtual ~IAppWindow() = default;
        virtual void pollEvents() = 0;
        virtual bool shouldClose() const = 0;
        virtual void *getNativeHandle() const = 0;
    };
} // namespace vst
