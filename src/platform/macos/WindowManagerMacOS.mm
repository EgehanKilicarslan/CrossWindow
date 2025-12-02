/**
 * @file WindowManagerMacOS.mm
 * @brief macOS implementation of WindowManager using Cocoa/Accessibility APIs
 */

#import "WindowManagerMacOS.h"
#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#include <algorithm>
#include <cctype>

namespace CrossWindow
{

    WindowManagerMacOS::WindowManagerMacOS() = default;

    WindowManagerMacOS::~WindowManagerMacOS()
    {
        Shutdown();
    }

    bool WindowManagerMacOS::Initialize()
    {
        // Check if we have accessibility permissions
        NSDictionary *options = @{(id)kAXTrustedCheckOptionPrompt: @YES};
        bool trusted = AXIsProcessTrustedWithOptions((CFDictionaryRef)options);
        
        if (!trusted)
        {
            SetLastError("Accessibility permissions required. Please grant access in System Preferences.");
            // Still allow initialization, but some features may not work
        }
        
        m_initialized = true;
        return true;
    }

    bool WindowManagerMacOS::IsInitialized() const
    {
        return m_initialized;
    }

    void WindowManagerMacOS::Shutdown()
    {
        m_initialized = false;
    }

    bool WindowManagerMacOS::ToLowerCompare(const std::string &str, const std::string &pattern)
    {
        std::string lowerStr = str;
        std::string lowerPattern = pattern;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
        std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
        return lowerStr.find(lowerPattern) != std::string::npos;
    }

    std::vector<WindowInfo> WindowManagerMacOS::GetAllWindows()
    {
        std::vector<WindowInfo> result;
        
        if (!m_initialized)
        {
            SetLastError("WindowManager not initialized");
            return result;
        }

        @autoreleasepool {
            // Get list of all windows
            CFArrayRef windowList = CGWindowListCopyWindowInfo(
                kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
                kCGNullWindowID);
            
            if (!windowList)
            {
                return result;
            }
            
            NSArray *windows = (__bridge_transfer NSArray *)windowList;
            
            for (NSDictionary *window in windows)
            {
                WindowInfo info;
                
                // Get window ID (our handle)
                NSNumber *windowID = window[(id)kCGWindowNumber];
                info.handle = (void *)(uintptr_t)[windowID unsignedIntValue];
                
                // Get window title
                NSString *title = window[(id)kCGWindowName];
                if (title)
                {
                    info.title = [title UTF8String];
                }
                
                // Get owner name (process name)
                NSString *ownerName = window[(id)kCGWindowOwnerName];
                if (ownerName)
                {
                    info.processName = [ownerName UTF8String];
                    info.className = info.processName;
                }
                
                // Get process ID
                NSNumber *pid = window[(id)kCGWindowOwnerPID];
                info.processId = [pid unsignedIntValue];
                
                // Get bounds
                NSDictionary *bounds = window[(id)kCGWindowBounds];
                if (bounds)
                {
                    CGRect rect;
                    CGRectMakeWithDictionaryRepresentation((CFDictionaryRef)bounds, &rect);
                    info.rect.x = (int)rect.origin.x;
                    info.rect.y = (int)rect.origin.y;
                    info.rect.width = (int)rect.size.width;
                    info.rect.height = (int)rect.size.height;
                }
                
                // Check visibility
                NSNumber *onScreen = window[(id)kCGWindowIsOnscreen];
                info.isVisible = [onScreen boolValue];
                
                // Skip windows without titles (usually not user-visible windows)
                if (!info.title.empty())
                {
                    result.push_back(info);
                }
            }
        }
        
        return result;
    }

    void WindowManagerMacOS::EnumerateWindows(const EnumWindowsCallback &callback)
    {
        if (!m_initialized)
        {
            return;
        }

        auto windows = GetAllWindows();
        for (const auto &info : windows)
        {
            if (!callback(info))
            {
                break;
            }
        }
    }

