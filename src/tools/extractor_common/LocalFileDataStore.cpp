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

#include "LocalFileDataStore.h"
#include "StringFormat.h"
#include <fstream>

LocalFileDataStore* LocalFileDataStore::getInstance()
{
    static LocalFileDataStore instance;

    return &instance;
}

void LocalFileDataStore::LoadFileDataIDsToLocalStorage()
{
    // Keep maps empty in non-custom mode so lookups always miss and fall back to CASC.
    if (!IsCustomMode())
        return;

    std::string listfilePath = Trinity::StringFormat("{}\\listfile.csv", m_customPath);
    std::ifstream fileDataMap(listfilePath);
    if (!fileDataMap)
    {
        printf("WARNING (-custom): Could not open custom listfile '%s', all files will be read from CASC.\n", listfilePath.c_str());
        return;
    }

    std::string line;
    while (std::getline(fileDataMap, line))
    {
        if (line.empty())
            continue;

        const auto pos = line.find(';');
        if (pos == std::string::npos)
            continue;

        uint32 fileData;
        try
        {
            fileData = uint32(std::stoul(line.substr(0, pos)));
        }
        catch (...)
        {
            // Skip malformed lines (non-numeric FileDataId).
            continue;
        }

        const std::string filePath = line.substr(pos + 1);
        if (filePath.empty())
            continue;

        std::string fileName = filePath;
        if (auto slashPos = filePath.rfind("/"); slashPos != std::string::npos)
            fileName = filePath.substr(slashPos + 1);
        // Store the full relative path so the map extractor can locate the
        // file under <customPath>\<relativePath>.
        m_fileDataToName.emplace(fileData, filePath);
        // vmap4_extractor matches by plain file name (basename).
        m_validFiles.insert(fileName);
    }

    printf("Custom listfile '%s' loaded (" SZFMTD " entries).\n", listfilePath.c_str(), m_fileDataToName.size());
}
