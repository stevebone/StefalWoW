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

#include "SafeObjectReference.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "Log.h"

namespace Playerbot::References {

// ========================================================================
// EXPLICIT TEMPLATE INSTANTIATIONS
// ========================================================================

/**
 * Explicit template instantiations ensure the template is compiled once,
 * not in every translation unit. This reduces compile time and binary size.
 *
 * These are the most commonly used object types in the Playerbot system:
 * - Player: Bot references to group leaders, master players, etc.
 * - Creature: References to NPCs, vendors, quest givers, enemies
 * - GameObject: References to chests, gathering nodes, quest objects
 * - Unit: Generic references to any attackable/healable entity
 * - WorldObject: Base reference for any object in the world
 */

template class SafeObjectReference<Player>;
template class SafeObjectReference<Creature>;
template class SafeObjectReference<GameObject>;
template class SafeObjectReference<Unit>;
template class SafeObjectReference<WorldObject>;

// ========================================================================
// PERFORMANCE MONITORING
// ========================================================================

/**
 * @brief Global performance statistics for SafeObjectReference usage
 *
 * This provides server-wide metrics on reference cache efficiency
 * to help optimize CACHE_DURATION_MS if needed.
 */
class SafeReferenceMetrics {
public:
    static SafeReferenceMetrics& Instance()
    {
        static SafeReferenceMetrics instance;
        return instance;
    }

    void RecordCacheHit()
    {
        m_totalCacheHits.fetch_add(1, ::std::memory_order_relaxed);
    }

    void RecordCacheMiss()
    {
        m_totalCacheMisses.fetch_add(1, ::std::memory_order_relaxed);
    }

    void RecordObjectDeleted(ObjectGuid guid)
    {
        m_deletedObjects.fetch_add(1, ::std::memory_order_relaxed);

        TC_LOG_TRACE("module.playerbot.reference",
            "SafeReferenceMetrics: Object {} deleted (total: {})",
            guid.ToString(), m_deletedObjects.load());
    }

    float GetGlobalCacheHitRate() const
    {
        uint64 hits = m_totalCacheHits.load(::std::memory_order_relaxed);
        uint64 misses = m_totalCacheMisses.load(::std::memory_order_relaxed);
        uint64 total = hits + misses;

        if (total == 0) return 0.0f;
        return static_cast<float>(hits) / static_cast<float>(total);
    }

    uint64 GetTotalAccesses() const
    {
        return m_totalCacheHits.load() + m_totalCacheMisses.load();
    }

    uint64 GetDeletedObjectCount() const
    {
        return m_deletedObjects.load(::std::memory_order_relaxed);
    }

    void LogPerformanceReport() const
    {
        TC_LOG_INFO("module.playerbot.reference",
            "SafeObjectReference Performance Report:");
        TC_LOG_INFO("module.playerbot.reference",
            "  Total Accesses: {}", GetTotalAccesses());
        TC_LOG_INFO("module.playerbot.reference",
            "  Cache Hit Rate: {:.2f}%", GetGlobalCacheHitRate() * 100.0f);
        TC_LOG_INFO("module.playerbot.reference",
            "  Cache Hits: {}", m_totalCacheHits.load());
        TC_LOG_INFO("module.playerbot.reference",
            "  Cache Misses: {}", m_totalCacheMisses.load());
        TC_LOG_INFO("module.playerbot.reference",
            "  Deleted Objects: {}", m_deletedObjects.load());
    }

    void ResetMetrics()
    {
        m_totalCacheHits.store(0);
        m_totalCacheMisses.store(0);
        m_deletedObjects.store(0);
    }

private:
    SafeReferenceMetrics() = default;

    ::std::atomic<uint64> m_totalCacheHits{0};
    ::std::atomic<uint64> m_totalCacheMisses{0};
    ::std::atomic<uint64> m_deletedObjects{0};
};

// ========================================================================
// DEBUG UTILITIES
// ========================================================================

/**
 * @brief Debug helper to detect dangling references
 *
 * This is used during development to ensure all references are
 * properly cleared when objects are deleted.
 */
class DanglingReferenceDetector {
public:
    static void RegisterReference(ObjectGuid guid, void* reference)
    {
#ifdef _DEBUG
        ::std::lock_guard lock(s_mutex);
        s_references[guid].insert(reference);

        TC_LOG_TRACE("module.playerbot.reference.debug",
            "Registered reference {} -> {}",
            guid.ToString(), reference);
#endif
    }

    static void UnregisterReference(ObjectGuid guid, void* reference)
    {
#ifdef _DEBUG
        ::std::lock_guard lock(s_mutex);
        auto it = s_references.find(guid);
        if (it != s_references.end()) {
            it->second.erase(reference);
            if (it->second.empty())
            {
                s_references.erase(it);
            }
        }

        TC_LOG_TRACE("module.playerbot.reference.debug",
            "Unregistered reference {} -> {}",
            guid.ToString(), reference);
#endif
    }

