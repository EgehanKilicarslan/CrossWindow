/**
 * @file CrossWindow.h
 * @brief Cross-platform window management library
 *
 * A library for managing windows across Windows, Linux (X11/Wayland), and macOS.
 * Provides functionality to enumerate windows, get window titles, close windows,
 * and perform various window management operations.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Platform detection and export macros
#ifdef _WIN32
#define CROSSWINDOW_WINDOWS
#ifdef CROSSWINDOW_BUILD_SHARED
#define CROSSWINDOW_API __declspec(dllexport)
#elif defined(CROSSWINDOW_USE_SHARED)
#define CROSSWINDOW_API __declspec(dllimport)
#else
#define CROSSWINDOW_API
#endif
#elif defined(__linux__)
#define CROSSWINDOW_LINUX
#define CROSSWINDOW_API __attribute__((visibility("default")))
#elif defined(__APPLE__)
#define CROSSWINDOW_MACOS
#define CROSSWINDOW_API __attribute__((visibility("default")))
#else
#define CROSSWINDOW_STUB
#define CROSSWINDOW_API
#endif

namespace CrossWindow
{

    /**
     * @brief Platform-specific window handle type
     */
#ifdef CROSSWINDOW_WINDOWS
    using NativeHandle = void *; // HWND
#elif defined(CROSSWINDOW_LINUX)
    using NativeHandle = unsigned long; // X11 Window or Wayland surface id
#elif defined(CROSSWINDOW_MACOS)
    using NativeHandle = void *; // NSWindow* or CGWindowID
#else
    using NativeHandle = void *;
