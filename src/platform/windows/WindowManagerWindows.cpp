/**
 * @file WindowManagerWindows.cpp
 * @brief Windows implementation of WindowManager
 */

#include "WindowManagerWindows.h"
#include <algorithm>
#include <cctype>

#pragma comment(lib, "dwmapi.lib")

namespace CrossWindow
{

    // Helper structure for enum callback
    struct EnumWindowsData
    {
        std::vector<WindowInfo> *windows;
        WindowManagerWindows *manager;
        const EnumWindowsCallback *callback;
        bool continueEnum;
    };

    WindowManagerWindows::WindowManagerWindows() = default;

    WindowManagerWindows::~WindowManagerWindows()
    {
        Shutdown();
    }

    bool WindowManagerWindows::Initialize()
    {
        m_initialized = true;
        return true;
    }

    bool WindowManagerWindows::IsInitialized() const
    {
        return m_initialized;
    }

    void WindowManagerWindows::Shutdown()
    {
        m_initialized = false;
    }

    std::string WindowManagerWindows::GetWindowTitleInternal(HWND hwnd)
    {
        int length = GetWindowTextLengthW(hwnd);
        if (length == 0)
        {
            return "";
        }

        std::wstring wideTitle(length + 1, L'\0');
        GetWindowTextW(hwnd, &wideTitle[0], length + 1);
        wideTitle.resize(length);

        // Convert wide string to UTF-8
        int size = WideCharToMultiByte(CP_UTF8, 0, wideTitle.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0)
        {
            return "";
        }

        std::string title(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wideTitle.c_str(), -1, &title[0], size, nullptr, nullptr);
        return title;
    }

    std::string WindowManagerWindows::GetWindowClassInternal(HWND hwnd)
    {
        wchar_t className[256];
        if (GetClassNameW(hwnd, className, 256) == 0)
        {
            return "";
        }

        int size = WideCharToMultiByte(CP_UTF8, 0, className, -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0)
        {
            return "";
        }

        std::string result(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, className, -1, &result[0], size, nullptr, nullptr);
        return result;
    }

