-- Stefal patch for Custom_Demon_NPC_Fixes.sql
-- Apply AFTER the base file. Safe to re-run.

UPDATE world.creature_template SET `ScriptName` = 'npc_warlock_dreadstalker' WHERE `Entry` = 98035;
UPDATE world.creature_template SET `ScriptName` = 'npc_pet_warlock_demonic_tyrant' WHERE `Entry` = 135002;
UPDATE world.creature_template SET `ScriptName` = 'npc_pet_warlock_demonic_tyrant' WHERE `Entry` = 250289;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_diabolic_imp' WHERE `Entry` = 219161;
UPDATE world.creature_template SET `ScriptName` = 'npc_pet_warlock_wild_imp' WHERE `Entry` = 55659;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_dimensional_rift_shadowy_tear' WHERE `Entry` = 198547;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_dimensional_rift_unstable_tear' WHERE `Entry` = 196280;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_dimensional_rift_chaos_tear' WHERE `Entry` = 108493;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_diabolist_overlord' WHERE `Entry` = 228575;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_diabolist_mother_of_chaos' WHERE `Entry` = 228576;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_avatar_of_destruction_overfiend' WHERE `Entry` = 217429;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_diabolist_pit_lord' WHERE `Entry` = 228574;
UPDATE world.creature_template SET `ScriptName` = 'npc_pet_warlock_darkglare' WHERE `Entry` = 103673;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_demonic_gateway' WHERE `Entry` = 59262;
UPDATE world.creature_template SET `ScriptName` = 'npc_warl_demonic_gateway' WHERE `Entry` = 59271;

-- Eye of Kilrogg (4277): HealthModifier 0 caused dead summons
UPDATE world.creature_template_difficulty
SET `HealthModifier` = 1
WHERE `Entry` = 4277 AND `DifficultyID` = 0;
