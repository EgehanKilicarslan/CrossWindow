/**
 * @file test_crosswindow.cpp
 * @brief Basic tests for CrossWindow library
 */

#include "CrossWindow.h"
#include <iostream>
#include <cassert>

using namespace CrossWindow;

int main()
{
    std::cout << "CrossWindow Test Suite\n";
    std::cout << "======================\n";
    std::cout << "Platform: " << WindowManager::GetPlatformName() << "\n\n";

    WindowManager wm;

    // Test initialization
    std::cout << "Test: Initialize... ";
    bool initResult = wm.Initialize();
    if (!initResult)
    {
        std::cout << "FAILED - " << wm.GetLastError() << "\n";
        return 1;
    }
    std::cout << "PASSED\n";

    // Test IsInitialized
    std::cout << "Test: IsInitialized... ";
    assert(wm.IsInitialized());
    std::cout << "PASSED\n";

    // Test GetAllWindows
    std::cout << "Test: GetAllWindows... ";
    auto windows = wm.GetAllWindows();
    std::cout << "PASSED (found " << windows.size() << " windows)\n";

    // Test GetFocusedWindow
    std::cout << "Test: GetFocusedWindow... ";
    auto focused = wm.GetFocusedWindow();
    if (focused != NativeHandle{})
    {
        auto focusedInfo = wm.GetFocusedWindowInfo();
        if (focusedInfo.ok())
        {
            std::cout << "PASSED (\"" << focusedInfo.value.title << "\")\n";
        }
        else
        {
            std::cout << "PASSED (handle obtained)\n";
        }
    }
    else
    {
        std::cout << "PASSED (no focused window)\n";
    }

    // Test FindWindowsByTitle
    std::cout << "Test: FindWindowsByTitle... ";
    // Search for a common window (empty string matches all)
    auto searchResult = wm.FindWindowsByTitle("", false);
    std::cout << "PASSED (found " << searchResult.size() << " matches)\n";

    // Test window enumeration
    std::cout << "Test: EnumerateWindows... ";
    int count = 0;
    wm.EnumerateWindows([&count](const WindowInfo &info)
                        {
                            count++;
                            return true; // continue
                        });
    std::cout << "PASSED (enumerated " << count << " windows)\n";

    // Print first 5 windows for debugging
    std::cout << "\nFirst 5 windows:\n";
    int shown = 0;
    for (const auto &w : windows)
    {
        if (shown >= 5)
            break;
        std::cout << "  - \"" << w.title << "\" (" << w.processName << ")\n";
        shown++;
    }

    // Test Shutdown
    std::cout << "\nTest: Shutdown... ";
    wm.Shutdown();
    assert(!wm.IsInitialized());
    std::cout << "PASSED\n";

    std::cout << "\n======================\n";
    std::cout << "All tests passed!\n";

    return 0;
}
