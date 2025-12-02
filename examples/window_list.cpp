/**
 * @file window_list.cpp
 * @brief Example: List all open windows with their details
 */

#include "CrossWindow.h"
#include <iostream>
#include <iomanip>

using namespace CrossWindow;

std::string StateToString(WindowState state)
{
    std::string result;
    if (HasFlag(state, WindowState::Minimized))
        result += "Minimized ";
    if (HasFlag(state, WindowState::Maximized))
        result += "Maximized ";
    if (HasFlag(state, WindowState::Fullscreen))
        result += "Fullscreen ";
    if (HasFlag(state, WindowState::Hidden))
        result += "Hidden ";
    if (HasFlag(state, WindowState::Focused))
        result += "Focused ";
    if (HasFlag(state, WindowState::AlwaysOnTop))
        result += "AlwaysOnTop ";
    if (result.empty())
        result = "Normal";
    return result;
}

int main()
{
    std::cout << "CrossWindow - Window List Example\n";
    std::cout << "==================================\n";
    std::cout << "Platform: " << WindowManager::GetPlatformName() << "\n\n";

    WindowManager wm;
    if (!wm.Initialize())
    {
        std::cerr << "Failed to initialize: " << wm.GetLastError() << "\n";
        return 1;
    }

    auto windows = wm.GetAllWindows();

    std::cout << "Found " << windows.size() << " windows:\n\n";

    for (size_t i = 0; i < windows.size(); ++i)
    {
        const auto &w = windows[i];

        std::cout << "[" << std::setw(3) << (i + 1) << "] ";

        // Truncate title if too long
        std::string title = w.title;
        if (title.length() > 50)
        {
            title = title.substr(0, 47) + "...";
        }

        std::cout << "\"" << title << "\"\n";
        std::cout << "      Process: " << w.processName << " (PID: " << w.processId << ")\n";
        std::cout << "      Class: " << w.className << "\n";
        std::cout << "      Position: " << w.rect.x << ", " << w.rect.y << "\n";
        std::cout << "      Size: " << w.rect.width << " x " << w.rect.height << "\n";
        std::cout << "      State: " << StateToString(w.state) << "\n";
        std::cout << "      Visible: " << (w.isVisible ? "Yes" : "No") << "\n";
        std::cout << "\n";
    }

    // Show focused window
    auto focused = wm.GetFocusedWindowInfo();
    if (focused.ok())
    {
        std::cout << "Currently focused: \"" << focused.value.title << "\"\n";
    }

    wm.Shutdown();
    return 0;
}
