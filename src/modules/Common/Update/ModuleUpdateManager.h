#pragma once

#include "Define.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace Trinity
{
    class TC_GAME_API ModuleUpdateManager
    {
    public:
        using UpdateCallback = std::function<void(uint32)>;

        static ModuleUpdateManager* instance();

        bool RegisterModule(std::string const& moduleName, UpdateCallback callback);
        void UnregisterModule(std::string const& moduleName);
        void Update(uint32 diff);
        bool IsModuleRegistered(std::string const& moduleName) const;
        uint32 GetRegisteredModuleCount() const;

    private:
        ModuleUpdateManager() = default;
        ~ModuleUpdateManager() = default;
        ModuleUpdateManager(ModuleUpdateManager const&) = delete;
        ModuleUpdateManager& operator=(ModuleUpdateManager const&) = delete;

        struct ModuleUpdateInfo
        {
            std::string moduleName;
            UpdateCallback callback;
            uint32 updateCount = 0;
            uint32 totalUpdateTime = 0;
        };

        mutable std::mutex _modulesMutex;
        std::unordered_map<std::string, ModuleUpdateInfo> _registeredModules;
    };
}

#define sModuleUpdateManager Trinity::ModuleUpdateManager::instance()