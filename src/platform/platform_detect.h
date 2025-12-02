#pragma once

// Platform detection macros
#ifdef _WIN32
#define CROSSWINDOW_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__APPLE__)
#define CROSSWINDOW_PLATFORM_MACOS
#elif defined(__linux__)
#define CROSSWINDOW_PLATFORM_LINUX
#include <cstdlib>
#include <cstring>
#include <string>
#else
#define CROSSWINDOW_PLATFORM_STUB
#endif

namespace CrossWindow
{
    namespace Internal
    {

#ifdef CROSSWINDOW_PLATFORM_LINUX
        // Check if Wayland compositor is running
        inline bool IsWayland()
        {
            return std::getenv("WAYLAND_DISPLAY") != nullptr;
        }

        // Check if we're in a Wayland session (even if WAYLAND_DISPLAY is unset)
        inline bool IsWaylandSession()
        {
            const char *session_type = std::getenv("XDG_SESSION_TYPE");
            return session_type && std::string(session_type) == "wayland";
        }

        // Check if X11/XWayland is available for reading state
        inline bool HasX11Display()
        {
            return std::getenv("DISPLAY") != nullptr;
        }
#endif

    } // namespace Internal
} // namespace CrossWindow
