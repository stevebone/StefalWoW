-- Order Advancement: make the class-hall advisors talent-tree NPCs.
--
-- Interacting with a creature carrying UNIT_NPC_FLAG_2_GARRISON_TALENT_NPC (0x200 on npcflag2) opens the client's
-- Order Advancement (class-hall talent tree) UI. TrinityCore already implements the whole research engine (learn/
-- research/cost/timer/persist, driven by GarrTalent + GarrTalentRank DB2 - e.g. hunter tree 113: 6 talents, 2h-24h
-- research, 50-10000 Order Resources), but no NPC had the flag, so the UI could never be opened. Flag each class
-- hall's Order Advancement advisor (the "Further Advancement" targets). The client resolves which tree to show from
-- the player's class-order garrison (GarrType 3) + class, so one flagged advisor per hall is sufficient.
UPDATE `creature_template` SET `npcflag2` = `npcflag2` | 0x200 WHERE `entry` IN (
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
