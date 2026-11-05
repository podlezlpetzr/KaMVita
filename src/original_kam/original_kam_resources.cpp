#include "original_kam_resources.h"

#include <cstdio>
#include <cstring>
#include <limits>


namespace OriginalKaM {


namespace {


constexpr std::size_t PaletteOffset = 48;
constexpr std::size_t PaletteByteCount = 768;
constexpr std::size_t PaletteColorCount = 256;

constexpr std::size_t HousesDatBeastSize = 2100;
constexpr std::size_t HouseDatRecordSize = 1688;
constexpr std::size_t HouseDatRecordCount = 29;

constexpr std::size_t ExpectedHousesDatSize =
    HousesDatBeastSize +
    HouseDatRecordSize *
    HouseDatRecordCount;

std::uint16_t readU16(
    const std::uint8_t* bytes
)
{
    return
        static_cast<std::uint16_t>(
            bytes[0]
        ) |
        (
            static_cast<std::uint16_t>(
                bytes[1]
            ) << 8
        );
}


std::int16_t readI16(
    const std::uint8_t* bytes
)
{
    return static_cast<std::int16_t>(
        readU16(bytes)
    );
}


std::uint32_t readU32(
    const std::uint8_t* bytes
)
{
    return
        static_cast<std::uint32_t>(
            bytes[0]
        ) |
        (
            static_cast<std::uint32_t>(
                bytes[1]
            ) << 8
        ) |
        (
            static_cast<std::uint32_t>(
                bytes[2]
            ) << 16
        ) |
        (
            static_cast<std::uint32_t>(
                bytes[3]
            ) << 24
        );
}


std::int32_t readI32(
    const std::uint8_t* bytes
)
{
    return static_cast<std::int32_t>(
        readU32(bytes)
    );
}


std::uint32_t packRgba(
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha
)
{
    return
        static_cast<std::uint32_t>(red) |
        (
            static_cast<std::uint32_t>(
                green
            ) << 8
        ) |
        (
            static_cast<std::uint32_t>(
                blue
            ) << 16
        ) |
        (
            static_cast<std::uint32_t>(
                alpha
            ) << 24
        );
}


std::string joinPath(
    const std::string& root,
    const std::string& relative
)
{
    if (root.empty())
        return relative;

    if (
        root[root.size() - 1] == '/' ||
        root[root.size() - 1] == '\\'
    ) {
        return root + relative;
    }

    return root + "/" + relative;
}


} // namespace


RxSprite::RxSprite()
    : id(0),
      width(0),
      height(0),
      pivotX(0),
      pivotY(0)
{
}


HouseDatRecord::HouseDatRecord()
    : datIndex(-1),
      stonePicDat(-1),
      woodPicDat(-1),
      woodPalDat(-1),
      stonePalDat(-1),
      stonePicRx(0),
      woodPicRx(0),
      woodPalRx(0),
      stonePalRx(0),
      woodSteps(0),
      stoneSteps(0),
      entranceOffsetX(0),
      entranceOffsetY(0),
      entrancePixelX(0),
      entrancePixelY(0),
      woodCost(0),
      stoneCost(0)
{
}


Resources::Resources()
    : ready_(false)
{
}


bool Resources::initialize(
    const std::string& dataRoot
)
{
    ready_ = false;
    lastError_.clear();
    palette_.clear();

    dataRoot_ = dataRoot;

    housesDatPath_ = joinPath(
        dataRoot_,
        "defines/houses.dat"
    );

    housesRxPath_ = joinPath(
        dataRoot_,
        "gfx/res/houses.rx"
    );

    palettePath_ = joinPath(
        dataRoot_,
        "gfx/pal0.bbm"
    );

    if (!validateHousesDat())
        return false;

    if (!validateHousesRx())
        return false;

    if (!loadPalette())
        return false;

    HouseDatRecord mill;

    if (!readMillRecord(&mill))
        return false;

    if (
        mill.stonePicRx != 358 ||
        mill.woodPicRx != 359 ||
        mill.woodSteps != 21 ||
        mill.stoneSteps != 31
    ) {
        setError(
            "Mill record 22 does not match "
            "the expected KaM resource layout"
        );

        return false;
    }

    RxSprite stone;

    if (!loadRxSprite(
        mill.stonePicRx,
        &stone
    )) {
        return false;
    }

    if (
        stone.width != 111 ||
        stone.height != 113 ||
        stone.pivotX != -35 ||
        stone.pivotY != -85
    ) {
        setError(
            "Mill StonePic RX 358 geometry "
            "does not match expected values"
        );

        return false;
    }

    ready_ = true;
    return true;
}


bool Resources::isReady() const
{
    return ready_;
}


const std::string&
Resources::getDataRoot() const
{
    return dataRoot_;
}


const std::string&
Resources::getLastError() const
{
    return lastError_;
}


bool Resources::readFile(
    const std::string& path,
    std::vector<std::uint8_t>* output
) const
{
    if (!output) {
        setError(
            "readFile received null output"
        );

        return false;
    }

    output->clear();

    FILE* file = std::fopen(
        path.c_str(),
        "rb"
    );

    if (!file) {
        setError(
            "Could not open file: " + path
        );

        return false;
    }

    if (
        std::fseek(
            file,
            0,
            SEEK_END
        ) != 0
    ) {
        std::fclose(file);

        setError(
            "Could not seek file: " + path
        );

        return false;
    }

    const long size = std::ftell(file);

    if (size < 0) {
        std::fclose(file);

        setError(
            "Could not determine file size: " +
            path
        );

        return false;
    }

    if (
        std::fseek(
            file,
            0,
            SEEK_SET
        ) != 0
    ) {
        std::fclose(file);

        setError(
            "Could not rewind file: " + path
        );

        return false;
    }

    output->resize(
        static_cast<std::size_t>(size)
    );

    if (!output->empty()) {
        const std::size_t readCount =
            std::fread(
                output->data(),
                1,
                output->size(),
                file
            );

        if (readCount != output->size()) {
            std::fclose(file);
            output->clear();

            setError(
                "Short read from file: " + path
            );

            return false;
        }
    }

    std::fclose(file);
    return true;
}


bool Resources::validateHousesDat()
{
    std::vector<std::uint8_t> data;

    if (!readFile(
        housesDatPath_,
        &data
    )) {
        return false;
    }

    if (
        data.size() !=
        ExpectedHousesDatSize
    ) {
        setError(
            "Unexpected houses.dat size: " +
            std::to_string(data.size()) +
            ", expected " +
            std::to_string(
                ExpectedHousesDatSize
            )
        );

        return false;
    }

    return true;
}


bool Resources::validateHousesRx()
{
    std::vector<std::uint8_t> data;

    if (!readFile(
        housesRxPath_,
        &data
    )) {
        return false;
    }

    if (data.size() < 4) {
        setError(
            "houses.rx is too small"
        );

        return false;
    }

    const std::uint32_t spriteCount =
        readU32(data.data());

    if (
        spriteCount == 0 ||
        spriteCount >
            static_cast<std::uint32_t>(
                data.size() - 4
            )
    ) {
        setError(
            "Invalid houses.rx sprite count"
        );

        return false;
    }

    return true;
}


bool Resources::loadPalette()
{
    std::vector<std::uint8_t> raw;

    if (!readFile(
        palettePath_,
        &raw
    )) {
        return false;
    }

    if (
        raw.size() <
        PaletteOffset +
        PaletteByteCount
    ) {
        setError(
            "pal0.bbm does not contain "
            "a complete 256-color palette"
        );

        return false;
    }

    palette_.resize(
        PaletteColorCount
    );

    for (
        std::size_t index = 0;
        index < PaletteColorCount;
        ++index
    ) {
        const std::size_t offset =
            PaletteOffset +
            index * 3;

        ColorRGBA color;

        color.red = raw[offset + 0];
        color.green = raw[offset + 1];
        color.blue = raw[offset + 2];

        color.alpha =
            index == 0
                ? 0
                : 255;

        palette_[index] = color;
    }

    return true;
}


bool Resources::readHouseRecord(
    int datIndex,
    HouseDatRecord* output
) const
{
    if (!output) {
        setError(
            "readHouseRecord received null output"
        );

        return false;
    }

    if (
        datIndex < 0 ||
        datIndex >=
            static_cast<int>(
                HouseDatRecordCount
            )
    ) {
        setError(
            "House DAT index is out of range"
        );

        return false;
    }

    std::vector<std::uint8_t> data;

    if (!readFile(
        housesDatPath_,
        &data
    )) {
        return false;
    }

    if (
        data.size() !=
        ExpectedHousesDatSize
    ) {
        setError(
            "houses.dat layout changed"
        );

        return false;
    }

    const std::size_t recordOffset =
        HousesDatBeastSize +
        static_cast<std::size_t>(
            datIndex
        ) * HouseDatRecordSize;

    const std::uint8_t* record =
        data.data() + recordOffset;

    HouseDatRecord result;

    result.datIndex = datIndex;

    result.stonePicDat =
        readI16(record + 0);

    result.woodPicDat =
        readI16(record + 2);

    result.woodPalDat =
        readI16(record + 4);

    result.stonePalDat =
        readI16(record + 6);

    result.stonePicRx =
        result.stonePicDat + 1;

    result.woodPicRx =
        result.woodPicDat + 1;

    result.woodPalRx =
        result.woodPalDat + 1;

    result.stonePalRx =
        result.stonePalDat + 1;

    result.woodSteps =
        readU16(record + 1418);

    result.stoneSteps =
        readU16(record + 1420);

    result.entranceOffsetX =
        static_cast<std::int8_t>(
            record[1424]
        );

    result.entranceOffsetY =
        static_cast<std::int8_t>(
            record[1425]
        );

    result.entrancePixelX =
        static_cast<std::int8_t>(
            record[1426]
        );

    result.entrancePixelY =
        static_cast<std::int8_t>(
            record[1427]
        );

    result.woodCost =
        record[1528];

    result.stoneCost =
        record[1529];

    *output = result;
    return true;
}


bool Resources::getHouseDatIndex(
    HouseType type,
    int* output
) const
{
    if (!output) {
        setError(
            "getHouseDatIndex received null output"
        );

        return false;
    }

    int datIndex = -1;

    switch (type) {
        case HouseType::Store:
            datIndex = 11;
            break;

        case HouseType::Bakery:
            datIndex = 7;
            break;

        case HouseType::Farm:
            datIndex = 8;
            break;

        case HouseType::School:
            datIndex = 13;
            break;

        case HouseType::Quarry:
            datIndex = 14;
            break;

        case HouseType::Mill:
            datIndex = 22;
            break;
    }

    if (
        datIndex < 0 ||
        datIndex >=
            static_cast<int>(
                HouseDatRecordCount
            )
    ) {
        setError(
            "House type has no valid DAT mapping"
        );

        return false;
    }

    *output = datIndex;
    return true;
}


bool Resources::readHouseRecord(
    HouseType type,
    HouseDatRecord* output
) const
{
    int datIndex = -1;

    if (!getHouseDatIndex(
        type,
        &datIndex
    )) {
        return false;
    }

    return readHouseRecord(
        datIndex,
        output
    );
}


bool Resources::readMillRecord(
    HouseDatRecord* output
) const
{
    return readHouseRecord(
        HouseType::Mill,
        output
    );
}


bool Resources::readRxSpriteIndexed(
    int spriteId,
    RxSprite* output
) const
{
    if (!output) {
        setError(
            "readRxSpriteIndexed received "
            "null output"
        );

        return false;
    }

    if (spriteId <= 0) {
        setError(
            "RX sprite ID must be positive"
        );

        return false;
    }

    std::vector<std::uint8_t> data;

    if (!readFile(
        housesRxPath_,
        &data
    )) {
        return false;
    }

    if (data.size() < 4) {
        setError(
            "houses.rx header is missing"
        );

        return false;
    }

    const std::uint32_t spriteCount =
        readU32(data.data());

    if (
        spriteId >
        static_cast<int>(spriteCount)
    ) {
        setError(
            "RX sprite ID is out of range"
        );

        return false;
    }

    const std::size_t usedOffset = 4;
    const std::size_t recordsOffset =
        usedOffset + spriteCount;

    if (recordsOffset > data.size()) {
        setError(
            "houses.rx used table is truncated"
        );

        return false;
    }

    std::size_t cursor = recordsOffset;

    for (
        std::uint32_t currentId = 1;
        currentId <= spriteCount;
        ++currentId
    ) {
        const std::uint8_t used =
            data[
                usedOffset +
                currentId - 1
            ];

        if (used != 1)
            continue;

        if (
            cursor + 12 >
            data.size()
        ) {
            setError(
                "houses.rx sprite header "
                "is truncated"
            );

            return false;
        }

        const int width =
            readI16(
                data.data() + cursor
            );

        const int height =
            readI16(
                data.data() + cursor + 2
            );

        const int pivotX =
            readI32(
                data.data() + cursor + 4
            );

        const int pivotY =
            readI32(
                data.data() + cursor + 8
            );

        cursor += 12;

        if (
            width <= 0 ||
            height <= 0
        ) {
            setError(
                "houses.rx sprite has "
                "invalid dimensions"
            );

            return false;
        }

        const std::size_t widthSize =
            static_cast<std::size_t>(
                width
            );

        const std::size_t heightSize =
            static_cast<std::size_t>(
                height
            );

        if (
            widthSize >
            std::numeric_limits<
                std::size_t
            >::max() /
            heightSize
        ) {
            setError(
                "houses.rx sprite size "
                "overflow"
            );

            return false;
        }

        const std::size_t pixelCount =
            widthSize * heightSize;

        if (
            cursor + pixelCount >
            data.size()
        ) {
            setError(
                "houses.rx sprite pixels "
                "are truncated"
            );

            return false;
        }

        if (
            currentId ==
            static_cast<std::uint32_t>(
                spriteId
            )
        ) {
            RxSprite result;

            result.id = spriteId;
            result.width = width;
            result.height = height;
            result.pivotX = pivotX;
            result.pivotY = pivotY;

            result.indexedPixels.assign(
                data.begin() + cursor,
                data.begin() +
                    cursor +
                    pixelCount
            );

            *output = result;
            return true;
        }

        cursor += pixelCount;
    }

    setError(
        "RX sprite is marked unused "
        "or was not found"
    );

    return false;
}


void Resources::convertIndexedToRgba(
    RxSprite* sprite
) const
{
    if (!sprite)
        return;

    sprite->rgbaPixels.resize(
        sprite->indexedPixels.size()
    );

    for (
        std::size_t index = 0;
        index <
            sprite->indexedPixels.size();
        ++index
    ) {
        const std::uint8_t paletteIndex =
            sprite->indexedPixels[index];

        const ColorRGBA& color =
            palette_[paletteIndex];

        sprite->rgbaPixels[index] =
            packRgba(
                color.red,
                color.green,
                color.blue,
                color.alpha
            );
    }
}


bool Resources::loadRxSprite(
    int spriteId,
    RxSprite* output
) const
{
    if (!output) {
        setError(
            "loadRxSprite received null output"
        );

        return false;
    }

    if (
        palette_.size() !=
        PaletteColorCount
    ) {
        setError(
            "Palette is not initialized"
        );

        return false;
    }

    RxSprite sprite;

    if (!readRxSpriteIndexed(
        spriteId,
        &sprite
    )) {
        return false;
    }

    convertIndexedToRgba(
        &sprite
    );

    *output = sprite;
    return true;
}


void Resources::setError(
    const std::string& message
) const
{
    lastError_ = message;
}


} // namespace OriginalKaM
