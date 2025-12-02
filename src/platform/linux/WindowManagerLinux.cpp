/**
 * @file WindowManagerLinux.cpp
 * @brief Linux (X11) implementation of WindowManager
 */

#include "WindowManagerLinux.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <X11/Xutil.h>

// X11 headers define Success as a macro (value 0), which conflicts with our ErrorCode::Success
// Save the value and undefine the macro
#ifdef Success
static constexpr int X11Success = Success;
#undef Success
#else
static constexpr int X11Success = 0;
#endif

namespace CrossWindow
{

    WindowManagerLinux::WindowManagerLinux() = default;

    WindowManagerLinux::~WindowManagerLinux()
    {
        Shutdown();
    }

    bool WindowManagerLinux::Initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        m_display = XOpenDisplay(nullptr);
        if (!m_display)
        {
            SetLastError("Failed to open X11 display");
            return false;
        }

        m_rootWindow = DefaultRootWindow(m_display);
        InitializeAtoms();
        m_initialized = true;
        return true;
    }

    bool WindowManagerLinux::IsInitialized() const
    {
        return m_initialized;
    }

    void WindowManagerLinux::Shutdown()
    {
        if (m_display)
        {
            XCloseDisplay(m_display);
            m_display = nullptr;
        }
        m_initialized = false;
    }

    void WindowManagerLinux::InitializeAtoms()
    {
        m_atomNetClientList = XInternAtom(m_display, "_NET_CLIENT_LIST", False);
        m_atomNetActiveWindow = XInternAtom(m_display, "_NET_ACTIVE_WINDOW", False);
        m_atomNetWmName = XInternAtom(m_display, "_NET_WM_NAME", False);
        m_atomNetWmPid = XInternAtom(m_display, "_NET_WM_PID", False);
        m_atomNetWmState = XInternAtom(m_display, "_NET_WM_STATE", False);
        m_atomNetWmStateHidden = XInternAtom(m_display, "_NET_WM_STATE_HIDDEN", False);
        m_atomNetWmStateMaximizedVert = XInternAtom(m_display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
        m_atomNetWmStateMaximizedHorz = XInternAtom(m_display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        m_atomNetWmStateFullscreen = XInternAtom(m_display, "_NET_WM_STATE_FULLSCREEN", False);
        m_atomNetWmStateAbove = XInternAtom(m_display, "_NET_WM_STATE_ABOVE", False);
        m_atomNetCloseWindow = XInternAtom(m_display, "_NET_CLOSE_WINDOW", False);
        m_atomWmState = XInternAtom(m_display, "WM_STATE", False);
        m_atomWmChangeState = XInternAtom(m_display, "WM_CHANGE_STATE", False);
        m_atomUtf8String = XInternAtom(m_display, "UTF8_STRING", False);
        m_atomWmName = XInternAtom(m_display, "WM_NAME", False);
        m_atomWmClass = XInternAtom(m_display, "WM_CLASS", False);
        m_atomNetWmWindowOpacity = XInternAtom(m_display, "_NET_WM_WINDOW_OPACITY", False);
    }

    std::vector<Window> WindowManagerLinux::GetClientList()
    {
        std::vector<Window> windows;

        Atom actualType;
        int actualFormat;
        unsigned long numItems, bytesAfter;
        unsigned char *data = nullptr;

        int status = XGetWindowProperty(m_display, m_rootWindow, m_atomNetClientList,
                                        0, (~0L), False, XA_WINDOW,
                                        &actualType, &actualFormat, &numItems,
                                        &bytesAfter, &data);

        if (status == X11Success && data)
        {
            Window *windowList = reinterpret_cast<Window *>(data);
            for (unsigned long i = 0; i < numItems; ++i)
            {
                windows.push_back(windowList[i]);
            }
            XFree(data);
        }

        return windows;
    }

    std::string WindowManagerLinux::GetWindowTitleInternal(Window window)
    {
        std::string title;

        // Try _NET_WM_NAME first (UTF-8)
        Atom actualType;
        int actualFormat;
        unsigned long numItems, bytesAfter;
        unsigned char *data = nullptr;

        int status = XGetWindowProperty(m_display, window, m_atomNetWmName,
                                        0, (~0L), False, m_atomUtf8String,
                                        &actualType, &actualFormat, &numItems,
                                        &bytesAfter, &data);

        if (status == X11Success && data && numItems > 0)
        {
            title = std::string(reinterpret_cast<char *>(data), numItems);
            XFree(data);
            return title;
        }

        if (data)
            XFree(data);

        // Fall back to WM_NAME
        char *name = nullptr;
        if (XFetchName(m_display, window, &name) && name)
        {
            title = name;
            XFree(name);
        }

        return title;
    }

    std::string WindowManagerLinux::GetWindowClassInternal(Window window)
    {
        XClassHint classHint;
        if (XGetClassHint(m_display, window, &classHint))
        {
            std::string className = classHint.res_class ? classHint.res_class : "";
            if (classHint.res_name)
                XFree(classHint.res_name);
            if (classHint.res_class)
                XFree(classHint.res_class);
            return className;
        }
        return "";
    }

    uint32_t WindowManagerLinux::GetWindowPidInternal(Window window)
    {
        Atom actualType;
        int actualFormat;
        unsigned long numItems, bytesAfter;
        unsigned char *data = nullptr;

        int status = XGetWindowProperty(m_display, window, m_atomNetWmPid,
                                        0, 1, False, XA_CARDINAL,
                                        &actualType, &actualFormat, &numItems,
                                        &bytesAfter, &data);

        uint32_t pid = 0;
        if (status == X11Success && data && numItems > 0)
        {
            pid = *reinterpret_cast<uint32_t *>(data);
        }

        if (data)
            XFree(data);

        return pid;
    }

    std::string WindowManagerLinux::GetProcessNameFromPid(uint32_t pid)
    {
        if (pid == 0)
            return "";

        std::string path = "/proc/" + std::to_string(pid) + "/comm";
        std::ifstream file(path);
        std::string name;
        if (file.is_open() && std::getline(file, name))
        {
            // Remove trailing newline if present
            if (!name.empty() && name.back() == '\n')
            {
                name.pop_back();
            }
            return name;
        }
        return "";
    }

    bool WindowManagerLinux::HasWmState(Window window, Atom state)
    {
        Atom actualType;
        int actualFormat;
        unsigned long numItems, bytesAfter;
        unsigned char *data = nullptr;

        int status = XGetWindowProperty(m_display, window, m_atomNetWmState,
                                        0, (~0L), False, XA_ATOM,
                                        &actualType, &actualFormat, &numItems,
                                        &bytesAfter, &data);

        bool hasState = false;
        if (status == X11Success && data)
        {
            Atom *atoms = reinterpret_cast<Atom *>(data);
            for (unsigned long i = 0; i < numItems; ++i)
            {
                if (atoms[i] == state)
                {
                    hasState = true;
                    break;
                }
            }
            XFree(data);
        }

        return hasState;
    }

    void WindowManagerLinux::SendClientMessage(Window window, Atom messageType,
                                               long data0, long data1, long data2,
                                               long data3, long data4)
    {
        XEvent event;
        memset(&event, 0, sizeof(event));
        event.xclient.type = ClientMessage;
        event.xclient.serial = 0;
        event.xclient.send_event = True;
        event.xclient.message_type = messageType;
        event.xclient.window = window;
        event.xclient.format = 32;
        event.xclient.data.l[0] = data0;
        event.xclient.data.l[1] = data1;
        event.xclient.data.l[2] = data2;
        event.xclient.data.l[3] = data3;
        event.xclient.data.l[4] = data4;

        XSendEvent(m_display, m_rootWindow, False,
                   SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XFlush(m_display);
    }

    void WindowManagerLinux::SetWmState(Window window, bool add, Atom state1, Atom state2)
    {
        SendClientMessage(window, m_atomNetWmState,
                          add ? 1 : 0, // _NET_WM_STATE_ADD = 1, _NET_WM_STATE_REMOVE = 0
                          static_cast<long>(state1),
                          static_cast<long>(state2),
                          1); // Source indication (1 = application)
    }

    bool WindowManagerLinux::ToLowerCompare(const std::string &str, const std::string &pattern)
    {
        std::string lowerStr = str;
        std::string lowerPattern = pattern;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
        std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
        return lowerStr.find(lowerPattern) != std::string::npos;
    }

    std::vector<WindowInfo> WindowManagerLinux::GetAllWindows()
    {
        std::vector<WindowInfo> result;

        if (!m_initialized)
        {
            SetLastError("WindowManager not initialized");
            return result;
        }

        auto windows = GetClientList();
        for (Window w : windows)
        {
            auto info = GetWindowInfo(w);
            if (info.ok())
            {
                result.push_back(info.value);
            }
        }

        return result;
    }

    void WindowManagerLinux::EnumerateWindows(const EnumWindowsCallback &callback)
    {
        if (!m_initialized)
        {
            return;
        }

        auto windows = GetClientList();
        for (Window w : windows)
        {
            auto info = GetWindowInfo(w);
            if (info.ok())
            {
                if (!callback(info.value))
                {
                    break;
                }
            }
        }
    }

    std::vector<WindowInfo> WindowManagerLinux::FindWindowsByTitle(const std::string &titlePattern,
                                                                   bool caseSensitive)
    {
        std::vector<WindowInfo> result;

        if (!m_initialized)
        {
            return result;
        }

        auto windows = GetClientList();
        for (Window w : windows)
        {
            std::string title = GetWindowTitleInternal(w);

            bool matches = false;
            if (caseSensitive)
            {
                matches = title.find(titlePattern) != std::string::npos;
            }
            else
            {
                matches = ToLowerCompare(title, titlePattern);
            }

            if (matches)
            {
                auto info = GetWindowInfo(w);
                if (info.ok())
                {
                    result.push_back(info.value);
                }
            }
        }

        return result;
    }

    std::vector<WindowInfo> WindowManagerLinux::FindWindowsByProcess(const std::string &processName)
    {
        std::vector<WindowInfo> result;

        if (!m_initialized)
        {
            return result;
        }

        auto windows = GetClientList();
        for (Window w : windows)
        {
            uint32_t pid = GetWindowPidInternal(w);
            std::string procName = GetProcessNameFromPid(pid);

            if (ToLowerCompare(procName, processName))
            {
                auto info = GetWindowInfo(w);
                if (info.ok())
                {
                    result.push_back(info.value);
                }
            }
        }

        return result;
    }

    Result<WindowInfo> WindowManagerLinux::GetWindowInfo(NativeHandle handle)
    {
        Result<WindowInfo> result;
        Window window = static_cast<Window>(handle);

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsValidWindow(handle))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        result.value.handle = handle;
        result.value.title = GetWindowTitleInternal(window);
        result.value.className = GetWindowClassInternal(window);
        result.value.processId = GetWindowPidInternal(window);
        result.value.processName = GetProcessNameFromPid(result.value.processId);

        // Get geometry
        XWindowAttributes attrs;
        if (XGetWindowAttributes(m_display, window, &attrs))
        {
            // Get the absolute position
            Window child;
            int absX, absY;
            XTranslateCoordinates(m_display, window, m_rootWindow, 0, 0, &absX, &absY, &child);

            result.value.rect.x = absX;
            result.value.rect.y = absY;
            result.value.rect.width = attrs.width;
            result.value.rect.height = attrs.height;
            result.value.isVisible = (attrs.map_state == IsViewable);
        }

        // Get state
        result.value.state = WindowState::Normal;
        if (HasWmState(window, m_atomNetWmStateHidden))
        {
            result.value.state = result.value.state | WindowState::Minimized;
        }
        if (HasWmState(window, m_atomNetWmStateMaximizedVert) &&
            HasWmState(window, m_atomNetWmStateMaximizedHorz))
        {
            result.value.state = result.value.state | WindowState::Maximized;
        }
        if (HasWmState(window, m_atomNetWmStateFullscreen))
        {
            result.value.state = result.value.state | WindowState::Fullscreen;
        }
        if (HasWmState(window, m_atomNetWmStateAbove))
        {
            result.value.state = result.value.state | WindowState::AlwaysOnTop;
        }

        // Check if focused
        Window focusedWindow = static_cast<Window>(GetFocusedWindow());
        if (focusedWindow == window)
        {
            result.value.state = result.value.state | WindowState::Focused;
        }

        result.error = ErrorCode::Success;
        return result;
    }

    Result<std::string> WindowManagerLinux::GetWindowTitle(NativeHandle handle)
    {
        Result<std::string> result;

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsValidWindow(handle))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        result.value = GetWindowTitleInternal(static_cast<Window>(handle));
        result.error = ErrorCode::Success;
        return result;
    }

    Result<Rect> WindowManagerLinux::GetWindowRect(NativeHandle handle)
    {
        Result<Rect> result;
        Window window = static_cast<Window>(handle);

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsValidWindow(handle))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        XWindowAttributes attrs;
        if (XGetWindowAttributes(m_display, window, &attrs))
        {
            Window child;
            int absX, absY;
            XTranslateCoordinates(m_display, window, m_rootWindow, 0, 0, &absX, &absY, &child);

            result.value.x = absX;
            result.value.y = absY;
            result.value.width = attrs.width;
            result.value.height = attrs.height;
            result.error = ErrorCode::Success;
        }
        else
        {
            result.error = ErrorCode::OperationFailed;
            result.errorMessage = "Failed to get window attributes";
        }

        return result;
    }

    Result<WindowState> WindowManagerLinux::GetWindowState(NativeHandle handle)
    {
        Result<WindowState> result;
        Window window = static_cast<Window>(handle);

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsValidWindow(handle))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        result.value = WindowState::Normal;

        if (HasWmState(window, m_atomNetWmStateHidden))
        {
            result.value = result.value | WindowState::Minimized;
        }
        if (HasWmState(window, m_atomNetWmStateMaximizedVert) &&
            HasWmState(window, m_atomNetWmStateMaximizedHorz))
        {
            result.value = result.value | WindowState::Maximized;
        }
        if (HasWmState(window, m_atomNetWmStateFullscreen))
        {
            result.value = result.value | WindowState::Fullscreen;
        }
        if (HasWmState(window, m_atomNetWmStateAbove))
        {
            result.value = result.value | WindowState::AlwaysOnTop;
        }

        Window focusedWindow = static_cast<Window>(GetFocusedWindow());
        if (focusedWindow == window)
        {
            result.value = result.value | WindowState::Focused;
        }

        result.error = ErrorCode::Success;
        return result;
    }

    Result<uint32_t> WindowManagerLinux::GetWindowProcessId(NativeHandle handle)
    {
        Result<uint32_t> result;

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsValidWindow(handle))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        result.value = GetWindowPidInternal(static_cast<Window>(handle));
        result.error = ErrorCode::Success;
        return result;
    }

    bool WindowManagerLinux::IsWindowVisible(NativeHandle handle)
    {
        if (!m_initialized || !IsValidWindow(handle))
        {
            return false;
        }

        XWindowAttributes attrs;
        if (XGetWindowAttributes(m_display, static_cast<Window>(handle), &attrs))
        {
            return attrs.map_state == IsViewable;
        }
        return false;
    }

    bool WindowManagerLinux::IsValidWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return false;
        }

        Window window = static_cast<Window>(handle);
        XWindowAttributes attrs;

        // Set error handler to catch BadWindow errors
        XErrorHandler oldHandler = XSetErrorHandler([](Display *, XErrorEvent *) -> int
                                                    { return 0; });

        bool valid = XGetWindowAttributes(m_display, window, &attrs) != 0;

        XSetErrorHandler(oldHandler);
        return valid;
    }

    NativeHandle WindowManagerLinux::GetFocusedWindow()
    {
        if (!m_initialized)
        {
            return 0;
        }

        Atom actualType;
        int actualFormat;
        unsigned long numItems, bytesAfter;
        unsigned char *data = nullptr;

        int status = XGetWindowProperty(m_display, m_rootWindow, m_atomNetActiveWindow,
                                        0, 1, False, XA_WINDOW,
                                        &actualType, &actualFormat, &numItems,
                                        &bytesAfter, &data);

        Window activeWindow = 0;
        if (status == X11Success && data && numItems > 0)
        {
            activeWindow = *reinterpret_cast<Window *>(data);
        }

        if (data)
            XFree(data);

        return activeWindow;
    }

    Result<WindowInfo> WindowManagerLinux::GetFocusedWindowInfo()
    {
        NativeHandle focused = GetFocusedWindow();
        if (focused == 0)
        {
            Result<WindowInfo> result;
            result.error = ErrorCode::WindowNotFound;
            result.errorMessage = "No focused window found";
            return result;
        }
        return GetWindowInfo(focused);
    }

    ErrorCode WindowManagerLinux::CloseWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);
        SendClientMessage(window, m_atomNetCloseWindow, CurrentTime, 1);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::ForceCloseWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);
        XKillClient(m_display, window);
        XFlush(m_display);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::MinimizeWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);
        XIconifyWindow(m_display, window, DefaultScreen(m_display));
        XFlush(m_display);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::MaximizeWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);

        // First unminimize if minimized
        XMapWindow(m_display, window);

        // Then maximize
        SetWmState(window, true, m_atomNetWmStateMaximizedVert, m_atomNetWmStateMaximizedHorz);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::RestoreWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);

        // Unmap if minimized
        XMapWindow(m_display, window);

        // Remove maximized state
        SetWmState(window, false, m_atomNetWmStateMaximizedVert, m_atomNetWmStateMaximizedHorz);

        // Remove fullscreen state
        SetWmState(window, false, m_atomNetWmStateFullscreen, 0);

        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::ShowWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);
        XMapWindow(m_display, window);
        XFlush(m_display);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::HideWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);
        XUnmapWindow(m_display, window);
        XFlush(m_display);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::FocusWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);

        // Activate window using _NET_ACTIVE_WINDOW
        SendClientMessage(window, m_atomNetActiveWindow, 1, CurrentTime, 0);

        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::SetAlwaysOnTop(NativeHandle handle, bool topmost)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);
        SetWmState(window, topmost, m_atomNetWmStateAbove, 0);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::SetWindowRect(NativeHandle handle, const Rect &rect)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);
        XMoveResizeWindow(m_display, window, rect.x, rect.y, rect.width, rect.height);
        XFlush(m_display);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::MoveWindow(NativeHandle handle, int x, int y)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);
        XMoveWindow(m_display, window, x, y);
        XFlush(m_display);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::ResizeWindow(NativeHandle handle, int width, int height)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);
        XResizeWindow(m_display, window, width, height);
        XFlush(m_display);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::SetWindowTitle(NativeHandle handle, const std::string &title)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);

        // Set both WM_NAME and _NET_WM_NAME
        XStoreName(m_display, window, title.c_str());

        XChangeProperty(m_display, window, m_atomNetWmName, m_atomUtf8String,
                        8, PropModeReplace,
                        reinterpret_cast<const unsigned char *>(title.c_str()),
                        title.length());

        XFlush(m_display);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerLinux::SetWindowOpacity(NativeHandle handle, float opacity)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        if (!IsValidWindow(handle))
        {
            return ErrorCode::InvalidHandle;
        }

        Window window = static_cast<Window>(handle);

        // Clamp opacity to valid range
        opacity = std::max(0.0f, std::min(1.0f, opacity));

        // Convert to 32-bit value (0xFFFFFFFF = fully opaque)
        unsigned long opacityValue = static_cast<unsigned long>(opacity * 0xFFFFFFFF);

        XChangeProperty(m_display, window, m_atomNetWmWindowOpacity, XA_CARDINAL,
                        32, PropModeReplace,
                        reinterpret_cast<unsigned char *>(&opacityValue), 1);

        XFlush(m_display);
        return ErrorCode::Success;
    }

    std::string WindowManagerLinux::GetLastError() const
    {
        return m_lastError;
    }

    void WindowManagerLinux::SetLastError(const std::string &error)
    {
        m_lastError = error;
    }

} // namespace CrossWindow
