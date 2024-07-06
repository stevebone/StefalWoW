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

#include "TileAssembler.h"
#include "BoundingIntervalHierarchy.h"
#include "MapTree.h"
#include "Memory.h"
#include "StringFormat.h"
#include "VMapDefinitions.h"
#include <boost/filesystem/operations.hpp>
#include <set>

template<> struct BoundsTrait<VMAP::ModelSpawn*>
{
    static void getBounds(VMAP::ModelSpawn const* const& obj, G3D::AABox& out) { out = obj->getBounds(); }
};

namespace VMAP
{
    G3D::Vector3 ModelPosition::transform(G3D::Vector3 const& pIn) const
    {
        G3D::Vector3 out = pIn * iScale;
        out = iRotation * out;
        return out;
    }

    //=================================================================

    TileAssembler::TileAssembler(std::string pSrcDirName, std::string pDestDirName)
        : iDestDir(std::move(pDestDirName)), iSrcDir(std::move(pSrcDirName))
    {
        boost::filesystem::create_directories(iDestDir);
    }

    bool TileAssembler::convertWorld2()
    {
        bool success = readMapSpawns();
        if (!success)
            return false;

        float constexpr invTileSize = 1.0f / 533.33333f;

        // export Map data
        while (!mapData.empty())
        {
            MapSpawns data = std::move(mapData.front());
            mapData.pop_front();

            // build global map tree
            std::vector<ModelSpawn*> mapSpawns;
            mapSpawns.reserve(data.UniqueEntries.size());
            printf("Calculating model bounds for map %u...\n", data.MapId);
            for (auto entry = data.UniqueEntries.begin(); entry != data.UniqueEntries.end(); ++entry)
            {
                // M2 models don't have a bound set in WDT/ADT placement data, they're not used for LoS but are needed for pathfinding
                if (!(entry->second.flags & MOD_HAS_BOUND))
                    if (!calculateTransformedBound(entry->second))
                        continue;

                mapSpawns.push_back(&entry->second);
                spawnedModelFiles.insert(entry->second.name);

                std::map<uint32, std::set<uint32>>& tileEntries = (entry->second.flags & MOD_PARENT_SPAWN) ? data.ParentTileEntries : data.TileEntries;

                G3D::AABox const& bounds = entry->second.iBound;
                G3D::Vector2int16 low(int16(bounds.low().x * invTileSize), int16(bounds.low().y * invTileSize));
                G3D::Vector2int16 high(int16(bounds.high().x * invTileSize), int16(bounds.high().y * invTileSize));
                for (int x = low.x; x <= high.x; ++x)
                    for (int y = low.y; y <= high.y; ++y)
                        tileEntries[StaticMapTree::packTileID(x, y)].insert(entry->second.ID);
            }

            printf("Creating map tree for map %u...\n", data.MapId);
            BIH pTree;

            try
            {
                pTree.build(mapSpawns, BoundsTrait<ModelSpawn*>::getBounds);
            }
            catch (std::exception& e)
            {
                printf(R"(Exception "%s" when calling pTree.build)", e.what());
                return false;
            }

            // write map tree file
            std::string mapfilename = Trinity::StringFormat("{}/{:04}.vmtree", iDestDir, data.MapId);
            auto mapfile = Trinity::make_unique_ptr_with_deleter(fopen(mapfilename.c_str(), "wb"), &::fclose);
            if (!mapfile)
            {
                success = false;
                printf("Cannot open %s\n", mapfilename.c_str());
                break;
            }

            //general info
            if (success && fwrite(VMAP_MAGIC, 1, 8, mapfile.get()) != 8) success = false;
            // Nodes
            if (success && fwrite("NODE", 4, 1, mapfile.get()) != 1) success = false;
            if (success) success = pTree.writeToFile(mapfile.get());

            // spawn id to index map
            uint32 mapSpawnsSize = mapSpawns.size();
            if (success && fwrite("SIDX", 4, 1, mapfile.get()) != 1) success = false;
            if (success && fwrite(&mapSpawnsSize, sizeof(uint32), 1, mapfile.get()) != 1) success = false;
            for (uint32 i = 0; i < mapSpawnsSize; ++i)
            {
                if (success && fwrite(&mapSpawns[i]->ID, sizeof(uint32), 1, mapfile.get()) != 1) success = false;
            }

            mapfile = nullptr;

            // <====

            // write map tile files, similar to ADT files, only with extra BIH tree node info
            for (auto const& [tileId, spawns] : data.TileEntries)
            {
                uint32 x, y;
                StaticMapTree::unpackTileID(tileId, x, y);
                std::string tileFileName = Trinity::StringFormat("{}/{:04}_{:02}_{:02}.vmtile", iDestDir, data.MapId, y, x);
                auto tileFile = Trinity::make_unique_ptr_with_deleter(fopen(tileFileName.c_str(), "wb"), &::fclose);
                if (tileFile)
                {
                    std::set<uint32> const& parentTileEntries = data.ParentTileEntries[tileId];

                    uint32 nSpawns = spawns.size() + parentTileEntries.size();

                    // file header
                    if (success && fwrite(VMAP_MAGIC, 1, 8, tileFile.get()) != 8) success = false;
                    // write number of tile spawns
                    if (success && fwrite(&nSpawns, sizeof(uint32), 1, tileFile.get()) != 1) success = false;
                    // write tile spawns
                    for (auto spawnItr = spawns.begin(); spawnItr != spawns.end() && success; ++spawnItr)
                        success = ModelSpawn::writeToFile(tileFile.get(), data.UniqueEntries[*spawnItr]);

                    for (auto spawnItr = parentTileEntries.begin(); spawnItr != parentTileEntries.end() && success; ++spawnItr)
                        success = ModelSpawn::writeToFile(tileFile.get(), data.UniqueEntries[*spawnItr]);
                }
            }
        }

        // add an object models, listed in temp_gameobject_models file
        exportGameobjectModels();
        // export objects
        printf("\nConverting Model Files\n");
        for (std::string const& spawnedModelFile : spawnedModelFiles)
        {
            printf("Converting %s\n", spawnedModelFile.c_str());
            if (!convertRawFile(spawnedModelFile))
            {
                printf("error converting %s\n", spawnedModelFile.c_str());
                success = false;
                break;
            }
        }

        return success;
    }

