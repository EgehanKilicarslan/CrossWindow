# CrossWindow

A cross-platform C++ library for window management operations.

## Features

- **Window Enumeration**: Get list of all open windows
- **Window Information**: Get title, position, size, state, process info
- **Window Search**: Find windows by title or process name
- **Window Control**:
  - Close windows (gracefully or forcefully)
  - Minimize, maximize, restore windows
  - Show/hide windows
  - Focus windows
  - Set always-on-top
  - Move and resize windows
  - Set window title and opacity

## Supported Platforms

| Platform | Backend                 | Status                                          |
| -------- | ----------------------- | ----------------------------------------------- |
| Windows  | Win32 API               | ✅ Full support                                 |
| Linux    | X11                     | ✅ Full support                                 |
| macOS    | Cocoa/Accessibility API | ⚠️ Partial (requires accessibility permissions) |

## Building

### Requirements

- CMake 3.15 or later
- C++17 compatible compiler
- Platform-specific dependencies:
  - **Linux**: X11 development libraries (`libx11-dev` on Debian/Ubuntu)
  - **macOS**: Xcode command line tools
  - **Windows**: Windows SDK

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/EgehanKilicarslan/CrossWindow.git
cd CrossWindow

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
ctest
```

### CMake Options

| Option                       | Default | Description             |
| ---------------------------- | ------- | ----------------------- |
| `CROSSWINDOW_BUILD_SHARED`   | OFF     | Build as shared library |
| `CROSSWINDOW_BUILD_TESTS`    | ON      | Build test suite        |
| `CROSSWINDOW_BUILD_EXAMPLES` | ON      | Build example programs  |

## Usage

### Basic Example

```cpp
#include <CrossWindow.h>
#include <iostream>

int main()
{
    CrossWindow::WindowManager wm;

    if (!wm.Initialize())
    {
        std::cerr << "Failed to initialize\n";
        return 1;
    }

    // List all windows
    auto windows = wm.GetAllWindows();
    for (const auto& w : windows)
    {
        std::cout << w.title << " (" << w.processName << ")\n";
    }

    // Get focused window
    auto focused = wm.GetFocusedWindowInfo();
    if (focused.ok())
    {
        std::cout << "Focused: " << focused.value.title << "\n";
    }

    // Find windows by title
    auto matches = wm.FindWindowsByTitle("Firefox");
    for (const auto& w : matches)
    {
        // Minimize matching windows
        wm.MinimizeWindow(w.handle);
    }

    wm.Shutdown();
    return 0;
}
```

## Installation

### Option 1: System-wide Installation

```bash
# Build the library
mkdir build && cd build
cmake ..
cmake --build .

# Install (may require sudo on Linux/macOS)
sudo cmake --install .
```

This installs:

- Headers to `/usr/local/include/`
- Library to `/usr/local/lib/`
- CMake config to `/usr/local/lib/cmake/CrossWindow/`

### Option 2: Custom Installation Path

```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
cmake --build .
cmake --install .
```

### Option 3: Add as Subdirectory (Recommended for Projects)

Add CrossWindow to your project (e.g., as a git submodule):

```bash
git submodule add https://github.com/EgehanKilicarslan/CrossWindow.git external/CrossWindow
```

Then in your `CMakeLists.txt`:

```cmake
add_subdirectory(external/CrossWindow)
target_link_libraries(your_target PRIVATE CrossWindow)
```

### Option 4: FetchContent (CMake 3.14+)

In your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
    CrossWindow
    GIT_REPOSITORY https://github.com/EgehanKilicarslan/CrossWindow.git
    GIT_TAG master  # or a specific tag/commit
)

FetchContent_MakeAvailable(CrossWindow)

target_link_libraries(your_target PRIVATE CrossWindow)
```

## Using in Your Project

### Method 1: find_package (After Installation)

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyApp)

find_package(CrossWindow REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE CrossWindow::CrossWindow)
```

### Method 2: Subdirectory/FetchContent

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyApp)

# If using add_subdirectory or FetchContent (see above)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE CrossWindow)
```

### Method 3: pkg-config (Linux)

After installation, you can use pkg-config:

```bash
g++ main.cpp $(pkg-config --cflags --libs crosswindow) -o myapp
```

