-- Fix to add missing npc flags for bankers
-- This will add the account banker flag to banker npcs that do NOT have it
UPDATE creature_template SET npcflag = npcflag | 0x4 WHERE npcflag & 0x20000;