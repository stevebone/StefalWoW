-- DH Campaign Vault of the Wardens

-- Sniff Data
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=92782; -- Savage Felstalker
UPDATE `creature_template` SET `npcflag`=281474976710659, `unit_flags3`=0x200 WHERE `entry`=92718; -- Maiev Shadowsong
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=96847; -- Drelanim Whisperwind
UPDATE `creature_template` SET `faction`=2838, `npcflag`=36028797018968192, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x40000000 WHERE `entry`=149919; -- Worn Anvil
UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=100168; -- Ward of the Hunters
UPDATE `creature_template` SET `npcflag`=0, `unit_flags3`=0x41000000 WHERE `entry`=103658; -- Kayn's Cell
UPDATE `creature_template` SET `unit_flags3`=0x41000000 WHERE `entry`=103655; -- Altruis's Cell
UPDATE `creature_template` SET `unit_flags`=0x0 WHERE `entry`=97069; -- Wrath-Lord Lekos