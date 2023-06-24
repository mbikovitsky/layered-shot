#include <variant>

#include <Windows.h>

#include <wil/resource.h>


class DeviceContext final
{
private:
    std::variant<wil::unique_hdc, wil::unique_hdc_window> _dc;

public:
    static DeviceContext get_window_dc(HWND hwnd);
    static DeviceContext create_compatible_dc(HDC dc);

    HDC get() const noexcept;

    POINT set_window_origin(int x, int y);

    void select_object(HGDIOBJ object);

    void blit(int dst_x, int dst_y,
              DeviceContext const & src,
              int src_x, int src_y,
              int width, int height,
              DWORD rop);

private:
    DeviceContext(wil::unique_hdc dc) noexcept;
    DeviceContext(wil::unique_hdc_window dc) noexcept;
};