    std::vector<WindowInfo> WindowManagerMacOS::FindWindowsByTitle(const std::string &titlePattern,
                                                                    bool caseSensitive)
    {
        std::vector<WindowInfo> result;
        
        if (!m_initialized)
        {
            return result;
        }

        auto windows = GetAllWindows();
        for (const auto &info : windows)
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

    std::vector<WindowInfo> WindowManagerMacOS::FindWindowsByProcess(const std::string &processName)
    {
        std::vector<WindowInfo> result;
        
        if (!m_initialized)
        {
            return result;
        }

        auto windows = GetAllWindows();
        for (const auto &info : windows)
        {
            if (ToLowerCompare(info.processName, processName))
            {
                result.push_back(info);
            }
        }
        
        return result;
    }

    Result<WindowInfo> WindowManagerMacOS::GetWindowInfo(NativeHandle handle)
    {
        Result<WindowInfo> result;
        
        if (!m_initialized)
        {
            result.error = ErrorCode::NotInitialized;
            result.errorMessage = "WindowManager not initialized";
            return result;
        }

        CGWindowID windowID = (CGWindowID)(uintptr_t)handle;
        
        @autoreleasepool {
            CFArrayRef windowList = CGWindowListCopyWindowInfo(
                kCGWindowListOptionIncludingWindow,
                windowID);
            
            if (!windowList || CFArrayGetCount(windowList) == 0)
            {
                if (windowList) CFRelease(windowList);
                result.error = ErrorCode::InvalidHandle;
                result.errorMessage = "Invalid window handle";
                return result;
            }
            
            NSArray *windows = (__bridge_transfer NSArray *)windowList;
            NSDictionary *window = windows[0];
            
            result.value.handle = handle;
            
            NSString *title = window[(id)kCGWindowName];
            if (title)
            {
                result.value.title = [title UTF8String];
            }
            
            NSString *ownerName = window[(id)kCGWindowOwnerName];
            if (ownerName)
            {
                result.value.processName = [ownerName UTF8String];
                result.value.className = result.value.processName;
            }
            
            NSNumber *pid = window[(id)kCGWindowOwnerPID];
            result.value.processId = [pid unsignedIntValue];
            
            NSDictionary *bounds = window[(id)kCGWindowBounds];
            if (bounds)
            {
                CGRect rect;
                CGRectMakeWithDictionaryRepresentation((CFDictionaryRef)bounds, &rect);
                result.value.rect.x = (int)rect.origin.x;
                result.value.rect.y = (int)rect.origin.y;
                result.value.rect.width = (int)rect.size.width;
                result.value.rect.height = (int)rect.size.height;
            }
            
            NSNumber *onScreen = window[(id)kCGWindowIsOnscreen];
            result.value.isVisible = [onScreen boolValue];
            
            result.value.state = WindowState::Normal;
            if (!result.value.isVisible)
            {
                result.value.state = result.value.state | WindowState::Hidden;
            }
        }
        
        result.error = ErrorCode::Success;
        return result;
    }

    Result<std::string> WindowManagerMacOS::GetWindowTitle(NativeHandle handle)
    {
        Result<std::string> result;
        
        auto info = GetWindowInfo(handle);
        if (!info.ok())
        {
            result.error = info.error;
            result.errorMessage = info.errorMessage;
            return result;
        }
        
        result.value = info.value.title;
        result.error = ErrorCode::Success;
        return result;
    }

    Result<Rect> WindowManagerMacOS::GetWindowRect(NativeHandle handle)
    {
        Result<Rect> result;
        
        auto info = GetWindowInfo(handle);
        if (!info.ok())
        {
            result.error = info.error;
            result.errorMessage = info.errorMessage;
            return result;
        }
        
        result.value = info.value.rect;
        result.error = ErrorCode::Success;
        return result;
    }

    Result<WindowState> WindowManagerMacOS::GetWindowState(NativeHandle handle)
    {
        Result<WindowState> result;
        
        auto info = GetWindowInfo(handle);
        if (!info.ok())
        {
            result.error = info.error;
            result.errorMessage = info.errorMessage;
            return result;
        }
        
        result.value = info.value.state;
        result.error = ErrorCode::Success;
        return result;
    }

    Result<uint32_t> WindowManagerMacOS::GetWindowProcessId(NativeHandle handle)
    {
        Result<uint32_t> result;
        
        auto info = GetWindowInfo(handle);
        if (!info.ok())
        {
            result.error = info.error;
            result.errorMessage = info.errorMessage;
            return result;
        }
        
        result.value = info.value.processId;
        result.error = ErrorCode::Success;
        return result;
    }

    bool WindowManagerMacOS::IsWindowVisible(NativeHandle handle)
    {
        auto info = GetWindowInfo(handle);
        return info.ok() && info.value.isVisible;
    }

    bool WindowManagerMacOS::IsValidWindow(NativeHandle handle)
    {
        auto info = GetWindowInfo(handle);
        return info.ok();
    }

    NativeHandle WindowManagerMacOS::GetFocusedWindow()
    {
        if (!m_initialized)
        {
            return nullptr;
        }

        @autoreleasepool {
            // Get the frontmost application
            NSRunningApplication *frontApp = [[NSWorkspace sharedWorkspace] frontmostApplication];
            if (!frontApp)
            {
                return nullptr;
            }
            
            pid_t pid = [frontApp processIdentifier];
            
            // Find windows belonging to this application
            CFArrayRef windowList = CGWindowListCopyWindowInfo(
                kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
                kCGNullWindowID);
            
            if (!windowList)
            {
                return nullptr;
            }
            
            NSArray *windows = (__bridge_transfer NSArray *)windowList;
            
            for (NSDictionary *window in windows)
            {
                NSNumber *windowPid = window[(id)kCGWindowOwnerPID];
                if ([windowPid intValue] == pid)
                {
                    NSNumber *windowID = window[(id)kCGWindowNumber];
                    NSString *title = window[(id)kCGWindowName];
                    
                    // Skip windows without titles
                    if (title && [title length] > 0)
                    {
                        return (void *)(uintptr_t)[windowID unsignedIntValue];
                    }
                }
            }
        }
        
        return nullptr;
    }

    Result<WindowInfo> WindowManagerMacOS::GetFocusedWindowInfo()
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

    // Helper function to get AXUIElement for a window
    static AXUIElementRef GetAXWindowForPid(pid_t pid, CGWindowID targetWindowID)
    {
        AXUIElementRef appElement = AXUIElementCreateApplication(pid);
        if (!appElement)
        {
            return nullptr;
        }
        
        CFArrayRef windows = nullptr;
        AXError error = AXUIElementCopyAttributeValue(appElement, kAXWindowsAttribute, (CFTypeRef *)&windows);
        CFRelease(appElement);
        
        if (error != kAXErrorSuccess || !windows)
        {
            return nullptr;
        }
        
        AXUIElementRef result = nullptr;
        CFIndex count = CFArrayGetCount(windows);
        
        for (CFIndex i = 0; i < count; i++)
        {
            AXUIElementRef window = (AXUIElementRef)CFArrayGetValueAtIndex(windows, i);
            
            // Try to match by position/size since we can't directly get CGWindowID from AXUIElement
            // This is a workaround - in practice, for most operations we'd use the first window
            if (i == 0)
            {
                result = (AXUIElementRef)CFRetain(window);
                break;
            }
        }
        
        CFRelease(windows);
        return result;
    }

    ErrorCode WindowManagerMacOS::CloseWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        @autoreleasepool {
            auto info = GetWindowInfo(handle);
            if (!info.ok())
            {
                return info.error;
            }
            
            CGWindowID windowID = (CGWindowID)(uintptr_t)handle;
            AXUIElementRef window = GetAXWindowForPid(info.value.processId, windowID);
            
            if (!window)
            {
                return ErrorCode::AccessDenied;
            }
            
            AXUIElementRef closeButton = nullptr;
            AXError error = AXUIElementCopyAttributeValue(window, kAXCloseButtonAttribute, (CFTypeRef *)&closeButton);
            
            if (error == kAXErrorSuccess && closeButton)
            {
                AXUIElementPerformAction(closeButton, kAXPressAction);
                CFRelease(closeButton);
            }
            
            CFRelease(window);
        }
        
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerMacOS::ForceCloseWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        auto info = GetWindowInfo(handle);
        if (!info.ok())
        {
            return info.error;
        }
        
        // Kill the process
        kill(info.value.processId, SIGKILL);
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerMacOS::MinimizeWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        @autoreleasepool {
            auto info = GetWindowInfo(handle);
            if (!info.ok())
            {
                return info.error;
            }
            
            CGWindowID windowID = (CGWindowID)(uintptr_t)handle;
            AXUIElementRef window = GetAXWindowForPid(info.value.processId, windowID);
            
            if (!window)
            {
                return ErrorCode::AccessDenied;
            }
            
            AXUIElementSetAttributeValue(window, kAXMinimizedAttribute, kCFBooleanTrue);
            CFRelease(window);
        }
        
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerMacOS::MaximizeWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        @autoreleasepool {
            auto info = GetWindowInfo(handle);
            if (!info.ok())
            {
                return info.error;
            }
            
            CGWindowID windowID = (CGWindowID)(uintptr_t)handle;
            AXUIElementRef window = GetAXWindowForPid(info.value.processId, windowID);
            
            if (!window)
            {
                return ErrorCode::AccessDenied;
            }
            
            // Get zoom button and press it
            AXUIElementRef zoomButton = nullptr;
            AXError error = AXUIElementCopyAttributeValue(window, kAXZoomButtonAttribute, (CFTypeRef *)&zoomButton);
            
            if (error == kAXErrorSuccess && zoomButton)
            {
                AXUIElementPerformAction(zoomButton, kAXPressAction);
                CFRelease(zoomButton);
            }
            
            CFRelease(window);
        }
        
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerMacOS::RestoreWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        @autoreleasepool {
            auto info = GetWindowInfo(handle);
            if (!info.ok())
            {
                return info.error;
            }
            
            CGWindowID windowID = (CGWindowID)(uintptr_t)handle;
            AXUIElementRef window = GetAXWindowForPid(info.value.processId, windowID);
            
            if (!window)
            {
                return ErrorCode::AccessDenied;
            }
            
            // Unminimize
            AXUIElementSetAttributeValue(window, kAXMinimizedAttribute, kCFBooleanFalse);
            CFRelease(window);
        }
        
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerMacOS::ShowWindow(NativeHandle handle)
    {
        return RestoreWindow(handle);
    }

    ErrorCode WindowManagerMacOS::HideWindow(NativeHandle handle)
    {
        return MinimizeWindow(handle);
    }

    ErrorCode WindowManagerMacOS::FocusWindow(NativeHandle handle)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        @autoreleasepool {
            auto info = GetWindowInfo(handle);
            if (!info.ok())
            {
                return info.error;
            }
            
            // Activate the application
            NSRunningApplication *app = [NSRunningApplication
                runningApplicationWithProcessIdentifier:info.value.processId];
            
            if (app)
            {
                [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
            }
            
            // Also try to raise the specific window via Accessibility API
            CGWindowID windowID = (CGWindowID)(uintptr_t)handle;
            AXUIElementRef window = GetAXWindowForPid(info.value.processId, windowID);
            
            if (window)
            {
                AXUIElementPerformAction(window, kAXRaiseAction);
                CFRelease(window);
            }
        }
        
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerMacOS::SetAlwaysOnTop(NativeHandle handle, bool topmost)
    {
        // macOS doesn't have a simple always-on-top for other applications' windows
        // This would require the application itself to set the window level
        SetLastError("SetAlwaysOnTop not supported for external windows on macOS");
        return ErrorCode::NotSupported;
    }

    ErrorCode WindowManagerMacOS::SetWindowRect(NativeHandle handle, const Rect &rect)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        @autoreleasepool {
            auto info = GetWindowInfo(handle);
            if (!info.ok())
            {
                return info.error;
            }
            
            CGWindowID windowID = (CGWindowID)(uintptr_t)handle;
            AXUIElementRef window = GetAXWindowForPid(info.value.processId, windowID);
            
            if (!window)
            {
                return ErrorCode::AccessDenied;
            }
            
            // Set position
            CGPoint position = CGPointMake(rect.x, rect.y);
            AXValueRef positionValue = AXValueCreate(kAXValueTypeCGPoint, &position);
            AXUIElementSetAttributeValue(window, kAXPositionAttribute, positionValue);
            CFRelease(positionValue);
            
            // Set size
            CGSize size = CGSizeMake(rect.width, rect.height);
            AXValueRef sizeValue = AXValueCreate(kAXValueTypeCGSize, &size);
            AXUIElementSetAttributeValue(window, kAXSizeAttribute, sizeValue);
            CFRelease(sizeValue);
            
            CFRelease(window);
        }
        
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerMacOS::MoveWindow(NativeHandle handle, int x, int y)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        @autoreleasepool {
            auto info = GetWindowInfo(handle);
            if (!info.ok())
            {
                return info.error;
            }
            
            CGWindowID windowID = (CGWindowID)(uintptr_t)handle;
            AXUIElementRef window = GetAXWindowForPid(info.value.processId, windowID);
            
            if (!window)
            {
                return ErrorCode::AccessDenied;
            }
            
            CGPoint position = CGPointMake(x, y);
            AXValueRef positionValue = AXValueCreate(kAXValueTypeCGPoint, &position);
            AXUIElementSetAttributeValue(window, kAXPositionAttribute, positionValue);
            CFRelease(positionValue);
            
            CFRelease(window);
        }
        
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerMacOS::ResizeWindow(NativeHandle handle, int width, int height)
    {
        if (!m_initialized)
        {
            return ErrorCode::NotInitialized;
        }

        @autoreleasepool {
            auto info = GetWindowInfo(handle);
            if (!info.ok())
            {
                return info.error;
            }
            
            CGWindowID windowID = (CGWindowID)(uintptr_t)handle;
            AXUIElementRef window = GetAXWindowForPid(info.value.processId, windowID);
            
            if (!window)
            {
                return ErrorCode::AccessDenied;
            }
            
            CGSize size = CGSizeMake(width, height);
            AXValueRef sizeValue = AXValueCreate(kAXValueTypeCGSize, &size);
            AXUIElementSetAttributeValue(window, kAXSizeAttribute, sizeValue);
            CFRelease(sizeValue);
            
            CFRelease(window);
        }
        
        return ErrorCode::Success;
    }

    ErrorCode WindowManagerMacOS::SetWindowTitle(NativeHandle handle, const std::string &title)
    {
        // Cannot set window title for external applications on macOS
        SetLastError("SetWindowTitle not supported for external windows on macOS");
        return ErrorCode::NotSupported;
    }

    ErrorCode WindowManagerMacOS::SetWindowOpacity(NativeHandle handle, float opacity)
    {
        // Cannot set window opacity for external applications on macOS
        SetLastError("SetWindowOpacity not supported for external windows on macOS");
        return ErrorCode::NotSupported;
    }

    std::string WindowManagerMacOS::GetLastError() const
    {
        return m_lastError;
    }

    void WindowManagerMacOS::SetLastError(const std::string &error)
    {
        m_lastError = error;
    }

} // namespace CrossWindow
