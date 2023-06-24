#include "psd.hpp"

#include <cstddef>

#include <Psd/PsdMallocAllocator.h>
#include <Psd/PsdNativeFile.h>

#include <wil/resource.h>


namespace {

class FailFastMallocAllocator final : public psd::Allocator
{
private:
    psd::MallocAllocator _allocator;

private:
    virtual void* DoAllocate(std::size_t size, std::size_t alignment) override
    {
        return FAIL_FAST_IF_NULL_ALLOC(_allocator.Allocate(size, alignment));
    }

    virtual void DoFree(void * ptr) override
    {
        _allocator.Free(ptr);
    }
};

} // namespace


PSDDocument::PSDDocument(unsigned int canvas_width,
                         unsigned int canvas_height,
                         unsigned int bits_per_channel,
                         psd::exportColorMode::Enum color_mode) :
    _allocator(std::make_unique<FailFastMallocAllocator>()),
    _document(nullptr)
{
    THROW_HR_IF(E_INVALIDARG,
                bits_per_channel != 8 && bits_per_channel != 16 && bits_per_channel != 32);

    _document = psd::CreateExportDocument(_allocator.get(),
                                          canvas_width,
                                          canvas_height,
                                          bits_per_channel,
                                          color_mode);
    THROW_IF_NULL_ALLOC(_document);
}

PSDDocument::~PSDDocument()
{
    psd::DestroyExportDocument(_document, _allocator.get());
}

PSDLayer PSDDocument::add_layer(std::string const & name)
{
    if (_document->layerCount == psd::ExportDocument::MAX_LAYER_COUNT)
    {
        THROW_HR_MSG(E_OUTOFMEMORY, "Too many layers");
    }

    auto const index = psd::AddLayer(_document, _allocator.get(), name.c_str());
    return PSDLayer(*this, index);
}

void PSDDocument::save(std::wstring const & filename)
{
    psd::NativeFile file(_allocator.get());
    if (!file.OpenWrite(filename.c_str()))
    {
        THROW_HR_MSG(E_FAIL, "file.OpenWrite(%ws)", filename.c_str());
    }
    auto close_file = wil::scope_exit([&file] () { file.Close(); });

    psd::WriteDocument(_document, _allocator.get(), &file);
}

PSDLayer::PSDLayer(PSDDocument & document, unsigned int index) :
    _document(&document),
    _index(index)
{
}
