#include <cassert>
#include <vector>
#include <string>
#include <string_view>
#include <ranges>

#include <Windows.h>
#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>

#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

#include <wil/result.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>

#include "dc.hpp"
#include "bitmap.hpp"
#include "psd.hpp"


namespace {

struct WindowInfo
{
    RECT coords;
    std::wstring title;
    BITMAP bitmap_info;
    std::vector<RGBQUAD> bitmap_data;
};

std::wstring get_window_text(HWND const hwnd)
{
    std::wstring text;

    auto const result = wil::AdaptFixedSizeToAllocatedResult(
        text,
        [hwnd] (PWSTR value, std::size_t length, std::size_t * needed) {
            SetLastError(0);
            auto const needed_without_null = GetWindowTextLengthW(hwnd);
            if (needed_without_null == 0)
            {
                RETURN_IF_WIN32_ERROR(GetLastError());
            }
            RETURN_HR_IF(E_FAIL, needed_without_null < 0);

            // If the text is empty, don't do anything.
            if (needed_without_null == 0)
            {
                *needed = 1;  // Just a null terminator
                return S_OK;
            }

            int needed_with_null = 0;
            RETURN_IF_FAILED(IntAdd(needed_without_null, 1, &needed_with_null));

            if (length < static_cast<std::size_t>(needed_with_null))
            {
                *needed = static_cast<std::size_t>(needed_with_null);
                return S_OK;
            }

            int buffer_size = 0;
            RETURN_IF_FAILED(SizeTToInt(length, &buffer_size));

            SetLastError(0);
            auto const copied_without_null = GetWindowTextW(hwnd, value, buffer_size);
            if (copied_without_null == 0)
            {
                RETURN_IF_WIN32_ERROR(GetLastError());
            }

            int copied_with_null = 0;
            RETURN_IF_FAILED(IntAdd(copied_without_null, 1, &copied_with_null));

            *needed = static_cast<std::size_t>(copied_with_null);
            return S_OK;
        }
    );
    if (FAILED(result))
    {
        THROW_HR_MSG(result, "GetWindowTextW(0x%p)", hwnd);
    }

    return text;
}

std::string wide_char_to_utf8(std::wstring_view wide_string)
{
    if (wide_string.empty())
    {
        return {};
    }

    int input_size = 0;
    THROW_IF_FAILED(SizeTToInt(wide_string.size(), &input_size));

    auto const required_size = WideCharToMultiByte(CP_UTF8,
                                                   WC_ERR_INVALID_CHARS,
                                                   wide_string.data(), input_size,
                                                   nullptr, 0,
                                                   nullptr, nullptr);
    if (required_size == 0)
    {
        THROW_WIN32_MSG(GetLastError(), "WideCharToMultiByte");
    }
    THROW_HR_IF(E_FAIL, required_size < 0);

    std::string result;
    result.resize(required_size);

    auto const copied = WideCharToMultiByte(CP_UTF8,
                                            WC_ERR_INVALID_CHARS,
                                            wide_string.data(), input_size,
                                            result.data(), required_size,
                                            nullptr, nullptr);
    if (copied != required_size)
    {
        THROW_HR(E_FAIL);
    }

    return result;
}

BOOL CALLBACK window_enum_proc(HWND hwnd, LPARAM lparam) noexcept
{
    try
    {
        assert(hwnd != nullptr);
        assert(lparam != 0);
        auto & windows = *reinterpret_cast<std::vector<WindowInfo> *>(lparam);

        if (!IsWindowVisible(hwnd)) { return TRUE; }
        if (IsIconic(hwnd)) { return TRUE; }

        DWORD cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))))
        {
            if (cloaked != 0)
            {
                return TRUE;
            }
        }

        auto window_dc = DeviceContext::get_window_dc(hwnd);

        RECT window_coords {};
        THROW_IF_WIN32_BOOL_FALSE(GetWindowRect(hwnd, &window_coords));

        auto bitmap = Bitmap::create_compatible_bitmap(
            window_dc.get(),
            window_coords.right - window_coords.left,
            window_coords.bottom - window_coords.top
        );

        {
            auto mem_dc = DeviceContext::create_compatible_dc(window_dc.get());
            mem_dc.select_object(bitmap.get());
            if (!PrintWindow(hwnd, mem_dc.get(), PW_RENDERFULLCONTENT))
            {
                THROW_HR_MSG(E_FAIL, "PrintWindow(0x%p)", hwnd);
            }
        }

        windows.emplace_back(WindowInfo {
            .coords = window_coords,
            .title = get_window_text(hwnd),
            .bitmap_info = bitmap.info(),
            .bitmap_data = bitmap.data(),
        });
    }
    CATCH_LOG()

    return TRUE;
}

} // namespace


int wmain()
{
    auto screen_left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    auto screen_top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    auto screen_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    auto screen_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    std::vector<WindowInfo> windows;

    // TODO: enumerate metro apps with NtUserBuildHwndList
    // https://stackoverflow.com/a/52488331/851560
    if (!EnumWindows(&window_enum_proc, reinterpret_cast<LPARAM>(&windows)))
    {
        THROW_WIN32_MSG(GetLastError(), "EnumWindows");
    }

    auto document = PSDDocument(screen_width, screen_height, 8, psd::exportColorMode::RGB);

    // EnumWindows returns windows in Z-order, from topmost to bottom-most.
    // Layers in the PSD are stacked from bottom to top.
    // So reverse the window order to fix that.
    for (auto const & window : windows | std::views::reverse)
    {
        auto const pixels =
            static_cast<std::size_t>(window.bitmap_info.bmWidth) *
            static_cast<std::size_t>(window.bitmap_info.bmHeight);

        std::vector<std::uint8_t> red(pixels);
        std::vector<std::uint8_t> green(pixels);
        std::vector<std::uint8_t> blue(pixels);
        std::vector<std::uint8_t> alpha(pixels);
        for (std::size_t i = 0; i < pixels; ++i)
        {
            auto const pixel = window.bitmap_data.at(i);
            red.at(i) = pixel.rgbRed;
            green.at(i) = pixel.rgbGreen;
            blue.at(i) = pixel.rgbBlue;
            alpha.at(i) = pixel.rgbReserved;  // Probably?
        }

        auto layer = document.add_layer(wide_char_to_utf8(window.title));
        layer.update8(psd::exportChannel::RED,
                      window.coords.left - screen_left,
                      window.coords.top - screen_top,
                      window.coords.right - screen_left,
                      window.coords.bottom - screen_top,
                      std::views::all(red),
                      psd::compressionType::RLE);
        layer.update8(psd::exportChannel::GREEN,
                      window.coords.left - screen_left,
                      window.coords.top - screen_top,
                      window.coords.right - screen_left,
                      window.coords.bottom - screen_top,
                      std::views::all(green),
                      psd::compressionType::RLE);
        layer.update8(psd::exportChannel::BLUE,
                      window.coords.left - screen_left,
                      window.coords.top - screen_top,
                      window.coords.right - screen_left,
                      window.coords.bottom - screen_top,
                      std::views::all(blue),
                      psd::compressionType::RLE);
        layer.update8(psd::exportChannel::ALPHA,
                      window.coords.left - screen_left,
                      window.coords.top - screen_top,
                      window.coords.right - screen_left,
                      window.coords.bottom - screen_top,
                      std::views::all(alpha),
                      psd::compressionType::RLE);
    }

    document.save(L"Screenshot.psd");
}
