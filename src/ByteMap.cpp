#include <algorithm>
#include <cassert>
#include <cstring>
#include <gtirb/ByteMap.hpp>
#include <gtirb/Utilities.hpp>

using namespace gtirb;

BOOST_CLASS_EXPORT_IMPLEMENT(gtirb::ByteMap);

constexpr uint64_t PageOffsetMask{static_cast<uint64_t>(gtirb::constants::PageSize) - 1};
constexpr uint64_t PageIndexMask{~PageOffsetMask};

// Align an address to a page boundary.
EA ByteMap::Impl::AddressToAlignedAddress(const EA x)
{
    return EA{x.get() & PageIndexMask};
}

// Give the offset within a page of an address
size_t ByteMap::Impl::AddressToOffset(const EA x)
{
    return static_cast<size_t>(x.get()) & PageOffsetMask;
}

// Number of bytes residing within the first page.
size_t ByteMap::Impl::BytesWithinFirstPage(const EA x, const size_t bytes)
{
    const size_t bytesToPageBoundary =
        gtirb::constants::PageSize - (static_cast<size_t>(x.get()) & PageOffsetMask);

    return bytes < bytesToPageBoundary ? bytes : bytesToPageBoundary;
}

bool ByteMap::empty() const
{
    return this->data.empty();
}

size_t ByteMap::size() const
{
    return this->data.size() * gtirb::constants::PageSize;
}

void ByteMap::setData(EA ea, uint8_t x)
{
    const auto pageAddress = ByteMap::Impl::AddressToAlignedAddress(ea);
    const auto pageOffset = ByteMap::Impl::AddressToOffset(ea);

    auto page = this->getOrCreatePage(pageAddress);
    (*page)[pageOffset] = x;
}

// The implementation of this function uses decltype so that its contents can be easily copy/pasted
// to other sizes of 'x'.  Not templated because there's only three options here.
void ByteMap::setData(EA ea, uint16_t x)
{
    const auto bytesThisPage = ByteMap::Impl::BytesWithinFirstPage(ea, sizeof(decltype(x)));

    if(bytesThisPage < sizeof(decltype(x)))
    {
        this->setData(ea, reinterpret_cast<uint8_t*>(&x), sizeof(decltype(x)));
        return;
    }

    const auto pageAddress = ByteMap::Impl::AddressToAlignedAddress(ea);
    const auto pageOffset = ByteMap::Impl::AddressToOffset(ea);

    auto page = this->getOrCreatePage(pageAddress);
    *(decltype(x)*)&((*page)[pageOffset]) = x;
}

void ByteMap::setData(EA ea, uint32_t x)
{
    const auto bytesThisPage = ByteMap::Impl::BytesWithinFirstPage(ea, sizeof(decltype(x)));

    if(bytesThisPage < sizeof(decltype(x)))
    {
        this->setData(ea, reinterpret_cast<uint8_t*>(&x), sizeof(decltype(x)));
        return;
    }

    const auto pageAddress = ByteMap::Impl::AddressToAlignedAddress(ea);
    const auto pageOffset = ByteMap::Impl::AddressToOffset(ea);

    auto page = this->getOrCreatePage(pageAddress);
    *(decltype(x)*)&((*page)[pageOffset]) = x;
}

void ByteMap::setData(EA ea, uint64_t x)
{
    const auto bytesThisPage = ByteMap::Impl::BytesWithinFirstPage(ea, sizeof(decltype(x)));

    if(bytesThisPage < sizeof(decltype(x)))
    {
        this->setData(ea, reinterpret_cast<uint8_t*>(&x), sizeof(decltype(x)));
        return;
    }

    const auto pageAddress = ByteMap::Impl::AddressToAlignedAddress(ea);
    const auto pageOffset = ByteMap::Impl::AddressToOffset(ea);

    auto page = this->getOrCreatePage(pageAddress);
    *(decltype(x)*)&((*page)[pageOffset]) = x;
}

void ByteMap::setData(EA ea, uint8_t* const x, size_t len)
{
    int64_t bytesRemaining = static_cast<int64_t>(len);
    auto currentBuffer = x;
    auto currentAddress = ea;

    while(bytesRemaining > 0)
    {
        const auto bytesThisPage =
            ByteMap::Impl::BytesWithinFirstPage(currentAddress, bytesRemaining);
        const auto pageAddress = ByteMap::Impl::AddressToAlignedAddress(currentAddress);
        const auto pageOffset = ByteMap::Impl::AddressToOffset(currentAddress);

        auto page = this->getOrCreatePage(pageAddress);

        std::memcpy(&(page[pageOffset]), currentBuffer, bytesThisPage);

        currentBuffer += bytesThisPage;
        currentAddress += EA{bytesThisPage};
        bytesRemaining -= static_cast<int64_t>(bytesThisPage);
    }

    // If we went to negative bytes remaining, something went wrong.
    assert(bytesRemaining >= 0);
}

