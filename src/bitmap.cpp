#include "bitmap.hpp"

#include <cassert>
#include <cstddef>

#include <intsafe.h>

#include <wil/result.h>


Bitmap Bitmap::create_compatible_bitmap(HDC dc, int width, int height)
{
    THROW_HR_IF_NULL(E_INVALIDARG, dc);

    auto const bitmap = CreateCompatibleBitmap(dc, width, height);
    if (bitmap == nullptr)
    {
        THROW_HR_MSG(E_FAIL, "CreateCompatibleBitmap(0x%p, %d, %d)", dc, width, height);
    }

    return Bitmap(bitmap, dc);
}

BITMAP Bitmap::info() const
{
    BITMAP header {};
    if (GetObjectW(_bitmap.get(), sizeof(header), &header) == 0)
    {
        THROW_WIN32_MSG(GetLastError(), "GetObjectW");
    }
    header.bmBits = nullptr;
    return header;
}

std::vector<RGBQUAD> Bitmap::data() const
{
    THROW_HR_IF_NULL_MSG(E_INVALIDARG, _dc, "Not a compatible bitmap");

    auto const bitmap_info = info();

    BITMAPINFOHEADER header {};
    header.biSize = sizeof(header);
    header.biWidth = bitmap_info.bmWidth;
    header.biHeight = -bitmap_info.bmHeight;
    header.biPlanes = 1;
    header.biBitCount = 32;
    header.biCompression = BI_RGB;

    THROW_HR_IF_MSG(E_INVALIDARG, header.biHeight == bitmap_info.bmHeight,
                    "Image height is too large");

    std::size_t width = 0;
    THROW_IF_FAILED(LongToSizeT(bitmap_info.bmWidth, &width));
    std::size_t height = 0;
    THROW_IF_FAILED(LongToSizeT(bitmap_info.bmHeight, &height));

    std::size_t buffer_size = 0;
    THROW_IF_FAILED(SizeTMult(width, height, &buffer_size));

    std::vector<RGBQUAD> buffer(buffer_size);

    auto const result = GetDIBits(_dc, _bitmap.get(),
                                  0,  // Start scan line
                                  bitmap_info.bmHeight,
                                  buffer.data(),
                                  (BITMAPINFO *)&header,
                                  DIB_RGB_COLORS);
    if (result != bitmap_info.bmHeight)
    {
        THROW_HR_MSG(E_FAIL, "GetDIBits");
    }

    return buffer;
}

Bitmap::Bitmap(HBITMAP bitmap, HDC dc) noexcept :
    _bitmap(bitmap),
    _dc(dc)
{
    assert(bitmap != nullptr);
}