### Method 4: Manual Linking

```bash
# Linux
g++ main.cpp -I/usr/local/include -L/usr/local/lib -lCrossWindow -lX11 -o myapp

# macOS
clang++ main.cpp -I/usr/local/include -L/usr/local/lib -lCrossWindow -framework Cocoa -framework ApplicationServices -o myapp
```

## API Reference

### WindowManager Class

#### Initialization

- `bool Initialize()` - Initialize the window manager
- `bool IsInitialized() const` - Check if initialized
- `void Shutdown()` - Cleanup resources

#### Window Enumeration

- `std::vector<WindowInfo> GetAllWindows()` - Get all visible windows
- `void EnumerateWindows(callback)` - Enumerate windows with callback
- `std::vector<WindowInfo> FindWindowsByTitle(pattern, caseSensitive)` - Search by title
- `std::vector<WindowInfo> FindWindowsByProcess(processName)` - Search by process

#### Window Information

- `Result<WindowInfo> GetWindowInfo(handle)` - Get window details
- `Result<std::string> GetWindowTitle(handle)` - Get window title
- `Result<Rect> GetWindowRect(handle)` - Get position and size
- `Result<WindowState> GetWindowState(handle)` - Get window state
- `Result<uint32_t> GetWindowProcessId(handle)` - Get owning process ID
- `bool IsWindowVisible(handle)` - Check if visible
- `bool IsValidWindow(handle)` - Check if handle is valid

#### Active Window

- `NativeHandle GetFocusedWindow()` - Get focused window handle
- `Result<WindowInfo> GetFocusedWindowInfo()` - Get focused window info

#### Window Control

- `ErrorCode CloseWindow(handle)` - Close gracefully
- `ErrorCode ForceCloseWindow(handle)` - Force close (terminates process)
- `ErrorCode MinimizeWindow(handle)` - Minimize
- `ErrorCode MaximizeWindow(handle)` - Maximize
- `ErrorCode RestoreWindow(handle)` - Restore
- `ErrorCode ShowWindow(handle)` - Show hidden window
- `ErrorCode HideWindow(handle)` - Hide window
- `ErrorCode FocusWindow(handle)` - Bring to foreground
- `ErrorCode SetAlwaysOnTop(handle, topmost)` - Set always on top
- `ErrorCode SetWindowRect(handle, rect)` - Move and resize
- `ErrorCode MoveWindow(handle, x, y)` - Move
- `ErrorCode ResizeWindow(handle, width, height)` - Resize
- `ErrorCode SetWindowTitle(handle, title)` - Set title
- `ErrorCode SetWindowOpacity(handle, opacity)` - Set transparency

### Data Types

#### WindowInfo

```cpp
struct WindowInfo {
    NativeHandle handle;      // Platform window handle
    std::string title;        // Window title
    std::string className;    // Window class/app name
    Rect rect;                // Position and size
    WindowState state;        // Current state flags
    uint32_t processId;       // Owning process ID
    std::string processName;  // Process name
    bool isVisible;           // Visibility status
};
```

#### WindowState Flags

- `Normal` - Normal state
- `Minimized` - Window is minimized
- `Maximized` - Window is maximized
- `Fullscreen` - Window is fullscreen
- `Hidden` - Window is hidden
- `Focused` - Window has focus
- `AlwaysOnTop` - Window is always on top

#### ErrorCode

- `Success` - Operation succeeded
- `InvalidHandle` - Invalid window handle
- `AccessDenied` - Permission denied
- `WindowNotFound` - Window not found
- `OperationFailed` - Operation failed
- `NotSupported` - Not supported on this platform
- `NotInitialized` - WindowManager not initialized

## Platform Notes

### Linux

- Requires X11 display server (works with XWayland on Wayland sessions)
- Uses EWMH/NetWM hints for window management

### macOS

- Requires accessibility permissions for window control operations
- Grant access in System Preferences > Security & Privacy > Accessibility
- Some operations (SetWindowTitle, SetWindowOpacity, SetAlwaysOnTop) are not supported for external windows

### Windows

- Full functionality with Win32 API
- Some operations may require elevated privileges

## License

MIT License - see [LICENSE](LICENSE) for details.
