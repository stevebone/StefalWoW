/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "cascfile.h"
#include "LocalFileDataStore.h"
#include "StringFormat.h"
#include <CascLib.h>
#include <cstdio>
#include <fstream>

CASCFile::CASCFile(std::shared_ptr<CASC::Storage const> casc, const char* filename, bool warnNoExist /*= true*/) :
    eof(false),
    buffer(nullptr),
    pointer(0),
    size(0)
{
    // In custom mode, if the file name is known in the custom listfile,
    // try to read it from the local Custom\ folder first (relative path matches the listfile).
    if (sLocalFileDataStore->IsCustomMode())
    {
        auto const& validFiles = sLocalFileDataStore->GetValidFiles();
        std::string filenameStr(filename);
        std::size_t basePos = filenameStr.find_last_of("\\/");
        std::string baseName = basePos != std::string::npos ? filenameStr.substr(basePos + 1) : filenameStr;
        if (validFiles.find(baseName) != validFiles.end())
        {
            std::string localPath = Trinity::StringFormat("{}\\{}", sLocalFileDataStore->GetCustomPath(), filename);
            std::ifstream file(localPath, std::ios::binary | std::ios::ate);
            if (file)
            {
                init(file, filename);
                return;
            }
        }
    }

    std::unique_ptr<CASC::File> file(casc->OpenFile(filename, CASC_LOCALE_ALL_WOW, false));
    if (!file)
    {
        if (warnNoExist || GetCascError() != ERROR_FILE_NOT_FOUND)
            fprintf(stderr, "Can't open %s: %s\n", filename, CASC::HumanReadableCASCError(GetCascError()));
        eof = true;
        return;
    }

    init(file.get(), filename);
}

CASCFile::CASCFile(std::shared_ptr<CASC::Storage const> casc, uint32 fileDataId, std::string const& description, bool warnNoExist /*= true*/) :
    eof(false),
    buffer(nullptr),
    pointer(0),
    size(0)
{
    // In custom mode, if the fileDataId is mapped in the custom listfile,
    // try to read the corresponding file from the local Custom\ folder first.
    if (sLocalFileDataStore->IsCustomMode())
    {
        auto const& fileDataToName = sLocalFileDataStore->GetFileDataToName();
        auto it = fileDataToName.find(fileDataId);
        if (it != fileDataToName.end())
        {
            std::string localPath = Trinity::StringFormat("{}\\{}", sLocalFileDataStore->GetCustomPath(), it->second);
            std::ifstream file(localPath, std::ios::binary | std::ios::ate);
            if (file)
            {
                init(file, description.c_str());
                return;
            }
        }
    }

    std::unique_ptr<CASC::File> file(casc->OpenFile(fileDataId, CASC_LOCALE_ALL_WOW, false));
    if (!file)
    {
        if (warnNoExist || GetCascError() != ERROR_FILE_NOT_FOUND)
            fprintf(stderr, "Can't open %s: %s\n", description.c_str(), CASC::HumanReadableCASCError(GetCascError()));
        eof = true;
        return;
    }

    init(file.get(), description.c_str());
}

void CASCFile::init(CASC::File* file, const char* description)
{
    int64 fileSize = file->GetSize();
    if (fileSize == -1)
    {
        fprintf(stderr, "Can't open %s, failed to get size: %s!\n", description, CASC::HumanReadableCASCError(GetCascError()));
        eof = true;
        return;
    }

    size = fileSize;

    uint32 read = 0;
    buffer = new char[size];
    if (!file->ReadFile(buffer, size, &read) || size != read)
    {
        fprintf(stderr, "Can't read %s, size=%u read=%u: %s\n", description, uint32(size), uint32(read), CASC::HumanReadableCASCError(GetCascError()));
        eof = true;
        return;
    }
}

void CASCFile::init(std::ifstream& file, const char* description)
{
    int64 fileSize = file.tellg();
    if (fileSize <= 0)
    {
        fprintf(stderr, "Can't open %s, failed to get size!\n", description);
        eof = true;
        return;
    }

    size = fileSize;
    buffer = new char[size];
    file.seekg(0, std::ios::beg);
    if (!file.read(buffer, size))
    {
        fprintf(stderr, "Can't read %s, size=%u!\n", description, uint32(size));
        delete[] buffer;
        buffer = nullptr;
        size = 0;
        eof = true;
        return;
    }
    file.close();
}

size_t CASCFile::read(void* dest, size_t bytes)
{
    if (eof)
        return 0;

    size_t rpos = pointer + bytes;
    if (rpos > size)
    {
        bytes = size - pointer;
        eof = true;
    }

    memcpy(dest, &(buffer[pointer]), bytes);

    pointer = rpos;

    return bytes;
}

void CASCFile::seek(int offset)
{
    pointer = offset;
    eof = (pointer >= size);
}

void CASCFile::seekRelative(int offset)
{
    pointer += offset;
    eof = (pointer >= size);
}

void CASCFile::close()
{
    delete[] buffer;
    buffer = nullptr;
    eof = true;
}
