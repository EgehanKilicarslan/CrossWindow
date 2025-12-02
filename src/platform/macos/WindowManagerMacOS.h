/**
 * @file WindowManagerMacOS.h
 * @brief macOS implementation of WindowManager
 */

#pragma once

#include "../../WindowManagerImpl.h"

namespace CrossWindow
{

    class WindowManagerMacOS : public WindowManagerImplBase
    {
    public:
        WindowManagerMacOS();
        ~WindowManagerMacOS() override;

        bool Initialize() override;
        bool IsInitialized() const override;
        void Shutdown() override;

        // Enumeration
        std::vector<WindowInfo> GetAllWindows() override;
        void EnumerateWindows(const EnumWindowsCallback &callback) override;
        std::vector<WindowInfo> FindWindowsByTitle(const std::string &titlePattern,
                                                   bool caseSensitive) override;
        std::vector<WindowInfo> FindWindowsByProcess(const std::string &processName) override;

        // Information
        Result<WindowInfo> GetWindowInfo(NativeHandle handle) override;
        Result<std::string> GetWindowTitle(NativeHandle handle) override;
        Result<Rect> GetWindowRect(NativeHandle handle) override;
        Result<WindowState> GetWindowState(NativeHandle handle) override;
        Result<uint32_t> GetWindowProcessId(NativeHandle handle) override;
        bool IsWindowVisible(NativeHandle handle) override;
        bool IsValidWindow(NativeHandle handle) override;

        // Active window
        NativeHandle GetFocusedWindow() override;
        Result<WindowInfo> GetFocusedWindowInfo() override;

        // Manipulation
        ErrorCode CloseWindow(NativeHandle handle) override;
        ErrorCode ForceCloseWindow(NativeHandle handle) override;
        ErrorCode MinimizeWindow(NativeHandle handle) override;
        ErrorCode MaximizeWindow(NativeHandle handle) override;
        ErrorCode RestoreWindow(NativeHandle handle) override;
        ErrorCode ShowWindow(NativeHandle handle) override;
        ErrorCode HideWindow(NativeHandle handle) override;
        ErrorCode FocusWindow(NativeHandle handle) override;
        ErrorCode SetAlwaysOnTop(NativeHandle handle, bool topmost) override;
        ErrorCode SetWindowRect(NativeHandle handle, const Rect &rect) override;
        ErrorCode MoveWindow(NativeHandle handle, int x, int y) override;
        ErrorCode ResizeWindow(NativeHandle handle, int width, int height) override;
        ErrorCode SetWindowTitle(NativeHandle handle, const std::string &title) override;
        ErrorCode SetWindowOpacity(NativeHandle handle, float opacity) override;

        std::string GetLastError() const override;
        void SetLastError(const std::string &error) override;

    private:
        bool ToLowerCompare(const std::string &str, const std::string &pattern);
    };

} // namespace CrossWindow
