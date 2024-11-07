#include "ChatPackets.h"
#include "WorldPacket.h"
#include "SpellPackets.h"

namespace Noblegarden
{
    #ifndef NOBLEGARDEN_EFFECTS_HANDLER
    #define NOBLEGARDEN_EFFECTS_HANDLER
    class EffectsHandler
    {

    // Structures
    public:
        struct EffectData
        {
            uint32 ID = 0;
            uint8 Mode = 0;
        };

        struct EffectStore
        {
            std::map<uint32, EffectData*> Effects;
            uint32 LastEffectCastTimer = 0;

            bool HasEffect(uint32 id);
            void AddEffect(uint32 id, EffectData* data);
            void RemoveEffect(uint32 id);
            void Clear(uint32 id);
            int Size();
        };

        struct UnitInfo
        {
            bool IsPlayer = false;
            bool IsCreature = false;
            uint64 Key = 0;
            Unit* UnitPtr = nullptr;
            EffectStore* Store = nullptr;
        };

        struct SyncEvent : public BasicEvent
        {
        private:
            Player* m_observer;
            UnitInfo* m_unit_info;

        public:
            SyncEvent(Player* observer, UnitInfo* unitInfo) :
                m_observer(observer),
                m_unit_info(unitInfo) { };

            bool Execute(uint64, uint32) override;
        };

    // Init
    public:
        EffectsHandler()
        {
            Init();
        }

        static EffectsHandler& GetInstance() 
		{
            static EffectsHandler instance;
            return instance;
        }

        EffectsHandler(EffectsHandler const&) = delete;
        void operator=(EffectsHandler const&) = delete;

    // API
    public:
        void Add(Unit* unit, uint32 id, uint8 mode, ChatHandler* handler = nullptr);
        void Toggle(Unit* unit, uint32 id, uint8 mode, ChatHandler* handler = nullptr);
        void Oneshot(Unit* unit, uint32 id, uint8 mode, ChatHandler* handler = nullptr);
        void Remove(Unit* unit, uint32 id, ChatHandler* handler = nullptr);
        void Channel(Unit* unit, Unit* target, uint32 id, ChatHandler* handler = nullptr);
        void Cast(Unit* unit, Unit* target, uint32 id, float duration, ChatHandler* handler = nullptr);
        void Reset(Unit* unit, ChatHandler* handler = nullptr);
        bool HasEffect(Unit* unit, uint32 id, ChatHandler* handler = nullptr);

    private:
        UnitInfo* GetUnitInfo(Unit* unit);
        void Sync(Player* observer, Unit* observable);
        void Init();

    // Datatables
    private:
        std::map<uint64, EffectStore*> m_player_stores;
        std::map<uint64, EffectStore*> m_creature_stores;
    };
    #endif
};