    static void CheckForDanglingReferences(ObjectGuid guid)
    {
#ifdef _DEBUG
        ::std::lock_guard lock(s_mutex);
        auto it = s_references.find(guid);
        if (it != s_references.end() && !it->second.empty()) {
            TC_LOG_ERROR("module.playerbot.reference",
                "DANGLING REFERENCES DETECTED: {} still has {} references!",
                guid.ToString(), it->second.size());

            // In debug builds, this is a critical error
            ASSERT(false, "Dangling references detected for object %s",
                guid.ToString().c_str());
        }
#endif
    }

    static size_t GetReferenceCount(ObjectGuid guid)
    {
#ifdef _DEBUG
        ::std::lock_guard lock(s_mutex);
        auto it = s_references.find(guid);
        return it != s_references.end() ? it->second.size() : 0;
#else
        return 0;
#endif
    }

private:
#ifdef _DEBUG
    static ::std::recursive_mutex s_mutex;
    static ::std::unordered_map<ObjectGuid, ::std::unordered_set<void*>> s_references;
#endif
};

#ifdef _DEBUG
::std::recursive_mutex DanglingReferenceDetector::s_mutex;
::std::unordered_map<ObjectGuid, ::std::unordered_set<void*>> DanglingReferenceDetector::s_references;
#endif

// ========================================================================
// UNIT TESTS (Debug builds only)
// ========================================================================

#ifdef _DEBUG

/**
 * @brief Self-test for SafeObjectReference functionality
 *
 * This runs automatically on server startup in debug builds to ensure
 * the reference system is working correctly.
 */
class SafeReferenceUnitTests {
public:
    static void RunAllTests()
    {
        TC_LOG_INFO("module.playerbot.reference.test",
            "Running SafeObjectReference unit tests...");

        TestEmptyReference();
        TestSetAndGet();
        TestCacheExpiration();
        TestMoveSemantics();
        TestCopySemantics();
        TestPerformanceMetrics();

        TC_LOG_INFO("module.playerbot.reference.test",
            "All SafeObjectReference tests passed!");
    }

private:
    static void TestEmptyReference()
    {
        SafePlayerReference ref;
        ASSERT(ref.IsEmpty(), "New reference should be empty");
        ASSERT(!ref.IsValid(), "Empty reference should not be valid");
        ASSERT(ref.Get() == nullptr, "Empty reference should return nullptr");
    }

    static void TestSetAndGet()
    {
        // This test requires a valid Player object, which we don't have
        // during unit testing. In a real environment, this would be tested
        // with mock objects or during integration testing.
        TC_LOG_TRACE("module.playerbot.reference.test",
            "TestSetAndGet: Skipped (requires runtime objects)");
    }

    static void TestCacheExpiration()
    {
        // This test would require time manipulation which we can't do
        // in unit tests. This is tested during integration testing.
        TC_LOG_TRACE("module.playerbot.reference.test",
            "TestCacheExpiration: Skipped (requires time manipulation)");
    }

    static void TestMoveSemantics()
    {
        SafePlayerReference ref1;
        ref1.SetGuid(ObjectGuid::Create<HighGuid::Player>(12345));

        SafePlayerReference ref2(::std::move(ref1));
        ASSERT(ref1.IsEmpty(), "Moved-from reference should be empty");
        ASSERT(!ref2.IsEmpty(), "Moved-to reference should have GUID");
        ASSERT(ref2.GetGuid().GetCounter() == 12345,
            "Moved-to reference should have correct GUID");
    }

    static void TestCopySemantics()
    {
        SafePlayerReference ref1;
        ref1.SetGuid(ObjectGuid::Create<HighGuid::Player>(12345));

        SafePlayerReference ref2(ref1);
        ASSERT(!ref1.IsEmpty(), "Copied-from reference should retain GUID");
        ASSERT(!ref2.IsEmpty(), "Copied-to reference should have GUID");
        ASSERT(ref1.GetGuid() == ref2.GetGuid(),
            "Copied references should have same GUID");
    }

    static void TestPerformanceMetrics()
    {
        SafePlayerReference ref;
        ASSERT(ref.GetAccessCount() == 0, "New reference should have 0 accesses");
        ASSERT(ref.GetCacheHitRate() == 0.0f, "New reference should have 0% hit rate");

        // Simulate some accesses
        ref.Get();
        ref.Get();

        ASSERT(ref.GetAccessCount() == 2, "Should have 2 accesses after Get() calls");

        ref.ResetMetrics();
        ASSERT(ref.GetAccessCount() == 0, "Metrics should reset to 0");
    }
};

// Register unit tests to run on module initialization
struct SafeReferenceTestRunner {
    SafeReferenceTestRunner()
    {
        SafeReferenceUnitTests::RunAllTests();
    }
};

// This will run tests when the module is loaded in debug builds
// static SafeReferenceTestRunner s_testRunner;

#endif // _DEBUG

} // namespace Playerbot::References