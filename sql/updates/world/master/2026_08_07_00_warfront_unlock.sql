--
-- BfA Warfronts: data prerequisites for the Blizzlike unlock path (contribution -> siege without GM commands).
--
-- 1) `world_state` template rows for every world state a warfront ManagedWorldState drives.
--
--    WorldStateMgr::SaveValueInDb() early-returns for any world state that has no `world_state` template row, and
--    WorldStateMgr::GetValue() cannot restore one either. Without these rows the warfront contribution bars are
--    broadcast live (SetValue still works on the realm-wide map) but are LOST on every restart - the bar silently
--    resets to empty and the 4-day fill starts over, so the zone can never reach SIEGE on its own.
--
--    Ids traced from ManagedWorldState.db2 (build 68275):
--      MWS  12 Stromgarde/Horde     Stage 15622  Progress 15624  Occurrences 15625, 16228
--      MWS 113 Stromgarde/Alliance  Stage 16033  Progress 16035  Occurrences 16036, 16229
--      MWS 114 Darkshore/Alliance   Stage 16707  Progress 16712  Occurrences 16714, 16716
--      MWS 115 Darkshore/Horde      Stage 16708  Progress 16711  Occurrences 16713, 16715
--
--    All four states share AccumulationStateTargetValue = 500000000 and DepletionStateTargetValue = 30, so an EMPTY
--    progress bar is 30, not 0 - that is the DefaultValue used below. Stage/Occurrences start at 0.
--    MapIDs/AreaIDs are left empty on purpose: these are realm-wide world states, which is what makes
--    WorldStateMgr broadcast them to every session (SMSG_UPDATE_WORLD_STATE / SMSG_INIT_WORLD_STATES).
--
DELETE FROM `world_state` WHERE `ID` IN
    (15622, 15624, 15625, 16228,
     16033, 16035, 16036, 16229,
     16707, 16712, 16714, 16716,
     16708, 16711, 16713, 16715);

INSERT INTO `world_state` (`ID`, `DefaultValue`, `MapIDs`, `AreaIDs`, `ScriptName`, `Comment`) VALUES
-- Battle for Stromgarde - Horde contribution bar (ManagedWorldState 12)
(15622,  0, '', '', '', 'Warfront Stromgarde (Horde) - contribution current stage'),
(15624, 30, '', '', '', 'Warfront Stromgarde (Horde) - contribution progress'),
(15625,  0, '', '', '', 'Warfront Stromgarde (Horde) - contribution occurrences 1'),
(16228,  0, '', '', '', 'Warfront Stromgarde (Horde) - contribution occurrences 2'),
-- Battle for Stromgarde - Alliance contribution bar (ManagedWorldState 113)
(16033,  0, '', '', '', 'Warfront Stromgarde (Alliance) - contribution current stage'),
(16035, 30, '', '', '', 'Warfront Stromgarde (Alliance) - contribution progress'),
(16036,  0, '', '', '', 'Warfront Stromgarde (Alliance) - contribution occurrences 1'),
(16229,  0, '', '', '', 'Warfront Stromgarde (Alliance) - contribution occurrences 2'),
-- The Battle for Darkshore - Alliance contribution bar (ManagedWorldState 114)
(16707,  0, '', '', '', 'Warfront Darkshore (Alliance) - contribution current stage'),
(16712, 30, '', '', '', 'Warfront Darkshore (Alliance) - contribution progress'),
(16714,  0, '', '', '', 'Warfront Darkshore (Alliance) - contribution occurrences 1'),
(16716,  0, '', '', '', 'Warfront Darkshore (Alliance) - contribution occurrences 2'),
-- The Battle for Darkshore - Horde contribution bar (ManagedWorldState 115)
(16708,  0, '', '', '', 'Warfront Darkshore (Horde) - contribution current stage'),
(16711, 30, '', '', '', 'Warfront Darkshore (Horde) - contribution progress'),
(16713,  0, '', '', '', 'Warfront Darkshore (Horde) - contribution occurrences 1'),
(16715,  0, '', '', '', 'Warfront Darkshore (Horde) - contribution occurrences 2');

--
-- 2) The war-table recruiters become real Contribution Collectors.
--
--    The retail warfront collectors ("Warfront Alliance/Horde Contribution Dummy" 143709 / 143707) already carry
--    UNIT_NPC_FLAG_2_CONTRIBUTION_COLLECTOR (npcflag 0x40000000000), but they ship with CreatureDisplayID 13069 -
--    the invisible stalker - plus UNIT_FLAG_UNINTERACTIBLE (0x02000000) and faction 7, so a player can never click
--    one. They are map markers, not interactables.
--
--    The visible war-table recruiters are what the player actually talks to, so they take over the collector role:
--      Ralston Karn 142721 (Alliance, Boralus)  - gossip menu 23182, ScriptName npc_warfront_recruiter
--      Throk        138949 (Horde,    Zuldazar) - gossip menu 23112, ScriptName npc_warfront_recruiter
--
--    npcflag is a single uint64 whose HIGH dword is NPCFlags2, so UNIT_NPC_FLAG_2_CONTRIBUTION_COLLECTOR (0x400)
--    becomes 0x400_00000000. Both recruiters keep GOSSIP|QUESTGIVER (0x3):
--      0x40000000000 | 0x3 = 0x40000000003 = 4398046511107
--
UPDATE `creature_template` SET `npcflag` = `npcflag` | 4398046511104 WHERE `entry` IN (142721, 138949);

--
-- 3) Sanity note (no data change): the collector <-> contribution authorization itself lives in
--    CreatureXContribution.db2 and is applied as a hotfix - see
--    sql/updates/hotfixes/master/2026_08_07_00_warfront_contribution.sql.
--
