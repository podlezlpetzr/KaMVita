#pragma once

#include <cstdint>

static constexpr int MAX_SAVE_FILES = 64;
static constexpr int SAVE_NAME_LENGTH = 64;
static constexpr int SAVE_PATH_LENGTH = 256;

struct SaveFileEntry {
    char name[SAVE_NAME_LENGTH];
    char path[SAVE_PATH_LENGTH];
    std::uint64_t modifiedTime;
};


// ============================================================
// REMAKE-STYLE RUNTIME SAVE LIST
//
// The list is rebuilt from real .sav files in the Saves
// directory. selectedIndex refers to the currently selected
// file in that enumerated list.
//
// No numbered slot identity is stored here.
// ============================================================

struct SaveFileList {
    SaveFileEntry entries[MAX_SAVE_FILES];
    int count;
    int selectedIndex;
};

void initSaveFileList(
    SaveFileList* list
);

bool refreshSaveFileList(
    SaveFileList* list
);

const SaveFileEntry* getSelectedSaveFile(
    const SaveFileList* list
);

bool selectSaveFile(
    SaveFileList* list,
    int index
);

bool ensureSaveDirectory();

bool buildNamedSavePath(
    const char* saveName,
    char* outputPath,
    int outputPathSize
);

int enumerateSaveFiles(
    SaveFileEntry* entries,
    int maximumEntries
);
