/**
 * @file WindowManagerLinux.h
 * @brief Linux (X11) implementation of WindowManager
 */

#pragma once

#include "../../WindowManagerImpl.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

namespace CrossWindow
{

    class WindowManagerLinux : public WindowManagerImplBase
    {
    public:
        WindowManagerLinux();
        ~WindowManagerLinux() override;

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
        Display *m_display = nullptr;
        Window m_rootWindow = 0;

        // Atom cache
        Atom m_atomNetClientList = 0;
        Atom m_atomNetActiveWindow = 0;
        Atom m_atomNetWmName = 0;
        Atom m_atomNetWmPid = 0;
        Atom m_atomNetWmState = 0;
        Atom m_atomNetWmStateHidden = 0;
        Atom m_atomNetWmStateMaximizedVert = 0;
        Atom m_atomNetWmStateMaximizedHorz = 0;
        Atom m_atomNetWmStateFullscreen = 0;
        Atom m_atomNetWmStateAbove = 0;
        Atom m_atomNetCloseWindow = 0;
        Atom m_atomWmState = 0;
        Atom m_atomWmChangeState = 0;
        Atom m_atomUtf8String = 0;
        Atom m_atomWmName = 0;
        Atom m_atomWmClass = 0;
        Atom m_atomNetWmWindowOpacity = 0;

        void InitializeAtoms();
        std::string GetWindowTitleInternal(Window window);
        std::string GetWindowClassInternal(Window window);
        uint32_t GetWindowPidInternal(Window window);
        std::string GetProcessNameFromPid(uint32_t pid);
        bool HasWmState(Window window, Atom state);
        void SendClientMessage(Window window, Atom messageType, long data0 = 0,
                               long data1 = 0, long data2 = 0, long data3 = 0, long data4 = 0);
        void SetWmState(Window window, bool add, Atom state1, Atom state2 = 0);
        std::vector<Window> GetClientList();
        bool ToLowerCompare(const std::string &str, const std::string &pattern);
    };

} // namespace CrossWindow
