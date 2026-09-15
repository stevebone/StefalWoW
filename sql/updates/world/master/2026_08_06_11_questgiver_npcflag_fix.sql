-- Companion to 2026_08_06_10 (Wowhead quest relations): any creature that STARTS or ENDS a quest
-- must carry UNIT_NPC_FLAG_QUESTGIVER (2), else TC logs an error and the turn-in never shows in-game.
-- Additive (|= 2), scoped to quest-relation creatures currently missing the flag.
-- NOTE: a few are Legion campaign NPCs whose flag is normally phase-managed; static flag may show the
-- '?' slightly out of phase-context — acceptable vs. an un-turn-in-able orphan quest. World DB is rebuildable.
UPDATE `creature_template` SET `npcflag` = `npcflag` | 2
WHERE (`npcflag` & 2) = 0
  AND (EXISTS(SELECT 1 FROM `creature_queststarter` s WHERE s.id = `creature_template`.`entry`)
    OR EXISTS(SELECT 1 FROM `creature_questender` e WHERE e.id = `creature_template`.`entry`));
