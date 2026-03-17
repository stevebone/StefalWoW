#include "ModuleUpdateManager.h"
#include "Log.h"
#include "Timer.h"
#include <chrono>

namespace Trinity
{
    ModuleUpdateManager* ModuleUpdateManager::instance()
    {
        static ModuleUpdateManager instance;
        return &instance;
    }

    bool ModuleUpdateManager::RegisterModule(std::string const& moduleName, UpdateCallback callback)
    {
        if (moduleName.empty() || !callback)
        {
            TC_LOG_ERROR("server.loading", "ModuleUpdateManager: Cannot register module with empty name or invalid callback");
            return false;
        }

        std::lock_guard<std::mutex> lock(_modulesMutex);

        if (_registeredModules.find(moduleName) != _registeredModules.end())
        {
            TC_LOG_WARN("server.loading", "ModuleUpdateManager: Module '{}' is already registered for updates", moduleName);
            return false;
        }

        ModuleUpdateInfo info;
        info.moduleName = moduleName;
        info.callback = std::move(callback);
        info.updateCount = 0;
        info.totalUpdateTime = 0;

        _registeredModules[moduleName] = std::move(info);

        TC_LOG_INFO("server.loading", "ModuleUpdateManager: Successfully registered module '{}' for world updates", moduleName);
        return true;
    }

    void ModuleUpdateManager::UnregisterModule(std::string const& moduleName)
    {
        std::lock_guard<std::mutex> lock(_modulesMutex);

        auto it = _registeredModules.find(moduleName);
        if (it != _registeredModules.end())
        {
            TC_LOG_INFO("server.loading", "ModuleUpdateManager: Unregistered module '{}' (processed {} updates, avg time: {:.3f}ms)",
                       moduleName, it->second.updateCount,
                       it->second.updateCount > 0 ? (it->second.totalUpdateTime / static_cast<double>(it->second.updateCount) / 1000.0) : 0.0);
            _registeredModules.erase(it);
        }
    }

    void ModuleUpdateManager::Update(uint32 diff)
    {

        std::lock_guard<std::mutex> lock(_modulesMutex);


        for (auto& [moduleName, info] : _registeredModules)
        {

            try
            {
                auto startTime = std::chrono::high_resolution_clock::now();

                info.callback(diff);

                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

                info.updateCount++;
                info.totalUpdateTime += static_cast<uint32>(duration.count());

                // Log performance warning if update takes too long
                if (duration.count() > 50000) // 50ms threshold
                {
                    TC_LOG_WARN("server.loading", "ModuleUpdateManager: Module '{}' update took {}ms (threshold: 50ms)",
                               moduleName, duration.count() / 1000.0f);
                }
            }
            catch (std::exception const& ex)
            {
                TC_LOG_ERROR("server.loading", "ModuleUpdateManager: Exception in module '{}' update: {}", moduleName, ex.what());
            }
            catch (...)
            {
                TC_LOG_ERROR("server.loading", "ModuleUpdateManager: Unknown exception in module '{}' update", moduleName);
            }
        }
    }

    bool ModuleUpdateManager::IsModuleRegistered(std::string const& moduleName) const
    {
        std::lock_guard<std::mutex> lock(_modulesMutex);
        return _registeredModules.find(moduleName) != _registeredModules.end();
    }

    uint32 ModuleUpdateManager::GetRegisteredModuleCount() const
    {
        std::lock_guard<std::mutex> lock(_modulesMutex);
        return static_cast<uint32>(_registeredModules.size());
    }
}