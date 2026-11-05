#pragma once

#include <cstdint>
#include <string>
#include <vector>


namespace OriginalKaM {


enum class HouseType {
    Store,
    Bakery,
    Farm,
    School,
    Quarry,
    Mill
};


struct ColorRGBA {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};


struct RxSprite {
    int id;
    int width;
    int height;
    int pivotX;
    int pivotY;

    std::vector<std::uint8_t> indexedPixels;
    std::vector<std::uint32_t> rgbaPixels;

    RxSprite();
};


struct HouseDatRecord {
    int datIndex;

    int stonePicDat;
    int woodPicDat;
    int woodPalDat;
    int stonePalDat;

    int stonePicRx;
    int woodPicRx;
    int woodPalRx;
    int stonePalRx;

    int woodSteps;
    int stoneSteps;

    int entranceOffsetX;
    int entranceOffsetY;

    int entrancePixelX;
    int entrancePixelY;

    int woodCost;
    int stoneCost;

    HouseDatRecord();
};


class Resources {
public:
    Resources();

    bool initialize(
        const std::string& dataRoot
    );

    bool isReady() const;

    const std::string& getDataRoot() const;
    const std::string& getLastError() const;

    // Raw DAT-index API kept for diagnostics.
    bool readHouseRecord(
        int datIndex,
        HouseDatRecord* output
    ) const;

    // Logical House API. Never assumes enum ordinal
    // equals the original houses.dat record index.
    bool getHouseDatIndex(
        HouseType type,
        int* output
    ) const;

    bool readHouseRecord(
        HouseType type,
        HouseDatRecord* output
    ) const;

    // Compatibility wrapper.
    bool readMillRecord(
        HouseDatRecord* output
    ) const;

    bool loadRxSprite(
        int spriteId,
        RxSprite* output
    ) const;

private:
    bool loadPalette();
    bool validateHousesDat();
    bool validateHousesRx();

    bool readFile(
        const std::string& path,
        std::vector<std::uint8_t>* output
    ) const;

    bool readRxSpriteIndexed(
        int spriteId,
        RxSprite* output
    ) const;

    void convertIndexedToRgba(
        RxSprite* sprite
    ) const;

    void setError(
        const std::string& message
    ) const;

private:
    std::string dataRoot_;
    std::string housesDatPath_;
    std::string housesRxPath_;
    std::string palettePath_;

    std::vector<ColorRGBA> palette_;

    bool ready_;

    mutable std::string lastError_;
};


} // namespace OriginalKaM
