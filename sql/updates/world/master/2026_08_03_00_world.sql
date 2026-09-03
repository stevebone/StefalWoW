-- Order-hall "Further Advancement" quests: give the Order Advancement advisors their gossip flag.
--
-- Quests 46778-46789 ("Further Advancement", one per class hall) carry a custom Type-3 (TALKTO) objective directing the
-- player to the hall's Order Advancement advisor. Most advisors already have a gossip npcflag, but three were left at
-- npcflag=0, so the client can't treat them as talk-to targets and renders the objective as "Slay <advisor>" (and it
-- can never be completed - you can't open gossip). Give them the gossip flag (matching the working advisors, npcflag=1):
--   108050 Survivalist Bahn      (Hunter, 46783)
--    98939 Number Nine Jia       (Monk,   46785)
--   112199 Journeyman Goldmine   (Mage,   46781)
UPDATE `creature_template` SET `npcflag` = `npcflag` | 1 WHERE `entry` IN (108050, 98939, 112199);

-- --- merged: upstream/TDB additions under the same filename (add/add union) ---

DELETE FROM `spell_proc` WHERE `SpellId` IN (1253590);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1253590,0x00,6,0x00000000,0x00800000,0x00000000,0x00000000,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0,0,0,0); -- Master the Darkness

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_pri_master_the_darkness');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(1253590, 'spell_pri_master_the_darkness');

DELETE FROM `spell_proc` WHERE `SpellId` IN (1253591);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(1253591,0x00,6,0x00000001,0x00000000,0x00000000,0x00000000,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0,0,0,1); -- Master the Darkness