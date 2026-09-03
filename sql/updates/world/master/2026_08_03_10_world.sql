-- Order Advancement: make the class-hall advisors talent-tree NPCs.
--
-- Interacting with a creature carrying UNIT_NPC_FLAG_2_GARRISON_TALENT_NPC opens the client's Order Advancement
-- (class-hall talent tree) UI. In this schema npcflag is a single 64-bit column: the low 32 bits are npcflag and the
-- high 32 bits are npcflag2, so the GarrisonTalentNpc bit (npcflag2 0x200) is 0x200 << 32 = 0x20000000000.
--
-- Most hall advisors already carry this bit (e.g. Winstone Wolfe = 0x20000000001 = talent+gossip); six were missing it
-- - Survivalist Bahn (Hunter), Number Nine Jia (Monk), Journeyman Goldmine (Rogue) were at npcflag 0/gossip-only, and
-- Leafbeard (Druid), Loramus (Warlock), Archon Torias (DK) had gossip only. TrinityCore already implements the whole
-- research engine (GarrTalent + GarrTalentRank driven; hunter tree 113 = 6 talents, 2h-24h, 50-10000 Order Resources),
-- so once the advisor can open the UI the full research/cost/timer/persist flow runs. OR the bit in (idempotent).
UPDATE `creature_template` SET `npcflag` = `npcflag` | 0x20000000000 WHERE `entry` IN (
    108050, -- Survivalist Bahn        (Hunter,      tree 113)
     97989, -- Leafbeard the Storied   (Druid,       tree 107)
    108331, -- Chronicler Elrianne     (Priest,      tree 134)
    108018, -- Archivist Melinda       (Mage,        tree 116)
     98939, -- Number Nine Jia         (Monk,        tree 4)
    107994, -- Einar the Runecaster    (Shaman,      tree 31)
    105998, -- Winstone Wolfe          (Warrior,     tree 122)
    108527, -- Loramus Thalipedes      (Warlock,     tree 110)
    112199, -- Journeyman Goldmine     (Rogue,       tree 131)
    109901, -- Sir Alamande Graythorn  (Paladin,     tree 119)
    110725, -- Archon Torias           (Death Knight,tree 128)
     97485  -- Archivist Zubashi       (Demon Hunter,tree 125)
);