    bool TileAssembler::readMapSpawns()
    {
        std::string fname = iSrcDir + "/dir_bin";
        auto dirf = Trinity::make_unique_ptr_with_deleter(fopen(fname.c_str(), "rb"), &::fclose);
        if (!dirf)
        {
            printf("Could not read dir_bin file!\n");
            return false;
        }
        printf("Read coordinate mapping...\n");
        uint32 mapID, check;
        std::map<uint32, MapSpawns> data;
        while (!feof(dirf.get()))
        {
            // read mapID, Flags, NameSet, UniqueId, Pos, Rot, Scale, Bound_lo, Bound_hi, name
            check = fread(&mapID, sizeof(uint32), 1, dirf.get());
            if (check == 0) // EoF...
                break;

            ModelSpawn spawn;
            if (!ModelSpawn::readFromFile(dirf.get(), spawn))
                break;

            auto [itr, isNew] = data.try_emplace(mapID);
            if (isNew)
            {
                itr->second.MapId = mapID;
                printf("spawning Map %u\n", mapID);
            }

            itr->second.UniqueEntries.emplace(spawn.ID, spawn);
        }

        mapData.resize(data.size());
        auto dst = mapData.begin();
        for (auto src = data.begin(); src != data.end(); ++src, ++dst)
            *dst = std::move(src->second);

        bool success = (ferror(dirf.get()) == 0);
        return success;
    }

    bool TileAssembler::calculateTransformedBound(ModelSpawn &spawn)
    {
        std::string modelFilename(iSrcDir);
        modelFilename.push_back('/');
        modelFilename.append(spawn.name);

        ModelPosition modelPosition;
        modelPosition.iDir = spawn.iRot;
        modelPosition.iScale = spawn.iScale;
        modelPosition.init();

        WorldModel_Raw raw_model;
        if (!raw_model.Read(modelFilename.c_str()))
            return false;

        uint32 groups = raw_model.groupsArray.size();
        if (groups != 1)
            printf("Warning: '%s' does not seem to be a M2 model!\n", modelFilename.c_str());

        G3D::AABox rotated_bounds;
        for (int i = 0; i < 8; ++i)
            rotated_bounds.merge(modelPosition.transform(raw_model.groupsArray[0].bounds.corner(i)));

        spawn.iBound = rotated_bounds + spawn.iPos;
        spawn.flags |= MOD_HAS_BOUND;
        return true;
    }

#pragma pack(push, 1)
    struct WMOLiquidHeader
    {
        int xverts, yverts, xtiles, ytiles;
        float pos_x;
        float pos_y;
        float pos_z;
        short material;
    };
#pragma pack(pop)
    //=================================================================
    bool TileAssembler::convertRawFile(const std::string& pModelFilename)
    {
        bool success = true;
        std::string filename = iSrcDir;
        if (filename.length() >0)
            filename.push_back('/');
        filename.append(pModelFilename);

        WorldModel_Raw raw_model;
        if (!raw_model.Read(filename.c_str()))
            return false;

        // write WorldModel
        WorldModel model;
        model.setFlags(raw_model.Flags);
        model.setRootWmoID(raw_model.RootWMOID);
        if (!raw_model.groupsArray.empty())
        {
            std::vector<GroupModel> groupsArray;

            uint32 groups = raw_model.groupsArray.size();
            for (uint32 g = 0; g < groups; ++g)
            {
                GroupModel_Raw& raw_group = raw_model.groupsArray[g];
                groupsArray.push_back(GroupModel(raw_group.mogpflags, raw_group.GroupWMOID, raw_group.bounds));
                groupsArray.back().setMeshData(std::move(raw_group.vertexArray), std::move(raw_group.triangles));
                groupsArray.back().setLiquidData(raw_group.liquid.release());
            }

            model.setGroupModels(groupsArray);
        }

        success = model.writeFile(iDestDir + "/" + pModelFilename + ".vmo");
        //std::cout << "readRawFile2: '" << pModelFilename << "' tris: " << nElements << " nodes: " << nNodes << std::endl;
        return success;
    }

