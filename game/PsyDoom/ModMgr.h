#pragma once

#include "Doom/cdmaptbl.h"
#include "Macros.h"
#include "Wess/psxcd.h"

#include <functional>

class WadList;

BEGIN_NAMESPACE(ModMgr)

// A callback invoked for every file added by the modding mechanism that is an Ogg Vorbis music track.
// Receives the track number for the track, and the file id for the track.
typedef std::function<void (const int32_t trackNum, const CdFileId& discFile)> EnumOggVorbisMusicCallback;

void init() noexcept;
void shutdown() noexcept;
void addUserWads(WadList& wadList) noexcept;

// File overrides mechanism
bool areOverridesAvailableForFile(const CdFileId discFile) noexcept;
bool isFileOverriden(const PsxCd_File& file) noexcept;
bool openOverridenFile(const CdFileId discFile, PsxCd_File& fileOut) noexcept;
void closeOverridenFile(PsxCd_File& file) noexcept;
int32_t readFromOverridenFile(void* const pDest, int32_t numBytes, PsxCd_File& file) noexcept;
int32_t seekForOverridenFile(PsxCd_File& file, int32_t offset, const PsxCd_SeekMode mode) noexcept;
int32_t tellForOverridenFile(const PsxCd_File& file) noexcept;
int32_t getOverridenFileSize(const CdFileId discFile) noexcept;
void enumOggVorbisMusicFiles(const EnumOggVorbisMusicCallback& callback) noexcept;

END_NAMESPACE(ModMgr)
