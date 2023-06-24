#include <memory>
#include <string>
#include <cstdint>
#include <ranges>
#include <cstddef>
#include <type_traits>

#include <Windows.h>
#include <intsafe.h>

#include <Psd/Psd.h>
#include <Psd/PsdAllocator.h>
#include <Psd/PsdExport.h>
#include <Psd/PsdExportDocument.h>

#include <wil/result.h>


class PSDDocument final
{
    friend class PSDLayer;

private:
    std::unique_ptr<psd::Allocator> _allocator;
    psd::ExportDocument * _document;

public:
    PSDDocument(unsigned int canvas_width,
                unsigned int canvas_height,
                unsigned int bits_per_channel,
                psd::exportColorMode::Enum color_mode);

    ~PSDDocument();

    PSDDocument(PSDDocument const &) = delete;
    PSDDocument & operator=(PSDDocument const &) = delete;

    PSDLayer add_layer(std::string const & name);

    void save(std::wstring const & filename);
};

class PSDLayer final
{
    friend class PSDDocument;

private:
    PSDDocument * _document;
    unsigned int _index;

public:
    template<std::ranges::view T>
    void update8(psd::exportChannel::Enum channel,
                 int left, int top,
                 int right, int bottom,
                 T data,
                 psd::compressionType::Enum compression)
    {
        static_assert(std::is_same_v<std::ranges::range_value_t<T>, std::uint8_t>);

        THROW_HR_IF(E_INVALIDARG, _document->_document->bitsPerChannel != 8);

        THROW_HR_IF(E_INVALIDARG, right < left);
        THROW_HR_IF(E_INVALIDARG, bottom < top);

        std::size_t width = right - left;
        std::size_t height = bottom - top;

        std::size_t pixels = 0;
        THROW_IF_FAILED(SizeTMult(width, height, &pixels));
        THROW_HR_IF(E_INVALIDARG, std::ranges::size(data) != pixels);

        psd::UpdateLayer(_document->_document,
                         _document->_allocator.get(),
                         _index,
                         channel,
                         left, top,
                         right, bottom,
                         std::ranges::data(data),
                         compression);
    }

private:
    PSDLayer(PSDDocument & document, unsigned int index);
};
