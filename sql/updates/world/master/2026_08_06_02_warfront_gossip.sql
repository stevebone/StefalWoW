-- BfA Warfronts P2 - war-table recruiter gossip hook
--
-- Binds the C++ CreatureAI script `npc_warfront_recruiter` (src/server/scripts/Warfronts/npc_ralston_karn.cpp)
-- to the two Battle-for-Stromgarde war-table NPCs so their gossip shows the current warfront state and, while the
-- zone is in SIEGE, offers the challenging faction an "Enroll in the assault on Stromgarde." option that enqueues
-- the player via WarfrontMgr::EnqueuePlayer -> WarfrontQueue. The gossip options themselves are built in C++
-- (AddGossipItemFor), so no gossip_menu_option rows are required; only the ScriptName binding is.
--
--   Ralston Karn (142721) - Alliance, Boralus (map 1643), existing empty gossip menu 23182
--   Throk        (138949) - Horde,    Zuldazar,           existing empty gossip menu 23112
--
-- Both currently have an empty ScriptName. Existing quest-giver behaviour is preserved: OnGossipHello still calls
-- PrepareQuestMenu when the NPC is a quest giver.

UPDATE `creature_template` SET `ScriptName` = 'npc_warfront_recruiter' WHERE `entry` IN (142721, 138949);