    void TileAssembler::exportGameobjectModels()
    {
        auto model_list = Trinity::make_unique_ptr_with_deleter(fopen((iSrcDir + "/" + "temp_gameobject_models").c_str(), "rb"), &::fclose);
        if (!model_list)
            return;

        char ident[8];
        if (fread(ident, 1, 8, model_list.get()) != 8 || memcmp(ident, VMAP::RAW_VMAP_MAGIC, 8) != 0)
            return;

        auto model_list_copy = Trinity::make_unique_ptr_with_deleter(fopen((iDestDir + "/" + GAMEOBJECT_MODELS).c_str(), "wb"), &::fclose);
        if (!model_list_copy)
            return;

        fwrite(VMAP::VMAP_MAGIC, 1, 8, model_list_copy.get());

        uint32 name_length, displayId;
        char buff[500];
        while (true)
        {
            if (fread(&displayId, sizeof(uint32), 1, model_list.get()) != 1)
                if (feof(model_list.get()))   // EOF flag is only set after failed reading attempt
                    break;

            if (fread(&name_length, sizeof(uint32), 1, model_list.get()) != 1
                || name_length >= sizeof(buff)
                || fread(&buff, sizeof(char), name_length, model_list.get()) != name_length)
            {
                printf("\nFile 'temp_gameobject_models' seems to be corrupted\n");
                break;
            }

            std::string model_name(buff, name_length);

            WorldModel_Raw raw_model;
            if (!raw_model.Read((iSrcDir + "/" + model_name).c_str()) )
                continue;

            spawnedModelFiles.insert(model_name);
            G3D::AABox bounds;
            for (GroupModel_Raw const& groupModel : raw_model.groupsArray)
                for (G3D::Vector3 const& vertice : groupModel.vertexArray)
                    bounds.merge(vertice);

            if (bounds.isEmpty())
            {
                printf("\nModel %s has empty bounding box\n", model_name.c_str());
                continue;
            }

            if (!bounds.isFinite())
            {
                printf("\nModel %s has invalid bounding box\n", model_name.c_str());
                continue;
            }

            fwrite(&displayId, sizeof(uint32), 1, model_list_copy.get());
            fwrite(&name_length, sizeof(uint32), 1, model_list_copy.get());
            fwrite(&buff, sizeof(char), name_length, model_list_copy.get());
            fwrite(&bounds.low(), sizeof(G3D::Vector3), 1, model_list_copy.get());
            fwrite(&bounds.high(), sizeof(G3D::Vector3), 1, model_list_copy.get());
        }
    }

// temporary use defines to simplify read/check code (close file and return at fail)
#define READ_OR_RETURN(V, S) if (fread((V), (S), 1, rf) != 1) do { \
                                printf("%s readfail, op = %s\n", __FUNCTION__, #V); return false; } while(false)
#define CMP_OR_RETURN(V, S)  if (strcmp((V), (S)) != 0)       do { \
                                printf("%s cmpfail, %s!=%s\n", __FUNCTION__, V, S);return false; } while(false)