uint8_t ByteMap::getData8(EA x) const
{
	return this->getData(x, 1)[0];
}

uint16_t ByteMap::getData16(EA x) const
{
	const auto byteArray = this->getData(x, sizeof(uint16_t));
    const auto wordArray = gtirb::utilities::ByteArray8To16(byteArray, false);
    const auto word = wordArray[0];
    return word;
}

uint32_t ByteMap::getData32(EA x) const
{
	const auto byteArray = this->getData(x, sizeof(uint32_t));
    const auto wordArray = gtirb::utilities::ByteArray8To32(byteArray, false);
    const auto word = wordArray[0];
    return word;
}

uint64_t ByteMap::getData64(EA x) const
{
	const auto byteArray = this->getData(x, sizeof(uint64_t));
    const auto wordArray = gtirb::utilities::ByteArray8To64(byteArray, false);
    const auto word = wordArray[0];
    return word;
}

std::vector<uint8_t> ByteMap::getData(EA x, size_t bytes) const
{
    std::vector<uint8_t> buffer(bytes, uint8_t{0});

    size_t currentBufferPos{0};
    int64_t bytesRemaining = static_cast<int64_t>(bytes);
    auto currentAddress = x;

    while(bytesRemaining > 0)
    {
        const auto bytesThisPage =
            ByteMap::Impl::BytesWithinFirstPage(currentAddress, bytesRemaining);
        const auto pageAddress = ByteMap::Impl::AddressToAlignedAddress(currentAddress);

        const auto page = this->getPage(pageAddress);

        if(page != nullptr)
        {
            const auto pageOffset = ByteMap::Impl::AddressToOffset(currentAddress);

            auto pageBegin = std::begin(*page);
            std::advance(pageBegin, pageOffset);

            auto bufferBegin = std::begin(buffer);
            std::advance(bufferBegin, currentBufferPos);

            std::copy_n(pageBegin, bytesThisPage, bufferBegin);
        }

        currentBufferPos += bytesThisPage;
        currentAddress += static_cast<EA>(bytesThisPage);
        bytesRemaining -= static_cast<int64_t>(bytesThisPage);
    }

    // If we went to negative bytes remaining, something went wrong.
    assert(bytesRemaining >= 0);

    return buffer;
}

std::vector<uint8_t> ByteMap::getDataUntil(EA x, uint8_t sentinel, size_t maxBytes) const
{
    std::vector<uint8_t> buffer;
    auto bytesRemaining = static_cast<int64_t>(maxBytes);
    auto currentAddress = x;

    while(bytesRemaining > 0)
    {
        const auto bytesThisPage =
            ByteMap::Impl::BytesWithinFirstPage(currentAddress, bytesRemaining);
        const auto pageAddress = ByteMap::Impl::AddressToAlignedAddress(currentAddress);
        const auto page = this->getPage(pageAddress);

        if(page != nullptr)
        {
            const auto pageOffset = ByteMap::Impl::AddressToOffset(currentAddress);

            for(size_t i = 0; i < bytesThisPage; ++i)
            {
                const auto byte = (*page)[pageOffset + i];
                buffer.push_back(byte);

                if(byte == sentinel)
                {
                    return buffer;
                }
            }

            currentAddress += static_cast<EA>(bytesThisPage);
            bytesRemaining -= static_cast<int64_t>(bytesThisPage);
        }
        else
        {
            if(sentinel == '\0')
            {
                buffer.push_back('\0');
            }

            break;
        }
    }

    // If we went to negative bytes remaining, something went wrong.
    assert(bytesRemaining >= 0);

    return buffer;
}

const ByteMap::Page* const ByteMap::getPage(const EA x) const
{
    assert(ByteMap::Impl::AddressToOffset(x) == 0);

    const auto it = this->data.find(x);
    if(it != std::end(this->data))
    {
        return &(it->second);
    }

    return nullptr;
}

ByteMap::Page* ByteMap::getOrCreatePage(const EA x)
{
    assert(ByteMap::Impl::AddressToOffset(x) == 0);
    auto page = &(this->data[x]);
    return page;
}