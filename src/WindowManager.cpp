/**
 * @file WindowManager.cpp
 * @brief Main WindowManager implementation - delegates to platform-specific impl
 */

#include "CrossWindow.h"
#include "WindowManagerImpl.h"

// Include platform-specific implementations
#ifdef CROSSWINDOW_WINDOWS
#include "platform/windows/WindowManagerWindows.h"
#elif defined(CROSSWINDOW_LINUX)
#include "platform/linux/WindowManagerLinux.h"
#elif defined(CROSSWINDOW_MACOS)
#include "platform/macos/WindowManagerMacOS.h"
#else
#include "platform/stub/WindowManagerStub.h"
#endif

namespace CrossWindow
{

    // Define the private Impl class as a wrapper around WindowManagerImplBase
    class WindowManager::Impl
    {
    public:
        std::unique_ptr<WindowManagerImplBase> impl;

        explicit Impl(std::unique_ptr<WindowManagerImplBase> p) : impl(std::move(p)) {}
    };

    WindowManager::WindowManager()
    {
#ifdef CROSSWINDOW_WINDOWS
        m_impl = std::make_unique<Impl>(std::make_unique<WindowManagerWindows>());
#elif defined(CROSSWINDOW_LINUX)
        m_impl = std::make_unique<Impl>(std::make_unique<WindowManagerLinux>());
#elif defined(CROSSWINDOW_MACOS)
        m_impl = std::make_unique<Impl>(std::make_unique<WindowManagerMacOS>());
#else
        m_impl = std::make_unique<Impl>(std::make_unique<WindowManagerStub>());
#endif
    }

    WindowManager::~WindowManager()
    {
        if (m_impl && m_impl->impl && m_impl->impl->IsInitialized())
        {
            m_impl->impl->Shutdown();
        }
    }

    WindowManager::WindowManager(WindowManager &&) noexcept = default;
    WindowManager &WindowManager::operator=(WindowManager &&) noexcept = default;

    bool WindowManager::Initialize()
    {
        return m_impl->impl->Initialize();
    }

    bool WindowManager::IsInitialized() const
    {
        return m_impl->impl->IsInitialized();
    }

    void WindowManager::Shutdown()
    {
        m_impl->impl->Shutdown();
    }

    std::vector<WindowInfo> WindowManager::GetAllWindows()
    {
        return m_impl->impl->GetAllWindows();
    }

    void WindowManager::EnumerateWindows(const EnumWindowsCallback &callback)
    {
        m_impl->impl->EnumerateWindows(callback);
    }

    std::vector<WindowInfo> WindowManager::FindWindowsByTitle(const std::string &titlePattern,
                                                              bool caseSensitive)
    {
        return m_impl->impl->FindWindowsByTitle(titlePattern, caseSensitive);
    }

    std::vector<WindowInfo> WindowManager::FindWindowsByProcess(const std::string &processName)
    {
        return m_impl->impl->FindWindowsByProcess(processName);
    }

    Result<WindowInfo> WindowManager::GetWindowInfo(NativeHandle handle)
    {
        return m_impl->impl->GetWindowInfo(handle);
    }

    Result<std::string> WindowManager::GetWindowTitle(NativeHandle handle)
    {
        return m_impl->impl->GetWindowTitle(handle);
    }

    Result<Rect> WindowManager::GetWindowRect(NativeHandle handle)
    {
        return m_impl->impl->GetWindowRect(handle);
    }

    Result<WindowState> WindowManager::GetWindowState(NativeHandle handle)
    {
        return m_impl->impl->GetWindowState(handle);
    }

    Result<uint32_t> WindowManager::GetWindowProcessId(NativeHandle handle)
    {
        return m_impl->impl->GetWindowProcessId(handle);
    }

    bool WindowManager::IsWindowVisible(NativeHandle handle)
    {
        return m_impl->impl->IsWindowVisible(handle);
    }

    bool WindowManager::IsValidWindow(NativeHandle handle)
    {
        return m_impl->impl->IsValidWindow(handle);
    }

    NativeHandle WindowManager::GetFocusedWindow()
    {
        return m_impl->impl->GetFocusedWindow();
    }

    Result<WindowInfo> WindowManager::GetFocusedWindowInfo()
    {
        return m_impl->impl->GetFocusedWindowInfo();
    }

    ErrorCode WindowManager::CloseWindow(NativeHandle handle)
    {
        return m_impl->impl->CloseWindow(handle);
    }

    ErrorCode WindowManager::ForceCloseWindow(NativeHandle handle)
    {
        return m_impl->impl->ForceCloseWindow(handle);
    }

    ErrorCode WindowManager::MinimizeWindow(NativeHandle handle)
    {
        return m_impl->impl->MinimizeWindow(handle);
    }

    ErrorCode WindowManager::MaximizeWindow(NativeHandle handle)
    {
        return m_impl->impl->MaximizeWindow(handle);
    }

    ErrorCode WindowManager::RestoreWindow(NativeHandle handle)
    {
        return m_impl->impl->RestoreWindow(handle);
    }

    ErrorCode WindowManager::ShowWindow(NativeHandle handle)
    {
        return m_impl->impl->ShowWindow(handle);
    }

    ErrorCode WindowManager::HideWindow(NativeHandle handle)
    {
        return m_impl->impl->HideWindow(handle);
    }

    ErrorCode WindowManager::FocusWindow(NativeHandle handle)
    {
        return m_impl->impl->FocusWindow(handle);
    }

    ErrorCode WindowManager::SetAlwaysOnTop(NativeHandle handle, bool topmost)
    {
        return m_impl->impl->SetAlwaysOnTop(handle, topmost);
    }

    ErrorCode WindowManager::SetWindowRect(NativeHandle handle, const Rect &rect)
    {
        return m_impl->impl->SetWindowRect(handle, rect);
    }

    ErrorCode WindowManager::MoveWindow(NativeHandle handle, int x, int y)
    {
        return m_impl->impl->MoveWindow(handle, x, y);
    }

    ErrorCode WindowManager::ResizeWindow(NativeHandle handle, int width, int height)
    {
        return m_impl->impl->ResizeWindow(handle, width, height);
    }

    ErrorCode WindowManager::SetWindowTitle(NativeHandle handle, const std::string &title)
    {
        return m_impl->impl->SetWindowTitle(handle, title);
    }

    ErrorCode WindowManager::SetWindowOpacity(NativeHandle handle, float opacity)
    {
        return m_impl->impl->SetWindowOpacity(handle, opacity);
    }

    std::string WindowManager::GetLastError() const
    {
        return m_impl->impl->GetLastError();
    }

    const char *WindowManager::GetPlatformName()
    {
#ifdef CROSSWINDOW_WINDOWS
        return "Windows";
#elif defined(CROSSWINDOW_LINUX)
        return "Linux";
#elif defined(CROSSWINDOW_MACOS)
        return "macOS";
#else
        return "Stub";
#endif
    }

} // namespace CrossWindow
