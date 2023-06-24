#include <vector>

#include <Windows.h>

#include <wil/resource.h>


class Bitmap final
{
private:
    wil::unique_hbitmap _bitmap;
    HDC _dc;  // DC for a compatible bitmap

public:
    static Bitmap create_compatible_bitmap(HDC dc, int width, int height);

    HBITMAP get() const noexcept { return _bitmap.get(); }

    BITMAP info() const;

    std::vector<RGBQUAD> data() const;

private:
    Bitmap(HBITMAP bitmap, HDC dc) noexcept;
};
