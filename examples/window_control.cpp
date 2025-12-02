/**
 * @file window_control.cpp
 * @brief Example: Interactive window control demo
 */

#include "CrossWindow.h"
#include <iostream>
#include <string>
#include <limits>

using namespace CrossWindow;

void PrintMenu()
{
    std::cout << "\n=== Window Control Menu ===\n";
    std::cout << "1. List all windows\n";
    std::cout << "2. Find window by title\n";
    std::cout << "3. Get focused window info\n";
    std::cout << "4. Minimize a window\n";
    std::cout << "5. Maximize a window\n";
    std::cout << "6. Restore a window\n";
    std::cout << "7. Close a window\n";
    std::cout << "8. Focus a window\n";
    std::cout << "9. Move a window\n";
    std::cout << "0. Exit\n";
    std::cout << "Choice: ";
}

void ListWindows(WindowManager &wm)
{
    auto windows = wm.GetAllWindows();
    std::cout << "\nFound " << windows.size() << " windows:\n";

    for (size_t i = 0; i < windows.size(); ++i)
    {
        std::cout << "[" << i << "] " << windows[i].title
                  << " (" << windows[i].processName << ")\n";
    }
}

NativeHandle SelectWindow(WindowManager &wm)
{
    auto windows = wm.GetAllWindows();

    if (windows.empty())
    {
        std::cout << "No windows found.\n";
        return NativeHandle{};
    }

    ListWindows(wm);

    std::cout << "Enter window number: ";
    size_t index;
    std::cin >> index;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (index >= windows.size())
    {
        std::cout << "Invalid selection.\n";
        return NativeHandle{};
    }

    return windows[index].handle;
}

int main()
{
    std::cout << "CrossWindow - Interactive Window Control\n";
    std::cout << "========================================\n";
    std::cout << "Platform: " << WindowManager::GetPlatformName() << "\n";

    WindowManager wm;
    if (!wm.Initialize())
    {
        std::cerr << "Failed to initialize: " << wm.GetLastError() << "\n";
        return 1;
    }

    int choice = -1;
    while (choice != 0)
    {
        PrintMenu();
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        NativeHandle handle;

        switch (choice)
        {
        case 1:
            ListWindows(wm);
            break;

        case 2:
        {
            std::cout << "Enter search term: ";
            std::string term;
            std::getline(std::cin, term);

            auto results = wm.FindWindowsByTitle(term);
            std::cout << "Found " << results.size() << " matching windows:\n";
            for (const auto &w : results)
            {
                std::cout << "  - " << w.title << " (" << w.processName << ")\n";
            }
        }
        break;

        case 3:
        {
            auto focused = wm.GetFocusedWindowInfo();
            if (focused.ok())
            {
                std::cout << "Focused window: " << focused.value.title << "\n";
                std::cout << "Process: " << focused.value.processName << "\n";
                std::cout << "Position: " << focused.value.rect.x << ", "
                          << focused.value.rect.y << "\n";
                std::cout << "Size: " << focused.value.rect.width << " x "
                          << focused.value.rect.height << "\n";
            }
            else
            {
                std::cout << "No focused window.\n";
            }
        }
        break;

        case 4:
            handle = SelectWindow(wm);
            if (handle)
            {
                auto result = wm.MinimizeWindow(handle);
                std::cout << (result == ErrorCode::Success ? "Minimized!" : "Failed.") << "\n";
            }
            break;

        case 5:
            handle = SelectWindow(wm);
            if (handle)
            {
                auto result = wm.MaximizeWindow(handle);
                std::cout << (result == ErrorCode::Success ? "Maximized!" : "Failed.") << "\n";
            }
            break;

        case 6:
            handle = SelectWindow(wm);
            if (handle)
            {
                auto result = wm.RestoreWindow(handle);
                std::cout << (result == ErrorCode::Success ? "Restored!" : "Failed.") << "\n";
            }
            break;

        case 7:
            handle = SelectWindow(wm);
            if (handle)
            {
                auto result = wm.CloseWindow(handle);
                std::cout << (result == ErrorCode::Success ? "Close request sent!" : "Failed.") << "\n";
            }
            break;

        case 8:
            handle = SelectWindow(wm);
            if (handle)
            {
                auto result = wm.FocusWindow(handle);
                std::cout << (result == ErrorCode::Success ? "Focused!" : "Failed.") << "\n";
            }
            break;

        case 9:
            handle = SelectWindow(wm);
            if (handle)
            {
                int x, y;
                std::cout << "Enter new X position: ";
                std::cin >> x;
                std::cout << "Enter new Y position: ";
                std::cin >> y;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                auto result = wm.MoveWindow(handle, x, y);
                std::cout << (result == ErrorCode::Success ? "Moved!" : "Failed.") << "\n";
            }
            break;

        case 0:
            std::cout << "Goodbye!\n";
            break;

        default:
            std::cout << "Invalid choice.\n";
            break;
        }
    }

    wm.Shutdown();
    return 0;
}
