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

#ifndef LocalFileDataStore_h__
#define LocalFileDataStore_h__

#include "Define.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

// Stores mappings between FileDataId and file path of custom (non-CASC) files.
// Used by the map_extractor and vmap4_extractor to read custom maps placed in a
// local directory (the "Custom" subfolder of the input path) instead of CASC.
//
// This is only active when the "-custom" command line option is passed to the tools.
class LocalFileDataStore
{
    private:
        std::unordered_map<uint32, std::string> m_fileDataToName;
        std::unordered_set<std::string> m_validFiles;
        std::string m_customPath;

    public:
        static LocalFileDataStore* getInstance();

        // Returns true if custom (local) file reading mode is enabled.
        bool IsCustomMode() const { return !m_customPath.empty(); }

        // Sets the root directory where custom files are located (typically the "Custom" subfolder of the input path).
        void SetCustomPath(std::string const& path) { m_customPath = path; }
        std::string const& GetCustomPath() const { return m_customPath; }

        // Loads "listfile.csv" from the custom path and fills the FileDataId -> name maps.
        // Format of each line: <FileDataId>;<path/to/file.ext>
        //   m_fileDataToName maps FileDataId -> full relative path (e.g. "world/maps/mymap/mymap.wdt")
        //   m_validFiles    holds the plain file names (basename) for vmap4_extractor lookups.
        void LoadFileDataIDsToLocalStorage();

        std::unordered_map<uint32, std::string> const& GetFileDataToName() const { return m_fileDataToName; }
        std::unordered_set<std::string> const& GetValidFiles() const { return m_validFiles; }
};

#define sLocalFileDataStore LocalFileDataStore::getInstance()

#endif // LocalFileDataStore_h__