#endif

    /**
     * @brief Rectangle structure for window geometry
     */
    struct Rect
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    /**
     * @brief Window state flags
     */
    enum class WindowState : uint32_t
    {
        Normal = 0,
        Minimized = 1 << 0,
        Maximized = 1 << 1,
        Fullscreen = 1 << 2,
        Hidden = 1 << 3,
        Focused = 1 << 4,
        AlwaysOnTop = 1 << 5
    };

    // Enable bitwise operations on WindowState
    inline WindowState operator|(WindowState a, WindowState b)
    {
        return static_cast<WindowState>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline WindowState operator&(WindowState a, WindowState b)
    {
        return static_cast<WindowState>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline bool HasFlag(WindowState state, WindowState flag)
    {
        return (static_cast<uint32_t>(state) & static_cast<uint32_t>(flag)) != 0;
    }

    /**
     * @brief Information about a window
     */
    struct WindowInfo
    {
        NativeHandle handle{}; ///< Native window handle
        std::string title;     ///< Window title
        std::string className; ///< Window class name (Windows) or app name
        Rect rect;             ///< Window position and size
        WindowState state = WindowState::Normal;
        uint32_t processId = 0;  ///< Process ID that owns this window
        std::string processName; ///< Name of the process
        bool isVisible = false;  ///< Whether window is visible
    };

    /**
     * @brief Error codes for window operations
     */
    enum class ErrorCode
    {
        Success = 0,
        InvalidHandle,
        AccessDenied,
        WindowNotFound,
        OperationFailed,
        NotSupported,
        NotInitialized
    };

    /**
     * @brief Result type for operations that can fail
     */
    template <typename T>
    struct Result
    {
        T value;
        ErrorCode error = ErrorCode::Success;
        std::string errorMessage;

        bool ok() const { return error == ErrorCode::Success; }
        operator bool() const { return ok(); }
    };

    /**
     * @brief Callback type for window enumeration
     * @return true to continue enumeration, false to stop
     */
    using EnumWindowsCallback = std::function<bool(const WindowInfo &)>;

    /**
     * @brief Window manager class - main interface for window operations
     */
    class CROSSWINDOW_API WindowManager
    {
    public:
        WindowManager();
        ~WindowManager();

        // Non-copyable, movable
        WindowManager(const WindowManager &) = delete;
        WindowManager &operator=(const WindowManager &) = delete;
        WindowManager(WindowManager &&) noexcept;
        WindowManager &operator=(WindowManager &&) noexcept;

        /**
         * @brief Initialize the window manager
         * @return true if initialization succeeded
         */
        bool Initialize();

        /**
         * @brief Check if the window manager is initialized
         */
        bool IsInitialized() const;

        /**
         * @brief Shutdown and cleanup resources
         */
        void Shutdown();

        // ============== Window Enumeration ==============

        /**
         * @brief Get all visible windows
         * @return Vector of WindowInfo for all visible windows
         */
        std::vector<WindowInfo> GetAllWindows();

        /**
         * @brief Enumerate all windows with a callback
         * @param callback Function called for each window
         */
        void EnumerateWindows(const EnumWindowsCallback &callback);

        /**
         * @brief Find windows by title (partial match)
         * @param titlePattern Substring to search for in window titles
         * @param caseSensitive Whether the search is case-sensitive
         * @return Vector of matching windows
         */
        std::vector<WindowInfo> FindWindowsByTitle(const std::string &titlePattern,
                                                   bool caseSensitive = false);

        /**
         * @brief Find windows by process name
         * @param processName Name of the process
         * @return Vector of windows owned by the process
         */
        std::vector<WindowInfo> FindWindowsByProcess(const std::string &processName);

        // ============== Window Information ==============

        /**
         * @brief Get information about a specific window
         * @param handle Native window handle
         * @return WindowInfo or error
         */
        Result<WindowInfo> GetWindowInfo(NativeHandle handle);

        /**
         * @brief Get the title of a window
         * @param handle Native window handle
         * @return Window title or error
         */
        Result<std::string> GetWindowTitle(NativeHandle handle);

        /**
         * @brief Get the window rectangle (position and size)
         * @param handle Native window handle
         * @return Window rect or error
         */
        Result<Rect> GetWindowRect(NativeHandle handle);

        /**
         * @brief Get the current state of a window
         * @param handle Native window handle
         * @return Window state or error
         */
        Result<WindowState> GetWindowState(NativeHandle handle);

        /**
         * @brief Get the process ID of a window
         * @param handle Native window handle
         * @return Process ID or error
         */
        Result<uint32_t> GetWindowProcessId(NativeHandle handle);

        /**
         * @brief Check if a window is visible
         * @param handle Native window handle
         * @return true if visible
         */
        bool IsWindowVisible(NativeHandle handle);

        /**
         * @brief Check if a window handle is valid
         * @param handle Native window handle
         * @return true if the handle refers to a valid window
         */
        bool IsValidWindow(NativeHandle handle);

        // ============== Active/Focused Window ==============

        /**
         * @brief Get the currently focused/foreground window
         * @return Handle to the focused window or null
         */
        NativeHandle GetFocusedWindow();

        /**
         * @brief Get information about the currently focused window
         * @return WindowInfo for the focused window
         */
        Result<WindowInfo> GetFocusedWindowInfo();

        // ============== Window Manipulation ==============

        /**
         * @brief Close a window gracefully
         * @param handle Native window handle
         * @return Error code
         */
        ErrorCode CloseWindow(NativeHandle handle);

        /**
         * @brief Force close a window (may cause data loss)
         * @param handle Native window handle
         * @return Error code
         */
        ErrorCode ForceCloseWindow(NativeHandle handle);

        /**
         * @brief Minimize a window
         * @param handle Native window handle
         * @return Error code
         */
        ErrorCode MinimizeWindow(NativeHandle handle);

        /**
         * @brief Maximize a window
         * @param handle Native window handle
         * @return Error code
         */
        ErrorCode MaximizeWindow(NativeHandle handle);

        /**
         * @brief Restore a window from minimized/maximized state
         * @param handle Native window handle
         * @return Error code
         */
        ErrorCode RestoreWindow(NativeHandle handle);

        /**
         * @brief Show a hidden window
         * @param handle Native window handle
         * @return Error code
         */
        ErrorCode ShowWindow(NativeHandle handle);

        /**
         * @brief Hide a window
         * @param handle Native window handle
         * @return Error code
         */
        ErrorCode HideWindow(NativeHandle handle);

        /**
         * @brief Bring window to foreground and give it focus
         * @param handle Native window handle
         * @return Error code
         */
        ErrorCode FocusWindow(NativeHandle handle);

        /**
         * @brief Set window always on top
         * @param handle Native window handle
         * @param topmost Whether to set or unset topmost
         * @return Error code
         */
        ErrorCode SetAlwaysOnTop(NativeHandle handle, bool topmost);

        /**
         * @brief Move and resize a window
         * @param handle Native window handle
         * @param rect New position and size
         * @return Error code
         */
        ErrorCode SetWindowRect(NativeHandle handle, const Rect &rect);

        /**
         * @brief Move a window
         * @param handle Native window handle
         * @param x New X position
         * @param y New Y position
         * @return Error code
         */
        ErrorCode MoveWindow(NativeHandle handle, int x, int y);

        /**
         * @brief Resize a window
         * @param handle Native window handle
         * @param width New width
         * @param height New height
         * @return Error code
         */
        ErrorCode ResizeWindow(NativeHandle handle, int width, int height);

        /**
         * @brief Set window title
         * @param handle Native window handle
         * @param title New title
         * @return Error code
         */
        ErrorCode SetWindowTitle(NativeHandle handle, const std::string &title);

        /**
         * @brief Set window opacity/transparency
         * @param handle Native window handle
         * @param opacity Opacity value (0.0 = transparent, 1.0 = opaque)
         * @return Error code
         */
        ErrorCode SetWindowOpacity(NativeHandle handle, float opacity);

        // ============== Utility ==============

        /**
         * @brief Get the last error message
         * @return Error message string
         */
        std::string GetLastError() const;

        /**
         * @brief Get platform name
         * @return "Windows", "Linux", "macOS", or "Stub"
         */
        static const char *GetPlatformName();

    private:
        class Impl;
        friend class Impl;
        std::unique_ptr<Impl> m_impl;
    };

    // Forward declare the Impl for platform implementations
    class WindowManagerImpl;

} // namespace CrossWindow
