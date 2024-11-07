#include "Chat.h"
#include "custom_effects_handler.h"
#include "GameTime.h"

namespace Noblegarden
{
    void Message(ChatHandler* handler, std::string message)
    {
        WorldPackets::Chat::Chat MessagePacket;
        MessagePacket.Initialize(CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, message);
        handler->GetPlayer()->SendDirectMessage(MessagePacket.Write());
    }

    void EffectsHandler::Init()
    {
        Player::OnMeetUnit = std::bind(&Noblegarden::EffectsHandler::Sync, this, std::placeholders::_1, std::placeholders::_2);
    }


    void EffectsHandler::Add(Unit* unit, uint32 id, uint8 mode, ChatHandler* handler)
    {
        if (mode == 0 || mode == 3)
        {
            Oneshot(unit, id, mode);
            return;
        }

        if (auto targetInfo = GetUnitInfo(unit))
        {
            if (targetInfo->Store->Size() >= 5)
            {
                if (handler)
                {
                    Message(handler, "You have reached max count of effects: 5");
                }

                delete targetInfo;
                return;
            }

            if (!targetInfo->Store->HasEffect(id))
            {
                auto data = new EffectData;
                data->ID = id;
                data->Mode = mode;

                targetInfo->Store->AddEffect(id, data);
                unit->SendPlaySpellVisualKit(id, mode, 0);
            }

            delete targetInfo;
        }
    }


    void EffectsHandler::Toggle(Unit* unit, uint32 id, uint8 mode, ChatHandler* /*handler*/)
    {
        if (HasEffect(unit, id)) Remove(unit, id); else Add(unit, id, mode);
    }


    void EffectsHandler::Oneshot(Unit* unit, uint32 id, uint8 mode, ChatHandler* /*handler*/)
    {
        unit->SendCancelSpellVisualKit(id);
        unit->SendPlaySpellVisualKit(id, mode, 0);
    }


    void EffectsHandler::Remove(Unit* unit, uint32 id, ChatHandler* /*handler*/)
    {
        if (auto targetInfo = GetUnitInfo(unit))
        {
            if (targetInfo->Store->HasEffect(id))
            {
                targetInfo->Store->RemoveEffect(id);
                unit->SendCancelSpellVisualKit(id);
            }

            delete targetInfo;
        }
    }


    void EffectsHandler::Channel(Unit* unit, Unit* target, uint32 id, ChatHandler* /*handler*/)
    {
        unit->ClearChannelObjects();

        if (id == 0 || unit->GetChannelSpellXSpellVisualId() == id)
        {
            SpellCastVisual visual;
            unit->SetChannelSpellId(0);
            unit->SetChannelVisual(visual);
            return;
        }

        auto spellVisual = sSpellXSpellVisualStore.LookupEntry(id);

        if (spellVisual)
        {
            SpellCastVisual visual;
            visual.SpellXSpellVisualID = id;
            visual.ScriptVisualID = 0;

            unit->SetChannelObject(0, target->GetGUID());
            unit->SetChannelSpellId(spellVisual->SpellID);
            unit->SetChannelVisual(visual);
        }

        return;
    }


    void EffectsHandler::Cast(Unit* unit, Unit* target, uint32 id, float duration, ChatHandler* handler)
    {
        auto minDuration    = 0.0f;
        auto maxDuration    = 5.0f;
        auto cooldownTime   = 200;

        if (auto targetInfo = GetUnitInfo(unit))
        {
            auto currentTime = GameTime::GetGameTimeMS();

            if (currentTime < targetInfo->Store->LastEffectCastTimer)
            {
                delete targetInfo;
                return;
            }

            if (duration  < minDuration || duration  > maxDuration)
            {
                if (handler)
                {
                    Message(handler, "Duration must be in range from " + std::to_string((int)minDuration) + " to " + std::to_string((int)maxDuration) + " seconds.");
                }

                delete targetInfo;
                return;
            }

            unit->SendPlaySpellVisual(target, id, 0, 0, duration, true);
            targetInfo->Store->LastEffectCastTimer = currentTime + cooldownTime;

            delete targetInfo;
        }
    }


    void EffectsHandler::Reset(Unit* unit, ChatHandler* /*handler*/)
    {
        if (auto targetInfo = GetUnitInfo(unit))
        {
            for (auto& pair : targetInfo->Store->Effects)
            {
                unit->SendCancelSpellVisualKit(pair.first);
            }

            targetInfo->Store->Effects.clear();

            delete targetInfo;
        }
    }


    bool EffectsHandler::HasEffect(Unit* unit, uint32 id, ChatHandler* /*handler*/)
    {
        auto exists = false;

        if (auto targetInfo = GetUnitInfo(unit))
        {
            exists = targetInfo->Store->HasEffect(id);
            delete targetInfo;
        }

        return exists;
    }


    EffectsHandler::UnitInfo* EffectsHandler::GetUnitInfo(Unit* unit)
    {
        auto result = new UnitInfo;

        if (auto player = unit->ToPlayer())
        {
            auto key = player->GetGUID().GetCounter();

            if (!m_player_stores.contains(key))
                m_player_stores.emplace(key, new EffectStore);

            result->Key         = key;
            result->UnitPtr     = unit;
            result->Store       = m_player_stores.at(key);
            result->IsPlayer    = true;
        }

        if (auto creature = unit->ToCreature())
        {
            auto spawnID = creature->GetSpawnId();
            auto key = spawnID > 0 ? spawnID : creature->GetGUID().GetCounter();

            if (!m_creature_stores.contains(key))
                m_creature_stores.emplace(key, new EffectStore);

            result->Key         = key;
            result->UnitPtr     = unit;
            result->Store       = m_creature_stores.at(key);
            result->IsCreature  = true;
        }

        return result->IsPlayer || result->IsCreature ? result : nullptr;
    }


    void EffectsHandler::Sync(Player* observer, Unit* observable)
    {
        if (auto unitInfo = GetUnitInfo(observable))
        {
            auto action = new SyncEvent(observer, unitInfo);
            auto delay = observer->m_Events.CalculateTime(Milliseconds(2000));
            observer->m_Events.AddEvent(action, delay, false);
        }
    }


    bool EffectsHandler::SyncEvent::Execute(uint64, uint32)
    {
        for (auto& data : m_unit_info->Store->Effects)
        {
            if (m_unit_info->UnitPtr)
            {
                WorldPackets::Spells::PlaySpellVisualKit packet;
                packet.Unit = m_unit_info->UnitPtr->GetGUID();
                packet.KitRecID = data.second->ID;
                packet.KitType = data.second->Mode;
                packet.Duration = 0.0f;
                m_observer->SendDirectMessage(packet.Write());
            }
        }

        delete m_unit_info;
        return true;
    }


    bool EffectsHandler::EffectStore::HasEffect(uint32 id)
    {
        return Effects.contains(id);
    }


    void EffectsHandler::EffectStore::AddEffect(uint32 id, EffectData* data)
    {
        Effects.emplace(id, data);
    }


    void EffectsHandler::EffectStore::RemoveEffect(uint32 id)
    {
        Effects.erase(id);
    }


    void EffectsHandler::EffectStore::Clear(uint32 id)
    {
        Effects.erase(id);
    }


    int EffectsHandler::EffectStore::Size()
    {
        return Effects.size();
    }
}

void AddSC_CustomEffectHandler()
{
    Noblegarden::EffectsHandler::GetInstance();
}
