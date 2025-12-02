/**
 * @file WindowManagerStub.h
 * @brief Stub implementation for unsupported platforms
 */

#pragma once

#include "../../WindowManagerImpl.h"

namespace CrossWindow
{

    class WindowManagerStub : public WindowManagerImplBase
    {
    public:
        WindowManagerStub() = default;
        ~WindowManagerStub() override = default;

        bool Initialize() override
        {
            SetLastError("Platform not supported");
            return false;
        }

        bool IsInitialized() const override { return false; }
        void Shutdown() override {}

        std::vector<WindowInfo> GetAllWindows() override { return {}; }
        void EnumerateWindows(const EnumWindowsCallback &) override {}
        std::vector<WindowInfo> FindWindowsByTitle(const std::string &, bool) override { return {}; }
        std::vector<WindowInfo> FindWindowsByProcess(const std::string &) override { return {}; }

        Result<WindowInfo> GetWindowInfo(NativeHandle) override
        {
            return {WindowInfo{}, ErrorCode::NotSupported, "Platform not supported"};
        }

        Result<std::string> GetWindowTitle(NativeHandle) override
        {
            return {std::string{}, ErrorCode::NotSupported, "Platform not supported"};
        }

        Result<Rect> GetWindowRect(NativeHandle) override
        {
            return {Rect{}, ErrorCode::NotSupported, "Platform not supported"};
        }

        Result<WindowState> GetWindowState(NativeHandle) override
        {
            return {WindowState::Normal, ErrorCode::NotSupported, "Platform not supported"};
        }

        Result<uint32_t> GetWindowProcessId(NativeHandle) override
        {
            return {0, ErrorCode::NotSupported, "Platform not supported"};
        }

        bool IsWindowVisible(NativeHandle) override { return false; }
        bool IsValidWindow(NativeHandle) override { return false; }

        NativeHandle GetFocusedWindow() override { return NativeHandle{}; }
        Result<WindowInfo> GetFocusedWindowInfo() override
        {
            return {WindowInfo{}, ErrorCode::NotSupported, "Platform not supported"};
        }

        ErrorCode CloseWindow(NativeHandle) override { return ErrorCode::NotSupported; }
        ErrorCode ForceCloseWindow(NativeHandle) override { return ErrorCode::NotSupported; }
        ErrorCode MinimizeWindow(NativeHandle) override { return ErrorCode::NotSupported; }
        ErrorCode MaximizeWindow(NativeHandle) override { return ErrorCode::NotSupported; }
        ErrorCode RestoreWindow(NativeHandle) override { return ErrorCode::NotSupported; }
        ErrorCode ShowWindow(NativeHandle) override { return ErrorCode::NotSupported; }
        ErrorCode HideWindow(NativeHandle) override { return ErrorCode::NotSupported; }
        ErrorCode FocusWindow(NativeHandle) override { return ErrorCode::NotSupported; }
        ErrorCode SetAlwaysOnTop(NativeHandle, bool) override { return ErrorCode::NotSupported; }
        ErrorCode SetWindowRect(NativeHandle, const Rect &) override { return ErrorCode::NotSupported; }
        ErrorCode MoveWindow(NativeHandle, int, int) override { return ErrorCode::NotSupported; }
        ErrorCode ResizeWindow(NativeHandle, int, int) override { return ErrorCode::NotSupported; }
        ErrorCode SetWindowTitle(NativeHandle, const std::string &) override { return ErrorCode::NotSupported; }
        ErrorCode SetWindowOpacity(NativeHandle, float) override { return ErrorCode::NotSupported; }

        std::string GetLastError() const override { return m_lastError; }
        void SetLastError(const std::string &error) override { m_lastError = error; }
    };

} // namespace CrossWindow