    std::string WindowManagerWindows::GetProcessNameFromPid(DWORD pid)
    {
        std::string name;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcess)
        {
            wchar_t processName[MAX_PATH];
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, processName, &size))
            {
                // Extract just the filename
                wchar_t *lastSlash = wcsrchr(processName, L'\\');
                wchar_t *fileName = lastSlash ? lastSlash + 1 : processName;

                int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, fileName, -1, nullptr, 0, nullptr, nullptr);
                if (sizeNeeded > 0)
                {
                    name.resize(sizeNeeded - 1);
                    WideCharToMultiByte(CP_UTF8, 0, fileName, -1, &name[0], sizeNeeded, nullptr, nullptr);
                }
            }
            CloseHandle(hProcess);
        }
        return name;
    }

    bool WindowManagerWindows::ToLowerCompare(const std::string &str, const std::string &pattern)
    {
        std::string lowerStr = str;
        std::string lowerPattern = pattern;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
        std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
        return lowerStr.find(lowerPattern) != std::string::npos;
    }

    BOOL CALLBACK WindowManagerWindows::EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        EnumWindowsData *data = reinterpret_cast<EnumWindowsData *>(lParam);

        // Skip invisible windows and child windows
        if (!IsWindowVisible(hwnd))
        {
            return TRUE;
        }

        HWND owner = GetWindow(hwnd, GW_OWNER);
        if (owner != nullptr)
        {
            return TRUE; // Skip owned windows
        }

        // Skip windows with no title
        int length = GetWindowTextLengthW(hwnd);
        if (length == 0)
        {
            return TRUE;
        }

        auto infoResult = data->manager->GetWindowInfo(hwnd);
        if (infoResult.ok())
        {
            if (data->windows)
            {
                data->windows->push_back(infoResult.value);
            }
            if (data->callback)
            {
                if (!(*data->callback)(infoResult.value))
                {
                    data->continueEnum = false;
                    return FALSE;
                }
            }
        }

        return TRUE;
    }

    std::vector<WindowInfo> WindowManagerWindows::GetAllWindows()
    {
        std::vector<WindowInfo> result;

        if (!m_initialized)
        {
            SetLastError("WindowManager not initialized");
            return result;
        }

        EnumWindowsData data;
        data.windows = &result;
        data.manager = this;
        data.callback = nullptr;
        data.continueEnum = true;

        ::EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));

        return result;
    }

    void WindowManagerWindows::EnumerateWindows(const EnumWindowsCallback &callback)
    {
        if (!m_initialized)
        {
            return;
        }

        EnumWindowsData data;
        data.windows = nullptr;
        data.manager = this;
        data.callback = &callback;
        data.continueEnum = true;

        ::EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    }

    std::vector<WindowInfo> WindowManagerWindows::FindWindowsByTitle(const std::string &titlePattern,
                                                                     bool caseSensitive)
    {
        std::vector<WindowInfo> result;

        if (!m_initialized)
        {
            return result;
        }

        auto allWindows = GetAllWindows();
        for (const auto &info : allWindows)
        {
            bool matches = false;
            if (caseSensitive)
            {
                matches = info.title.find(titlePattern) != std::string::npos;
            }
            else
            {
                matches = ToLowerCompare(info.title, titlePattern);
            }

            if (matches)
            {
                result.push_back(info);
            }
        }

        return result;
    }

    std::vector<WindowInfo> WindowManagerWindows::FindWindowsByProcess(const std::string &processName)
    {
        std::vector<WindowInfo> result;

        if (!m_initialized)
        {
            return result;
        }

        auto allWindows = GetAllWindows();
        for (const auto &info : allWindows)
        {
            if (ToLowerCompare(info.processName, processName))
            {
                result.push_back(info);
            }
        }

        return result;
    }

    Result<WindowInfo> WindowManagerWindows::GetWindowInfo(NativeHandle handle)
    {
        Result<WindowInfo> result;
        HWND hwnd = static_cast<HWND>(handle);

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsWindow(hwnd))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        result.value.handle = handle;
        result.value.title = GetWindowTitleInternal(hwnd);
        result.value.className = GetWindowClassInternal(hwnd);

        // Get process ID
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        result.value.processId = pid;
        result.value.processName = GetProcessNameFromPid(pid);

        // Get rect
        RECT rect;
        if (GetWindowRect(hwnd, &rect))
        {
            result.value.rect.x = rect.left;
            result.value.rect.y = rect.top;
            result.value.rect.width = rect.right - rect.left;
            result.value.rect.height = rect.bottom - rect.top;
        }

        // Get state
        result.value.state = WindowState::Normal;
        result.value.isVisible = ::IsWindowVisible(hwnd) != 0;

        if (IsIconic(hwnd))
        {
            result.value.state = result.value.state | WindowState::Minimized;
        }
        if (IsZoomed(hwnd))
        {
            result.value.state = result.value.state | WindowState::Maximized;
        }
        if (!result.value.isVisible)
        {
            result.value.state = result.value.state | WindowState::Hidden;
        }

        // Check always on top
        LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
        if (exStyle & WS_EX_TOPMOST)
        {
            result.value.state = result.value.state | WindowState::AlwaysOnTop;
        }

        // Check focused
        if (GetForegroundWindow() == hwnd)
        {
            result.value.state = result.value.state | WindowState::Focused;
        }

        result.error = ErrorCode::Success;
        return result;
    }

    Result<std::string> WindowManagerWindows::GetWindowTitle(NativeHandle handle)
    {
        Result<std::string> result;
        HWND hwnd = static_cast<HWND>(handle);

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsWindow(hwnd))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        result.value = GetWindowTitleInternal(hwnd);
        result.error = ErrorCode::Success;
        return result;
    }

    Result<Rect> WindowManagerWindows::GetWindowRect(NativeHandle handle)
    {
        Result<Rect> result;
        HWND hwnd = static_cast<HWND>(handle);

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsWindow(hwnd))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        RECT rect;
        if (::GetWindowRect(hwnd, &rect))
        {
            result.value.x = rect.left;
            result.value.y = rect.top;
            result.value.width = rect.right - rect.left;
            result.value.height = rect.bottom - rect.top;
            result.error = ErrorCode::Success;
        }
        else
        {
            result.error = ErrorCode::OperationFailed;
            result.errorMessage = "Failed to get window rect";
        }

        return result;
    }

    Result<WindowState> WindowManagerWindows::GetWindowState(NativeHandle handle)
    {
        Result<WindowState> result;
        HWND hwnd = static_cast<HWND>(handle);

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsWindow(hwnd))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        result.value = WindowState::Normal;

        if (IsIconic(hwnd))
        {
            result.value = result.value | WindowState::Minimized;
        }
        if (IsZoomed(hwnd))
        {
            result.value = result.value | WindowState::Maximized;
        }
        if (!::IsWindowVisible(hwnd))
        {
            result.value = result.value | WindowState::Hidden;
        }

        LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
        if (exStyle & WS_EX_TOPMOST)
        {
            result.value = result.value | WindowState::AlwaysOnTop;
        }

        if (GetForegroundWindow() == hwnd)
        {
            result.value = result.value | WindowState::Focused;
        }

        result.error = ErrorCode::Success;
        return result;
    }

    Result<uint32_t> WindowManagerWindows::GetWindowProcessId(NativeHandle handle)
    {
        Result<uint32_t> result;
        HWND hwnd = static_cast<HWND>(handle);

        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        if (!IsWindow(hwnd))
        {
            result.error = ErrorCode::InvalidHandle;
            result.errorMessage = "Invalid window handle";
            return result;
        }

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        result.value = pid;
        result.error = ErrorCode::Success;
        return result;
    }

    bool WindowManagerWindows::IsWindowVisible(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return false;
        }
        return ::IsWindowVisible(static_cast<HWND>(handle)) != 0;
    }

    bool WindowManagerWindows::IsValidWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return false;
        }
        return IsWindow(static_cast<HWND>(handle)) != 0;
    }

    NativeHandle WindowManagerWindows::GetFocusedWindow()
    {
        if (!m_initialized)
        {
            return nullptr;
        }
        return GetForegroundWindow();
    }

    Result<WindowInfo> WindowManagerWindows::GetFocusedWindowInfo()
    {
        NativeHandle focused = GetFocusedWindow();
        if (focused == nullptr)
        {
            Result<WindowInfo> result;
            result.error = ErrorCode::WindowNotFound;
            result.errorMessage = "No focused window found";
            return result;
        }
        return GetWindowInfo(focused);
    }

    ErrorCode WindowManagerWindows::CloseWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        // Send WM_CLOSE for graceful close
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::ForceCloseWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);

        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess)
        {
            TerminateProcess(hProcess, 1);
            CloseHandle(hProcess);
            return ErrorCode::Success;
        }

        return ErrorCode::AccessDenied;
    }

    ErrorCode WindowManagerWindows::MinimizeWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        ::ShowWindow(hwnd, SW_MINIMIZE);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::MaximizeWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        ::ShowWindow(hwnd, SW_MAXIMIZE);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::RestoreWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        ::ShowWindow(hwnd, SW_RESTORE);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::ShowWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        ::ShowWindow(hwnd, SW_SHOW);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::HideWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        ::ShowWindow(hwnd, SW_HIDE);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::FocusWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        // Restore if minimized
        if (IsIconic(hwnd))
        {
            ::ShowWindow(hwnd, SW_RESTORE);
        }

        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::SetAlwaysOnTop(NativeHandle handle, bool topmost)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        HWND insertAfter = topmost ? HWND_TOPMOST : HWND_NOTOPMOST;
        SetWindowPos(hwnd, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::SetWindowRect(NativeHandle handle, const Rect &rect)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        ::MoveWindow(hwnd, rect.x, rect.y, rect.width, rect.height, TRUE);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::MoveWindow(NativeHandle handle, int x, int y)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::ResizeWindow(NativeHandle handle, int width, int height)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::SetWindowTitle(NativeHandle handle, const std::string &title)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        // Convert UTF-8 to wide string
        int wideSize = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
        if (wideSize <= 0)
        {
            return ErrorCode::OperationFailed;
        }

        std::wstring wideTitle(wideSize, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wideTitle[0], wideSize);

        SetWindowTextW(hwnd, wideTitle.c_str());
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerWindows::SetWindowOpacity(NativeHandle handle, float opacity)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        HWND hwnd = static_cast<HWND>(handle);
        if (!IsWindow(hwnd))
        {
            return ErrorCode::InvalidHandle;
        }

        // Clamp opacity
        opacity = (std::max)(0.0f, (std::min)(1.0f, opacity));

        // Need to set WS_EX_LAYERED style first
        LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
        SetWindowLongW(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);

        BYTE alpha = static_cast<BYTE>(opacity * 255);
        SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);

        return ErrorCode::Success;
    }

    std::string WindowManagerWindows::GetLastError() const
    {
        return m_lastError;
    }

    void WindowManagerWindows::SetLastError(const std::string &error)
    {
        m_lastError = error;
    }

} // namespace CrossWindow
