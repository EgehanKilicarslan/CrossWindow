/**
 * @file WindowManagerImpl.h
 * @brief Internal implementation interface for WindowManager
 */

#pragma once

#include "CrossWindow.h"

namespace CrossWindow
{

    /**
     * @brief Base interface for platform-specific implementations
     */
    class WindowManagerImplBase
    {
    public:
        virtual ~WindowManagerImplBase() = default;

        virtual bool Initialize() = 0;
        virtual bool IsInitialized() const = 0;
        virtual void Shutdown() = 0;

        // Enumeration
        virtual std::vector<WindowInfo> GetAllWindows() = 0;
        virtual void EnumerateWindows(const EnumWindowsCallback &callback) = 0;
        virtual std::vector<WindowInfo> FindWindowsByTitle(const std::string &titlePattern,
                                                           bool caseSensitive) = 0;
        virtual std::vector<WindowInfo> FindWindowsByProcess(const std::string &processName) = 0;

        // Information
        virtual Result<WindowInfo> GetWindowInfo(NativeHandle handle) = 0;
        virtual Result<std::string> GetWindowTitle(NativeHandle handle) = 0;
        virtual Result<Rect> GetWindowRect(NativeHandle handle) = 0;
        virtual Result<WindowState> GetWindowState(NativeHandle handle) = 0;
        virtual Result<uint32_t> GetWindowProcessId(NativeHandle handle) = 0;
        virtual bool IsWindowVisible(NativeHandle handle) = 0;
        virtual bool IsValidWindow(NativeHandle handle) = 0;

        // Active window
        virtual NativeHandle GetFocusedWindow() = 0;
        virtual Result<WindowInfo> GetFocusedWindowInfo() = 0;

        // Manipulation
        virtual ErrorCode CloseWindow(NativeHandle handle) = 0;
        virtual ErrorCode ForceCloseWindow(NativeHandle handle) = 0;
        virtual ErrorCode MinimizeWindow(NativeHandle handle) = 0;
        virtual ErrorCode MaximizeWindow(NativeHandle handle) = 0;
        virtual ErrorCode RestoreWindow(NativeHandle handle) = 0;
        virtual ErrorCode ShowWindow(NativeHandle handle) = 0;
        virtual ErrorCode HideWindow(NativeHandle handle) = 0;
        virtual ErrorCode FocusWindow(NativeHandle handle) = 0;
        virtual ErrorCode SetAlwaysOnTop(NativeHandle handle, bool topmost) = 0;
        virtual ErrorCode SetWindowRect(NativeHandle handle, const Rect &rect) = 0;
        virtual ErrorCode MoveWindow(NativeHandle handle, int x, int y) = 0;
        virtual ErrorCode ResizeWindow(NativeHandle handle, int width, int height) = 0;
        virtual ErrorCode SetWindowTitle(NativeHandle handle, const std::string &title) = 0;
        virtual ErrorCode SetWindowOpacity(NativeHandle handle, float opacity) = 0;

        // Error handling
        virtual std::string GetLastError() const = 0;
        virtual void SetLastError(const std::string &error) = 0;

    protected:
        bool m_initialized = false;
        std::string m_lastError;
    };

} // namespace CrossWindow
