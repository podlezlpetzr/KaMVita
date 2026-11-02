#include "save_slots.h"

#include <cctype>
#include <cstdio>
#include <cstring>

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>


// ============================================================
// REMAKE-STYLE NAMED SAVE DIRECTORY
//
// KaM Remake works with named save files and scans a Saves
// directory for .sav files.
//
// KaMVita equivalent:
//
// ux0:data/kamvita/Saves/<name>.sav
//
// Save v12 payload remains owned by saveGame/loadGame.
// This module only owns filenames and enumeration.
// ============================================================

static constexpr const char* SAVE_ROOT_DIRECTORY =
    "ux0:data/kamvita";

static constexpr const char* SAVE_DIRECTORY =
    "ux0:data/kamvita/Saves";

static constexpr const char* SAVE_EXTENSION =
    ".sav";


static bool hasSavExtension(
    const char* fileName
)
{
    if (!fileName)
        return false;

    const std::size_t length =
        std::strlen(fileName);

    constexpr std::size_t extensionLength = 4;

    if (length <= extensionLength)
        return false;

    const char* extension =
        fileName +
        length -
        extensionLength;

    return
        extension[0] == '.' &&
        std::tolower(
            static_cast<unsigned char>(
                extension[1]
            )
        ) == 's' &&
        std::tolower(
            static_cast<unsigned char>(
                extension[2]
            )
        ) == 'a' &&
        std::tolower(
            static_cast<unsigned char>(
                extension[3]
            )
        ) == 'v';
}


static bool isAllowedSaveNameCharacter(
    char value
)
{
    const unsigned char character =
        static_cast<unsigned char>(
            value
        );

    return
        std::isalnum(character) ||
        value == ' ' ||
        value == '-' ||
        value == '_';
}


static void copySaveDisplayName(
    char* destination,
    int destinationSize,
    const char* fileName
)
{
    if (
        !destination ||
        destinationSize <= 0
    ) {
        return;
    }

    destination[0] = '\0';

    if (!fileName)
        return;

    std::size_t length =
        std::strlen(fileName);

    if (
        length >= 4 &&
        hasSavExtension(fileName)
    ) {
        length -= 4;
    }

    const std::size_t maximumLength =
        static_cast<std::size_t>(
            destinationSize - 1
        );

    if (length > maximumLength)
        length = maximumLength;

    std::memcpy(
        destination,
        fileName,
        length
    );

    destination[length] = '\0';
}


bool ensureSaveDirectory()
{
    sceIoMkdir(
        SAVE_ROOT_DIRECTORY,
        0777
    );

    const int result =
        sceIoMkdir(
            SAVE_DIRECTORY,
            0777
        );

    if (result >= 0)
        return true;

    SceIoStat status = {};

    return
        sceIoGetstat(
            SAVE_DIRECTORY,
            &status
        ) >= 0;
}


bool buildNamedSavePath(
    const char* saveName,
    char* outputPath,
    int outputPathSize
)
{
    if (
        !saveName ||
        !outputPath ||
        outputPathSize <= 0
    ) {
        return false;
    }

    outputPath[0] = '\0';

    if (saveName[0] == '\0')
        return false;

    char safeName[
        SAVE_NAME_LENGTH
    ] = {};

    int safeLength = 0;

    for (
        int index = 0;
        saveName[index] != '\0';
        ++index
    ) {
        if (
            safeLength >=
            SAVE_NAME_LENGTH - 1
        ) {
            break;
        }

        const char character =
            saveName[index];

        if (
            !isAllowedSaveNameCharacter(
                character
            )
        ) {
            continue;
        }

        // Prevent leading spaces in generated filenames.
        if (
            safeLength == 0 &&
            character == ' '
        ) {
            continue;
        }

        safeName[safeLength++] =
            character;
    }

    while (
        safeLength > 0 &&
        safeName[safeLength - 1] == ' '
    ) {
        --safeLength;
    }

    safeName[safeLength] = '\0';

    if (safeLength == 0)
        return false;

    if (!ensureSaveDirectory())
        return false;

    const int written =
        std::snprintf(
            outputPath,
            static_cast<std::size_t>(
                outputPathSize
            ),
            "%s/%s%s",
            SAVE_DIRECTORY,
            safeName,
            SAVE_EXTENSION
        );

    return
        written > 0 &&
        written < outputPathSize;
}