    bool GroupModel_Raw::Read(FILE* rf)
    {
        char blockId[5];
        blockId[4] = 0;
        int blocksize;

        READ_OR_RETURN(&mogpflags, sizeof(uint32));
        READ_OR_RETURN(&GroupWMOID, sizeof(uint32));

        G3D::Vector3 vec1, vec2;
        READ_OR_RETURN(&vec1, sizeof(G3D::Vector3));

        READ_OR_RETURN(&vec2, sizeof(G3D::Vector3));
        bounds.set(vec1, vec2);

        READ_OR_RETURN(&liquidflags, sizeof(uint32));

        // will this ever be used? what is it good for anyway??
        uint32 branches;
        READ_OR_RETURN(&blockId, 4);
        CMP_OR_RETURN(blockId, "GRP ");
        READ_OR_RETURN(&blocksize, sizeof(int));
        READ_OR_RETURN(&branches, sizeof(uint32));
        for (uint32 b = 0; b < branches; ++b)
        {
            uint32 indexes;
            // indexes for each branch (not used jet)
            READ_OR_RETURN(&indexes, sizeof(uint32));
        }

        // ---- indexes
        READ_OR_RETURN(&blockId, 4);
        CMP_OR_RETURN(blockId, "INDX");
        READ_OR_RETURN(&blocksize, sizeof(int));
        uint32 nindexes;
        READ_OR_RETURN(&nindexes, sizeof(uint32));
        if (nindexes > 0)
        {
            std::unique_ptr<uint32[]> indexarray = std::make_unique<uint32[]>(nindexes);
            READ_OR_RETURN(indexarray.get(), nindexes * sizeof(uint32));
            triangles.reserve(nindexes / 3);
            for (uint32 i = 0; i < nindexes; i += 3)
                triangles.push_back({ .idx0 = indexarray[i], .idx1 = indexarray[i + 1], .idx2 = indexarray[i + 2] });
        }

        // ---- vectors
        READ_OR_RETURN(&blockId, 4);
        CMP_OR_RETURN(blockId, "VERT");
        READ_OR_RETURN(&blocksize, sizeof(int));
        uint32 nvectors;
        READ_OR_RETURN(&nvectors, sizeof(uint32));

        if (nvectors > 0)
        {
            std::unique_ptr<float[]> vectorarray = std::make_unique<float[]>(nvectors * 3);
            READ_OR_RETURN(vectorarray.get(), nvectors * sizeof(float) * 3);
            for (uint32 i = 0; i < nvectors; ++i)
                vertexArray.push_back(G3D::Vector3(&vectorarray[3 * i]));
        }
        // ----- liquid
        liquid = nullptr;
        if (liquidflags & 3)
        {
            READ_OR_RETURN(&blockId, 4);
            CMP_OR_RETURN(blockId, "LIQU");
            READ_OR_RETURN(&blocksize, sizeof(int));
            uint32 liquidType;
            READ_OR_RETURN(&liquidType, sizeof(uint32));
            if (liquidflags & 1)
            {
                WMOLiquidHeader hlq;
                READ_OR_RETURN(&hlq, sizeof(WMOLiquidHeader));
                liquid.reset(new WmoLiquid(hlq.xtiles, hlq.ytiles, G3D::Vector3(hlq.pos_x, hlq.pos_y, hlq.pos_z), liquidType));
                uint32 size = hlq.xverts * hlq.yverts;
                READ_OR_RETURN(liquid->GetHeightStorage(), size * sizeof(float));
                size = hlq.xtiles * hlq.ytiles;
                READ_OR_RETURN(liquid->GetFlagsStorage(), size);
            }
            else
            {
                liquid.reset(new WmoLiquid(0, 0, G3D::Vector3::zero(), liquidType));
                liquid->GetHeightStorage()[0] = bounds.high().z;
            }
        }

        return true;
    }

    bool WorldModel_Raw::Read(const char * path)
    {
        auto file = Trinity::make_unique_ptr_with_deleter(fopen(path, "rb"), &::fclose);
        if (!file)
        {
            printf("ERROR: Can't open raw model file: %s\n", path);
            return false;
        }

        FILE* rf = file.get();

        char ident[9];
        ident[8] = '\0';

        READ_OR_RETURN(&ident, 8);
        CMP_OR_RETURN(ident, RAW_VMAP_MAGIC);

        // we have to read one int. This is needed during the export and we have to skip it here
        uint32 tempNVectors;
        READ_OR_RETURN(&tempNVectors, sizeof(tempNVectors));

        uint32 groups;
        READ_OR_RETURN(&groups, sizeof(uint32));
        READ_OR_RETURN(&RootWMOID, sizeof(uint32));
        READ_OR_RETURN(&Flags, sizeof(Flags));

        groupsArray.resize(groups);
        bool succeed = true;
        for (uint32 g = 0; g < groups && succeed; ++g)
            succeed = groupsArray[g].Read(rf);

        return succeed;
    }

    // drop of temporary use defines
    #undef READ_OR_RETURN
    #undef READ_OR_RETURN_WITH_DELETE
    #undef CMP_OR_RETURN
}
