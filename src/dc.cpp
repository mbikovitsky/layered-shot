#include "dc.hpp"

#include <cassert>
#include <utility>

#include <wil/result.h>


DeviceContext DeviceContext::get_window_dc(HWND hwnd)
{
    THROW_HR_IF_NULL(E_INVALIDARG, hwnd);

    auto window_dc = wil::unique_hdc_window(wil::window_dc(GetWindowDC(hwnd), hwnd));
    if (!window_dc)
    {
        THROW_HR_MSG(E_FAIL, "GetWindowDC(0x%p)", hwnd);
    }
    return DeviceContext(std::move(window_dc));
}

DeviceContext DeviceContext::create_compatible_dc(HDC dc)
{
    auto compatible_dc = wil::unique_hdc(CreateCompatibleDC(dc));
    if (!compatible_dc)
    {
        THROW_HR_MSG(E_FAIL, "CreateCompatibleDC(0x%p)", dc);
    }
    return DeviceContext(std::move(compatible_dc));
}

HDC DeviceContext::get() const noexcept
{
    return std::visit([] (auto const & dc) { return dc.get(); }, _dc);
}

POINT DeviceContext::set_window_origin(int x, int y)
{
    POINT previous {};
    if (!SetWindowOrgEx(get(), x, y, &previous))
    {
        THROW_HR_MSG(E_FAIL, "SetWindowOrgEx(0x%p, %d, %d)", get(), x, y);
    }
    return previous;
}

void DeviceContext::select_object(HGDIOBJ object)
{
    auto const result = SelectObject(get(), object);
    if (result == nullptr || result == HGDI_ERROR)
    {
        THROW_HR_MSG(E_FAIL, "SelectObject(0x%p, 0x%p)", get(), object);
    }
}

void DeviceContext::blit(int dst_x, int dst_y,
                         DeviceContext const & src,
                         int src_x, int src_y,
                         int width, int height,
                         DWORD rop)
{
    THROW_IF_WIN32_BOOL_FALSE(BitBlt(get(), dst_x, dst_y,
                                     width, height,
                                     src.get(), src_x, src_y,
                                     rop));
}

DeviceContext::DeviceContext(wil::unique_hdc dc) noexcept :
    _dc(std::move(dc))
{
    assert(get());
}

DeviceContext::DeviceContext(wil::unique_hdc_window dc) noexcept :
    _dc(std::move(dc))
{
    assert(get());
}