int enumerateSaveFiles(
    SaveFileEntry* entries,
    int maximumEntries
)
{
    if (
        !entries ||
        maximumEntries <= 0
    ) {
        return 0;
    }

    if (!ensureSaveDirectory())
        return 0;

    const int directory =
        sceIoDopen(
            SAVE_DIRECTORY
        );

    if (directory < 0)
        return 0;

    int entryCount = 0;

    while (
        entryCount < maximumEntries
    ) {
        SceIoDirent directoryEntry = {};

        const int readResult =
            sceIoDread(
                directory,
                &directoryEntry
            );

        if (readResult <= 0)
            break;

        const char* fileName =
            directoryEntry.d_name;

        if (
            !fileName ||
            fileName[0] == '\0' ||
            !hasSavExtension(fileName)
        ) {
            continue;
        }

        SaveFileEntry& entry =
            entries[entryCount];

        entry = {};

        copySaveDisplayName(
            entry.name,
            SAVE_NAME_LENGTH,
            fileName
        );

        const int written =
            std::snprintf(
                entry.path,
                SAVE_PATH_LENGTH,
                "%s/%s",
                SAVE_DIRECTORY,
                fileName
            );

        if (
            written <= 0 ||
            written >= SAVE_PATH_LENGTH
        ) {
            entry = {};
            continue;
        }

        entry.modifiedTime = 0;

        ++entryCount;
    }

    sceIoDclose(
        directory
    );

    return entryCount;
}


// ============================================================
// REMAKE-STYLE RUNTIME SAVE LIST
// ============================================================

void initSaveFileList(
    SaveFileList* list
)
{
    if (!list)
        return;

    *list = {};

    list->count = 0;
    list->selectedIndex = -1;
}


bool refreshSaveFileList(
    SaveFileList* list
)
{
    if (!list)
        return false;

    char previouslySelectedPath[
        SAVE_PATH_LENGTH
    ] = {};

    if (
        list->selectedIndex >= 0 &&
        list->selectedIndex < list->count
    ) {
        std::snprintf(
            previouslySelectedPath,
            SAVE_PATH_LENGTH,
            "%s",
            list->entries[
                list->selectedIndex
            ].path
        );
    }

    SaveFileEntry refreshedEntries[
        MAX_SAVE_FILES
    ] = {};

    const int refreshedCount =
        enumerateSaveFiles(
            refreshedEntries,
            MAX_SAVE_FILES
        );

    if (
        refreshedCount < 0 ||
        refreshedCount > MAX_SAVE_FILES
    ) {
        return false;
    }

    for (
        int index = 0;
        index < MAX_SAVE_FILES;
        ++index
    ) {
        list->entries[index] = {};
    }

    for (
        int index = 0;
        index < refreshedCount;
        ++index
    ) {
        list->entries[index] =
            refreshedEntries[index];
    }

    list->count = refreshedCount;
    list->selectedIndex = -1;

    // Preserve selection by real save path when possible.
    if (
        previouslySelectedPath[0] != '\0'
    ) {
        for (
            int index = 0;
            index < list->count;
            ++index
        ) {
            if (
                std::strcmp(
                    list->entries[index].path,
                    previouslySelectedPath
                ) == 0
            ) {
                list->selectedIndex = index;
                break;
            }
        }
    }

    // If no previous selection survives, select the first
    // available named save. Empty directory stays selected=-1.
    if (
        list->selectedIndex < 0 &&
        list->count > 0
    ) {
        list->selectedIndex = 0;
    }

    return true;
}


const SaveFileEntry* getSelectedSaveFile(
    const SaveFileList* list
)
{
    if (
        !list ||
        list->selectedIndex < 0 ||
        list->selectedIndex >= list->count
    ) {
        return nullptr;
    }

    return &list->entries[
        list->selectedIndex
    ];
}


bool selectSaveFile(
    SaveFileList* list,
    int index
)
{
    if (
        !list ||
        index < 0 ||
        index >= list->count
    ) {
        return false;
    }

    list->selectedIndex = index;

    return true;
}
