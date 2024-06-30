SET @ATIDSPAWN := 68;
SET @ATID := 66;
SET @ATCP := 54;

DELETE FROM `areatrigger_template` WHERE (`Id` = @ATID+0 AND `IsCustom` = 1);
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `VerifiedBuild`) VALUES 
(@ATID+0, 1, 1, 0);

DELETE FROM `areatrigger_create_properties` WHERE (`Id`= @ATCP+0 AND `IsCustom`=1);
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `TimeToTarget`, `TimeToTargetScale`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`) VALUES 
(@ATCP+0, 1, @ATID+0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 8, 3, 8, 8, 3, 8, 0, 0, '', 0);

DELETE FROM `areatrigger` WHERE `SpawnId`=@ATIDSPAWN+0;
INSERT INTO `areatrigger` (`SpawnId`, `AreaTriggerCreatePropertiesId`, `IsCustom`, `MapId`, `SpawnDifficulties`, `PosX`, `PosY`, `PosZ`, `Orientation`, `PhaseUseFlags`, `PhaseId`, `PhaseGroup`, `ScriptName`, `Comment`, `VerifiedBuild`) VALUES 
(@ATIDSPAWN+0, @ATCP+0, 1, 1643, '0', 1041.622, -596.877, 1.3597, 3.11838, 0, 0, 0, 'at_boralus_get_your_bearings_ferry', 'Boralus - Hub Tour Ferry', 0);


-- Creature templates
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=32548; -- Corastrasza
UPDATE `creature_template` SET `speed_walk`=1, `speed_run`=1.142857193946838378, `unit_flags`=0 WHERE `entry`=25716; -- General Cerulean
UPDATE `creature_template` SET `speed_walk`=1, `speed_run`=1.142857193946838378, `unit_flags`=0 WHERE `entry`=25717; -- Coldarra Scalesworn
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26265; -- Saragosa's End Invisman
UPDATE `creature_template` SET `speed_run`=1.142857193946838378, `unit_flags2`=2048 WHERE `entry`=33087; -- Signal Fire Invisman
UPDATE `creature_template` SET `speed_run`=1.142857193946838378, `unit_flags`=0 WHERE `entry`=25712; -- Warbringer Goredrak
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25721; -- Arcane Serpent
UPDATE `creature_template` SET `BaseAttackTime`=3000, `unit_flags`=64 WHERE `entry`=32357; -- Old Crystalbark
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26127; -- Nexus Drake Hatchling
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=28960; -- Totally Generic Bunny (JSB)
UPDATE `creature_template` SET `speed_walk`=5.599999904632568359, `speed_run`=2, `unit_flags2`=2048 WHERE `entry`=32534; -- Scalesworn Elite
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25711; -- Spirit of the North
UPDATE `creature_template` SET `unit_flags3`=8388608 WHERE `entry`=27906; -- Warmage Hollister
UPDATE `creature_template` SET `unit_flags3`=8388608 WHERE `entry`=27904; -- Warmage Watkins
UPDATE `creature_template` SET `unit_flags3`=8388608 WHERE `entry`=27173; -- Warmage Calandra
UPDATE `creature_template` SET `npcflag`=131 WHERE `entry`=26110; -- Librarian Serrah
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26175; -- Coldarra - Drake Hunt Invisman
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26117; -- Raelorasz
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=45396; -- Dave's Industrial Light and Magic Bunny (Medium)(Sessile)(Large AOI)
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=24770; -- Nexus Watcher
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26373; -- Coldarra Spell FX InvisMan
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25718; -- Coldarra Mage Slayer
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=27610; -- Dead Caribou
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26219; -- Iruk
UPDATE `creature_template` SET `npcflag`=1, `unit_flags3`=8388608 WHERE `entry`=27189; -- Unu'pe Spearman
UPDATE `creature_template` SET `npcflag`=1 WHERE `entry`=29965; -- Unu'pe Turtle Rider
UPDATE `creature_template` SET `unit_flags`=256, `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=26172; -- Slain Cultist
UPDATE `creature_template` SET `speed_run`=1.142857193946838378, `unit_flags3`=402653184 WHERE `entry`=32361; -- Icehorn
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25473; -- Temple C
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25472; -- Temple B
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25493; -- West En'kilah Cauldron
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25492; -- Central En'kilah Cauldron
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25471; -- Temple A
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25490; -- East En'kilah Cauldron
UPDATE `creature_template` SET `speed_walk`=1, `BaseAttackTime`=250 WHERE `entry`=25390; -- En'kilah Hatchling
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26093; -- Naxxanar Target
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25386; -- En'kilah Crypt Fiend
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25601; -- Prince Valanar
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26094; -- Naxxanar Caster
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25383; -- En'kilah Abomination
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=67108864 WHERE `entry`=26174; -- Carrion Condor
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25387; -- En'kilah Gargoyle
UPDATE `creature_template` SET `unit_flags3`=134217728 WHERE `entry`=25454; -- Tundra Crawler
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25615; -- Plagued Magnataur
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25452; -- Scourged Mammoth
UPDATE `creature_template` SET `unit_flags`=0, `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=26160; -- Taunka'le Pack Kodo
UPDATE `creature_template` SET `unit_flags`=0, `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=26159; -- Taunka'le Evacuee
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26041; -- Lightning Target
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26266; -- Heigarr the Horrible
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25762; -- Vrykul Landing Ship
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25760; -- Kvaldir Raider
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25764; -- Kaskala Defender
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=25984; -- Crashed Recon Pilot
UPDATE `creature_template` SET `faction`=21, `unit_flags`=33554432, `unit_flags2`=2080, `unit_flags3`=1 WHERE `entry`=27999; -- Rock Shield Trigger (Borean Tundra)
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25664; -- South Sinkhole
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25665; -- Northeast Sinkhole
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25666; -- Northwest Sinkhole
UPDATE `creature_template` SET `faction`=21 WHERE `entry`=25660; -- Festering Ghoul
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=32544; -- Steam Cloud
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25427; -- Kaganishu
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25430; -- Magmothregar
UPDATE `creature_template` SET `unit_flags3`=0 WHERE `entry`=21757; -- Big Electromental Flavor
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25434; -- Magmoth Crusher
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25429; -- Magmoth Forager
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25432; -- Mate of Magmothregar
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25433; -- Offspring of Magmothregar
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25428; -- Magmoth Shaman
UPDATE `creature_template` SET `unit_flags3`=8388608 WHERE `entry`=27073; -- Bor'gorok Battleguard
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25226; -- Scalder
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25847; -- East Crash
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25862; -- Khu'nok the Behemoth
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25845; -- Northwest Crash
UPDATE `creature_template` SET `unit_flags`=33554432, `unit_flags2`=2048 WHERE `entry`=25781; -- Oil Pool
UPDATE `creature_template` SET `unit_flags3`=524288 WHERE `entry`=26817; -- Fizzcrank Fighter
UPDATE `creature_template` SET `speed_run`=3.428571462631225585, `unit_flags2`=2048 WHERE `entry`=25765; -- Fizzcrank Bomber
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=67108864 WHERE `entry`=25750; -- Oil-soaked Caribou
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=67108864 WHERE `entry`=25748; -- Oil-covered Hawk
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=67108864 WHERE `entry`=25817; -- Oiled Fledgeling
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25418; -- Churn
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25419; -- Boiling Spirit
UPDATE `creature_template` SET `npcflag`=0, `unit_flags2`=2048, `unit_flags3`=1 WHERE `entry`=25384; -- Steeljaw's Corpse
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=1 WHERE `entry`=25343; -- Dead Caravan Worker
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=1 WHERE `entry`=25342; -- Dead Caravan Guard
UPDATE `creature_template` SET `unit_flags`=768, `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=25841; -- Fizzcrank Recon Pilot
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25846; -- South Crash
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=67108864 WHERE `entry`=25791; -- Oil-stained Wolf
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25355; -- Beryl Hound
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25431; -- Kaskala Ancestor
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=24862; -- Mage Hunter Target
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=24795; -- Surristrasz
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25364; -- Red Guardian Drake
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25724; -- Ascended Mage Hunter
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25594; -- Beryl Point InvisMan
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=54242; -- Borean Beam Target
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25732; -- Warmage Preston
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25727; -- Warmage Moran
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=36155; -- Anzim Controller Bunny
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25356; -- Warmage Anzim
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=27888; -- Warmage Archus
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25733; -- Warmage Austin
UPDATE `creature_template` SET `unit_flags3`=268435456 WHERE `entry`=25968; -- "Lunchbox"
UPDATE `creature_template` SET `unit_flags3`=524288 WHERE `entry`=25801; -- Nedar, Lord of Rhinos
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=24614; -- Wooly Mammoth
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25743; -- Wooly Mammoth Bull
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25385; -- William Allerton
UPDATE `creature_template` SET `unit_flags`=0 WHERE `entry`=25650; -- Plagued Scavenger
UPDATE `creature_template` SET `unit_flags`=0 WHERE `entry`=26083; -- Gerald Green
UPDATE `creature_template` SET `BaseAttackTime`=2000 WHERE `entry`=25843; -- Northsea Thug
UPDATE `creature_template` SET `npcflag`=80 WHERE `entry`=26992; -- Brynna Wilson
UPDATE `creature_template` SET `unit_flags`=33554432 WHERE `entry`=27365; -- Stabled Horse
UPDATE `creature_template` SET `unit_flags3`=524288 WHERE `entry`=25220; -- Civilian Recruit
UPDATE `creature_template` SET `unit_flags3`=524288 WHERE `entry`=25313; -- Valiance Keep Footman
UPDATE `creature_template` SET `unit_flags`=768, `unit_flags2`=2048 WHERE `entry`=24580; -- Riplash Siren
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=26451; -- Ragnar Drakkarlund
UPDATE `creature_template` SET `speed_run`=1.142857193946838378, `unit_flags3`=268435456 WHERE `entry`=26452; -- Leviroth
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25450; -- Veehja
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=24662; -- Riplash Sorceress
UPDATE `creature_template` SET `faction`=1692, `unit_flags2`=2048 WHERE `entry`=24576; -- Riplash Myrmidon
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=26449; -- Gamel the Cruel
UPDATE `creature_template` SET `unit_flags3`=335544320 WHERE `entry`=25498; -- Aspatha the Broodmother
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25797; -- Bloodmage Alkor
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25680; -- Marsh Caribou
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=25789; -- Gammothra the Tormentor
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25464; -- Bloodspore Moth
UPDATE `creature_template` SET `unit_flags3`=335544320 WHERE `entry`=25774; -- Rockfang
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25675; -- Tundra Wolf
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25465; -- Kel'Thuzad
UPDATE `creature_template` SET `unit_flags3`=524288 WHERE `entry`=25466; -- Necropolis Beam (Target)
UPDATE `creature_template` SET `speed_walk`=1, `unit_flags3`=524288 WHERE `entry`=25469; -- Mindless Aberration
UPDATE `creature_template` SET `faction`=1989, `BaseAttackTime`=2000, `unit_flags2`=2048 WHERE `entry`=208182; -- Crazed Looter
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25481; -- Landing Crawler
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25482; -- Sand Turtle
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25334; -- Horde Siege Tank
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=1 WHERE `entry`=28150; -- Destroyed Siege Tank
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=27064; -- Abandoned Fuel Tank
UPDATE `creature_template` SET `unit_flags2`=0 WHERE `entry`=25349; -- Scourge Plague Spreader
UPDATE `creature_template` SET `unit_flags3`=524288 WHERE `entry`=25625; -- Warsong Aberration
UPDATE `creature_template` SET `faction`=14, `unit_flags`=832, `unit_flags2`=2048 WHERE `entry`=25618; -- Varidus the Flenser
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=24771; -- Coldarra Invisman
UPDATE `creature_template` SET `unit_flags3`=134217728 WHERE `entry`=25489; -- Wooly Rhino Bull
UPDATE `creature_template` SET `unit_flags3`=134217728 WHERE `entry`=25487; -- Wooly Rhino Matriarch
UPDATE `creature_template` SET `unit_flags`=768, `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=25328; -- Shadowstalker Luther
UPDATE `creature_template` SET `unit_flags`=0, `unit_flags2`=2048, `unit_flags3`=134217728 WHERE `entry`=25488; -- Wooly Rhino Calf
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=25362; -- Warsong Swine
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25289; -- Wind Master To'bor
UPDATE `creature_template` SET `unit_flags`=33555200 WHERE `entry`=25075; -- Zeppelin Controls
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=25286; -- Warsong Wind Rider
UPDATE `creature_template` SET `unit_flags3`=8388608 WHERE `entry`=31726; -- Grunt Gritch
UPDATE `creature_template` SET `unit_flags3`=8388608 WHERE `entry`=31727; -- Grunt Grikee
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=32520; -- Totally Generic Bunny (All Phase)
UPDATE `creature_template` SET `unit_flags`=256 WHERE `entry`=32647; -- Warsong Hold Practice Dummy

DELETE FROM `creature_template_addon` WHERE `entry` IN (68845, 71163, 54242);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(68845, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 68845 (Nexus Whelpling)
(71163, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '142256'), -- 71163 (Unborn Val'kyr) - Unborn Val'kyr
(54242, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''); -- 54242 (Borean Beam Target)

UPDATE `creature_template_addon` SET `visibilityDistanceType`=3 WHERE `entry`=45396; -- 45396 (Dave's Industrial Light and Magic Bunny (Medium)(Sessile)(Large AOI))
UPDATE `creature_template_addon` SET `AnimTier`=3, `auras`='36630' WHERE `entry`=24770; -- 24770 (Nexus Watcher) - Resistances

DELETE FROM `creature_template_movement` WHERE `CreatureId` IN (68845);
INSERT INTO `creature_template_movement` (`CreatureId`,`HoverInitiallyEnabled`,`Chase`,`Random`,`InteractionPauseTimer`) VALUES
(68845,1,0,0,NULL);

-- Difficulties
UPDATE `creature_template_difficulty` ctd
LEFT JOIN `creature_template_difficulty` ctd2 ON ctd.`Entry`=ctd2.`Entry`
SET
    ctd.`DamageModifier`=ctd2.`DamageModifier`,
    ctd.`LootID`=ctd2.`LootID`,
    ctd.`PickPocketLootID`=ctd2.`PickPocketLootID`,
    ctd.`SkinLootID`=ctd2.`SkinLootID`,
    ctd.`GoldMin`=ctd2.`GoldMin`,
    ctd.`GoldMax`=ctd2.`GoldMax`
WHERE ctd.`Entry` IN (25434,25416,25429,25432,25433,25428,25444,26848,25326,25736,27074,27065,27058,25339,25455,27069,27067,27063,25374,27073,26112,25376,25199,34381,25211,25205,25415,25203,25209,25208,28375,25210,24288,25309,25216,25201,25217,25206,25226,25197,25847,25845,25781,25780,26602,26600,26817,26645,26601,26804,25881,25748,25817,25417,25418,25204,25419,25198,25215,25336,25338,25335,25308,25384,25343,25802,25880,25351,25342,25350,25441,25443,25841,25846,25810,25812,25809,25819,25811,25808,25292,25442,25321,25322,25448,25355,25353,25449,25431,24862,27271,27301,27298,27147,27138,27139,27137,27143,25480,27291,27302,27300,27299,27142,27140,27141,25262,25291,24795,25364,25584,25316,25585,54242,25732,25727,36155,25356,27888,25733,25801,25995,25804,25806,25838,25687,25686,25385,25803,25981,26161,26084,26083,26085,25839,25843,25617,24637,25800,27393,25281,25282,25297,25826,25248,27011,26999,25239,25235,32564,25240,26992,25238,25241,26988,25302,25271,25827,25251,26879,24357,27010,25245,29158,25737,25219,26989,27364,27365,27012,26991,26987,26994,26993,25301,25250,25285,26990,27385,27000,27001,26998,26997,26995,25828,25300,25317,25220,26155,29617,26249,26248,25264,25253,25306,25311,24042,25313,24580,26451,25523,25520,26452,25450,24662,24576,26449,25613,25522,25636,25521,25435,25498,25797,25501,25380,25381,25440,25836,25979,25789,24567,24469,25464,25774,25332,25675,25465,25466,25469,25513,208182,28440,25310,25510,25511,25512,25479,25496,25504,25475,25503,25481,25329,25482,25476,25334,27107,27110,25333,28150,27064,27106,27108,25349,25669,25671,25625,25618,25624,24771,25655,25672,25489,25487,25421,25414,25607,25438,25439,25437,25426,25379,25327,25279,25403,25394,25446,25402,25288,25328,25404,25488,25405,25361,25280,32565,27440,27441,25289,32474,25075,25247,25237,26044,25278,25274,25272,25269,26109,25978,26078,25273,25284,25286,31726,31727,25256,25277,26945,26974,26975,26972,29155,26968,26969,29233,26982,26980,26981,26976,26977,32647,25242,25459,25275,25244,26941,26938,25276,25243,31790,31788,31789,31793,31791,31792,25650,25623,26549,25307,25222,35133,35131,47576,25816,25825,25468,61158,25467,60761,25611,25600,25609,25610,24562,25294,26538,27432,47577,35135,35132,31724,31725,31723,31720,24566,25684,25983,26158,25602,26847,26159,25982,26157,26104,26048,26041,25700,25701,25699,25786,26790,26266,25760,25764,25685,25984,25807,27999,26766,26768,26767,25665,25666,25622,24957,25670,25660,26126,19871,25814,25425,25427,25430,25589,21757,25477,25702,26619,26598,25747,25590,26599,26596,26597,25767,26634,27566,27187,27186,27189,29965,25514,71163,26172,25715,26162,32361,25668,27860,25422,26073,25473,26115,25392,25516,25534,25472,26076,25493,25492,25471,25391,27859,25378,25490,25390,26093,25377,26094,25393,25725,25604,25606,25454,25705,25615,25452,25682,25658,25678,25714,25713,25721,32357,26127,28960,32534,25711,27906,27904,30051,25314,27173,27046,26110,26175,26117,25709,25728,45396,24770,25722,25707,26373,25719,25718,27529,27569,27494,27193,27610,31805,26556,31807,27195,27190,26403,26218,28382,26219,26169,26213,27194,25720,32548,25716,25717,29655,26265,33087,25712,27306,68845,28195,129141,26186,26187,26191,26189,26173,26185,26171,26202,26170,25396,26103,25386,25601,25383,26174,25387,26725,24730,26996,26160,25849,24709,26697,26709,26720,26721,24706,24703,26986,24733,26718,24702,25726,25762,25664,25652,24563,25619,25739,24601,25677,25758,32544,25752,25753,24021,23837,25792,25793,25862,62697,25766,25765,25750,25791,18955,25724,25594,25968,25850,24614,24613,25743,62693,25986,26252,25679,29328,26098,25249,18376,18375,24959,15214,25298,25299,385,25395,25605,25234,25651,25228,25582,24921,25227,25829,25680,25470,62695,25596,31685,25362,26047,25445,18380,18378,18379,32520,26043)
AND ctd.`DifficultyID`=0 AND ctd2.`DifficultyID`=1 AND ctd2.`Entry` IS NOT NULL;

DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID` IN (1,2,4,5,12,14) AND `Entry` IN (25434,25416,25429,25432,25433,25428,25444,26848,25326,25736,27074,27065,27058,25339,25455,27069,27067,27063,25374,27073,26112,25376,25199,34381,25211,25205,25415,25203,25209,25208,28375,25210,24288,25309,25216,25201,25217,25206,25226,25197,25847,25845,25781,25780,26602,26600,26817,26645,26601,26804,25881,25748,25817,25417,25418,25204,25419,25198,25215,25336,25338,25335,25308,25384,25343,25802,25880,25351,25342,25350,25441,25443,25841,25846,25810,25812,25809,25819,25811,25808,25292,25442,25321,25322,25448,25355,25353,25449,25431,24862,27271,27301,27298,27147,27138,27139,27137,27143,25480,27291,27302,27300,27299,27142,27140,27141,25262,25291,24795,25364,25584,25316,25585,54242,25732,25727,36155,25356,27888,25733,25801,25995,25804,25806,25838,25687,25686,25385,25803,25981,26161,26084,26083,26085,25839,25843,25617,24637,25800,27393,25281,25282,25297,25826,25248,27011,26999,25239,25235,32564,25240,26992,25238,25241,26988,25302,25271,25827,25251,26879,24357,27010,25245,29158,25737,25219,26989,27364,27365,27012,26991,26987,26994,26993,25301,25250,25285,26990,27385,27000,27001,26998,26997,26995,25828,25300,25317,25220,26155,29617,26249,26248,25264,25253,25306,25311,24042,25313,24580,26451,25523,25520,26452,25450,24662,24576,26449,25613,25522,25636,25521,25435,25498,25797,25501,25380,25381,25440,25836,25979,25789,24567,24469,25464,25774,25332,25675,25465,25466,25469,25513,208182,28440,25310,25510,25511,25512,25479,25496,25504,25475,25503,25481,25329,25482,25476,25334,27107,27110,25333,28150,27064,27106,27108,25349,25669,25671,25625,25618,25624,24771,25655,25672,25489,25487,25421,25414,25607,25438,25439,25437,25426,25379,25327,25279,25403,25394,25446,25402,25288,25328,25404,25488,25405,25361,25280,32565,27440,27441,25289,32474,25075,25247,25237,26044,25278,25274,25272,25269,26109,25978,26078,25273,25284,25286,31726,31727,25256,25277,26945,26974,26975,26972,29155,26968,26969,29233,26982,26980,26981,26976,26977,32647,25242,25459,25275,25244,26941,26938,25276,25243,31790,31788,31789,31793,31791,31792,25650,25623,26549,25307,25222,35133,35131,47576,25816,25825,25468,61158,25467,60761,25611,25600,25609,25610,24562,25294,26538,27432,47577,35135,35132,31724,31725,31723,31720,24566,25684,25983,26158,25602,26847,26159,25982,26157,26104,26048,26041,25700,25701,25699,25786,26790,26266,25760,25764,25685,25984,25807,27999,26766,26768,26767,25665,25666,25622,24957,25670,25660,26126,19871,25814,25425,25427,25430,25589,21757,25477,25702,26619,26598,25747,25590,26599,26596,26597,25767,26634,27566,27187,27186,27189,29965,25514,71163,26172,25715,26162,32361,25668,27860,25422,26073,25473,26115,25392,25516,25534,25472,26076,25493,25492,25471,25391,27859,25378,25490,25390,26093,25377,26094,25393,25725,25604,25606,25454,25705,25615,25452,25682,25658,25678,25714,25713,25721,32357,26127,28960,32534,25711,27906,27904,30051,25314,27173,27046,26110,26175,26117,25709,25728,45396,24770,25722,25707,26373,25719,25718,27529,27569,27494,27193,27610,31805,26556,31807,27195,27190,26403,26218,28382,26219,26169,26213,27194,155076,25720,32548,25716,25717,29655,26265,33087,25712,27306,68845,28195,129141,26186,26187,26191,26189,26173,26185,26171,26202,26170,25396,26103,25386,25601,25383,26174,25387,26725,24730,26996,26160,25849,24709,26697,26709,26720,26721,24706,24703,26986,24733,26718,24702,25726,25762,25664,25652,24563,25619,25739,24601,25677,25758,32544,25752,25753,24021,23837,25792,25793,25862,62697,25766,25765,25750,25791,18955,25724,25594,25968,25850,24614,24613,25743,62693,25986,26252,25679,29328,26098,25249,18376,18375,24959,15214,25298,25299,385,25395,25605,25234,25651,25228,25582,24921,25227,25829,25680,25470,62695,25596,31685,25362,26047,25445,18380,18378,18379,32520,26043));

UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`DifficultyID`=0 AND `Entry` IN (27306,68845,28195,129141,26186,26187,26191,26189,26173,26185,26171,26202,26170,25396,26103,25386,25601,25383,26174,25387,26725,24730,26996,26160,25849,24709,26697,26709,26720,26721,24706,24703,26986,24733,26718,24702,25726,25762,25664,25652,24563,25619,25739,24601,25677,25758,32544,25752,25753,24021,23837,25792,25793,25862,62697,25766,25765,25750,25791,18955,25724,25594,25968,25850,24614,24613,25743,62693,25986,26252,25679,29328,26098,25249,18376,18375,24959,15214,25298,25299,385,25395,25605,25234,25651,25228,25582,24921,25227,25829,25680,25470,62695,25596,31685,25362,26047,25445,18380,18378,18379,32520,26043));
UPDATE `creature_template_difficulty` SET `ContentTuningID`=740, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=155076 AND `DifficultyID`=0); -- 155076 (Spell Bunny) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25720 AND `DifficultyID`=0); -- 25720 (Inquisitor Caleras) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=32548 AND `DifficultyID`=0); -- 32548 (Corastrasza) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25716 AND `DifficultyID`=0); -- 25716 (General Cerulean) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25717 AND `DifficultyID`=0); -- 25717 (Coldarra Scalesworn) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=29655 AND `DifficultyID`=0); -- 29655 (Malygos) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=26265 AND `DifficultyID`=0); -- 26265 (Saragosa's End Invisman) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=33087 AND `DifficultyID`=0); -- 33087 (Signal Fire Invisman) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25712 AND `DifficultyID`=0); -- 25712 (Warbringer Goredrak) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25713 AND `DifficultyID`=0); -- 25713 (Blue Drakonid Supplicant) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25721 AND `DifficultyID`=0); -- 25721 (Arcane Serpent) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=32357 AND `DifficultyID`=0); -- 32357 (Old Crystalbark) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=26127 AND `DifficultyID`=0); -- 26127 (Nexus Drake Hatchling) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=28960 AND `DifficultyID`=0); -- 28960 (Totally Generic Bunny (JSB)) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=32534 AND `DifficultyID`=0); -- 32534 (Scalesworn Elite) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=524288, `VerifiedBuild`=54847 WHERE (`Entry`=25711 AND `DifficultyID`=0); -- 25711 (Spirit of the North) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=27906 AND `DifficultyID`=0); -- 27906 (Warmage Hollister) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=27904 AND `DifficultyID`=0); -- 27904 (Warmage Watkins) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=30051 AND `DifficultyID`=0); -- 30051 (Librarian Tiare) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25314 AND `DifficultyID`=0); -- 25314 (Archmage Berinand) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=27173 AND `DifficultyID`=0); -- 27173 (Warmage Calandra) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=881, `VerifiedBuild`=54847 WHERE (`Entry`=27046 AND `DifficultyID`=0); -- 27046 (Warmage Adami) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=26110 AND `DifficultyID`=0); -- 26110 (Librarian Serrah) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=26175 AND `DifficultyID`=0); -- 26175 (Coldarra - Drake Hunt Invisman) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=26117 AND `DifficultyID`=0); -- 26117 (Raelorasz) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25709 AND `DifficultyID`=0); -- 25709 (Glacial Ancient) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25728 AND `DifficultyID`=0); -- 25728 (Coldarra Wyrmkin) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=45396 AND `DifficultyID`=0); -- 45396 (Dave's Industrial Light and Magic Bunny (Medium)(Sessile)(Large AOI)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=24770 AND `DifficultyID`=0); -- 24770 (Nexus Watcher) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25722 AND `DifficultyID`=0); -- 25722 (Coldarra Spellweaver) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25707 AND `DifficultyID`=0); -- 25707 (Magic-bound Ancient) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=26373 AND `DifficultyID`=0); -- 26373 (Coldarra Spell FX InvisMan) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25719 AND `DifficultyID`=0); -- 25719 (Coldarra Spellbinder) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25718 AND `DifficultyID`=0); -- 25718 (Coldarra Mage Slayer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|536871168, `VerifiedBuild`=54847 WHERE (`Entry`=27529 AND `DifficultyID`=0); -- 27529 (Unu'pe Vision - Smoke Target (DND)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|536871168, `VerifiedBuild`=54847 WHERE (`Entry`=27569 AND `DifficultyID`=0); -- 27569 (Unu'pe Vision - Villager Bunny (DND)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=27494 AND `DifficultyID`=0); -- 27494 (Mystic Tomkin) -
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=27193 AND `DifficultyID`=0); -- 27193 (Alornerk) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=256, `VerifiedBuild`=54847 WHERE (`Entry`=27610 AND `DifficultyID`=0); -- 27610 (Dead Caribou) - Sessile
UPDATE `creature_template_difficulty` SET `ContentTuningID`=966, `VerifiedBuild`=54847 WHERE (`Entry`=31805 AND `DifficultyID`=0); -- 31805 (Nagojut) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=26556 AND `DifficultyID`=0); -- 26556 (Agloolik) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=966, `VerifiedBuild`=54847 WHERE (`Entry`=31807 AND `DifficultyID`=0); -- 31807 (Aumanil) - 
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=27195 AND `DifficultyID`=0); -- 27195 (Tarralikitak) - 
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=27190 AND `DifficultyID`=0); -- 27190 (Tupit) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=26403 AND `DifficultyID`=0); -- 26403 (Kaskala Lookout) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26218 AND `DifficultyID`=0); -- 26218 (Elder Muahit) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=28382 AND `DifficultyID`=0); -- 28382 (Hotawa) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435712, `VerifiedBuild`=54847 WHERE (`Entry`=26219 AND `DifficultyID`=0); -- 26219 (Iruk) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26169 AND `DifficultyID`=0); -- 26169 (Ataika) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26213 AND `DifficultyID`=0); -- 26213 (Utaik) - CanSwim
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=27194 AND `DifficultyID`=0); -- 27194 (Trapper Saghani) - 
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=27566 AND `DifficultyID`=0); -- 27566 (Unu'pe Villager) - 
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=27187 AND `DifficultyID`=0); -- 27187 (Caregiver Poallu) - 
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=27186 AND `DifficultyID`=0); -- 27186 (Oogrooq) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=27189 AND `DifficultyID`=0); -- 27189 (Unu'pe Spearman) - CanSwim
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=29965 AND `DifficultyID`=0); -- 29965 (Unu'pe Turtle Rider) - 
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=25514 AND `DifficultyID`=0); -- 25514 (Rocknar) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|805306368, `VerifiedBuild`=54847 WHERE (`Entry`=71163 AND `DifficultyID`=0); -- 71163 (Unborn Val'kyr) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=256, `VerifiedBuild`=54847 WHERE (`Entry`=26172 AND `DifficultyID`=0); -- 26172 (Slain Cultist) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25715 AND `DifficultyID`=0); -- 25715 (Frozen Elemental) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=26162 AND `DifficultyID`=0); -- 26162 (Transborea Generator 001) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=32361 AND `DifficultyID`=0); -- 32361 (Icehorn) - CanSwim
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=25668 AND `DifficultyID`=0); -- 25668 (Vengeful Taunka Spirit) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27860 AND `DifficultyID`=0); -- 27860 (Luthion the Vile) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25422 AND `DifficultyID`=0); -- 25422 (Mystical Webbing) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|536870912, `VerifiedBuild`=54847 WHERE (`Entry`=26073 AND `DifficultyID`=0); -- 26073 (High Priest Talet-Kha) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25473 AND `DifficultyID`=0); -- 25473 (Temple C) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=26115 AND `DifficultyID`=0); -- 26115 (Darkfallen Bloodbearer) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25392 AND `DifficultyID`=0); -- 25392 (High Priest Andorath) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25516 AND `DifficultyID`=0); -- 25516 (Snow Tracker Grumm) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25534 AND `DifficultyID`=0); -- 25534 (En'kilah Blood Globe) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25472 AND `DifficultyID`=0); -- 25472 (Temple B) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=26076 AND `DifficultyID`=0); -- 26076 (High Priest Naferset) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25493 AND `DifficultyID`=0); -- 25493 (West En'kilah Cauldron) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25492 AND `DifficultyID`=0); -- 25492 (Central En'kilah Cauldron) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25471 AND `DifficultyID`=0); -- 25471 (Temple A) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25391 AND `DifficultyID`=0); -- 25391 (En'kilah Focus Crystal) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27859 AND `DifficultyID`=0); -- 27859 (Vanthryn the Merciless) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25378 AND `DifficultyID`=0); -- 25378 (En'kilah Necromancer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25490 AND `DifficultyID`=0); -- 25490 (East En'kilah Cauldron) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25390 AND `DifficultyID`=0); -- 25390 (En'kilah Hatchling) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=26093 AND `DifficultyID`=0); -- 26093 (Naxxanar Target) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25377 AND `DifficultyID`=0); -- 25377 (Brittle Skeleton) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=26094 AND `DifficultyID`=0); -- 26094 (Naxxanar Caster) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25393 AND `DifficultyID`=0); -- 25393 (En'kilah Ghoul) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25725 AND `DifficultyID`=0); -- 25725 (Chieftain Gurgleboggle) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25604 AND `DifficultyID`=0); -- 25604 (Sage Highmesa) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25606 AND `DifficultyID`=0); -- 25606 (Hamat) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25454 AND `DifficultyID`=0); -- 25454 (Tundra Crawler) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25705 AND `DifficultyID`=0); -- 25705 (Bixie Wrenchshanker) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25615 AND `DifficultyID`=0); -- 25615 (Plagued Magnataur) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25452 AND `DifficultyID`=0); -- 25452 (Scourged Mammoth) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25682 AND `DifficultyID`=0); -- 25682 (Lich-Lord Chillwinter) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|805306624, `VerifiedBuild`=54847 WHERE (`Entry`=25658 AND `DifficultyID`=0); -- 25658 (Longrunner Bristlehorn) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25678 AND `DifficultyID`=0); -- 25678 (Doctor Razorgrin) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25714 AND `DifficultyID`=0); -- 25714 (Tinky Wickwhistle) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25684 AND `DifficultyID`=0); -- 25684 (Talramas Abomination) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25983 AND `DifficultyID`=0); -- 25983 (Dorain Frosthoof) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26158 AND `DifficultyID`=0); -- 26158 (Mother Tauranook) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25602 AND `DifficultyID`=0); -- 25602 (Greatmother Taiga) - CanSwim
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=26847 AND `DifficultyID`=0); -- 26847 (Omu Spiritbreeze) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26159 AND `DifficultyID`=0); -- 26159 (Taunka'le Evacuee) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25982 AND `DifficultyID`=0); -- 25982 (Sage Earth and Sky) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26157 AND `DifficultyID`=0); -- 26157 (Taunka'le Brave) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26104 AND `DifficultyID`=0); -- 26104 (Iron Eyes) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=26048 AND `DifficultyID`=0); -- 26048 (Storm Totem) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=26041 AND `DifficultyID`=0); -- 26041 (Lightning Target) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25700 AND `DifficultyID`=0); -- 25700 (Gorloc Hunter) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25701 AND `DifficultyID`=0); -- 25701 (Gorloc Dredger) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25699 AND `DifficultyID`=0); -- 25699 (Gorloc Mud Splasher) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25786 AND `DifficultyID`=0); -- 25786 (Friar Tuskus) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26790 AND `DifficultyID`=0); -- 26790 (Taunka'le Longrunner) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26266 AND `DifficultyID`=0); -- 26266 (Heigarr the Horrible) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25760 AND `DifficultyID`=0); -- 25760 (Kvaldir Raider) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25764 AND `DifficultyID`=0); -- 25764 (Kaskala Defender) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25685 AND `DifficultyID`=0); -- 25685 (Gorloc Waddler) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25984 AND `DifficultyID`=0); -- 25984 (Crashed Recon Pilot) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25807 AND `DifficultyID`=0); -- 25807 (Iggy "Tailspin" Cogtoggle) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=27999 AND `DifficultyID`=0); -- 27999 (Rock Shield Trigger (Borean Tundra)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26766 AND `DifficultyID`=0); -- 26766 (Brave Storming Sky) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26768 AND `DifficultyID`=0); -- 26768 (Snow Tracker Haloke) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26767 AND `DifficultyID`=0); -- 26767 (Longrunner Taima) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25665 AND `DifficultyID`=0); -- 25665 (Northeast Sinkhole) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25666 AND `DifficultyID`=0); -- 25666 (Northwest Sinkhole) - Floating
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=25622 AND `DifficultyID`=0); -- 25622 (Nerub'ar Tunneler) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=24957 AND `DifficultyID`=0); -- 24957 (Cult Plaguebringer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25670 AND `DifficultyID`=0); -- 25670 (ELM General Purpose Bunny (scale x3)) - Floating
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=25660 AND `DifficultyID`=0); -- 25660 (Festering Ghoul) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=26126 AND `DifficultyID`=0); -- 26126 (Bone Warrior) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=19871 AND `DifficultyID`=0); -- 19871 (World Trigger (Not Immune NPC)) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25814 AND `DifficultyID`=0); -- 25814 (Fizzcrank Mechagnome) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25425 AND `DifficultyID`=0); -- 25425 (Farseer Grimwalker's Spirit) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25427 AND `DifficultyID`=0); -- 25427 (Kaganishu) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25430 AND `DifficultyID`=0); -- 25430 (Magmothregar) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=`StaticFlags1`|805306624, `VerifiedBuild`=54847 WHERE (`Entry`=25589 AND `DifficultyID`=0); -- 25589 (Bonker Togglevolt) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`Entry`=21757 AND `DifficultyID`=0); -- 21757 (Big Electromental Flavor) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25477 AND `DifficultyID`=0); -- 25477 (Crafty Wobblesprocket) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25702 AND `DifficultyID`=0); -- 25702 (Mordle Cogspinner) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26619 AND `DifficultyID`=0); -- 26619 (Fizzcrank Paratrooper) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26598 AND `DifficultyID`=0); -- 26598 (Mistie Flitterdawn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|805306624, `VerifiedBuild`=54847 WHERE (`Entry`=25747 AND `DifficultyID`=0); -- 25747 (Jinky Wingnut) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25590 AND `DifficultyID`=0); -- 25590 (Fizzcrank Fullthrottle) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26599 AND `DifficultyID`=0); -- 26599 (Willis Wobblewheel) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26596 AND `DifficultyID`=0); -- 26596 ("Charlie" Northtop) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26597 AND `DifficultyID`=0); -- 26597 (Toby "Mother Goose" Ironbolt) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25767 AND `DifficultyID`=0); -- 25767 (Fizzcrank Pilot) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=`StaticFlags1`|268435456, `VerifiedBuild`=54847 WHERE (`Entry`=26634 AND `DifficultyID`=0); -- 26634 (Fizzcrank Watcher Rupert Keeneye) - CanSwim
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54847 WHERE (`DifficultyID`=0 AND `Entry` IN (31790,31788,31789,31793,31791,31792,25650,25623,26549,25307,25222,35133,35131,47576,25816,25825,25468,61158,25467,60761,25611,25600,25609,25610,24562,25294,26538,27432,47577,35135,35132,31724,31725,31723,31720,24566));
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25434 AND `DifficultyID`=0); -- 25434 (Magmoth Crusher) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25416 AND `DifficultyID`=0); -- 25416 (Simmer) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25429 AND `DifficultyID`=0); -- 25429 (Magmoth Forager) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25432 AND `DifficultyID`=0); -- 25432 (Mate of Magmothregar) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25433 AND `DifficultyID`=0); -- 25433 (Offspring of Magmothregar) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25428 AND `DifficultyID`=0); -- 25428 (Magmoth Shaman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25444 AND `DifficultyID`=0); -- 25444 (Magmoth Fire Totem) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=881, `VerifiedBuild`=54847 WHERE (`Entry`=26848 AND `DifficultyID`=0); -- 26848 (Kimbiza) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25326 AND `DifficultyID`=0); -- 25326 (Overlord Bor'gorok) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25736 AND `DifficultyID`=0); -- 25736 (Supply Master Taz'ishi) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27074 AND `DifficultyID`=0); -- 27074 (Bor'gorok Wolf) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27065 AND `DifficultyID`=0); -- 27065 (Breka Wolfsister) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27058 AND `DifficultyID`=0); -- 27058 (Korag Keeneye) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25339 AND `DifficultyID`=0); -- 25339 (Spirit Talker Snarlfang) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25455 AND `DifficultyID`=0); -- 25455 (Snarlfang's Totem) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27069 AND `DifficultyID`=0); -- 27069 (Matron Magah) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27067 AND `DifficultyID`=0); -- 27067 (Drigoth) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27063 AND `DifficultyID`=0); -- 27063 (Vrok) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=805830912, `VerifiedBuild`=54847 WHERE (`Entry`=25374 AND `DifficultyID`=0); -- 25374 (Ortrosh) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2110, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27073 AND `DifficultyID`=0); -- 27073 (Bor'gorok Battleguard) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26112 AND `DifficultyID`=0); -- 26112 (Bor'gorok Peon) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25376 AND `DifficultyID`=0); -- 25376 (Imperean) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25199 AND `DifficultyID`=0); -- 25199 (Brglmurgl) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=34381 AND `DifficultyID`=0); -- 34381 ([DND]Northrend Children's Week Trigger) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25211 AND `DifficultyID`=0); -- 25211 (Cleaver Bmurglbrm) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25205 AND `DifficultyID`=0); -- 25205 (Mrmrglmr) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25415 AND `DifficultyID`=0); -- 25415 (Enraged Tempest) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268697600, `VerifiedBuild`=54847 WHERE (`Entry`=25203 AND `DifficultyID`=0); -- 25203 (Glrggl) - Aquatic, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25209 AND `DifficultyID`=0); -- 25209 (Claximus) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25208 AND `DifficultyID`=0); -- 25208 (Lurgglbr) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=28375 AND `DifficultyID`=0); -- 28375 (Glrglrglr) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25210 AND `DifficultyID`=0); -- 25210 (Keymaster Urmgrgl) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=24288 AND `DifficultyID`=0); -- 24288 (ELM General Purpose Bunny Hide Body) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=25309 AND `DifficultyID`=0); -- 25309 (Borean - Westrift Cavern Anomaly) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25216 AND `DifficultyID`=0); -- 25216 (Winterfin Oracle) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25201 AND `DifficultyID`=0); -- 25201 (Winterfin Tadpole) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25217 AND `DifficultyID`=0); -- 25217 (Winterfin Warrior) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25206 AND `DifficultyID`=0); -- 25206 (Ahlurglgr) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268697600, `VerifiedBuild`=54847 WHERE (`Entry`=25226 AND `DifficultyID`=0); -- 25226 (Scalder) - Aquatic, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25197 AND `DifficultyID`=0); -- 25197 (King Mrgl-Mrgl) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25847 AND `DifficultyID`=0); -- 25847 (East Crash) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25845 AND `DifficultyID`=0); -- 25845 (Northwest Crash) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25781 AND `DifficultyID`=0); -- 25781 (Oil Pool) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25780 AND `DifficultyID`=0); -- 25780 (Abner Fizzletorque) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26602 AND `DifficultyID`=0); -- 26602 (Kara Thricestar) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26600 AND `DifficultyID`=0); -- 26600 (Chief Engineer Galpen Rolltie) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26817 AND `DifficultyID`=0); -- 26817 (Fizzcrank Fighter) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26645 AND `DifficultyID`=0); -- 26645 (Fizzcrank Engineering Crew) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26601 AND `DifficultyID`=0); -- 26601 (Fizzcrank Airman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=26804 AND `DifficultyID`=0); -- 26804 (Fizzcrank Bomber Invisible Bunny) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25881 AND `DifficultyID`=0); -- 25881 (Moria) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25748 AND `DifficultyID`=0); -- 25748 (Oil-covered Hawk) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25817 AND `DifficultyID`=0); -- 25817 (Oiled Fledgeling) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25417 AND `DifficultyID`=0); -- 25417 (Raging Boiler) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25418 AND `DifficultyID`=0); -- 25418 (Churn) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268697600, `VerifiedBuild`=54847 WHERE (`Entry`=25204 AND `DifficultyID`=0); -- 25204 (Glimmer Bay Orca) - Aquatic, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25419 AND `DifficultyID`=0); -- 25419 (Boiling Spirit) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25198 AND `DifficultyID`=0); -- 25198 (Winterfin Gatherer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25215 AND `DifficultyID`=0); -- 25215 (Winterfin Shorestriker) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25336 AND `DifficultyID`=0); -- 25336 (Grunt Ragefist) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25338 AND `DifficultyID`=0); -- 25338 (Warsong Caravan Guard) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25335 AND `DifficultyID`=0); -- 25335 (Longrunner Proudhoof) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=25308 AND `DifficultyID`=0); -- 25308 (Borean - Westrift Chasm Anomaly) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25384 AND `DifficultyID`=0); -- 25384 (Steeljaw's Corpse) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=805830912, `VerifiedBuild`=54847 WHERE (`Entry`=25343 AND `DifficultyID`=0); -- 25343 (Dead Caravan Worker) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25802 AND `DifficultyID`=0); -- 25802 (Kaw the Mammoth Destroyer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25880 AND `DifficultyID`=0); -- 25880 (Minion of Kaw) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25351 AND `DifficultyID`=0); -- 25351 (Ghostly Sage) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=805830912, `VerifiedBuild`=54847 WHERE (`Entry`=25342 AND `DifficultyID`=0); -- 25342 (Dead Caravan Guard) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25350 AND `DifficultyID`=0); -- 25350 (Risen Longrunner) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25441 AND `DifficultyID`=0); -- 25441 (North Platform) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25443 AND `DifficultyID`=0); -- 25443 (West Platform) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25841 AND `DifficultyID`=0); -- 25841 (Fizzcrank Recon Pilot) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25846 AND `DifficultyID`=0); -- 25846 (South Crash) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25810 AND `DifficultyID`=0); -- 25810 (Hierophant Cenius) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25812 AND `DifficultyID`=0); -- 25812 (Killinger the Den Watcher) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25809 AND `DifficultyID`=0); -- 25809 (Archdruid Lathorius) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=805830656, `VerifiedBuild`=54847 WHERE (`Entry`=25819 AND `DifficultyID`=0); -- 25819 (D.E.H.T.A. Enforcer) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25811 AND `DifficultyID`=0); -- 25811 (Zaza) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25808 AND `DifficultyID`=0); -- 25808 (D.E.H.T.A. Enforcer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25292 AND `DifficultyID`=0); -- 25292 (Etaruk) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25442 AND `DifficultyID`=0); -- 25442 (East Platform) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25321 AND `DifficultyID`=0); -- 25321 (Kaskala Craftsman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25322 AND `DifficultyID`=0); -- 25322 (Kaskala Shaman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25448 AND `DifficultyID`=0); -- 25448 (Curator Insivius) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25355 AND `DifficultyID`=0); -- 25355 (Beryl Hound) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25353 AND `DifficultyID`=0); -- 25353 (Beryl Treasure Hunter) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25449 AND `DifficultyID`=0); -- 25449 (Beryl Reclaimer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25431 AND `DifficultyID`=0); -- 25431 (Kaskala Ancestor) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=24862 AND `DifficultyID`=0); -- 24862 (Mage Hunter Target) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27271 AND `DifficultyID`=0); -- 27271 (Initiate Knapp) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27301 AND `DifficultyID`=0); -- 27301 (Apprentice Trotter) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27298 AND `DifficultyID`=0); -- 27298 (Apprentice Ranch) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27147 AND `DifficultyID`=0); -- 27147 (Librarian Erickson) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27138 AND `DifficultyID`=0); -- 27138 (Apprentice Rosen) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27139 AND `DifficultyID`=0); -- 27139 (Librarian Whitley) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27137 AND `DifficultyID`=0); -- 27137 (Apprentice Fraser) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27143 AND `DifficultyID`=0); -- 27143 (Librarian Ingram) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25480 AND `DifficultyID`=0); -- 25480 (Librarian Normantis) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27291 AND `DifficultyID`=0); -- 27291 (Initiate Mehrtens) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27302 AND `DifficultyID`=0); -- 27302 (Initiate Park) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27300 AND `DifficultyID`=0); -- 27300 (Initiate Vernon) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27299 AND `DifficultyID`=0); -- 27299 (Initiate Greer) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27142 AND `DifficultyID`=0); -- 27142 (Librarian Jeffers) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27140 AND `DifficultyID`=0); -- 27140 (Librarian Andersen) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=27141 AND `DifficultyID`=0); -- 27141 (Librarian Hamilton) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25262 AND `DifficultyID`=0); -- 25262 (Librarian Donathan) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25291 AND `DifficultyID`=0); -- 25291 (Librarian Garren) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=881, `VerifiedBuild`=54847 WHERE (`Entry`=24795 AND `DifficultyID`=0); -- 24795 (Surristrasz) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=524288, `VerifiedBuild`=54847 WHERE (`Entry`=25364 AND `DifficultyID`=0); -- 25364 (Red Guardian Drake) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25584 AND `DifficultyID`=0); -- 25584 (Inquisitor Salrand) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25316 AND `DifficultyID`=0); -- 25316 (Beryl Sorcerer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25585 AND `DifficultyID`=0); -- 25585 (Beryl Mage Hunter) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=54242 AND `DifficultyID`=0); -- 54242 (Borean Beam Target) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=25732 AND `DifficultyID`=0); -- 25732 (Warmage Preston) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=25727 AND `DifficultyID`=0); -- 25727 (Warmage Moran) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=36155 AND `DifficultyID`=0); -- 36155 (Anzim Controller Bunny) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25356 AND `DifficultyID`=0); -- 25356 (Warmage Anzim) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=27888 AND `DifficultyID`=0); -- 27888 (Warmage Archus) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=25733 AND `DifficultyID`=0); -- 25733 (Warmage Austin) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=805830912, `VerifiedBuild`=54847 WHERE (`Entry`=25801 AND `DifficultyID`=0); -- 25801 (Nedar, Lord of Rhinos) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25995 AND `DifficultyID`=0); -- 25995 (Stampede Exit Point) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25804 AND `DifficultyID`=0); -- 25804 (Harold Lane) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25806 AND `DifficultyID`=0); -- 25806 (Loot Crazed Poacher) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=805830656, `VerifiedBuild`=54847 WHERE (`Entry`=25838 AND `DifficultyID`=0); -- 25838 (Hierophant Liandra) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25687 AND `DifficultyID`=0); -- 25687 (Gorloc Steam Belcher) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25686 AND `DifficultyID`=0); -- 25686 (Gorloc Gibberer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=256, `VerifiedBuild`=54847 WHERE (`Entry`=25385 AND `DifficultyID`=0); -- 25385 (William Allerton) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25803 AND `DifficultyID`=0); -- 25803 (Karen "I Don't Caribou" the Culler) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25981 AND `DifficultyID`=0); -- 25981 (Scourged Footman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=26161 AND `DifficultyID`=0); -- 26161 (Farshire Grain Credit) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26084 AND `DifficultyID`=0); -- 26084 (Jeremiah Hawning) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26083 AND `DifficultyID`=0); -- 26083 (Gerald Green) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26085 AND `DifficultyID`=0); -- 26085 (Wendy Darren) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25839 AND `DifficultyID`=0); -- 25839 (Northsea Mercenary) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25843 AND `DifficultyID`=0); -- 25843 (Northsea Thug) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25617 AND `DifficultyID`=0); -- 25617 (Farshire Militia) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `LevelScalingDeltaMax`=-1, `StaticFlags1`=268697600, `VerifiedBuild`=54847 WHERE (`Entry`=24637 AND `DifficultyID`=0); -- 24637 (Great Reef Shark) - Aquatic, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25800 AND `DifficultyID`=0); -- 25800 (Clam Master K) - Amphibious
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27393 AND `DifficultyID`=0); -- 27393 (Valiance Keep Fisherman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25281 AND `DifficultyID`=0); -- 25281 (Heldgarr Steelbeard) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25282 AND `DifficultyID`=0); -- 25282 (Ferlynn Windsong) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25297 AND `DifficultyID`=0); -- 25297 (Drill Dummy) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25826 AND `DifficultyID`=0); -- 25826 (Vindicator Yaala) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25248 AND `DifficultyID`=0); -- 25248 ("Salty" John Thorpe) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27011 AND `DifficultyID`=0); -- 27011 (Broff Bombast) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26999 AND `DifficultyID`=0); -- 26999 (Fendrig Redbeard) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25239 AND `DifficultyID`=0); -- 25239 (Thulrin) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25235 AND `DifficultyID`=0); -- 25235 (Hilda Stoneforge) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=32564 AND `DifficultyID`=0); -- 32564 (Logistics Officer Silverstone) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25240 AND `DifficultyID`=0); -- 25240 (Bromm) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26992 AND `DifficultyID`=0); -- 26992 (Brynna Wilson) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25238 AND `DifficultyID`=0); -- 25238 (Gamlen) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25241 AND `DifficultyID`=0); -- 25241 (Gelrynd) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26988 AND `DifficultyID`=0); -- 26988 (Argo Strongstout) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25302 AND `DifficultyID`=0); -- 25302 (Old Man Colburn) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25271 AND `DifficultyID`=0); -- 25271 (Valiance Keep Worker) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25827 AND `DifficultyID`=0); -- 25827 (Tom Hegger) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25251 AND `DifficultyID`=0); -- 25251 (Leryssa) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26879 AND `DifficultyID`=0); -- 26879 (Tomas Riverwell) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=24357 AND `DifficultyID`=0); -- 24357 (Maethor Skyshadow) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27010 AND `DifficultyID`=0); -- 27010 (Celidh Aletracker) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25245 AND `DifficultyID`=0); -- 25245 (James Deacon) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=29158 AND `DifficultyID`=0); -- 29158 (Magister Dath'omere) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25737 AND `DifficultyID`=0); -- 25737 (Airman Skyhopper) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25219 AND `DifficultyID`=0); -- 25219 (Borean Queue Trigger LAB) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26989 AND `DifficultyID`=0); -- 26989 (Rollick MacKreel) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27364 AND `DifficultyID`=0); -- 27364 (Jimmy the Stable Boy) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27365 AND `DifficultyID`=0); -- 27365 (Stabled Horse) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27012 AND `DifficultyID`=0); -- 27012 (Beem Goldsprocket) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26991 AND `DifficultyID`=0); -- 26991 (Sock Brightbolt) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26987 AND `DifficultyID`=0); -- 26987 (Falorn Nightwhisper) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26994 AND `DifficultyID`=0); -- 26994 (Kirea Moondancer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26993 AND `DifficultyID`=0); -- 26993 (Old Man Robert) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25301 AND `DifficultyID`=0); -- 25301 (Counselor Talbot) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25250 AND `DifficultyID`=0); -- 25250 (General Arlos) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25285 AND `DifficultyID`=0); -- 25285 (Harbinger Vurenn) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26990 AND `DifficultyID`=0); -- 26990 (Alexis Marlowe) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27385 AND `DifficultyID`=0); -- 27385 (Ronald Anderson) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27000 AND `DifficultyID`=0); -- 27000 (Trapper Jack) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27001 AND `DifficultyID`=0); -- 27001 (Darin Goodstitch) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26998 AND `DifficultyID`=0); -- 26998 (Rosemary Bovard) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26997 AND `DifficultyID`=0); -- 26997 (Alestos) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26995 AND `DifficultyID`=0); -- 26995 (Tink Brightbolt) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25828 AND `DifficultyID`=0); -- 25828 (Guard Mitchells) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25300 AND `DifficultyID`=0); -- 25300 (Draenei Delegate) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25317 AND `DifficultyID`=0); -- 25317 (Civilian Recruit) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25220 AND `DifficultyID`=0); -- 25220 (Civilian Recruit) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26155 AND `DifficultyID`=0); -- 26155 (Mark Hanes) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=29617 AND `DifficultyID`=0); -- 29617 (Valiance Keep Defender) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=26249 AND `DifficultyID`=0); -- 26249 (Southern Sinkhole Kill Credit) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=26248 AND `DifficultyID`=0); -- 26248 (Northern Sinkhole Kill Credit) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25264 AND `DifficultyID`=0); -- 25264 (Justicar Julia Celeste) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25253 AND `DifficultyID`=0); -- 25253 (Valiance Keep Footman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25306 AND `DifficultyID`=0); -- 25306 (Valiance Keep Cannoneer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25311 AND `DifficultyID`=0); -- 25311 (Valiance Keep Rifleman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=24042 AND `DifficultyID`=0); -- 24042 (Generic Trigger LAB - OLD) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25313 AND `DifficultyID`=0); -- 25313 (Valiance Keep Footman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=256, `VerifiedBuild`=54847 WHERE (`Entry`=24580 AND `DifficultyID`=0); -- 24580 (Riplash Siren) - Sessile
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26451 AND `DifficultyID`=0); -- 26451 (Ragnar Drakkarlund) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25523 AND `DifficultyID`=0); -- 25523 (Skadir Mariner) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25520 AND `DifficultyID`=0); -- 25520 (Skadir Runecaster) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26452 AND `DifficultyID`=0); -- 26452 (Leviroth) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25450 AND `DifficultyID`=0); -- 25450 (Veehja) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=256, `VerifiedBuild`=54847 WHERE (`Entry`=24662 AND `DifficultyID`=0); -- 24662 (Riplash Sorceress) - Sessile
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=24576 AND `DifficultyID`=0); -- 24576 (Riplash Myrmidon) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26449 AND `DifficultyID`=0); -- 26449 (Gamel the Cruel) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25613 AND `DifficultyID`=0); -- 25613 (Skadir Mistweaver) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25522 AND `DifficultyID`=0); -- 25522 (Skadir Raider) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25636 AND `DifficultyID`=0); -- 25636 (Captured Tuskarr Prisoner) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25521 AND `DifficultyID`=0); -- 25521 (Skadir Longboatsman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25435 AND `DifficultyID`=0); -- 25435 (Karuk) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25498 AND `DifficultyID`=0); -- 25498 (Aspatha the Broodmother) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268960000, `VerifiedBuild`=54847 WHERE (`Entry`=25797 AND `DifficultyID`=0); -- 25797 (Bloodmage Alkor) - Sessile, Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25501 AND `DifficultyID`=0); -- 25501 (Gammoth Tender) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25380 AND `DifficultyID`=0); -- 25380 (Primal Mighthorn) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25381 AND `DifficultyID`=0); -- 25381 (Bloodmage Laurith) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25440 AND `DifficultyID`=0); -- 25440 (Scout Tungok) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25836 AND `DifficultyID`=0); -- 25836 (Loot Crazed Diver) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25979 AND `DifficultyID`=0); -- 25979 (Loot Crazed Hunter) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25789 AND `DifficultyID`=0); -- 25789 (Gammothra the Tormentor) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=24567 AND `DifficultyID`=0); -- 24567 (Den Vermin) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=24469 AND `DifficultyID`=0); -- 24469 (Magnataur Huntress) - Amphibious
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=25464 AND `DifficultyID`=0); -- 25464 (Bloodspore Moth) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25774 AND `DifficultyID`=0); -- 25774 (Rockfang) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25332 AND `DifficultyID`=0); -- 25332 (Stitched Warsong Horror) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25675 AND `DifficultyID`=0); -- 25675 (Tundra Wolf) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=25465 AND `DifficultyID`=0); -- 25465 (Kel'Thuzad) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25466 AND `DifficultyID`=0); -- 25466 (Necropolis Beam (Target)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25469 AND `DifficultyID`=0); -- 25469 (Mindless Aberration) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25513 AND `DifficultyID`=0); -- 25513 (4th Kvaldir Vessel (Bor's Anvil)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2523, `StaticFlags1`=268435456, `VerifiedBuild`=54847 WHERE (`Entry`=208182 AND `DifficultyID`=0); -- 208182 (Crazed Looter) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=28440 AND `DifficultyID`=0); -- 28440 (Tundra Penguin) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=0, `VerifiedBuild`=54847 WHERE (`Entry`=25310 AND `DifficultyID`=0); -- 25310 (Borean - Westrift Cleftcliff Anomaly) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25510 AND `DifficultyID`=0); -- 25510 (1st Kvaldir Vessel (The Serpent's Maw)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25511 AND `DifficultyID`=0); -- 25511 (2nd Kvaldir Vessel (The Kur Drakkar)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25512 AND `DifficultyID`=0); -- 25512 (3rd Kvaldir Vessel (Bor's Hammer)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25479 AND `DifficultyID`=0); -- 25479 (Kvaldir Mistweaver) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25496 AND `DifficultyID`=0); -- 25496 (Kvaldir Mist Lord) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25504 AND `DifficultyID`=0); -- 25504 (Mootoo the Younger) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25475 AND `DifficultyID`=0); -- 25475 (Mobu) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25503 AND `DifficultyID`=0); -- 25503 (Elder Mootoo) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25481 AND `DifficultyID`=0); -- 25481 (Landing Crawler) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25329 AND `DifficultyID`=0); -- 25329 (Annihilator Grek'lor) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25482 AND `DifficultyID`=0); -- 25482 (Sand Turtle) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25476 AND `DifficultyID`=0); -- 25476 (Waltor of Pal'ea) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25334 AND `DifficultyID`=0); -- 25334 (Horde Siege Tank) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27107 AND `DifficultyID`=0); -- 27107 (Injured Warsong Mage) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27110 AND `DifficultyID`=0); -- 27110 (Injured Warsong Engineer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25333 AND `DifficultyID`=0); -- 25333 (Undying Aggressor) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=256, `VerifiedBuild`=54847 WHERE (`Entry`=28150 AND `DifficultyID`=0); -- 28150 (Destroyed Siege Tank) - Sessile
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=27064 AND `DifficultyID`=0); -- 27064 (Abandoned Fuel Tank) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27106 AND `DifficultyID`=0); -- 27106 (Injured Warsong Warrior) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27108 AND `DifficultyID`=0); -- 27108 (Injured Warsong Shaman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25349 AND `DifficultyID`=0); -- 25349 (Scourge Plague Spreader) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25669 AND `DifficultyID`=0); -- 25669 (Warsong Grainery Credit) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25671 AND `DifficultyID`=0); -- 25671 (Torp's Farm Credit) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25625 AND `DifficultyID`=0); -- 25625 (Warsong Aberration) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25618 AND `DifficultyID`=0); -- 25618 (Varidus the Flenser) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25624 AND `DifficultyID`=0); -- 25624 (Infested Prisoner) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54847 WHERE (`Entry`=24771 AND `DifficultyID`=0); -- 24771 (Coldarra Invisman) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `VerifiedBuild`=54847 WHERE (`Entry`=25655 AND `DifficultyID`=0); -- 25655 (Bane) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25672 AND `DifficultyID`=0); -- 25672 (Warsong Slaughterhouse Credit) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25489 AND `DifficultyID`=0); -- 25489 (Wooly Rhino Bull) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25487 AND `DifficultyID`=0); -- 25487 (Wooly Rhino Matriarch) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25421 AND `DifficultyID`=0); -- 25421 (Warsong Hold Shaman) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25414 AND `DifficultyID`=0); -- 25414 (Warsong Hold Warrior) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25607 AND `DifficultyID`=0); -- 25607 (Farmer Torp) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25438 AND `DifficultyID`=0); -- 25438 (Shadowstalker Canarius) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25439 AND `DifficultyID`=0); -- 25439 (Warsong Scout) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25437 AND `DifficultyID`=0); -- 25437 (Shadowstalker Ickoris) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25426 AND `DifficultyID`=0); -- 25426 (Ug'thor Bloodfrenzy) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25379 AND `DifficultyID`=0); -- 25379 (Warden Nork Bloodfrenzy) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25327 AND `DifficultyID`=0); -- 25327 (Quartermaster Holgoth) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25279 AND `DifficultyID`=0); -- 25279 (Overlord Razgor) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25403 AND `DifficultyID`=0); -- 25403 (Nerub'ar Sinkhole (East)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25394 AND `DifficultyID`=0); -- 25394 (Shadowstalker Barthus) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25446 AND `DifficultyID`=0); -- 25446 (Warsong Captain) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25402 AND `DifficultyID`=0); -- 25402 (Nerub'ar Sinkhole (South)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25288 AND `DifficultyID`=0); -- 25288 (Turida Coldwind) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268960000, `VerifiedBuild`=54847 WHERE (`Entry`=25328 AND `DifficultyID`=0); -- 25328 (Shadowstalker Luther) - Sessile, Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25404 AND `DifficultyID`=0); -- 25404 (Nerub'ar Sinkhole (West)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25488 AND `DifficultyID`=0); -- 25488 (Wooly Rhino Calf) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25405 AND `DifficultyID`=0); -- 25405 (Nerub'ar Sinkhole (North)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25361 AND `DifficultyID`=0); -- 25361 (Alliance Deserter) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25280 AND `DifficultyID`=0); -- 25280 (Foreman Mortuus) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=32565 AND `DifficultyID`=0); -- 32565 (Gara Skullcrush) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27440 AND `DifficultyID`=0); -- 27440 (Argah) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=27441 AND `DifficultyID`=0); -- 27441 (Sagai) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25289 AND `DifficultyID`=0); -- 25289 (Wind Master To'bor) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=32474 AND `DifficultyID`=0); -- 32474 (Fishy Ser'ji) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=524544, `VerifiedBuild`=54847 WHERE (`Entry`=25075 AND `DifficultyID`=0); -- 25075 (Zeppelin Controls) - Sessile, Amphibious
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25247 AND `DifficultyID`=0); -- 25247 (Endorah) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25237 AND `DifficultyID`=0); -- 25237 (Garrosh Hellscream) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26044 AND `DifficultyID`=0); -- 26044 (Durkot Wolfbrother) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25278 AND `DifficultyID`=0); -- 25278 (Williamson) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25274 AND `DifficultyID`=0); -- 25274 (Armorer Orkuruk) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25272 AND `DifficultyID`=0); -- 25272 (Sauranok the Mystic) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25269 AND `DifficultyID`=0); -- 25269 (Horde Zeppelin (Northrend)) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26109 AND `DifficultyID`=0); -- 26109 (Off-duty Battleguard) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25978 AND `DifficultyID`=0); -- 25978 (Ambassador Talonga) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=44, `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26078 AND `DifficultyID`=0); -- 26078 (Blakelow) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25273 AND `DifficultyID`=0); -- 25273 (Warsong Recruitment Officer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=25284 AND `DifficultyID`=0); -- 25284 (Nerub'ar Victim) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25286 AND `DifficultyID`=0); -- 25286 (Warsong Wind Rider) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=31726 AND `DifficultyID`=0); -- 31726 (Grunt Gritch) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=31727 AND `DifficultyID`=0); -- 31727 (Grunt Grikee) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25256 AND `DifficultyID`=0); -- 25256 (High Overlord Saurfang) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25277 AND `DifficultyID`=0); -- 25277 (Chief Engineer Leveny) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26945 AND `DifficultyID`=0); -- 26945 (Zend'li Venomtusk) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26974 AND `DifficultyID`=0); -- 26974 (Tansy Wildmane) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26975 AND `DifficultyID`=0); -- 26975 (Arthur Henslowe) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26972 AND `DifficultyID`=0); -- 26972 (Orn Tenderhoof) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=29155 AND `DifficultyID`=0); -- 29155 (Magistrix Kaelana) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26968 AND `DifficultyID`=0); -- 26968 (Drikka) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26969 AND `DifficultyID`=0); -- 26969 (Raenah) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=29233 AND `DifficultyID`=0); -- 29233 (Nurse Applewood) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26982 AND `DifficultyID`=0); -- 26982 (Geba'li) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26980 AND `DifficultyID`=0); -- 26980 (Eorain Dawnstrike) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26981 AND `DifficultyID`=0); -- 26981 (Crog Steelspine) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26976 AND `DifficultyID`=0); -- 26976 (Brunna Ironaxe) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26977 AND `DifficultyID`=0); -- 26977 (Adelene Sunlance) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536871168, `VerifiedBuild`=54847 WHERE (`Entry`=32647 AND `DifficultyID`=0); -- 32647 (Warsong Hold Practice Dummy) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25242 AND `DifficultyID`=0); -- 25242 (Warsong Battleguard) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268435456, `VerifiedBuild`=54847 WHERE (`Entry`=25459 AND `DifficultyID`=0); -- 25459 (Yanni) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25275 AND `DifficultyID`=0); -- 25275 (Warsong Blacksmith) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=805830912, `VerifiedBuild`=54847 WHERE (`Entry`=25244 AND `DifficultyID`=0); -- 25244 (Warsong Marksman) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26941 AND `DifficultyID`=0); -- 26941 (Brokkan Bear-Arms) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=26938 AND `DifficultyID`=0); -- 26938 (Groll) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25276 AND `DifficultyID`=0); -- 25276 (Goblin Siege Engineer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54847 WHERE (`Entry`=25243 AND `DifficultyID`=0); -- 25243 (Warsong Honor Guard) - Amphibious, CanSwim

DELETE FROM `creature_questitem` WHERE (`CreatureEntry` IN (24562,24563,24601,24613,24614,24637,25203,25204,25209,25210,25227,25294,25350,25351,25355,25383,25386,25387,25392,25415,25427,25445,25449,25452,25464,25479,25496,25498,25514,25520,25521,25522,25523,25582,25585,25609,25611,25613,25619,25622,25650,25651,25655,25660,25668,25678,25680,25682,25684,25685,25686,25687,25699,25700,25701,25707,25709,25712,25715,25716,25719,25720,25721,25725,25726,25728,25743,25750,25760,25789,25981,26073,26076,26126,26202,26252,26266,26451) AND `DifficultyID`=1);
UPDATE `creature_questitem` SET `VerifiedBuild`=54847 WHERE (`CreatureEntry`=25728 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25619 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=24563 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25726 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25415 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25415 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25684 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25203 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25203 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25210 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25720 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25721 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25514 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25387 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25682 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25700 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=24601 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25750 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25709 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25709 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25204 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25350 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25355 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25668 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25351 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25585 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25707 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25707 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=24613 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25725 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=24614 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=24614 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=26252 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25981 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25715 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25715 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25701 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=26266 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25622 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25660 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25660 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=26126 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25650 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25743 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25743 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=24637 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25719 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25452 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25452 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25760 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25209 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25651 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=26451 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=26202 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25699 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25522 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=26073 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25427 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25449 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25521 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25716 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25523 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25680 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25386 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25383 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25678 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25227 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25613 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25464 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25582 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25789 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25479 AND `DifficultyID`=0 AND `Idx`=2) OR (`CreatureEntry`=25479 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25479 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25498 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25611 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25611 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25392 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25686 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25496 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=25496 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25655 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25685 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25687 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25520 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=24562 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25294 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25712 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=26076 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25609 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=25445 AND `DifficultyID`=0 AND `Idx`=0);

-- Models
UPDATE `creature_model_info` SET `VerifiedBuild`=54847 WHERE `DisplayID` IN (23331, 28050, 25252, 25195, 15435, 20736, 14886, 23333, 24960, 25256, 28828, 23329, 24915, 24916, 26716, 23082, 24567, 24566, 23556, 23573, 24961, 25225, 24140, 47635, 27478, 18695, 23332, 26870, 25530, 24387, 27729, 24126, 25198, 27731, 24389, 25529, 24386, 24384, 26361, 26345, 26343, 26344, 25532, 24388, 25531, 24390, 24391, 24385, 26665, 26666, 23241, 48650, 26500, 7919, 5231, 23655, 23681, 23348, 23349, 22249, 26286, 26702, 23652, 26703, 23649, 24881, 14698, 23510, 23242, 24826, 23103, 22506, 24880, 2606, 23554, 10005, 11402, 17308, 25514, 1065, 24992, 12818, 23483, 23352, 17311, 23272, 10914, 24564, 23327, 24855, 17252, 23301, 23312, 10612, 23330, 14700, 15958, 15962, 22608, 26658, 26765, 23644, 30562, 23433, 23650, 23271, 24125, 23651, 22600, 24000, 23645, 24004, 24012, 24014, 22599, 22595, 22121, 23499, 23653, 22610, 23643, 24011, 22592, 23555, 16988, 21688, 23351, 24038, 25668, 26127, 23278, 25633, 26126, 23392, 24025, 24027, 24026, 23300, 23913, 23563, 23571, 23366, 23343, 23313, 7847, 23307, 23294, 15880, 24997, 23303, 24996, 570, 11489, 775, 24114, 24947, 24108, 24119, 24103, 28065, 23188, 27191, 24873, 23263, 24053, 23220, 6888, 23324, 24051, 23956, 23354, 23264, 23953, 23935, 23957, 23965, 23367, 24851, 12231, 24852, 24853, 27192, 4589, 21999, 24253, 23116, 23341, 24457, 24456, 24258, 24254, 23163, 16998, 24263, 24259, 24257, 23164, 14502, 386, 5025, 5490, 5187, 25194, 22989, 757, 2597, 23004, 3940, 506, 3617, 22986, 15984, 32790, 391, 11293, 1305, 15369, 22987, 5243, 478, 346, 4920, 23000, 22984, 1994, 23362, 27578, 27280, 42781, 31126, 23378, 23955, 23967, 23368, 24006, 23968, 6774, 24008, 24005, 23365, 26510, 23464, 22633, 23403, 1070, 5488, 14514, 1762, 21975, 5450, 5561, 486, 540, 652, 23121, 23123, 23170, 23126, 23125, 23124, 23171, 23248, 23174, 23247, 23127, 23463, 23249, 23461, 23387, 23462, 23172, 23246, 23173, 24906, 25976, 23430, 22003, 23393, 20872, 23395, 29419, 23399, 23394, 23401, 23398, 23400, 25981, 25975, 25973, 25977, 23206, 23145, 23203, 24909, 23205, 23204, 24485, 24504, 24487, 24341, 24342, 24343, 24344, 24346, 23223, 24501, 24505, 24488, 24486, 24347, 24340, 6371, 24345, 23032, 23055, 16925, 26699, 12821, 23260, 23221, 25249, 22769, 23337, 23336, 27165, 23149, 23338, 26266, 23386, 23389, 23490, 26423, 23434, 23493, 23426, 26425, 23418, 23417, 28291, 23416, 23712, 21243, 23321, 23311, 23320, 328, 1557, 27714, 27712, 27713, 27717, 27715, 27716, 23282, 26575, 23496, 23388, 23497, 569, 218, 23512, 94, 1060, 22576, 88841, 22578, 23280, 22577, 88847, 23279, 12200, 23385, 24145, 24606, 24607, 23049, 23050, 3019, 24602, 24601, 23027, 24281, 24289, 23013, 23001, 28058, 23011, 24285, 23005, 23012, 24284, 1720, 2733, 23073, 23025, 4125, 23072, 29625, 29623, 17718, 17703, 23410, 23083, 22041, 22339, 24298, 23010, 25956, 23342, 23061, 23062, 24293, 338, 24280, 24295, 24288, 24290, 24291, 23063, 23026, 35701, 23052, 23065, 24283, 24600, 24297, 24287, 24294, 24282, 24296, 24818, 23064, 24821, 23066, 24819, 3422, 22999, 23402, 22998, 23273, 23274, 23035, 23068, 23409, 23883, 23077, 23031, 9758, 20216, 32791, 17582, 6766, 11253, 23553, 24382, 25632, 23237, 23384, 27185, 23197, 23169, 23198, 23427, 23492, 23491, 347, 15903, 24849, 24848, 27170, 23316, 22088, 32789, 22496, 15961, 22044, 27195, 27174, 23214, 17612, 10627, 23142, 23141, 23391, 25391, 25390, 45880, 25773, 25775, 25772, 25774, 23239, 23218, 23238, 999, 23118, 14661, 22199, 23184, 23182, 24274, 25092, 24256, 23179, 23183, 24471, 23181, 23289, 23288, 19595, 22181, 23285, 23299, 23873, 23283, 23286, 11686, 6122, 23287, 23284, 23485, 1072, 26268, 1236, 26297, 23039, 23185, 23186, 23275, 23196, 23194, 23195, 23193, 23147, 14589, 23167, 23115, 23047, 23176, 23201, 23014, 23053, 23117, 26296, 18657, 25237, 23152, 23151, 44108, 23150, 23016, 23880, 23048, 28059, 24638, 24639, 23054, 28001, 22873, 23021, 23006, 9509, 23502, 23046, 23042, 23040, 23036, 24170, 23488, 23508, 23505, 23916, 23037, 23038, 23075, 23041, 35702, 29626, 29624, 16213, 17722, 23044, 17720, 23022, 17721, 27685, 27687, 27684, 27686, 21342, 23045, 24185, 24212, 24214, 24197, 25946, 24192, 24194, 23079, 23024, 23078, 26063, 23074, 24220, 24218, 24219, 23007, 24215, 24216, 23015, 23018, 23819, 23019, 19283, 23017, 23210, 23076, 23043, 23501, 23009, 24183, 23023, 24180, 23080, 23008);
UPDATE `creature_model_info` SET `BoundingRadius`=0.450000017881393432, `VerifiedBuild`=54847 WHERE `DisplayID`=1079;
UPDATE `creature_model_info` SET `BoundingRadius`=0.975292801856994628, `CombatReach`=1.5, `VerifiedBuild`=54847 WHERE `DisplayID`=23304;
UPDATE `creature_model_info` SET `BoundingRadius`=0.729359745979309082, `CombatReach`=1, `VerifiedBuild`=54847 WHERE `DisplayID`=4973;
UPDATE `creature_model_info` SET `BoundingRadius`=1.086323738098144531, `VerifiedBuild`=54847 WHERE `DisplayID`=22123;
UPDATE `creature_model_info` SET `BoundingRadius`=0.74217301607131958, `VerifiedBuild`=54847 WHERE `DisplayID`=14732;
UPDATE `creature_template_model` SET `VerifiedBuild`=54847 WHERE (`Idx`=0 AND `CreatureID` IN (25709,26115,27859,26767,25767,27074,27067,25226,25204,25419,25350,25811,25355,24795,25827,26995,25607,25437,25402,25288,25405,35132,25459,27306,25849,25665,26600,25584,25724,25240,27012,25498,24567,25512,25611,25489,25272,32474,26945,30051,27569,25615,25786,25664,24021,25210,25309,26645,25198,25880,25342,25819,25321,27298,27302,25291,31790,25238,26987,25228,25829,25618,31685,25414,32565,18380,31725,31720,25728,27494,26219,71163,26186,25422,25606,24733,27063,27073,25292,25353,25431,24862,27299,26085,25501,25349,25244,26976,25256,25717,68845,26373,29965,26172,25472,26986,25666,25619,26599,24288,25862,26804,25748,27888,25679,25307,25222,29158,26248,25253,25450,25675,25438,25394,26078,29155,26977,27904,26110,27187,25390,25386,25383,25678,26725,25739,32544,26597,25339,25211,25847,62697,25791,25812,25364,26991,26990,25227,25613,25464,25504,25403,26941,24566,25719,25534,25493,26174,25452,24709,25760,25433,25199,25209,25206,25197,54242,25282,27385,25285,25651,26451,25836,25468,25466,27110,27106,27108,25421,25426,25361,47577,18378,32548,25712,27566,26173,32361,27860,26076,25396,24730,26160,24706,25736,25205,27138,27291,25828,26249,25435,28440,25481,25609,25445,25289,25274,26117,25722,129141,26187,25726,26766,26848,25374,25415,25442,25448,25733,25804,26098,26549,25297,18376,26993,25301,25511,25600,25278,25269,25075,26968,25713,25711,27189,25670,25758,26634,27058,34381,25793,35133,27365,27000,25582,25789,25479,25334,26538,32520,25716,27173,25314,25473,25516,25391,25490,21757,25434,25817,25732,62693,25271,27010,24921,25523,24662,26449,25680,25467,25624,26044,25276,27441,27906,45396,27529,26171,25377,25454,26721,25982,25702,25428,23837,25781,25215,25308,25810,27137,27300,25316,25743,31792,24637,26989,26155,25636,25797,24469,25470,27064,31724,26969,26109,26265,26213,27186,26170,25604,25477,25343,25727,25981,26083,25617,25251,26994,25310,25510,25503,25482,25672,25362,26047,25237,27440,25273,27046,26556,26162,25392,26697,25762,26768,25814,26619,25590,25416,25376,27301,25986,25686,25385,26084,25311,25313,24580,25496,25655,25487,25327,25280,25284,31807,26218,25684,26720,26790,25984,25752,26598,27065,25203,28375,25384,25841,25480,27393,32564,26988,25395,25234,25979,61158,25329,28150,25610,25379,25279,25242,29655,26175,27190,26191,26189,26202,26094,26157,25699,25652,25216,25335,25808,27271,25968,31789,25623,25843,25302,25522,25469,60761,32647,27432,25983,26709,26718,25685,19871,25201,26817,25417,25338,25441,25846,27142,25594,25356,25838,25687,31793,25281,24959,25520,25465,25475,25476,24562,25294,31727,26974,155076,25668,24702,24957,25430,25589,25217,25765,25881,25351,25322,18955,25585,18375,26879,25249,26452,24576,27107,25596,26975,25978,26980,25707,25492,25378,25601,26041,27999,25432,25418,25995,24613,27011,35131,25245,27364,47576,25250,25605,25333,25404,26981,31723,32357,24770,28195,27195,26169,27194,25715,25602,25701,26266,25764,25807,25622,25677,25660,26126,26596,25455,25845,27140,25850,29328,26161,25650,25839,25826,25239,24357,25299,27001,25317,25264,25669,25488,31726,25720,25721,26127,28960,27193,25514,26103,25387,25682,26996,26847,26104,25700,24601,25747,25429,25444,26602,26601,25750,25802,27147,27141,36155,25806,26999,25235,25241,25737,29617,25825,25380,25440,62695,26972,25286,25243,25718,26403,26185,26073,25658,25714,26159,25425,25427,25792,25809,25449,27143,25803,25219,25300,25816,25306,25521,25332,24771,25439,25328,35135,26982,25277,25275,33087,32534,28382,25471,26093,25393,25725,25705,25326,27069,26112,25780,25336,25262,25801,24614,26252,31788,25800,26997,385,208182,25671,25625,25446,26938,26043,27610,31805,26158,24703,26048,24563,25753,25208,25766,25443,27139,31791,25248,26992,15214,25298,26998,25220,24042,25381,25774,25513,25247,29233)) OR (`Idx`=3 AND `CreatureID` IN (25767,25198,25880,27073,25244,25361,28440,25271,25276,25215,26109,25617,25814,26619,25684,25979,61158,25610,25242,25216,25808,25469,25201,26817,25338,25217,25596,25660,25317,26601,25806,25286,25243,26159,25306,25275,208182,25220)) OR (`Idx`=2 AND `CreatureID` IN (25767,27074,25355,25198,25880,27073,25244,25361,27566,32361,28440,25271,25276,25215,24637,26109,25981,25617,25362,25814,26619,25684,25979,61158,25610,25242,25216,25808,25469,27432,25201,26817,25338,25217,25596,25715,25660,26126,25839,25317,26601,25806,25286,25243,26159,25306,25275,26112,208182,25625,25220)) OR (`Idx`=1 AND `CreatureID` IN (25767,27074,25419,25350,25355,25402,25405,27306,25665,24567,25512,27569,25664,24021,25309,26645,25198,25880,25342,25819,25321,25414,27073,25353,25431,24862,25501,25349,25244,26373,29965,26172,25472,25666,24288,26804,26248,25739,25847,25364,25403,25534,25493,54242,25651,25836,25468,25466,27110,27106,27108,25421,25361,27566,32361,26249,28440,25609,25415,25442,25511,27189,25670,34381,25479,32520,25473,25490,25271,24921,25467,25276,45396,27529,25428,23837,25215,25308,24637,26109,26265,25343,25981,25617,25310,25510,25672,25362,26047,26162,25814,26619,25496,25684,26790,27393,25234,25979,61158,25610,25242,29655,26175,26189,26094,26157,25216,25808,25968,25843,25469,32647,27432,19871,25201,26817,25417,25338,25441,25846,25594,24959,155076,25668,24957,25217,25351,25322,18955,27107,25596,25492,26041,27999,25995,25605,25404,25715,25764,25660,26126,25845,26161,25839,25317,25669,28960,25429,26601,36155,25806,25286,25243,26403,26159,25449,25219,25300,25306,24771,25439,25275,33087,25471,26093,25393,26112,208182,25671,25625,26043,25443,15214,25220,24042,25513));

-- Gameobject templates
DELETE FROM `gameobject_template` WHERE `entry`=385811;
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`, `Data0`, `Data1`, `Data2`, `Data3`, `Data4`, `Data5`, `Data6`, `Data7`, `Data8`, `Data9`, `Data10`, `Data11`, `Data12`, `Data13`, `Data14`, `Data15`, `Data16`, `Data17`, `Data18`, `Data19`, `Data20`, `Data21`, `Data22`, `Data23`, `Data24`, `Data25`, `Data26`, `Data27`, `Data28`, `Data29`, `Data30`, `Data31`, `Data32`, `Data33`, `Data34`, `ContentTuningId`, `VerifiedBuild`) VALUES
(385811, 10, 9011, 'Portal to the Azure Archives', '', '', '', 1.299999952316284179, 0, 0, 0, 3000, 0, 0, 1, 0, 0, 0, 405304, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 107651, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2424, 54847); -- Portal to the Azure Archives

UPDATE `gameobject_template` SET `VerifiedBuild`=54847 WHERE `entry` IN (187388, 189979, 148544, 169278, 190547, 188131, 186844, 187316, 187240, 187856, 190536, 188113, 188164, 195034, 187900, 187995, 188029, 148540, 169288, 188013, 182071, 186842, 187880, 187867, 194687, 188140, 191225, 187885, 187886, 188085, 186836, 191075, 187870, 194694, 191832, 191228, 175544, 188025, 188027, 148567, 169264, 187565, 190546, 186837, 194953, 187450, 191081, 187189, 187249, 188521, 187237, 216714, 187320, 195035, 190342, 190334, 189316, 187454, 188030, 191304, 148554, 187241, 191082, 191699, 191697, 188102, 214523, 194026, 187318, 189329, 188038, 188044, 188022, 191089, 188014, 148556, 169292, 187247, 189978, 192052, 187868, 188092, 194692, 194685, 188087, 207492, 187261, 187456, 169284, 188015, 188656, 187863, 187860, 208632, 191287, 187432, 194688, 192053, 188088, 187367, 191090, 169266, 148550, 191370, 187558, 187187, 187694, 194693, 188106, 187984, 188032, 187376, 169289, 169267, 187258, 186835, 187852, 191073, 188657, 187854, 194690, 191636, 214507, 187677, 188108, 188031, 169281, 188112, 188016, 187184, 187243, 192046, 404905, 187858, 187862, 191635, 187695, 191698, 188007, 188006, 187364, 188005, 169290, 169269, 187772, 188018, 194682, 194683, 187321, 188004, 169270, 188008, 187859, 278347, 187884, 187659, 191637, 187387, 195033, 188003, 187986, 187697, 188034, 169282, 187568, 194677, 187448, 192051, 188653, 187864, 187855, 187660, 191639, 188133, 187999, 188651, 188043, 188000, 169280, 169277, 188028, 187889, 187244, 191070, 191074, 191080, 191076, 194689, 194684, 188100, 191142, 188141, 191303, 188037, 169285, 169275, 187259, 186841, 186840, 188692, 187658, 187251, 187248, 187866, 194696, 194691, 193602, 187317, 188002, 188033, 189287, 169273, 169268, 188104, 190194, 187449, 187246, 187188, 187902, 187250, 188655, 187861, 194686, 195032, 187319, 187901, 188652, 188035, 187996, 169283, 148547, 186839, 187980, 187560, 187106, 191078, 187185, 191638, 186238, 195031, 188612, 189315, 187897, 187268, 188042, 191088, 188001, 169279, 148549, 182072, 187239, 187433, 191288, 194025, 190335, 187899, 169272, 278574, 187851, 190193, 187245, 191072, 404907, 187871, 194681, 187693, 192573, 188089, 188454, 169265, 187453, 190169, 188017, 194954, 187447, 188010, 187857, 187773, 195030, 194680, 187314, 187322, 187373, 187997, 169271, 148551, 191079, 187909, 191077, 195029, 188514, 187315, 192033, 187998, 187898, 187255, 207528, 188041, 188040, 187915, 188026, 169276, 169274, 188163, 187256, 186838, 186843, 187186, 191542, 187872, 188091, 187661, 194695, 189317, 188036, 188039, 187982, 148557, 169293, 190548, 190287, 187082, 191071, 189973, 187452, 187655, 152614, 192572, 187085, 188024, 169287, 169291, 188012, 188084, 190556, 187242, 187873, 187874, 193981, 190352);
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187690; -- En'kilah Cauldron
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry` IN (187369, 190567); -- Cage
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187664; -- "Elder Takret"
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187561; -- Arcane Prison
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187663; -- "Elder Sagani"
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187683; -- Pneumatic Tank Transjigamarig
UPDATE `gameobject_template` SET `Data1`=0, `Data30`=23094, `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187689; -- Crafty's Stuff
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=188120; -- Fields, Factories and Workshops
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187905; -- Massive Glowing Egg
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=194151; -- Signal Fire
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry` IN (188101, 188103); -- Coldarra Geological Monitor
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187389; -- Falling Rocks
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187577; -- Warsong Banner
UPDATE `gameobject_template` SET `Data1`=0, `Data30`=23085, `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187671; -- Tuskarr Ritual Object
UPDATE `gameobject_template` SET `Data1`=0, `Data30`=23093, `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187684; -- Super Strong Metal Plate
UPDATE `gameobject_template` SET `Data1`=0, `Data30`=23093, `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187686; -- Super Strong Metal Plate
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry` IN (187987, 188109); -- South Point Station Valve
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187879; -- Den of Dying Plague Cauldron
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry` IN (187985, 188107); -- North Point Station Valve
UPDATE `gameobject_template` SET `Data1`=0, `Data30`=23085, `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187670; -- Tuskarr Ritual Object
UPDATE `gameobject_template` SET `Data1`=0, `Data30`=23093, `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187685; -- Super Strong Metal Plate
UPDATE `gameobject_template` SET `Data1`=0, `Data30`=62019, `ContentTuningId`=677, `VerifiedBuild`=54847 WHERE `entry`=244447; -- White Murloc Egg
UPDATE `gameobject_template` SET `Data1`=0, `Data30`=23093, `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187687; -- Super Strong Metal Plate
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187881; -- Talramas Scourge Summoning Circle
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187703; -- Kvaldir Inferno
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187673; -- Farseer Grimwalker's Remains
UPDATE `gameobject_template` SET `ContentTuningId`=44, `VerifiedBuild`=54847 WHERE `entry`=187662; -- "Elder Kesuk"

DELETE FROM `gameobject_template_addon` WHERE `entry`=385811;
INSERT INTO `gameobject_template_addon` (`entry`, `faction`, `flags`, `WorldEffectID`, `AIAnimKitID`) VALUES
(385811, 0, 262144, 0, 5605); -- Portal to the Azure Archives

UPDATE `gameobject_template_addon` SET `faction`=35, `flags`=8192 WHERE `entry`=214523; -- Instance Portal (Raid 10/25 No Heroic)
UPDATE `gameobject_template_addon` SET `flags`=2113540 WHERE `entry`=187885; -- Gurgleboggle's Bauble
UPDATE `gameobject_template_addon` SET `flags`=2113540 WHERE `entry`=187886; -- Burblegobble's Bauble
UPDATE `gameobject_template_addon` SET `flags`=2113540 WHERE `entry`=187697; -- Crafty's Tools
UPDATE `gameobject_template_addon` SET `flags`=278528 WHERE `entry`=191303; -- Firethorn
UPDATE `gameobject_template_addon` SET `flags`=2113540 WHERE `entry`=187673; -- Farseer Grimwalker's Remains
UPDATE `gameobject_template_addon` SET `faction`=114, `WorldEffectID`=2773 WHERE `entry`=187376; -- NPC Fishing Bobber
UPDATE `gameobject_template_addon` SET `flags`=2113540 WHERE `entry`=187577; -- Warsong Banner
UPDATE `gameobject_template_addon` SET `flags`=278528 WHERE `entry`=189979; -- Rich Cobalt Deposit
UPDATE `gameobject_template_addon` SET `flags`=2113540 WHERE `entry`=188120; -- Fields, Factories and Workshops
UPDATE `gameobject_template_addon` SET `flags`=2113540 WHERE `entry`=188131; -- Wine Crate
UPDATE `gameobject_template_addon` SET `flags`=2113540 WHERE `entry`=187980; -- First Aid Supplies
UPDATE `gameobject_template_addon` SET `WorldEffectID`=2437 WHERE `entry`=244447; -- White Murloc Egg
UPDATE `gameobject_template_addon` SET `flags`=0 WHERE `entry`=191079; -- Campfire
UPDATE `gameobject_template_addon` SET `flags`=4 WHERE `entry`=187905; -- Massive Glowing Egg
UPDATE `gameobject_template_addon` SET `flags`=0 WHERE `entry`=187909; -- Coldrock Clam
UPDATE `gameobject_template_addon` SET `flags`=2113540 WHERE `entry`=187683; -- Pneumatic Tank Transjigamarig
UPDATE `gameobject_template_addon` SET `flags`=1048608 WHERE `entry`=208632; -- Coldarra Disc
UPDATE `gameobject_template_addon` SET `flags`=36 WHERE `entry`=187884; -- Evanor's Prison

UPDATE `gameobject_questitem` SET `VerifiedBuild`=54847 WHERE (`Idx`=0 AND `GameObjectEntry` IN (188140,187885,187886,187899,187673,188113,188164,187900,187689,187577,187671,188017,188120,188131,188015,187901,187980,187902,187683,187897,187687,187685,187677,188016,187684,188018,187686,187697,187670,187660,187659,187898,187661));



-- Creature templates
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=1 WHERE `entry`=32448; -- Alexstrasza's Gift
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=32295; -- Alexstrasza the Life-Binder
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=1 WHERE `entry`=31253; -- Alexstrasza the Life-Binder
UPDATE `creature_template` SET `unit_flags2`=2080, `unit_flags3`=524289 WHERE `entry`=30592; -- Static Field
UPDATE `creature_template` SET `unit_flags3`=1 WHERE `entry`=30334; -- Surge of Power
UPDATE `creature_template` SET `unit_flags3`=524289 WHERE `entry`=30282; -- Arcane Overload
UPDATE `creature_template` SET `faction`=35, `unit_flags`=512, `unit_flags3`=1 WHERE `entry`=30248; -- Hover Disk
UPDATE `creature_template` SET `faction`=35, `unit_flags`=512, `unit_flags3`=1 WHERE `entry`=30234; -- Hover Disk
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=30118; -- Portal (Malygos)
UPDATE `creature_template` SET `unit_flags3`=1 WHERE `entry`=30090; -- Vortex
UPDATE `creature_template` SET `unit_flags2`=0, `unit_flags3`=524289 WHERE `entry`=30084; -- Power Spark
UPDATE `creature_template` SET `unit_flags`=64, `unit_flags2`=0 WHERE `entry`=28859; -- Malygos

UPDATE `creature_template_addon` SET `AnimTier`=3 WHERE `entry`=32448; -- 32448 (Alexstrasza's Gift) - Alexstrasza's Gift Visual
UPDATE `creature_template_addon` SET `AnimTier`=0 WHERE `entry`=30161; -- 30161 (Wyrmrest Skytalon)

-- Difficulties
DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID` IN (1,3) AND `Entry` IN (32448,32295,31253,30592,30334,30282,30249,30248,30245,30234,30161,30118,30090,30084,28859));
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID`=4 AND `Entry` IN (30084);

UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `VerifiedBuild`=54904 WHERE (`Entry`=32448 AND `DifficultyID`=0); -- 32448 (Alexstrasza's Gift) - Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=3, `LevelScalingDeltaMax`=3, `ContentTuningID`=1019, `VerifiedBuild`=54904 WHERE (`Entry`=32295 AND `DifficultyID`=0); -- 32295 (Alexstrasza the Life-Binder) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `StaticFlags1`=536870912, `VerifiedBuild`=54904 WHERE (`Entry`=31253 AND `DifficultyID`=0); -- 31253 (Alexstrasza the Life-Binder) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `VerifiedBuild`=54904 WHERE (`Entry`=30592 AND `DifficultyID`=0); -- 30592 (Static Field) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `StaticFlags1`=536870912, `VerifiedBuild`=54904 WHERE (`Entry`=30334 AND `DifficultyID`=0); -- 30334 (Surge of Power) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `StaticFlags1`=536870912, `VerifiedBuild`=54904 WHERE (`Entry`=30282 AND `DifficultyID`=0); -- 30282 (Arcane Overload) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=30249 AND `DifficultyID`=0); -- 30249 (Scion of Eternity) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `VerifiedBuild`=54904 WHERE (`Entry`=30248 AND `DifficultyID`=0); -- 30248 (Hover Disk) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=30245 AND `DifficultyID`=0); -- 30245 (Nexus Lord) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `StaticFlags1`=536870912, `VerifiedBuild`=54904 WHERE (`Entry`=30234 AND `DifficultyID`=0); -- 30234 (Hover Disk) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `StaticFlags1`=0, `VerifiedBuild`=54904 WHERE (`Entry`=30161 AND `DifficultyID`=0); -- 30161 (Wyrmrest Skytalon) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `VerifiedBuild`=54904 WHERE (`Entry`=30118 AND `DifficultyID`=0); -- 30118 (Portal (Malygos)) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `VerifiedBuild`=54904 WHERE (`Entry`=30090 AND `DifficultyID`=0); -- 30090 (Vortex) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `StaticFlags1`=536870912, `VerifiedBuild`=54904 WHERE (`Entry`=30084 AND `DifficultyID`=0); -- 30084 (Power Spark) - Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=3, `LevelScalingDeltaMax`=3, `ContentTuningID`=1019, `StaticFlags2`=2, `VerifiedBuild`=54904 WHERE (`Entry`=28859 AND `DifficultyID`=0); -- 28859 (Malygos) - Amphibious, Floating
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=34067 WHERE (`Entry`=30248 AND `DifficultyID`=4); -- Hover Disk
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=34058, `TypeFlags2`=128 WHERE (`Entry`=30245 AND `DifficultyID`=4); -- Nexus Lord
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=34035 WHERE (`Entry`=30234 AND `DifficultyID`=4); -- Hover Disk
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=33880 WHERE (`Entry`=30161 AND `DifficultyID`=4); -- Wyrmrest Skytalon
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=31035, `TypeFlags2`=128 WHERE (`Entry`=28859 AND `DifficultyID`=4); -- Malygos
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=34072, `TypeFlags2`=128 WHERE (`Entry`=30249 AND `DifficultyID`=4); -- Scion of Eternity
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54904 WHERE (`DifficultyID`=0 AND `Entry` IN (155076,32548,32534,32520,26175,26127,25721,24770)) OR (`DifficultyID`=4 AND `Entry`=30248);
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=30249 AND `DifficultyID`=4); -- 30249 (Scion of Eternity) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=30245 AND `DifficultyID`=4); -- 30245 (Nexus Lord) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=536870912, `VerifiedBuild`=54904 WHERE (`Entry`=30234 AND `DifficultyID`=4); -- 30234 (Hover Disk) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0, `VerifiedBuild`=54904 WHERE (`Entry`=30161 AND `DifficultyID`=4); -- 30161 (Wyrmrest Skytalon) - 
UPDATE `creature_template_difficulty` SET `StaticFlags2`=2, `VerifiedBuild`=54904 WHERE (`Entry`=28859 AND `DifficultyID`=4); -- 28859 (Malygos) - Amphibious, Floating

DELETE FROM `creature_questitem` WHERE (`CreatureEntry`=28859 AND `DifficultyID` IN (0,1) AND `Idx`=0);
INSERT INTO `creature_questitem` (`CreatureEntry`, `DifficultyID`, `Idx`, `ItemId`, `VerifiedBuild`) VALUES
(28859, 0, 0, 44650, 54904); -- Malygos

DELETE FROM `creature_questitem` WHERE (`CreatureEntry`=28859 AND `DifficultyID`=4 AND `Idx`=0);
INSERT INTO `creature_questitem` (`CreatureEntry`, `DifficultyID`, `Idx`, `ItemId`, `VerifiedBuild`) VALUES
(28859, 4, 0, 44651, 54904); -- Malygos

-- Models
UPDATE `creature_model_info` SET `VerifiedBuild`=54904 WHERE `DisplayID` IN (27569, 27401, 24316, 24317, 24319, 26876, 25835, 14501, 11686, 26753, 26752, 16925);
UPDATE `creature_template_model` SET `VerifiedBuild`=54904 WHERE (`Idx`=0 AND `CreatureID` IN (59072,30090,69066,18362,31253,305,54423,32158,47654,35362,22517,30234,64993,304,68288,56921,71486,308,40625,30245,30118,40725,32295,54741,65011,14505,30161,30282,28363,32448,60941,53488,30592,25064,40165,31698,54879,30249,28859,30248,30084,70356,30334)) OR (`Idx`=1 AND `CreatureID` IN (30090,31253,22517,30245,30118,40725,32448,53488,30592,40165,30249,30334)) OR (`Idx`=3 AND `CreatureID` IN (30245,30249)) OR (`Idx`=2 AND `CreatureID` IN (30245,30249));
UPDATE `creature_template_model` SET `VerifiedBuild`=54904 WHERE (`Idx`=0 AND `CreatureID` IN (32534,26175,32548,155076,32520,356,24770,34337,26127,18379,25721,25414,25421)) OR (`Idx`=1 AND `CreatureID` IN (26175,155076,32520,25414,25421));

-- Gameobject templates
UPDATE `gameobject_template` SET `Data1`=0, `Data10`=1, `Data12`=1, `Data15`=0, `Data25`=1094, `Data30`=88624, `ContentTuningId`=1019, `VerifiedBuild`=54904 WHERE `entry`=193905; -- Alexstrasza's Gift
UPDATE `gameobject_template` SET `Data1`=0, `Data15`=0, `Data25`=1094, `Data30`=26859, `ContentTuningId`=1019, `VerifiedBuild`=54904 WHERE `entry`=194158; -- Heart of Magic
UPDATE `gameobject_template` SET `Data26`=1, `ContentTuningId`=1019, `VerifiedBuild`=54904 WHERE `entry`=193908; -- Exit Portal
UPDATE `gameobject_template` SET `ContentTuningId`=1019, `VerifiedBuild`=54904 WHERE `entry`=193958; -- The Focusing Iris
UPDATE `gameobject_template` SET `VerifiedBuild`=54904 WHERE `entry`=193070;
UPDATE `gameobject_template` SET `VerifiedBuild`=54904 WHERE `entry` IN (214523, 214507, 208632, 188514, 188521, 193602, 188100, 188133);
UPDATE `gameobject_template` SET `Data1`=0, `Data10`=1, `Data12`=1, `Data15`=0, `Data25`=1094, `Data30`=88632, `ContentTuningId`=1019, `VerifiedBuild`=54904 WHERE `entry`=193967; -- Alexstrasza's Gift
UPDATE `gameobject_template` SET `Data1`=0, `Data15`=0, `Data25`=1094, `Data30`=26860, `ContentTuningId`=1019, `VerifiedBuild`=54904 WHERE `entry`=194159; -- Heart of Magic
UPDATE `gameobject_template` SET `ContentTuningId`=1019, `VerifiedBuild`=54904 WHERE `entry`=193960; -- The Focusing Iris

UPDATE `gameobject_template_addon` SET `flags`=16388 WHERE `entry`=194158; -- Heart of Magic
UPDATE `gameobject_template_addon` SET `flags`=2113536 WHERE `entry`=193905; -- Alexstrasza's Gift
UPDATE `gameobject_template_addon` SET `flags`=1048608 WHERE `entry`=193070; -- Nexus Raid Platform
UPDATE `gameobject_template_addon` SET `flags`=16388 WHERE `entry`=194159; -- Heart of Magic
UPDATE `gameobject_template_addon` SET `flags`=2113536 WHERE `entry`=193967; -- Alexstrasza's Gift

UPDATE `gameobject_questitem` SET `VerifiedBuild`=54904 WHERE (`GameObjectEntry`=194158 AND `Idx`=0);
UPDATE `gameobject_questitem` SET `VerifiedBuild`=54904 WHERE (`GameObjectEntry`=194159 AND `Idx`=0);


-- Creature templates
UPDATE `creature_template` SET `speed_walk`=1, `unit_flags2`=2048 WHERE `entry`=28276; -- Greater Ley-Whelp
UPDATE `creature_template` SET `speed_walk`=1.799999952316284179, `speed_run`=0.642857134342193603, `unit_flags3`=524288 WHERE `entry`=28239; -- Arcane Beam
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=1 WHERE `entry`=28236; -- Azure Ring Captain
UPDATE `creature_template` SET `unit_flags3`=1 WHERE `entry`=28183; -- Centrifuge Core
UPDATE `creature_template` SET `unit_flags3`=524289 WHERE `entry`=28166; -- Unstable Sphere
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=28153; -- Snowflake
UPDATE `creature_template` SET `unit_flags3`=524288 WHERE `entry`=28012; -- Image of Belgaristrasz
UPDATE `creature_template` SET `speed_walk`=1, `unit_flags3`=524288 WHERE `entry`=27756; -- Ruby Drake
UPDATE `creature_template` SET `speed_walk`=1, `unit_flags3`=524288 WHERE `entry`=27755; -- Amber Drake
UPDATE `creature_template` SET `speed_walk`=1, `unit_flags3`=524288 WHERE `entry`=27692; -- Emerald Drake
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=27656; -- Ley-Guardian Eregos
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=27655; -- Mage-Lord Urom
UPDATE `creature_template` SET `speed_walk`=1, `unit_flags2`=2048 WHERE `entry`=27654; -- Drakos the Interrogator
UPDATE `creature_template` SET `speed_run`=1.142857193946838378 WHERE `entry`=27653; -- Phantasmal Water
UPDATE `creature_template` SET `speed_walk`=1, `speed_run`=1.142857193946838378, `unit_flags3`=0 WHERE `entry`=27642; -- Phantasmal Mammoth
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=27641; -- Centrifuge Construct
UPDATE `creature_template` SET `speed_walk`=1, `unit_flags2`=2048 WHERE `entry`=27638; -- Azure Ring Guardian
UPDATE `creature_template` SET `speed_walk`=1 WHERE `entry`=27636; -- Azure Ley-Whelp
UPDATE `creature_template` SET `speed_walk`=1 WHERE `entry`=27635; -- Azure Spellbinder
UPDATE `creature_template` SET `speed_walk`=1, `speed_run`=1.142857193946838378 WHERE `entry`=27633; -- Azure Inquisitor
UPDATE `creature_template` SET `unit_flags`=64, `unit_flags2`=2048 WHERE `entry`=27447; -- Varos Cloudstrider
UPDATE `creature_template` SET `unit_flags3`=524288 WHERE `entry`=30879; -- Planar Anomaly

UPDATE `creature_template_addon` SET `AnimTier`=0, `auras`='' WHERE `entry`=27756; -- 27756 (Ruby Drake)
UPDATE `creature_template_addon` SET `AnimTier`=0, `auras`='' WHERE `entry`=27755; -- 27755 (Amber Drake)
UPDATE `creature_template_addon` SET `AnimTier`=0, `auras`='' WHERE `entry`=27692; -- 27692 (Emerald Drake)
UPDATE `creature_template_addon` SET `AnimTier`=3 WHERE `entry`=27656; -- 27656 (Ley-Guardian Eregos)
UPDATE `creature_template_addon` SET `AnimTier`=3, `SheathState`=1 WHERE `entry`=30879; -- 30879 (Planar Anomaly)

-- Difficulties
UPDATE `creature_template_difficulty` ctd
LEFT JOIN `creature_template_difficulty` ctd2 ON ctd.`Entry`=ctd2.`Entry`
SET
    ctd.`DamageModifier`=ctd2.`DamageModifier`,
    ctd.`LootID`=ctd2.`LootID`,
    ctd.`PickPocketLootID`=ctd2.`PickPocketLootID`,
    ctd.`SkinLootID`=ctd2.`SkinLootID`,
    ctd.`GoldMin`=ctd2.`GoldMin`,
    ctd.`GoldMax`=ctd2.`GoldMax`
WHERE ctd.`Entry` IN (32261,28276,28239,28236,28183,28166,28153,28012,27756,27755,27692,27659,27658,27657,27656,27655,27654,27653,27651,27650,27649,27648,27647,27645,27644,27642,27641,27640,27639,27638,27636,27635,27633,27447)
AND ctd.`DifficultyID`=0 AND ctd2.`DifficultyID`=1 AND ctd2.`Entry` IS NOT NULL;

DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=1 AND `Entry` IN (32261,28276,28239,28236,28183,28166,28153,28012,27756,27755,27692,27659,27658,27657,27656,27655,27654,27653,27651,27650,27649,27648,27647,27645,27644,27642,27641,27640,27639,27638,27636,27635,27633,27447));

DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=2 AND `Entry` IN (28236,27756,27755,27692,22517));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `LevelScalingDeltaMin`, `LevelScalingDeltaMax`, `ContentTuningID`, `StaticFlags1`, `StaticFlags2`, `StaticFlags3`, `StaticFlags4`, `StaticFlags5`, `StaticFlags6`, `StaticFlags7`, `StaticFlags8`, `VerifiedBuild`) VALUES
(28236, 2, 0, 0, 1093, 805830656, 0, 0, 0, 0, 0, 0, 0, 54904), -- 28236 (Azure Ring Captain) - CanSwim, Floating
(27756, 2, 0, 0, 1093, 524288, 0, 0, 0, 0, 0, 0, 0, 54904), -- 27756 (Ruby Drake) - 
(27755, 2, 0, 0, 1093, 524288, 0, 0, 0, 0, 0, 0, 0, 54904), -- 27755 (Amber Drake) - 
(27692, 2, 0, 0, 1093, 524288, 0, 0, 0, 0, 0, 0, 0, 54904), -- 27692 (Emerald Drake) - 
(22517, 2, 0, 0, 2672, 536870912, 0, 0, 0, 0, 0, 0, 0, 54904); -- 22517 (World Trigger (Large AOI)) - Floating

DELETE FROM `creature_template_difficulty` WHERE (`Entry`=30161 AND `DifficultyID`=2);
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `HealthScalingExpansion`, `HealthModifier`, `ManaModifier`, `CreatureDifficultyID`, `TypeFlags`, `TypeFlags2`) VALUES
(30161, 2, 2, 7.936510086059570312, 1, 33879, 0, 0); -- Wyrmrest Skytalon

UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=3 WHERE (`Entry`=22517 AND `DifficultyID`=0); -- World Trigger (Large AOI)
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2671, `VerifiedBuild`=54904 WHERE (`Entry`=22517 AND `DifficultyID`=0); -- 22517 (World Trigger (Large AOI)) - Floating
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54904 WHERE (`Entry`=53488 AND `DifficultyID`=0);
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=32261 AND `DifficultyID`=0); -- 32261 (Crystal Spider) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `ContentTuningID`=246, `StaticFlags1`=805830656, `VerifiedBuild`=54904 WHERE (`Entry`=28276 AND `DifficultyID`=0); -- 28276 (Greater Ley-Whelp) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `VerifiedBuild`=54904 WHERE (`Entry`=28239 AND `DifficultyID`=0); -- 28239 (Arcane Beam) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=805830656, `VerifiedBuild`=54904 WHERE (`Entry`=28236 AND `DifficultyID`=0); -- 28236 (Azure Ring Captain) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=805830912, `VerifiedBuild`=54904 WHERE (`Entry`=28183 AND `DifficultyID`=0); -- 28183 (Centrifuge Core) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=28166 AND `DifficultyID`=0); -- 28166 (Unstable Sphere) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=28153 AND `DifficultyID`=0); -- 28153 (Snowflake) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=28012 AND `DifficultyID`=0); -- 28012 (Image of Belgaristrasz) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=524288, `VerifiedBuild`=54904 WHERE (`Entry`=27756 AND `DifficultyID`=0); -- 27756 (Ruby Drake) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=524288, `VerifiedBuild`=54904 WHERE (`Entry`=27755 AND `DifficultyID`=0); -- 27755 (Amber Drake) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=524288, `VerifiedBuild`=54904 WHERE (`Entry`=27692 AND `DifficultyID`=0); -- 27692 (Emerald Drake) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27659 AND `DifficultyID`=0); -- 27659 (Eternos) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27658 AND `DifficultyID`=0); -- 27658 (Belgaristrasz) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27657 AND `DifficultyID`=0); -- 27657 (Verdisa) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=246, `StaticFlags1`=805830656, `StaticFlags2`=0, `VerifiedBuild`=54904 WHERE (`Entry`=27656 AND `DifficultyID`=0); -- 27656 (Ley-Guardian Eregos) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27655 AND `DifficultyID`=0); -- 27655 (Mage-Lord Urom) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27654 AND `DifficultyID`=0); -- 27654 (Drakos the Interrogator) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27653 AND `DifficultyID`=0); -- 27653 (Phantasmal Water) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27651 AND `DifficultyID`=0); -- 27651 (Phantasmal Fire) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `LevelScalingDeltaMax`=-1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27650 AND `DifficultyID`=0); -- 27650 (Phantasmal Air) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `LevelScalingDeltaMax`=-1, `ContentTuningID`=246, `VerifiedBuild`=54904 WHERE (`Entry`=27649 AND `DifficultyID`=0); -- 27649 (Phantasmal Murloc) - Amphibious
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `ContentTuningID`=246, `VerifiedBuild`=54904 WHERE (`Entry`=27648 AND `DifficultyID`=0); -- 27648 (Phantasmal Naga) - Amphibious
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27647 AND `DifficultyID`=0); -- 27647 (Phantasmal Ogre) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `LevelScalingDeltaMax`=-1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27645 AND `DifficultyID`=0); -- 27645 (Phantasmal Cloudscraper) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27644 AND `DifficultyID`=0); -- 27644 (Phantasmal Wolf) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27642 AND `DifficultyID`=0); -- 27642 (Phantasmal Mammoth) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27641 AND `DifficultyID`=0); -- 27641 (Centrifuge Construct) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27640 AND `DifficultyID`=0); -- 27640 (Ring-Lord Conjurer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `LevelScalingDeltaMax`=-1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27639 AND `DifficultyID`=0); -- 27639 (Ring-Lord Sorceress) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `ContentTuningID`=246, `StaticFlags1`=805830656, `VerifiedBuild`=54904 WHERE (`Entry`=27638 AND `DifficultyID`=0); -- 27638 (Azure Ring Guardian) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27636 AND `DifficultyID`=0); -- 27636 (Azure Ley-Whelp) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27635 AND `DifficultyID`=0); -- 27635 (Azure Spellbinder) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27633 AND `DifficultyID`=0); -- 27633 (Azure Inquisitor) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=246, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27447 AND `DifficultyID`=0); -- 27447 (Varos Cloudstrider) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=246, `StaticFlags1`=536870912, `VerifiedBuild`=54904 WHERE (`Entry`=30879 AND `DifficultyID`=0); -- 30879 (Planar Anomaly) - Floating
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28472 WHERE (`Entry`=27653 AND `DifficultyID`=2); -- Phantasmal Water
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=29821 WHERE (`Entry`=28276 AND `DifficultyID`=2); -- Greater Ley-Whelp
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28438 WHERE (`Entry`=27645 AND `DifficultyID`=2); -- Phantasmal Cloudscraper
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28467 WHERE (`Entry`=27651 AND `DifficultyID`=2); -- Phantasmal Fire
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28478, `TypeFlags2`=128 WHERE (`Entry`=27654 AND `DifficultyID`=2); -- Drakos the Interrogator
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28452 WHERE (`Entry`=27648 AND `DifficultyID`=2); -- Phantasmal Naga
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28457 WHERE (`Entry`=27649 AND `DifficultyID`=2); -- Phantasmal Murloc
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28462 WHERE (`Entry`=27650 AND `DifficultyID`=2); -- Phantasmal Air
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28420 WHERE (`Entry`=27641 AND `DifficultyID`=2); -- Centrifuge Construct
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28491, `TypeFlags2`=128 WHERE (`Entry`=27656 AND `DifficultyID`=2); -- Ley-Guardian Eregos
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28426 WHERE (`Entry`=27642 AND `DifficultyID`=2); -- Phantasmal Mammoth
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=2, `CreatureDifficultyID`=29739 WHERE (`Entry`=28239 AND `DifficultyID`=2); -- Arcane Beam
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28415 WHERE (`Entry`=27640 AND `DifficultyID`=2); -- Ring-Lord Conjurer
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28448 WHERE (`Entry`=27647 AND `DifficultyID`=2); -- Phantasmal Ogre
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28410 WHERE (`Entry`=27639 AND `DifficultyID`=2); -- Ring-Lord Sorceress
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=29557 WHERE (`Entry`=28153 AND `DifficultyID`=2); -- Snowflake
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28009, `TypeFlags2`=128 WHERE (`Entry`=27447 AND `DifficultyID`=2); -- Varos Cloudstrider
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28381 WHERE (`Entry`=27633 AND `DifficultyID`=2); -- Azure Inquisitor
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28404 WHERE (`Entry`=27638 AND `DifficultyID`=2); -- Azure Ring Guardian
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28433 WHERE (`Entry`=27644 AND `DifficultyID`=2); -- Phantasmal Wolf
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28397 WHERE (`Entry`=27636 AND `DifficultyID`=2); -- Azure Ley-Whelp
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28483, `TypeFlags2`=128 WHERE (`Entry`=27655 AND `DifficultyID`=2); -- Mage-Lord Urom
UPDATE `creature_template_difficulty` SET `CreatureDifficultyID`=28392 WHERE (`Entry`=27635 AND `DifficultyID`=2); -- Azure Spellbinder
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=2, `HealthModifier`=10, `ManaModifier`=5, `CreatureDifficultyID`=195364, `TypeFlags`=8 WHERE (`Entry`=28236 AND `DifficultyID`=2); -- Azure Ring Captain
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=2, `HealthModifier`=5.952380180358886718, `CreatureDifficultyID`=195366, `TypeFlags`=72 WHERE (`Entry`=27755 AND `DifficultyID`=2); -- Amber Drake
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=2, `HealthModifier`=5.952380180358886718, `CreatureDifficultyID`=195367, `TypeFlags`=72 WHERE (`Entry`=27756 AND `DifficultyID`=2); -- Ruby Drake
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=3, `HealthModifier`=1.35000002384185791, `CreatureDifficultyID`=18489 WHERE (`Entry`=22517 AND `DifficultyID`=2); -- World Trigger (Large AOI)
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=2, `HealthModifier`=5.952380180358886718, `CreatureDifficultyID`=195365, `TypeFlags`=72 WHERE (`Entry`=27692 AND `DifficultyID`=2); -- Emerald Drake
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54904 WHERE (`DifficultyID`=2 AND `Entry` IN (53488,27649));
UPDATE `creature_template_difficulty` SET `StaticFlags1`=805830656, `VerifiedBuild`=54904 WHERE (`Entry`=28276 AND `DifficultyID`=2); -- 28276 (Greater Ley-Whelp) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1093, `VerifiedBuild`=54904 WHERE (`Entry`=28239 AND `DifficultyID`=2); -- 28239 (Arcane Beam) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMax`=1, `ContentTuningID`=1093, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=28153 AND `DifficultyID`=2); -- 28153 (Snowflake) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=805830656, `StaticFlags2`=0, `VerifiedBuild`=54904 WHERE (`Entry`=27656 AND `DifficultyID`=2); -- 27656 (Ley-Guardian Eregos) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27655 AND `DifficultyID`=2); -- 27655 (Mage-Lord Urom) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27654 AND `DifficultyID`=2); -- 27654 (Drakos the Interrogator) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27653 AND `DifficultyID`=2); -- 27653 (Phantasmal Water) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27651 AND `DifficultyID`=2); -- 27651 (Phantasmal Fire) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27650 AND `DifficultyID`=2); -- 27650 (Phantasmal Air) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `VerifiedBuild`=54904 WHERE (`Entry`=27648 AND `DifficultyID`=2); -- 27648 (Phantasmal Naga) - Amphibious
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27647 AND `DifficultyID`=2); -- 27647 (Phantasmal Ogre) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=0, `LevelScalingDeltaMax`=0, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27645 AND `DifficultyID`=2); -- 27645 (Phantasmal Cloudscraper) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=0, `LevelScalingDeltaMax`=0, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27644 AND `DifficultyID`=2); -- 27644 (Phantasmal Wolf) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMax`=0, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27642 AND `DifficultyID`=2); -- 27642 (Phantasmal Mammoth) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27641 AND `DifficultyID`=2); -- 27641 (Centrifuge Construct) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27640 AND `DifficultyID`=2); -- 27640 (Ring-Lord Conjurer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27639 AND `DifficultyID`=2); -- 27639 (Ring-Lord Sorceress) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=805830656, `VerifiedBuild`=54904 WHERE (`Entry`=27638 AND `DifficultyID`=2); -- 27638 (Azure Ring Guardian) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27636 AND `DifficultyID`=2); -- 27636 (Azure Ley-Whelp) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27635 AND `DifficultyID`=2); -- 27635 (Azure Spellbinder) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27633 AND `DifficultyID`=2); -- 27633 (Azure Inquisitor) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27447 AND `DifficultyID`=2); -- 27447 (Varos Cloudstrider) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1019, `VerifiedBuild`=54904 WHERE (`Entry`=30161 AND `DifficultyID`=2); -- 30161 (Wyrmrest Skytalon) - 

-- Models
UPDATE `creature_model_info` SET `VerifiedBuild`=54904 WHERE `DisplayID` IN (27718, 26089, 28046, 28419, 17612, 25347, 25011, 25852, 25853, 24746, 24759, 24742, 27034, 25010, 27032, 5450, 25148, 25146, 25149, 25150, 25151, 25147, 25153, 25145, 24943, 25303, 25304, 25302, 25305, 25307, 25306, 28080, 26088, 25250, 25195, 27033);
UPDATE `creature_model_info` SET `BoundingRadius`=2.23112344741821289, `VerifiedBuild`=54904 WHERE `DisplayID`=25854;
UPDATE `creature_model_info` SET `BoundingRadius`=2, `CombatReach`=4, `VerifiedBuild`=54904 WHERE `DisplayID`=28107;
UPDATE `creature_template_model` SET `VerifiedBuild`=54904 WHERE (`Idx`=0 AND `CreatureID` IN (27655,28183,27638,27644,28236,28166,32261,27651,27657,27645,27659,27447,27658,27653,27650,27635,27692,27647,28276,28012,28239,27755,27649,27642,28153,27756,27641,27636,27656,27648,27654,27633,27639,27640)) OR (`Idx`=1 AND `CreatureID` IN (28183,28239,27639,27640)) OR (`Idx`=2 AND `CreatureID` IN (27639,27640));
UPDATE `creature_template_model` SET `VerifiedBuild`=54904 WHERE (`CreatureID`=30879 AND `Idx` IN (1,0));

-- Gameobject templates
UPDATE `gameobject_template` SET `VerifiedBuild`=54904 WHERE `entry` IN (191351, 190172, 190343, 189985, 188715);
UPDATE `gameobject_template` SET `Data1`=0, `Data15`=0, `Data25`=2013, `Data30`=24462, `ContentTuningId`=245, `VerifiedBuild`=54904 WHERE `entry`=191349; -- Cache of Eregos
UPDATE `gameobject_template` SET `Data8`=0, `VerifiedBuild`=54904 WHERE `entry`=189986; -- Dragon Cage
UPDATE `gameobject_template` SET `Data8`=0, `VerifiedBuild`=54904 WHERE `entry`=193995; -- Dragon Cage Door
UPDATE `gameobject_template` SET `ContentTuningId`=245, `VerifiedBuild`=54904 WHERE `entry`=190402; -- Blaze

UPDATE `gameobject_template_addon` SET `flags`=2113536 WHERE `entry`=191349; -- Cache of Eregos


-- Creature templates
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=27949; -- Alliance Commander
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=27837; -- Nexus 70 - Buying Time Bunny
UPDATE `creature_template` SET `unit_flags3`=524289 WHERE `entry`=27101; -- Crystal Spike Initial Trigger
UPDATE `creature_template` SET `speed_run`=1.428571462631225585, `unit_flags3`=524289 WHERE `entry`=27099; -- Crystal Spike
UPDATE `creature_template` SET `unit_flags2`=2080, `unit_flags3`=524289 WHERE `entry`=27079; -- Crystal Spike Trigger
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26805; -- Alliance Cleric
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26802; -- Alliance Ranger
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26800; -- Alliance Berserker
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26794; -- Ormorok the Tree-Shaper
UPDATE `creature_template` SET `unit_flags`=64 WHERE `entry`=26792; -- Crystalline Protector
UPDATE `creature_template` SET `unit_flags`=64 WHERE `entry`=26782; -- Crystalline Keeper
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26763; -- Anomalus
UPDATE `creature_template` SET `unit_flags`=320, `unit_flags2`=2048, `unit_flags3`=67108864 WHERE `entry`=26761; -- Crazed Mana-Wyrm
UPDATE `creature_template` SET `unit_flags`=64, `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=26737; -- Crazed Mana-Surge
UPDATE `creature_template` SET `unit_flags`=320, `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=26736; -- Azure Skyrazor
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26731; -- Grand Magus Telestra
UPDATE `creature_template` SET `unit_flags`=64, `unit_flags3`=67108864 WHERE `entry`=26730; -- Mage Slayer
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26723; -- Keristrasza
UPDATE `creature_template` SET `unit_flags`=64, `unit_flags2`=2048 WHERE `entry`=26722; -- Azure Magus
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26716; -- Azure Warder
UPDATE `creature_template` SET `unit_flags2`=0, `unit_flags3`=524288 WHERE `entry`=32665; -- Crystalline Tangler
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26796; -- Commander Stoutbeard
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=27947; -- Horde Commander
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26803; -- Horde Cleric
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26801; -- Horde Ranger
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26799; -- Horde Berserker
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=26798; -- Commander Kolurg

UPDATE `creature_template_addon` SET `AnimTier`=3 WHERE `entry`=26761; -- 26761 (Crazed Mana-Wyrm) - Permanent Feign Death
UPDATE `creature_template_addon` SET `AnimTier`=3 WHERE `entry`=26736; -- 26736 (Azure Skyrazor)

-- Difficulties
UPDATE `creature_template_difficulty` ctd
LEFT JOIN `creature_template_difficulty` ctd2 ON ctd.`Entry`=ctd2.`Entry`
SET
    ctd.`DamageModifier`=ctd2.`DamageModifier`,
    ctd.`LootID`=ctd2.`LootID`,
    ctd.`PickPocketLootID`=ctd2.`PickPocketLootID`,
    ctd.`SkinLootID`=ctd2.`SkinLootID`,
    ctd.`GoldMin`=ctd2.`GoldMin`,
    ctd.`GoldMax`=ctd2.`GoldMax`
WHERE ctd.`Entry` IN (26798,27947,26803,26801,26799,26796,55537,55536,55535,27837,54638,55531,26793,28231,27949,27101,27099,27079,27048,26918,26805,26802,26800,26794,26792,26782,26763,26761,26746,26737,26736,26735,26734,26731,26730,26729,26728,26727,26723,26722,26716)
AND ctd.`DifficultyID`=0 AND ctd2.`DifficultyID`=1 AND ctd2.`Entry` IS NOT NULL;

DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=1 AND `Entry` IN (26798,27947,26803,26801,26799,26796,55537,55536,55535,27837,54638,55531,26793,28231,27949,27101,27099,27079,27048,26918,26805,26802,26800,26794,26792,26782,26763,26761,26746,26737,26736,26735,26734,26731,26730,26729,26728,26727,26723,26722,26716));

DELETE FROM `creature_template_difficulty` WHERE `DifficultyID`=2 AND `Entry`=22515;
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `LevelScalingDeltaMin`, `LevelScalingDeltaMax`, `ContentTuningID`, `StaticFlags1`, `StaticFlags2`, `StaticFlags3`, `StaticFlags4`, `StaticFlags5`, `StaticFlags6`, `StaticFlags7`, `StaticFlags8`, `VerifiedBuild`) VALUES
(22515, 2, 0, 0, 2672, 536870912, 0, 0, 0, 0, 0, 0, 0, 54904); -- 22515 (World Trigger) - Floating

UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=3, `ContentTuningID`=2671, `VerifiedBuild`=54904 WHERE (`Entry`=22515 AND `DifficultyID`=0); -- 22515 (World Trigger) - Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=245, `VerifiedBuild`=54904 WHERE (`Entry`=55537 AND `DifficultyID`=0); -- 55537 (Image of Warmage Kaitlyn) - CannotTurn
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=245, `VerifiedBuild`=54904 WHERE (`Entry`=55536 AND `DifficultyID`=0); -- 55536 (Image of Warmage Kaitlyn) - CannotTurn
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=245, `VerifiedBuild`=54904 WHERE (`Entry`=55535 AND `DifficultyID`=0); -- 55535 (Image of Warmage Kaitlyn) - CannotTurn
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=245, `StaticFlags1`=536870912, `VerifiedBuild`=54904 WHERE (`Entry`=55531 AND `DifficultyID`=0); -- 55531 (Warmage Kaitlyn) - Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54904 WHERE (`Entry`=54638 AND `DifficultyID`=0);
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26793 AND `DifficultyID`=0); -- 26793 (Crystalline Frayer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=28231 AND `DifficultyID`=0); -- 28231 (Crystalline Tender) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27949 AND `DifficultyID`=0); -- 27949 (Alliance Commander) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=47, `VerifiedBuild`=54904 WHERE (`Entry`=27837 AND `DifficultyID`=0); -- 27837 (Nexus 70 - Buying Time Bunny) - Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=245, `StaticFlags1`=536871168, `VerifiedBuild`=54904 WHERE (`Entry`=27101 AND `DifficultyID`=0); -- 27101 (Crystal Spike Initial Trigger) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=245, `StaticFlags1`=536871168, `VerifiedBuild`=54904 WHERE (`Entry`=27099 AND `DifficultyID`=0); -- 27099 (Crystal Spike) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=245, `StaticFlags1`=536871168, `VerifiedBuild`=54904 WHERE (`Entry`=27079 AND `DifficultyID`=0); -- 27079 (Crystal Spike Trigger) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-1, `LevelScalingDeltaMax`=-1, `ContentTuningID`=245, `StaticFlags1`=536870912, `VerifiedBuild`=54904 WHERE (`Entry`=27048 AND `DifficultyID`=0); -- 27048 (Breath Caster) - Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=245, `StaticFlags1`=805830912, `VerifiedBuild`=54904 WHERE (`Entry`=26918 AND `DifficultyID`=0); -- 26918 (Chaotic Rift) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26805 AND `DifficultyID`=0); -- 26805 (Alliance Cleric) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26802 AND `DifficultyID`=0); -- 26802 (Alliance Ranger) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26800 AND `DifficultyID`=0); -- 26800 (Alliance Berserker) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26794 AND `DifficultyID`=0); -- 26794 (Ormorok the Tree-Shaper) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26792 AND `DifficultyID`=0); -- 26792 (Crystalline Protector) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26782 AND `DifficultyID`=0); -- 26782 (Crystalline Keeper) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26763 AND `DifficultyID`=0); -- 26763 (Anomalus) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=805830656, `VerifiedBuild`=54904 WHERE (`Entry`=26761 AND `DifficultyID`=0); -- 26761 (Crazed Mana-Wyrm) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26746 AND `DifficultyID`=0); -- 26746 (Crazed Mana-Wraith) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26737 AND `DifficultyID`=0); -- 26737 (Crazed Mana-Surge) - Sessile, Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=805830656, `VerifiedBuild`=54904 WHERE (`Entry`=26736 AND `DifficultyID`=0); -- 26736 (Azure Skyrazor) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26735 AND `DifficultyID`=0); -- 26735 (Azure Scale-Binder) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26734 AND `DifficultyID`=0); -- 26734 (Azure Enforcer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26731 AND `DifficultyID`=0); -- 26731 (Grand Magus Telestra) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26730 AND `DifficultyID`=0); -- 26730 (Mage Slayer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26729 AND `DifficultyID`=0); -- 26729 (Steward) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26728 AND `DifficultyID`=0); -- 26728 (Mage Hunter Initiate) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26727 AND `DifficultyID`=0); -- 26727 (Mage Hunter Ascendant) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26723 AND `DifficultyID`=0); -- 26723 (Keristrasza) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26722 AND `DifficultyID`=0); -- 26722 (Azure Magus) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26716 AND `DifficultyID`=0); -- 26716 (Azure Warder) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26681 WHERE (`Entry`=26805 AND `DifficultyID`=2); -- Alliance Cleric
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=29715 WHERE (`Entry`=28231 AND `DifficultyID`=2); -- Crystalline Tender
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26476 WHERE (`Entry`=26734 AND `DifficultyID`=2); -- Azure Enforcer
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26456 WHERE (`Entry`=26729 AND `DifficultyID`=2); -- Steward
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26599 WHERE (`Entry`=26782 AND `DifficultyID`=2); -- Crystalline Keeper
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26481 WHERE (`Entry`=26735 AND `DifficultyID`=2); -- Azure Scale-Binder
UPDATE `creature_template_difficulty` SET `HealthModifier`=0.192013502120971679, `CreatureDifficultyID`=26486 WHERE (`Entry`=26736 AND `DifficultyID`=2); -- Azure Skyrazor
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26445 WHERE (`Entry`=26727 AND `DifficultyID`=2); -- Mage Hunter Ascendant
UPDATE `creature_template_difficulty` SET `HealthModifier`=1.78125, `CreatureDifficultyID`=26514 WHERE (`Entry`=26746 AND `DifficultyID`=2); -- Crazed Mana-Wraith
UPDATE `creature_template_difficulty` SET `HealthModifier`=40, `CreatureDifficultyID`=26631, `TypeFlags2`=128 WHERE (`Entry`=26794 AND `DifficultyID`=2); -- Ormorok the Tree-Shaper
UPDATE `creature_template_difficulty` SET `HealthModifier`=18.75, `CreatureDifficultyID`=26620 WHERE (`Entry`=26792 AND `DifficultyID`=2); -- Crystalline Protector
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26451 WHERE (`Entry`=26728 AND `DifficultyID`=2); -- Mage Hunter Initiate
UPDATE `creature_template_difficulty` SET `HealthModifier`=46.875, `CreatureDifficultyID`=26639, `TypeFlags2`=128 WHERE (`Entry`=26796 AND `DifficultyID`=2); -- Commander Stoutbeard
UPDATE `creature_template_difficulty` SET `HealthModifier`=18.75, `CreatureDifficultyID`=26428 WHERE (`Entry`=26722 AND `DifficultyID`=2); -- Azure Magus
UPDATE `creature_template_difficulty` SET `HealthModifier`=0.076806001365184783, `CreatureDifficultyID`=26550, `TypeFlags`=1025 WHERE (`Entry`=26761 AND `DifficultyID`=2); -- Crazed Mana-Wyrm
UPDATE `creature_template_difficulty` SET `HealthModifier`=22.5, `CreatureDifficultyID`=26466, `TypeFlags2`=128 WHERE (`Entry`=26731 AND `DifficultyID`=2); -- Grand Magus Telestra
UPDATE `creature_template_difficulty` SET `HealthModifier`=18.75, `CreatureDifficultyID`=26415 WHERE (`Entry`=26716 AND `DifficultyID`=2); -- Azure Warder
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26492 WHERE (`Entry`=26737 AND `DifficultyID`=2); -- Crazed Mana-Surge
UPDATE `creature_template_difficulty` SET `HealthModifier`=0.75, `CreatureDifficultyID`=27264 WHERE (`Entry`=27099 AND `DifficultyID`=2); -- Crystal Spike
UPDATE `creature_template_difficulty` SET `HealthModifier`=0.84375, `CreatureDifficultyID`=26626 WHERE (`Entry`=26793 AND `DifficultyID`=2); -- Crystalline Frayer
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26668 WHERE (`Entry`=26802 AND `DifficultyID`=2); -- Alliance Ranger
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26461 WHERE (`Entry`=26730 AND `DifficultyID`=2); -- Mage Slayer
UPDATE `creature_template_difficulty` SET `HealthModifier`=4.21875, `CreatureDifficultyID`=26921 WHERE (`Entry`=26918 AND `DifficultyID`=2); -- Chaotic Rift
UPDATE `creature_template_difficulty` SET `HealthModifier`=40, `CreatureDifficultyID`=26557, `TypeFlags2`=128 WHERE (`Entry`=26763 AND `DifficultyID`=2); -- Anomalus
UPDATE `creature_template_difficulty` SET `HealthModifier`=9.375, `CreatureDifficultyID`=26657 WHERE (`Entry`=26800 AND `DifficultyID`=2); -- Alliance Berserker
UPDATE `creature_template_difficulty` SET `HealthModifier`=47.5, `CreatureDifficultyID`=26433, `TypeFlags2`=128 WHERE (`Entry`=26723 AND `DifficultyID`=2); -- Keristrasza
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26793 AND `DifficultyID`=2); -- 26793 (Crystalline Frayer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=28231 AND `DifficultyID`=2); -- 28231 (Crystalline Tender) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=1094, `StaticFlags1`=536871168, `VerifiedBuild`=54904 WHERE (`Entry`=27099 AND `DifficultyID`=2); -- 27099 (Crystal Spike) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMax`=0, `ContentTuningID`=1094, `StaticFlags1`=805830912, `VerifiedBuild`=54904 WHERE (`Entry`=26918 AND `DifficultyID`=2); -- 26918 (Chaotic Rift) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26805 AND `DifficultyID`=2); -- 26805 (Alliance Cleric) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26802 AND `DifficultyID`=2); -- 26802 (Alliance Ranger) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26800 AND `DifficultyID`=2); -- 26800 (Alliance Berserker) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26796 AND `DifficultyID`=2); -- 26796 (Commander Stoutbeard) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26794 AND `DifficultyID`=2); -- 26794 (Ormorok the Tree-Shaper) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26792 AND `DifficultyID`=2); -- 26792 (Crystalline Protector) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26782 AND `DifficultyID`=2); -- 26782 (Crystalline Keeper) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26763 AND `DifficultyID`=2); -- 26763 (Anomalus) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=805830656, `VerifiedBuild`=54904 WHERE (`Entry`=26761 AND `DifficultyID`=2); -- 26761 (Crazed Mana-Wyrm) - Sessile, Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26746 AND `DifficultyID`=2); -- 26746 (Crazed Mana-Wraith) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26737 AND `DifficultyID`=2); -- 26737 (Crazed Mana-Surge) - Sessile, Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=805830656, `VerifiedBuild`=54904 WHERE (`Entry`=26736 AND `DifficultyID`=2); -- 26736 (Azure Skyrazor) - Amphibious, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26735 AND `DifficultyID`=2); -- 26735 (Azure Scale-Binder) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26734 AND `DifficultyID`=2); -- 26734 (Azure Enforcer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26731 AND `DifficultyID`=2); -- 26731 (Grand Magus Telestra) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26730 AND `DifficultyID`=2); -- 26730 (Mage Slayer) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26729 AND `DifficultyID`=2); -- 26729 (Steward) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26728 AND `DifficultyID`=2); -- 26728 (Mage Hunter Initiate) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26727 AND `DifficultyID`=2); -- 26727 (Mage Hunter Ascendant) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26723 AND `DifficultyID`=2); -- 26723 (Keristrasza) - Sessile, Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26722 AND `DifficultyID`=2); -- 26722 (Azure Magus) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=1094, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26716 AND `DifficultyID`=2); -- 26716 (Azure Warder) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=805306624, `VerifiedBuild`=54904 WHERE (`Entry`=32665 AND `DifficultyID`=0);
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26796 AND `DifficultyID`=0);
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=27947 AND `DifficultyID`=0); -- 27947 (Horde Commander) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26803 AND `DifficultyID`=0); -- 26803 (Horde Cleric) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26801 AND `DifficultyID`=0); -- 26801 (Horde Ranger) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26799 AND `DifficultyID`=0); -- 26799 (Horde Berserker) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26803 AND `DifficultyID`=2); -- 26803 (Horde Cleric) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26801 AND `DifficultyID`=2); -- 26801 (Horde Ranger) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26799 AND `DifficultyID`=2); -- 26799 (Horde Berserker) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26798 AND `DifficultyID`=2); -- 26798 (Commander Kolurg) - Amphibious, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=245, `StaticFlags1`=268959744, `VerifiedBuild`=54904 WHERE (`Entry`=26798 AND `DifficultyID`=0);

DELETE FROM `creature_questitem` WHERE (`CreatureEntry`=26792 AND `DifficultyID`=1);

DELETE FROM `creature_quest_currency` WHERE (`CurrencyId`=1166 AND `CreatureId` IN (26798,26796,26794,26731,26763,26723));
INSERT INTO `creature_quest_currency` (`CreatureId`, `CurrencyId`, `VerifiedBuild`) VALUES
(26798, 1166, 54904), -- Commander Kolurg - 1166
(26796, 1166, 54904), -- Commander Stoutbeard - 1166
(26794, 1166, 54904), -- Ormorok the Tree-Shaper - 1166
(26731, 1166, 54904), -- Grand Magus Telestra - 1166
(26763, 1166, 54904), -- Anomalus - 1166
(26723, 1166, 54904); -- Keristrasza - 1166

-- Models
UPDATE `creature_model_info` SET `VerifiedBuild`=54904 WHERE `DisplayID` IN (39155, 25570, 27609, 24366, 19595, 23767, 25206, 24356, 24355, 24353, 26298, 27472, 27469, 26259, 42981, 27810, 14253, 14886, 24066, 24906, 24323, 24320, 24322, 24321, 24318, 24313, 24312, 24314, 24307, 25249, 25251);
UPDATE `creature_template_model` SET `VerifiedBuild`=54904 WHERE (`Idx`=0 AND `CreatureID` IN (28231,27837,26734,26792,22515,27048,26763,54638,26728,55536,26737,26716,26730,55535,29911,27101,27099,26736,26800,27079,26793,26802,26723,55537,26918,26805,26761,26729,55531,26735,27949,26794,26731,26722,26782,26746,26727)) OR (`Idx`=1 AND `CreatureID` IN (27837,22515,27048,54638,26728,26918,26729,26727)) OR (`Idx`=3 AND `CreatureID` IN (26728,26729,26727)) OR (`Idx`=2 AND `CreatureID` IN (26728,26729,26727));

-- Gameobject templates
UPDATE `gameobject_template` SET `VerifiedBuild`=54904 WHERE `entry` IN (188537, 192788, 191016, 188527, 190174, 188526, 181030, 188528);

UPDATE `gameobject_template_addon` SET `flags`=4 WHERE `entry`=192788; -- Berinand's Research

UPDATE `gameobject_questitem` SET `VerifiedBuild`=54904 WHERE (`GameObjectEntry`=192788 AND `Idx`=0);




DELETE FROM `disables` WHERE `sourceType`=9 AND `entry`=11595;
INSERT INTO `disables` (`sourceType`, `entry`, `flags`, `params_0`, `params_1`, `comment`) VALUES 
(9, 11595, 0, '', '', 'Disable Darnassian and Gilnean Refugees phase');


DELETE FROM `creature_template_addon` WHERE `entry` IN (139250 /*139250 (Abyssal Spawn) - Void Form*/);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(139250, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '266301'); -- 139250 (Abyssal Spawn) - Void Form

UPDATE `creature_template_difficulty` SET `ContentTuningID`=188, `StaticFlags1`=268435456, `VerifiedBuild`=54762 WHERE (`Entry`=139250 AND `DifficultyID`=0); -- 139250 (Abyssal Spawn) - CanSwim

UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags2`=2048, `unit_flags3`=524288 WHERE `entry`=139250; -- Abyssal Spawn

-- Umbral Hulk smart ai
SET @ENTRY := 148913;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(@ENTRY, 0, 0, 0, '', 0, 0, 100, 0, 12700, 12700, 14600, 14600, 11, 262739, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Every 14.6 seconds (12.7s initially) (IC) - Self: Cast spell 262739 on Victim'),
(@ENTRY, 0, 1, 0, '', 0, 0, 100, 0, 16300, 16300, 14600, 14600, 11, 262719, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Every 14.6 seconds (16.3s initially) (IC) - Self: Cast spell 262719 on Self');

-- Drowned Harbinger smart ai
SET @ENTRY := 148912;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY AND `id` BETWEEN 2 AND 6;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(@ENTRY, 0, 2, 0, '', 4, 0, 10, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On aggro - Self: Talk 0 to invoker'),
(@ENTRY, 0, 3, 0, '', 6, 0, 10, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On death - Self: Talk 1 to invoker'),
(@ENTRY, 0, 4, 0, '', 0, 0, 100, 0, 14000, 14000, 17000, 25000, 11, 273544, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Every 17 - 25 seconds (14 - 14s initially) (IC) - Self: Cast spell  273544 on Self'),
(@ENTRY, 0, 5, 0, '', 0, 0, 100, 0, 22500, 22500, 18300, 23100, 11, 273467, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Every 18.3 - 23.1 seconds (22.5 - 22.5s initially) (IC) - Self: Cast spell  273467 on Victim'),
(@ENTRY, 0, 6, 0, '', 0, 0, 100, 0, 7900, 7900, 23100, 32800, 11, 264892, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Every 23.1 - 32.8 seconds (7.9 - 7.9s initially) (IC) - Self: Cast spell  264892 on Victim');

-- Drowned Convert smart ai
SET @ENTRY := 148911;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(@ENTRY, 0, 0, 0, '', 4, 0, 10, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On aggro - Self: Talk 0 to invoker'),
(@ENTRY, 0, 1, 0, '', 6, 0, 10, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On death - Self: Talk 1 to invoker'),
(@ENTRY, 0, 2, 0, '', 0, 0, 100, 0, 6000, 8500, 6000, 8500, 11, 274061, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Every 6 - 8.5 seconds (IC) - Self: Cast spell  274061 on Victim');

-- Corrupted Waters smart ai
SET @ENTRY := 148915;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(@ENTRY, 0, 0, 0, '', 6, 0, 100, 0, 0, 0, 0, 0, 11, 294503, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On death - Self: Cast spell  294503 on Self');

DELETE FROM `areatrigger_template` WHERE (`IsCustom`=0 AND `Id` IN (18235,18242,20893));
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `VerifiedBuild`) VALUES
(18235, 0, 0, 54762),
(18242, 0, 0, 54762),
(20893, 0, 0, 54762);

DELETE FROM `areatrigger_create_properties` WHERE (`IsCustom`=0 AND `Id` IN (13734,13744,16494));
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `TimeToTarget`, `TimeToTargetScale`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `VerifiedBuild`) VALUES
(13734, 0, 18235, 0, 0, 0, 0, 0, 0, -1, 0, 0, 5038, 15000, 4, 2, 2, 10, 10, 0, 0, 0, 0, 54762), -- Spell: 273467 (Void Orb)
(13744, 0, 18242, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 4000, 0, 1, 1, 0, 0, 0, 0, 0, 0, 54762), -- Spell: 273544 (Abyssal Portal)
(16494, 0, 20893, 0, 0, 0, 0, 0, 0, -1, 0, 228, 0, 10000, 0, 2, 2, 0, 0, 0, 0, 0, 0, 54762); -- Spell: 294503 (Pooled Corruption)

DELETE FROM `areatrigger_template_actions` WHERE `AreaTriggerId`=20893 AND `IsCustom`=0;
INSERT INTO `areatrigger_template_actions` (`AreaTriggerId`, `IsCustom`, `ActionType`, `ActionParam`, `TargetType`) VALUES
(20893, 0, 1, 294504, 2); -- Spell: 294503 (Pooled Corruption)

UPDATE `areatrigger_create_properties` SET `ScriptName`='at_void_orb_harbinger' WHERE `Id`=13734 AND `IsCustom`=0;
UPDATE `areatrigger_create_properties` SET `ScriptName`='at_abyssal_portal_harbinger' WHERE `Id`=13744 AND `IsCustom`=0;

DELETE FROM `creature_text` WHERE `CreatureID` IN(148911, 148912);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(148911, 0, 0, 'Sleep eternal in Ny''alotha.', 12, 178, 100, 1, 0, 103716, 149632, 0, 'Drowned Convert to Player'),
(148911, 0, 1, 'Gaze upon the shadow of your demise.', 12, 0, 100, 1, 0, 103714, 149629, 0, 'Drowned Convert to Player'),
(148911, 0, 2, 'Light dies in the endless fathoms.', 12, 0, 100, 1, 0, 103715, 149631, 0, 'Drowned Convert to Player'),
(148911, 0, 3, 'The stars fear our ascension.', 12, 0, 100, 1, 0, 103718, 149635, 0, 'Drowned Convert to Player'),
(148911, 0, 4, 'Your soul will know only hunger.', 12, 0, 100, 0, 0, 103716, 149636, 0, 'Drowned Convert to Player'),
(148911, 0, 5, 'Drown in the blood of the infinite.', 12, 0, 100, 1, 0, 103720, 149643, 0, 'Drowned Convert to Player'),
(148911, 1, 0, 'The seed... takes root...', 12, 0, 100, 1, 0, 103726, 149675, 0, 'Drowned Convert to Frothing Pustule'),
(148911, 1, 1, 'Even death... may... die...', 12, 0, 100, 1, 0, 103723, 149672, 0, 'Drowned Convert to Frothing Pustule'),
(148911, 1, 2, 'The endless shadow... embraces me...', 12, 0, 100, 1, 0, 103728, 149678, 0, 'Drowned Convert to Player'),
(148911, 1, 3, 'The cycle... continues...', 12, 0, 100, 0, 0, 103729, 149673, 0, 'Drowned Convert to Player'), -- BroadcastTextID: 90884 - 149673
(148911, 1, 4, 'I am but one... of many...', 12, 0, 100, 0, 0, 103725, 149674, 0, 'Drowned Convert to Player'),
(148911, 1, 5, 'I sleep... only to wake...', 12, 0, 100, 0, 0, 103729, 149680, 0, 'Drowned Convert to Player'),
-- 
(148912, 0, 0, 'Sleep eternal in Ny''alotha.', 12, 178, 100, 0, 0, 103716, 149632, 0, 'Drowned Harbinger to Player'),
(148912, 0, 1, 'Gaze upon the shadow of your demise.', 12, 0, 100, 0, 0, 103714, 149629, 0, 'Drowned Harbinger to Player'),
(148912, 0, 2, 'The currents wash you into the abyss.', 12, 0, 100, 0, 0, 103721, 149668, 0, 'Drowned Harbinger to Player'),
(148912, 0, 3, 'Light dies in the endless fathoms.', 12, 0, 100, 0, 0, 103715, 149631, 0, 'Drowned Harbinger to Player'),
(148912, 0, 4, 'Know the embrace of true nothingness.', 12, 0, 100, 0, 0, 103717, 149633, 0, 'Drowned Harbinger to Player'),
(148912, 0, 5, 'Your soul will know only hunger.', 12, 0, 100, 0, 0, 103716, 149636, 0, 'Drowned Harbinger to Player'),
(148912, 0, 6, 'Drown in the blood of the infinite.', 12, 0, 100, 1, 0, 103720, 149643, 0, 'Drowned Harbinger to Player'),
(148912, 1, 0, 'The seed... takes root...', 12, 0, 100, 0, 0, 103726, 149675, 0, 'Drowned Harbinger to Player'),
(148912, 1, 1, 'In the end... you will know the truth...', 12, 0, 100, 0, 0, 103722, 149671, 0, 'Drowned Harbinger to Player'),
(148912, 1, 2, 'The cycle... continues...', 12, 0, 100, 0, 0, 103724, 149673, 0, 'Drowned Harbinger to Frothing Pustule'), -- BroadcastTextID: 90884 - 149673
(148912, 1, 3, 'Your hope... wanes...', 12, 0, 100, 0, 0, 103727, 149677, 0, 'Drowned Harbinger to Player'),
(148912, 1, 4, 'I am but one... of many...', 12, 0, 100, 0, 0, 103725, 149674, 0, 'Drowned Harbinger to Player'),
(148912, 1, 5, 'Even death... may... die...', 12, 0, 100, 1, 0, 103723, 149672, 0, 'Drowned Harbinger to Frothing Pustule'),
(148912, 1, 6, 'The endless shadow... embraces me...', 12, 0, 100, 1, 0, 103728, 149678, 0, 'Drowned Harbinger to Player');





SET @ATID := 67;
SEt @ATCP := 55;
SET @ATIDSPAWN := 69;

-- Instance Template
DELETE FROM `instance_template` WHERE `map` = 1762;
INSERT INTO `instance_template` (`map`, `parent`, `script`) VALUES
(1762, 0, 'instance_kings_rest');

-- Creature Template Addon
DELETE FROM `creature_template_addon` WHERE `entry` IN (137020 /*137020 (Shadow of Zul) - Zul Shadowform*/);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(137020, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '269058'); -- 137020 (Shadow of Zul) - Zul Shadowform

-- Areatrigger
DELETE FROM `areatrigger_template` WHERE (`Id` = @ATID+0 AND `IsCustom` = 1);
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `VerifiedBuild`) VALUES 
(@ATID+0, 1, 1, 0);

DELETE FROM `areatrigger_create_properties` WHERE (`Id`= @ATCP+0 AND `IsCustom`=1);
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `TimeToTarget`, `TimeToTargetScale`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`) VALUES 
(@ATCP+0, 1, @ATID+0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 16, 18, 10, 16, 18, 10, 0, 0, '', 0);

DELETE FROM `areatrigger` WHERE `SpawnId` = @ATIDSPAWN+0;
INSERT INTO `areatrigger` (`SpawnId`, `AreaTriggerCreatePropertiesId`, `IsCustom`, `MapId`, `SpawnDifficulties`, `PosX`, `PosY`, `PosZ`, `Orientation`, `PhaseUseFlags`, `PhaseId`, `PhaseGroup`, `ScriptName`, `Comment`, `VerifiedBuild`) VALUES 
(@ATIDSPAWN+0, @ATCP+0, 1, 1762, '23,8,2', -945.071, 2602.210, 833.052, 1.556985, 0, 0, 0, 'at_kings_rest_trigger_intro_event_with_zul', 'KingsRest - Trigger intro Conversation for Zul', 0);

-- Conversation
DELETE FROM `conversation_template` WHERE `Id`=7690;
INSERT INTO `conversation_template` (`Id`, `FirstLineID`, `TextureKitId`, `VerifiedBuild`) VALUES
(7690, 17525, 0, 54904);

UPDATE `conversation_template` SET `ScriptName` = 'conversation_kings_rest_intro' WHERE `Id` = 7690;

DELETE FROM `conversation_actors` WHERE (`ConversationId`=7690 AND `Idx`=0);
INSERT INTO `conversation_actors` (`ConversationId`, `ConversationActorId`, `Idx`, `CreatureId`, `CreatureDisplayInfoId`, `NoActorObject`, `ActivePlayerObject`, `VerifiedBuild`) VALUES
(7690, 64206, 0, 0, 0, 0, 0, 54904); -- Full: 0x0

DELETE FROM `conversation_line_template` WHERE `Id` IN (17527, 17526, 17525);
INSERT INTO `conversation_line_template` (`Id`, `UiCameraID`, `ActorIdx`, `Flags`, `ChatType`, `VerifiedBuild`) VALUES
(17527, 0, 0, 0, 1, 54904),
(17526, 0, 0, 0, 1, 54904),
(17525, 0, 0, 0, 1, 54904);





-- Invalid DisplayIds
UPDATE `creature` SET `modelid`=0 WHERE `modelid` IN (1,239,251,252,370,533,959,1041,1405,2709,12346,14952,15939,16051,16622,18736,19338,21360,22106,22748,23033,23148,23504,23646,24446,24472,24922,24923,25030,25452,25572,25579,26101,26198,26223,26273,26363,26590,26762,27029,27074,27861,28283,35014);


SET @CGUID := 5000400;
SET @OGUID := 5000056;
SET @ATID := 68;

-- Creatures
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+156;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`, `size`) VALUES
(@CGUID+0, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1491.3228759765625, 2961.841064453125, 36.23116683959960937, 3.980869293212890625, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+1, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1529.671875, 2960.15283203125, 35.69658279418945312, 4.713648319244384765, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+2, 77819, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1667.1788330078125, 3077.600830078125, 36.67950057983398437, 5.796135902404785156, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Vigilant (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 156893 - Auchindoun Defense Construct)
(@CGUID+3, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1505.154541015625, 2962.052978515625, 36.231170654296875, 4.713648319244384765, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+4, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1583.7552490234375, 2961.96435546875, 36.23111724853515625, 4.05728912353515625, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+5, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1498.9896240234375, 2947.382080078125, 35.32217025756835937, 2.783493280410766601, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+6, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1553.6910400390625, 2944.291748046875, 35.32229232788085937, 0.073448918759822845, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+7, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1495.236083984375, 2945.859375, 35.69659042358398437, 2.554013013839721679, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+8, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1540.2100830078125, 2957.69091796875, 35.32219696044921875, 3.189603090286254882, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+9, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1505.8541259765625, 2944.1259765625, 36.231170654296875, 1.649249792098999023, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+10, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1563.2447509765625, 2959.335205078125, 35.32232666015625, 1.328914165496826171, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+11, 79248, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1543.7117919921875, 2955.87841796875, 35.32222747802734375, 3.343645572662353515, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Soulbinder (Area: Auchindoun - Difficulty: 0) CreateObject1
(@CGUID+12, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1498.8021240234375, 2958.611083984375, 35.32217025756835937, 3.472944498062133789, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+13, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1506.546875, 2960.385498046875, 35.69659042358398437, 4.713648319244384765, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+14, 77819, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1724.7291259765625, 3153.71533203125, 36.67934799194335937, 5.423369884490966796, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Vigilant (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 156893 - Auchindoun Defense Construct)
(@CGUID+15, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1527.439208984375, 2960.072998046875, 35.69659042358398437, 4.774088382720947265, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+16, 77704, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1529.6690673828125, 2949.3115234375, 35.22933197021484375, 0.005536025390028953, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Warden (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 155647 - NPC Reaction)
(@CGUID+17, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1592.4288330078125, 2961.96435546875, 36.23116302490234375, 4.64665842056274414, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+18, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1504.092041015625, 2960.241455078125, 35.69659042358398437, 4.713648319244384765, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+19, 82681, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 0, 1911.1961669921875, 2952.842041015625, 16.92805671691894531, 0, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Shield Stalker (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+20, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1491.4774169921875, 2944.37158203125, 36.23116683959960937, 2.294145584106445312, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+21, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1587.85595703125, 2944.014892578125, 36.23117828369140625, 1.516095519065856933, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+22, 77819, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1692.6285400390625, 3118.178955078125, 36.67943191528320312, 5.632548332214355468, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Vigilant (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 156893 - Auchindoun Defense Construct)
(@CGUID+23, 77947, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1661.9617919921875, 2913.55908203125, 34.84741973876953125, 1.613085746765136718, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Cleric (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+24, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1555.62158203125, 2946.5390625, 35.32230758666992187, 4.027932167053222656, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+25, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1597.045166015625, 2944.1171875, 36.231170654296875, 1.516095519065856933, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+26, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1556.579833984375, 2944.067626953125, 35.32231521606445312, 2.837917804718017578, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+27, 77694, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1567.5364990234375, 2962.7431640625, 35.32230377197265625, 2.071651697158813476, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Magus (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+28, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1504.607666015625, 2945.658935546875, 35.69659042358398437, 1.514774680137634277, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+29, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1530.19970703125, 2945.874267578125, 35.6965789794921875, 1.567850351333618164, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+30, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1528.7066650390625, 2944.012939453125, 36.23116683959960937, 1.581174135208129882, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+31, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1527.0850830078125, 2945.88623046875, 35.69659042358398437, 1.569900989532470703, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+32, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1495.3350830078125, 2960.201416015625, 35.69659042358398437, 3.68580031394958496, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+33, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1540.111083984375, 2948.291748046875, 35.32219314575195312, 3.135090351104736328, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+34, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1528.4879150390625, 2961.6337890625, 36.23116683959960937, 4.747385025024414062, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+35, 77819, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1762.4254150390625, 3183.012939453125, 36.6792449951171875, 5.268512248992919921, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Vigilant (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 156893 - Auchindoun Defense Construct)
(@CGUID+36, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1507.02783203125, 2945.72314453125, 35.69659042358398437, 1.567850351333618164, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+37, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1601.578125, 2961.96435546875, 36.231170654296875, 4.64665842056274414, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+38, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1606.435791015625, 2944.218017578125, 36.2311553955078125, 1.516095519065856933, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+39, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1611.0625, 2961.96435546875, 36.23115921020507812, 4.64665842056274414, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+40, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1615.3524169921875, 2944.31689453125, 36.2311553955078125, 1.516095519065856933, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+41, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1619.814208984375, 2961.96435546875, 36.22805404663085937, 5.218808174133300781, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+42, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1640.986083984375, 2960.064208984375, 35.5829010009765625, 5.559096813201904296, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+43, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1640.9600830078125, 2945.71533203125, 35.60343170166015625, 0.67940223217010498, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+44, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1642.1961669921875, 2927.654541015625, 35.24270248413085937, 0.01525938045233488, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+45, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1645.295166015625, 2977.645751953125, 35.23143386840820312, 0.008443285711109638, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+46, 77694, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1646.16845703125, 2958.1572265625, 35.1436614990234375, 5.929937362670898437, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Magus (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+47, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1649.515625, 2941.357666015625, 35.40304183959960937, 0.947370529174804687, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+48, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1644.810791015625, 2968.411376953125, 35.21453857421875, 6.212331295013427734, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+49, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1645.3629150390625, 2947.537353515625, 35.40304183959960937, 0.24801965057849884, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+50, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1646.65625, 2944.69091796875, 35.40304183959960937, 0.560388743877410888, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+51, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1646.623291015625, 2961.395751953125, 35.40304183959960937, 5.716198444366455078, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+52, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1646.486083984375, 2993.46533203125, 35.32218170166015625, 5.953157901763916015, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+53, 77134, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1897.4461669921875, 3162.686767578125, 30.88268470764160156, 0.973815202713012695, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Cleric (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 156955 - Void Form)
(@CGUID+54, 75839, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1911.63720703125, 3131.975830078125, 29.79482460021972656, 1.5778275728225708, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Vigilant Kaathar (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 157266 - Vigilant Submerge State, 42459 - Dual Wield)
(@CGUID+55, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1645.767333984375, 2985.91845703125, 35.32218170166015625, 0.008443285711109638, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+56, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1650.579833984375, 2962.982666015625, 35.14365005493164062, 5.50257110595703125, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+57, 77694, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1654.0242919921875, 2940.4462890625, 35.05950927734375, 1.081962466239929199, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Magus (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: )
(@CGUID+58, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1656.921875, 2915.362060546875, 34.84733963012695312, 0.768780052661895751, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+59, 77704, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1657.1614990234375, 2929.3212890625, 34.8416900634765625, 5.446918964385986328, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Warden (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+60, 77704, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1661.0364990234375, 2919.46875, 34.94428253173828125, 4.858886241912841796, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Warden (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+61, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1674.71875, 2953.107666015625, 35.1436614990234375, 3.135791301727294921, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (possible waypoints or random movement)
(@CGUID+62, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1645.654541015625, 2894.26220703125, 35.32217788696289062, 0.374487310647964477, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+63, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1659.7708740234375, 2925.875, 34.84703445434570312, 2.130390405654907226, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+64, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1647.232666015625, 3001.00341796875, 35.32218170166015625, 5.928806781768798828, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+65, 77694, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1662.3975830078125, 2939.756103515625, 35.14365005493164062, 1.624581456184387207, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Magus (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+66, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1658.9722900390625, 2914.057373046875, 34.8473663330078125, 1.240181326866149902, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+67, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1648.329833984375, 3008.029541015625, 35.31502532958984375, 5.819012165069580078, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+68, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1666.2603759765625, 2926.459228515625, 34.84720611572265625, 0.491822987794876098, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+69, 77950, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1664.234375, 2911.596435546875, 34.84752273559570312, 1.978927850723266601, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+70, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1666.376708984375, 2916.9765625, 34.847259521484375, 2.829090833663940429, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+71, 77952, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1667.001708984375, 2913.90966796875, 34.84732818603515625, 2.464233636856079101, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulbinder (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+72, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1659.05908203125, 2911.08935546875, 34.84760284423828125, 1.325223088264465332, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+73, 77951, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1669.2864990234375, 2928.670166015625, 34.84729766845703125, 3.686169624328613281, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Spirit-Tender (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+74, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1672.2117919921875, 2944.5634765625, 35.14365005493164062, 2.491034507751464843, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+75, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1664.75, 2915.018310546875, 34.84739303588867187, 2.360809803009033203, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+76, 77694, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1668.029541015625, 2941.395751953125, 35.14365005493164062, 2.242946624755859375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Magus (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+77, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1679.765625, 2935.8759765625, 35.19507598876953125, 3.192433118820190429, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+78, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.435791015625, 2986.002685546875, 35.32218170166015625, 2.876665592193603515, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+79, 77704, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1682.3680419921875, 2953.29248046875, 35.62078094482421875, 6.139192581176757812, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Warden (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+80, 77948, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.5625, 2931.239501953125, 35.19874954223632812, 4.822155952453613281, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Hoplite (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+81, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.3004150390625, 2947.290771484375, 35.40304183959960937, 3.880580902099609375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+82, 77812, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1894.51220703125, 3200.030517578125, 30.88272476196289062, 5.597739696502685546, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Soulbinder (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 156955 - Void Form)
(@CGUID+83, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.1163330078125, 2979.145751953125, 35.26027297973632812, 3.03649139404296875, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+84, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.0086669921875, 2971.538330078125, 35.21437835693359375, 3.03649139404296875, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+85, 79417, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 0, 1661.4757080078125, 3012.5546875, 40.77339553833007812, 4.136562347412109375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Invisible Stalker (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+86, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.439208984375, 2964.721435546875, 35.24824905395507812, 3.095417737960815429, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+87, 77952, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.59375, 2927.076416015625, 35.258575439453125, 1.515209197998046875, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulbinder (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+88, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1675.810791015625, 2946.27001953125, 35.40304183959960937, 1.084618210792541503, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+89, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1687.30908203125, 2950.84375, 35.69637680053710937, 2.504041910171508789, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+90, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1687.3489990234375, 2948.397705078125, 35.69637680053710937, 2.279029369354248046, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+91, 79417, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 0, 1669.638916015625, 3010.072998046875, 41.74965667724609375, 4.136562347412109375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Invisible Stalker (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+92, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1677.0191650390625, 2991.90625, 35.32218170166015625, 2.876665592193603515, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+93, 77832, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 0, 1911.73095703125, 3183.638916015625, 33.50413131713867187, 0, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Soul Aegis (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 156922 - Soul Aegis)
(@CGUID+94, 77950, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1690.13720703125, 2951.868896484375, 35.69637680053710937, 2.928548336029052734, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+95, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1677.5572509765625, 2997.974853515625, 35.31458282470703125, 3.031241893768310546, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+96, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1686.7447509765625, 2956.775146484375, 35.69637680053710937, 3.830792427062988281, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+97, 79417, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 0, 1666.7760009765625, 3010.6640625, 37.83771514892578125, 4.136562347412109375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Invisible Stalker (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+98, 77810, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1913.8941650390625, 3189.854248046875, 30.88270759582519531, 4.075302600860595703, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Soulbinder Nyami (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 157234 - Void Shell)
(@CGUID+99, 77950, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1688.88720703125, 2958.1884765625, 35.696380615234375, 3.787583589553833007, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+100, 79417, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 0, 1664.892333984375, 3011.3759765625, 42.05359649658203125, 4.136562347412109375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Invisible Stalker (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+101, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1690.3194580078125, 2946.46533203125, 35.69638442993164062, 2.364319801330566406, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+102, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1684.8785400390625, 2947.015625, 35.696380615234375, 1.946364879608154296, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+103, 79417, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 0, 1663.1875, 3012.079833984375, 37.778472900390625, 4.136562347412109375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Invisible Stalker (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+104, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1692.623291015625, 2948.35595703125, 35.69638442993164062, 2.63987278938293457, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+105, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1687.3663330078125, 2952.897705078125, 35.69637298583984375, 3.125909805297851562, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+106, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1684.638916015625, 2959.2666015625, 35.696380615234375, 4.355805873870849609, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+107, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1687.861083984375, 2955.04248046875, 35.696380615234375, 3.49931192398071289, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+108, 79417, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 0, 1666.7274169921875, 3011.040771484375, 43.77808380126953125, 4.136562347412109375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Invisible Stalker (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+109, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1693.55908203125, 2956.625, 35.696380615234375, 3.403077602386474609, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+110, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1694.51220703125, 2952.609375, 35.69637680053710937, 3.057715892791748046, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+111, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1675.8697509765625, 3023.09375, 35.25547027587890625, 2.940030813217163085, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+112, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1706.94970703125, 2948.984375, 35.32215118408203125, 0.615237057209014892, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+113, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.8646240234375, 3032.341064453125, 35.22306442260742187, 2.828145027160644531, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+114, 77704, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1712.595458984375, 2953.46533203125, 35.32215499877929687, 3.026159524917602539, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Warden (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+116, 77704, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1712.6771240234375, 2948.538330078125, 35.32215118408203125, 2.536875486373901367, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Warden (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+117, 77131, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1935.888916015625, 3183.606689453125, 30.8827056884765625, 3.201084375381469726, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Spirit-Tender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 156955 - Void Form)
(@CGUID+118, 77950, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1704.9930419921875, 2953.138916015625, 35.39653778076171875, 0.095412008464336395, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+119, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.017333984375, 3025.943603515625, 35.24180984497070312, 2.908563852310180664, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+120, 77704, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1673.6927490234375, 3032.5234375, 35.23430633544921875, 1.410390615463256835, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Warden (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+121, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1676.3489990234375, 3029.0478515625, 35.22306060791015625, 2.92891693115234375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+122, 77704, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1712.4444580078125, 2957.618896484375, 35.3221588134765625, 3.17419600486755371, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Warden (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+123, 77950, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1706.3819580078125, 2957.646728515625, 35.3221588134765625, 5.789441585540771484, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+124, 77950, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1707.4739990234375, 2960.529541015625, 35.69658660888671875, 5.496424674987792968, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+125, 77949, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1709.10595703125, 2946.35498046875, 35.69657135009765625, 0.644170701503753662, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+126, 77948, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1691.0399169921875, 3062.772705078125, 35.22322845458984375, 2.610810279846191406, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Hoplite (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 157739 - At Arms)
(@CGUID+127, 77950, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1689.5103759765625, 3066.12939453125, 35.13763427734375, 1.184453248977661132, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+128, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1691.513916015625, 3070.65625, 35.22322463989257812, 5.132762908935546875, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+129, 77948, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1693.328125, 3067.4775390625, 35.2232208251953125, 2.607628822326660156, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Hoplite (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 157739 - At Arms)
(@CGUID+130, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1673.967041015625, 3074.104248046875, 36.13217926025390625, 5.834946632385253906, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+131, 77694, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1675.6788330078125, 3068.34375, 35.22326278686523437, 2.696657180786132812, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Magus (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+132, 77694, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1679.357666015625, 3075.5859375, 35.22324371337890625, 2.604601860046386718, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Magus (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+133, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1714.7691650390625, 3100.943603515625, 35.22316360473632812, 2.435690879821777343, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+134, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1712.6978759765625, 3097.87939453125, 35.22316741943359375, 2.607788562774658203, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+135, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1701.6754150390625, 3113.021728515625, 35.22318649291992187, 0, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+136, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1716.7257080078125, 3104.069580078125, 35.2231597900390625, 2.5426483154296875, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+137, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1714.4478759765625, 3104.975830078125, 35.13762664794921875, 5.904528617858886718, 86400, 10, 0, 100, 1, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (possible waypoints or random movement)
(@CGUID+138, 77694, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1710.420166015625, 3113.875, 35.22317123413085937, 3.38626265525817871, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Magus (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: )
(@CGUID+139, 77694, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1728.7430419921875, 3139.203125, 35.22311782836914062, 2.261404037475585937, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Magus (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+140, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1727.720458984375, 3141.53125, 35.22311782836914062, 4.695310115814208984, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+141, 77696, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1745.9305419921875, 3138.907958984375, 35.22307968139648437, 0.293291807174682617, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Soulpriest (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+142, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1748.3021240234375, 3138.0087890625, 35.22307205200195312, 2.92891693115234375, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+143, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1748.373291015625, 3140.741455078125, 35.22307205200195312, 3.436568498611450195, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+144, 77133, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1896.875, 3155.9462890625, 31.05600738525390625, 1.607088327407836914, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Hoplite (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 157081 - Void Strikes)
(@CGUID+145, 77950, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1767.8507080078125, 3174.45751953125, 35.22301483154296875, 3.923529624938964843, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+146, 77695, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1765.99658203125, 3172.454833984375, 35.22301864624023437, 0.878998160362243652, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Arbiter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+147, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1780.7430419921875, 3161.4296875, 35.22298812866210937, 2.103954792022705078, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+148, 77693, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1774.1007080078125, 3157.08154296875, 35.22300338745117187, 2.221400260925292968, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Auchenai Defender (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 123167 - Mod Scale 95-100%)
(@CGUID+149, 77133, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1887.1146240234375, 3205.30908203125, 31.05605888366699218, 4.987659454345703125, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Hoplite (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 157081 - Void Strikes)
(@CGUID+150, 77133, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1938.482666015625, 3187.0478515625, 30.88270759582519531, 4.002863407135009765, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Hoplite (Area: Auchindoun - Difficulty: Heroic) CreateObject1 (Auras: 157081 - Void Strikes)
(@CGUID+151, 77130, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1889.5103759765625, 3200.78466796875, 30.88272476196289062, 0.426818430423736572, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+152, 77130, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1892.75, 3160.609375, 30.88268089294433593, 0.336703449487686157, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+153, 77132, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1893.5347900390625, 3155.532958984375, 31.05600738525390625, 0.973815202713012695, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+154, 77132, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1891.0208740234375, 3205.635498046875, 30.88256645202636718, 5.389794826507568359, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+155, 77130, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1940.923583984375, 3177.864501953125, 30.88269805908203125, 2.72194981575012207, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1), -- Sargerei Ritualist (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@CGUID+156, 77132, 1182, 6912, 6912, '1,2,8,23', 0, 0, 0, 1, 1940.204833984375, 3189.73779296875, 30.88271141052246093, 3.201084375381469726, 86400, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54904, 1); -- Sargerei Zealot (Area: Auchindoun - Difficulty: Heroic) CreateObject1

DELETE FROM `creature_addon` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+156;
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(@CGUID+0, 0, 0, 0, 0, 0, 1, 0, 505, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+5, 0, 0, 0, 0, 0, 1, 0, 505, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+7, 0, 0, 0, 0, 0, 1, 0, 505, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+11, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- Soulbinder Tuulani
(@CGUID+12, 0, 0, 0, 0, 0, 1, 0, 505, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+20, 0, 0, 0, 0, 0, 1, 0, 505, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+32, 0, 0, 0, 0, 0, 1, 0, 505, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+42, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+43, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+44, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+45, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+46, 0, 0, 0, 0, 0, 1, 0, 533, 0, 0, 0, 0, ''), -- Auchenai Magus
(@CGUID+48, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+49, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Auchenai Arbiter
(@CGUID+52, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+55, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+56, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Auchenai Arbiter
(@CGUID+58, 0, 0, 0, 0, 0, 1, 0, 533, 0, 0, 0, 0, ''), -- Auchenai Soulpriestv
(@CGUID+60, 0, 0, 0, 0, 0, 1, 0, 378, 0, 0, 0, 0, ''), -- Auchenai Warden
(@CGUID+62, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+64, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+65, 0, 0, 0, 0, 0, 1, 0, 533, 0, 0, 0, 0, ''), -- Auchenai Magus
(@CGUID+67, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+70, 0, 0, 0, 0, 0, 1, 0, 533, 0, 0, 0, 0, ''), -- Auchenai Soulpriest
(@CGUID+72, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+74, 0, 0, 0, 0, 0, 1, 0, 483, 0, 0, 0, 0, ''), -- Auchenai Ritualist
(@CGUID+75, 0, 0, 0, 0, 0, 1, 0, 533, 0, 0, 0, 0, ''), -- Auchenai Soulpriest
(@CGUID+76, 0, 0, 0, 0, 0, 1, 0, 533, 0, 0, 0, 0, ''), -- Auchenai Magus
(@CGUID+77, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+78, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+79, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Auchenai Warden
(@CGUID+81, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Auchenai Ritualist
(@CGUID+83, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+84, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+86, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+88, 0, 0, 0, 0, 0, 1, 0, 378, 0, 0, 0, 0, ''), -- Auchenai Soulpriest
(@CGUID+90, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+92, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+95, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+96, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+102, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+106, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+128, 0, 0, 0, 0, 0, 0, 0, 483, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+130, 0, 0, 0, 0, 0, 0, 0, 483, 0, 0, 0, 0, ''), -- Auchenai Arbiter
(@CGUID+131, 0, 0, 0, 0, 0, 1, 0, 533, 0, 0, 0, 0, ''), -- Auchenai Magus
(@CGUID+132, 0, 0, 0, 0, 0, 1, 0, 533, 0, 0, 0, 0, ''), -- Auchenai Magus
(@CGUID+135, 0, 0, 0, 0, 0, 0, 0, 533, 0, 0, 0, 0, ''), -- Auchenai Soulpriest
(@CGUID+139, 0, 0, 0, 0, 0, 1, 0, 378, 0, 0, 0, 0, ''), -- Auchenai Magus
(@CGUID+140, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+141, 0, 0, 0, 0, 0, 1, 0, 378, 0, 0, 0, 0, ''), -- Auchenai Soulpriest
(@CGUID+145, 0, 0, 0, 0, 0, 1, 0, 588, 0, 0, 0, 0, ''), -- Auchenai Zealot
(@CGUID+147, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(@CGUID+148, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '123167'); -- Auchenai Defender - 123167 - Mod Scale 95-100%

-- Gameobjects
DELETE FROM `gameobject` WHERE `guid` BETWEEN @OGUID+0 AND @OGUID+20;
INSERT INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `VerifiedBuild`, `size`, `visibility`) VALUES
(@OGUID+0, 231742, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1844.3779296875, 2953.1298828125, 15.16725635528564453, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 1, 54904, 1, 100), -- Soul Transporter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+1, 237459, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1827.97021484375, 2722.52978515625, 29.2783203125, 3.125251531600952148, 0, 0, 0.999966621398925781, 0.008170507848262786, 86400, 255, 0, 54904, 1, 100), -- Holy Barrier (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+2, 231743, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1802.8836669921875, 2953.053955078125, 25.48373794555664062, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 1, 54904, 1, 100), -- Soul Transporter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+3, 233010, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1737.890625, 2765.779052734375, 35.05662918090820312, 0.820305347442626953, 0, 0, 0.398749351501464843, 0.917059957981109619, 86400, 255, 1, 54904, 1, 100), -- Auchindoun Window 2 (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+4, 231741, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1830.89404296875, 3033.296142578125, 35.60332107543945312, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 1, 54904, 1, 100), -- Soul Transporter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+5, 231680, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1704.3170166015625, 2953.16552734375, 35.58184051513671875, 6.281350612640380859, 0, 0, -0.00091743469238281, 0.999999582767486572, 86400, 255, 0, 54904, 1, 100), -- Holy Barrier (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+6, 212899, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1483.564208984375, 2953.048583984375, 35.31077957153320312, 6.219974994659423828, 0, 0, -0.03159999847412109, 0.999500572681427001, 86400, 255, 1, 54904, 1, 100), -- Instance Portal (Party + Heroic + Challenge) (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+7, 233009, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1684.3145751953125, 2836.506591796875, 35.056640625, 0.471238493919372558, 0, 0, 0.233445167541503906, 0.972369968891143798, 86400, 255, 1, 54904, 1, 100), -- Auchindoun Window 1 (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+8, 230511, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1827.9700927734375, 3183.653564453125, 28.45749282836914062, 3.125251531600952148, 0, 0, 0.999966621398925781, 0.008170507848262786, 86400, 255, 0, 54904, 1, 100), -- Holy Barrier (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+9, 227633, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1482.0625, 2953.0556640625, 0, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 1, 54904, 1, 100), -- Auchindoun_Section_001 (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+10, 230398, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1665.140625, 3012.127197265625, 35.58184051513671875, 4.434296131134033203, 0, 0, -0.79828643798828125, 0.602277994155883789, 86400, 255, 1, 54904, 1, 100), -- Holy Barrier (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+11, 230399, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1911.6396484375, 3183.603759765625, 31.48963165283203125, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 0, 54904, 1, 100), -- Holy Barrier (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+12, 230397, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1665.14013671875, 2894.444091796875, 35.58184051513671875, 4.996966361999511718, 0, 0, -0.5996866226196289, 0.800234973430633544, 86400, 255, 1, 54904, 1, 100), -- Holy Barrier (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+13, 230400, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1911.6397705078125, 2722.507568359375, 31.57850456237792968, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 0, 54904, 1, 100), -- Fel Barrier (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+14, 231738, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1831.1392822265625, 2872.88037109375, 35.591522216796875, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 1, 54904, 1, 100), -- Soul Transporter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+15, 231679, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1617.25146484375, 2953.16552734375, 35.58184051513671875, 6.281350612640380859, 0, 0, -0.00091743469238281, 0.999999582767486572, 86400, 255, 0, 54904, 1, 100), -- Holy Barrier (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+16, 231736, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1911.621826171875, 3105.61083984375, 4.55859375, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 1, 54904, 1, 100), -- Soul Transporter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+17, 231737, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1911.716796875, 2800.406494140625, 4.55859375, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 1, 54904, 1, 100), -- Soul Transporter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+18, 237464, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1685.4444580078125, 2913.046142578125, 36.72615432739257812, 6.200767040252685546, 0.702855110168457031, -0.03005886077880859, -0.0282135009765625, 0.710137426853179931, 86400, 255, 1, 54904, 1, 100), -- Soulsever Blade (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+19, 231739, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1992.41748046875, 2872.893310546875, 35.37061691284179687, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 1, 54904, 1, 100), -- Soul Transporter (Area: Auchindoun - Difficulty: Heroic) CreateObject1
(@OGUID+20, 231740, 1182, 6912, 6912, '1,2,8,23', 0, 0, 1992.5048828125, 3033.2978515625, 35.36113739013671875, 3.141592741012573242, 0, 0, -1, 0, 86400, 255, 1, 54904, 1, 100); -- Soul Transporter (Area: Auchindoun - Difficulty: Heroic) CreateObject1

DELETE FROM `gameobject_addon` WHERE `guid` BETWEEN @OGUID+0 AND @OGUID+20;
INSERT INTO `gameobject_addon` (`guid`, `parent_rotation0`, `parent_rotation1`, `parent_rotation2`, `parent_rotation3`, `WorldEffectID`, `AIAnimKitID`) VALUES
(@OGUID+0, 0, 0, 1, -0.00000004371138828, 0, 5460), -- Soul Transporter
(@OGUID+1, 0, 0, 1, -0.00000004371138828, 0, 0), -- Holy Barrier
(@OGUID+2, 0, 0, 1, -0.00000004371138828, 0, 5460), -- Soul Transporter
(@OGUID+3, 0, 0, 1, -0.00000004371138828, 0, 0), -- Auchindoun Window 2
(@OGUID+4, 0, 0, 1, -0.00000004371138828, 0, 5460), -- Soul Transporter
(@OGUID+5, 0, 0, 1, -0.00000004371138828, 0, 0), -- Holy Barrier
(@OGUID+7, 0, 0, 1, -0.00000004371138828, 0, 0), -- Auchindoun Window 1
(@OGUID+8, 0, 0, 1, -0.00000004371138828, 0, 0), -- Holy Barrier
(@OGUID+9, 0, 0, 0, 1, 0, 0), -- Auchindoun_Section_001
(@OGUID+10, 0, 0, 1, -0.00000004371138828, 0, 0), -- Holy Barrier
(@OGUID+11, 0, 0, 1, -0.00000004371138828, 0, 0), -- Holy Barrier
(@OGUID+12, 0, 0, 1, -0.00000004371138828, 0, 0), -- Holy Barrier
(@OGUID+13, 0, 0, 1, -0.00000004371138828, 0, 0), -- Fel Barrier
(@OGUID+14, 0, 0, 1, -0.00000004371138828, 0, 5460), -- Soul Transporter
(@OGUID+15, 0, 0, 1, -0.00000004371138828, 0, 0), -- Holy Barrier
(@OGUID+16, 0, 0, 1, -0.00000004371138828, 0, 5460), -- Soul Transporter
(@OGUID+17, 0, 0, 1, -0.00000004371138828, 0, 5460), -- Soul Transporter
(@OGUID+19, 0, 0, 1, -0.00000004371138828, 0, 5460), -- Soul Transporter
(@OGUID+20, 0, 0, 1, -0.00000004371138828, 0, 5460); -- Soul Transporter

-- Template
DELETE FROM `instance_template` WHERE `map`=1182;
INSERT INTO `instance_template` (`map`, `parent`, `script`) VALUES
(1182, 0, 'instance_auchindoun');

UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=77947; -- Auchenai Cleric
UPDATE `creature_template` SET `unit_flags2`=2048, `unit_flags3`=16777216 WHERE `entry`=82681; -- Shield Stalker
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=79248; -- Soulbinder Tuulani
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=77819; -- Auchenai Vigilant
UPDATE `creature_template` SET `unit_flags2`=2048, `AIName`='SmartAI' WHERE `entry`=77131; -- Sargerei Spirit-Tender
UPDATE `creature_template` SET `unit_flags2`=2048, `AIName`='SmartAI' WHERE `entry`=77810; -- Soulbinder Nyami
UPDATE `creature_template` SET `unit_flags2`=67110912, `unit_flags3`=16777217 WHERE `entry`=77832; -- Soul Aegis
UPDATE `creature_template` SET `unit_flags`=33554688 WHERE `entry`=79417; -- Invisible Stalker
UPDATE `creature_template` SET `unit_flags2`=2048, `AIName`='SmartAI' WHERE `entry`=77812; -- Sargerei Soulbinder
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=75839; -- Vigilant Kaathar
UPDATE `creature_template` SET `unit_flags2`=2048, `AIName`='SmartAI' WHERE `entry`=77134; -- Sargerei Cleric
UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry`=77695; -- Auchenai Arbiter
UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry`=77694; -- Auchenai Magus
UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry`=77696; -- Auchenai Soulpriest

UPDATE `creature` SET `StringId`='CosmeticArcaneBomb' WHERE `guid`= @CGUID+138;
UPDATE `creature` SET `StringId`='EventArcaneBomb' WHERE `guid` = @CGUID+57;
UPDATE `creature` SET `StringId`='EventHalo' WHERE `guid`= @CGUID+61;
UPDATE `creature` SET `StringId`='EventRadiantFury' WHERE `guid`= @CGUID+56;

-- Template Addon
DELETE FROM `creature_template_addon` WHERE `entry` IN (77950, 77948, 77131, 77810, 77832, 77812, 75839, 77134, 77695, 77947, 82681, 77133, 77819, 77693, 77694, 77947, 77949, 77696);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(77950, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 77950 (Auchenai Zealot)
(77948, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '157739'), -- 77948 (Auchenai Hoplite) - At Arms
(77131, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, '156955'), -- 77131 (Sargerei Spirit-Tender) - Void Form
(77810, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 4, '157234'), -- 77810 (Soulbinder Nyami) - Void Shell
(77832, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 4, '156922'), -- 77832 (Soul Aegis) - Soul Aegis
(77812, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, '156955'), -- 77812 (Sargerei Soulbinder) - Void Form
(75839, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, '157266 42459'), -- 75839 (Vigilant Kaathar) - Vigilant Submerge State, Dual Wield
(77134, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, '156955'), -- 77134 (Sargerei Cleric) - Void Form
(77695, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 77695 (Auchenai Arbiter)
(77947, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 77947 (Auchenai Cleric)
(82681, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- 82681 (Shield Stalker)
(77133, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '157081'), -- 77133 (Sargerei Hoplite) - Void Strikes
(77819, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, '156893'), -- 77819 (Auchenai Vigilant) - Auchindoun Defense Construct
(77693, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '123167'), -- Auchenai Defender - 123167 - Mod Scale 95-100%
(77694, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Auchenai Magus
(77949, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- Auchenai Ritualist
(77696, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''); -- Auchenai Soulpriest

UPDATE `gameobject_template_addon` SET `flags`=48 WHERE `entry` IN (231679, 230397, 230399, 230398, 230511, 231680, 237459); -- Holy Barrier
UPDATE `gameobject_template_addon` SET `flags`=48, `AIAnimKitID`=5460 WHERE `entry` IN (231738, 231741, 231743, 231742); -- Soul Transporter
UPDATE `gameobject_template_addon` SET `flags`=48 WHERE `entry`=230400; -- Fel Barrier
UPDATE `gameobject_template_addon` SET `flags`=1048608 WHERE `entry`=227633; -- Auchindoun_Section_001
UPDATE `gameobject_template_addon` SET `flags`=48 WHERE `entry`=233009; -- Auchindoun Window 1
UPDATE `gameobject_template_addon` SET `flags`=48 WHERE `entry`=233010; -- Auchindoun Window 2

-- Areatrigger
DELETE FROM `areatrigger_template` WHERE (`Id`=@ATID AND `IsCustom`=1);
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `VerifiedBuild`) VALUES
(@ATID, 1, 1, 0);

DELETE FROM `areatrigger_create_properties` WHERE `Id`=1725 AND `IsCustom`=0;
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `SpellForVisuals`, `TimeToTarget`, `TimeToTargetScale`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`) VALUES
(1725, 0, @ATID, 1, 4, 0, 0, 0, 0, -1, 0, 0, NULL, 0, 0, 0, 6, 6, 0, 0, 0, 0, 0, 0, '', 0);

DELETE FROM `areatrigger_template` WHERE (`IsCustom`=0 AND `Id` = 6035);
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `VerifiedBuild`) VALUES
(6035, 0, 0, 54904);

DELETE FROM `areatrigger_create_properties` WHERE (`Id`=1569 AND `IsCustom`=0);
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `TimeToTarget`, `TimeToTargetScale`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `VerifiedBuild`) VALUES
(1569, 0, 6035, 0, 2, 470, 0, 469, 0, -1, 0, 0, 9446, 10000, 0, 0.75, 1.25, 0, 0, 0, 0, 0, 0, 54904); -- Spell: 154174 (Arcane Bomb)

-- Creature difficulties (Normal)
DELETE FROM `creature_template_difficulty` WHERE (`Entry`IN (77130, 77132, 77133) AND `DifficultyID`=1);

UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77132 AND `DifficultyID`=0); -- Sargerei Zealot
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77130 AND `DifficultyID`=0); -- Sargerei Ritualist
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77133 AND `DifficultyID`=0); -- Sargerei Hoplite
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77131 AND `DifficultyID`=0); -- Sargerei Spirit-Tender
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77810 AND `DifficultyID`=0); -- Soulbinder Nyami
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77948 AND `DifficultyID`=0); -- Auchenai Hoplite
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77832 AND `DifficultyID`=0); -- Soul Aegis
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77812 AND `DifficultyID`=0); -- Sargerei Soulbinder
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77952 AND `DifficultyID`=0); -- Auchenai Soulbinder
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77951 AND `DifficultyID`=0); -- Auchenai Spirit-Tender
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77950 AND `DifficultyID`=0); -- Auchenai Zealot
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77134 AND `DifficultyID`=0); -- Sargerei Cleric
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=75839 AND `DifficultyID`=0); -- Vigilant Kaathar
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77949 AND `DifficultyID`=0); -- Auchenai Ritualist
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77947 AND `DifficultyID`=0); -- Auchenai Cleric
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77704 AND `DifficultyID`=0); -- Auchenai Warden
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77694 AND `DifficultyID`=0); -- Auchenai Magus
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77819 AND `DifficultyID`=0); -- Auchenai Vigilant
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77695 AND `DifficultyID`=0); -- Auchenai Arbiter
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77696 AND `DifficultyID`=0); -- Auchenai Soulpriest
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=82681 AND `DifficultyID`=0); -- Shield Stalker
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=77693 AND `DifficultyID`=0); -- Auchenai Defender
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5 WHERE (`Entry`=79248 AND `DifficultyID`=0); -- Soulbinder Tuulani

UPDATE `creature_template_difficulty` SET `ContentTuningID`=2216, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77132 AND `DifficultyID`=0); -- 77132 (Sargerei Zealot) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2216, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77130 AND `DifficultyID`=0); -- 77130 (Sargerei Ritualist) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2216, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77133 AND `DifficultyID`=0); -- 77133 (Sargerei Hoplite) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2216, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77131 AND `DifficultyID`=0); -- 77131 (Sargerei Spirit-Tender) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=3, `LevelScalingDeltaMax`=3, `ContentTuningID`=2216, `StaticFlags1`=805306368, `VerifiedBuild`=54904 WHERE (`Entry`=77810 AND `DifficultyID`=0); -- 77810 (Soulbinder Nyami) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=54904 WHERE (`DifficultyID`=0 AND `Entry` IN (79417,88439));
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2216, `StaticFlags1`=536871168, `VerifiedBuild`=54904 WHERE (`Entry`=77832 AND `DifficultyID`=0); -- 77832 (Soul Aegis) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2216, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77812 AND `DifficultyID`=0); -- 77812 (Sargerei Soulbinder) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77948 AND `DifficultyID`=0); -- 77948 (Auchenai Hoplite) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77950 AND `DifficultyID`=0); -- 77950 (Auchenai Zealot) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77951 AND `DifficultyID`=0); -- 77951 (Auchenai Spirit-Tender) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77952 AND `DifficultyID`=0); -- 77952 (Auchenai Soulbinder) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2216, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77134 AND `DifficultyID`=0); -- 77134 (Sargerei Cleric) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2216, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=75839 AND `DifficultyID`=0); -- 75839 (Vigilant Kaathar) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77949 AND `DifficultyID`=0); -- 77949 (Auchenai Ritualist) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77947 AND `DifficultyID`=0); -- 77947 (Auchenai Cleric) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77704 AND `DifficultyID`=0); -- 77704 (Auchenai Warden) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77694 AND `DifficultyID`=0); -- 77694 (Auchenai Magus) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2216, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=79248 AND `DifficultyID`=0); -- 79248 (Soulbinder Tuulani) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77695 AND `DifficultyID`=0); -- 77695 (Auchenai Arbiter) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2216, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77819 AND `DifficultyID`=0); -- 77819 (Auchenai Vigilant) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77696 AND `DifficultyID`=0); -- 77696 (Auchenai Soulpriest) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2216, `StaticFlags1`=536871168, `VerifiedBuild`=54904 WHERE (`Entry`=82681 AND `DifficultyID`=0); -- 82681 (Shield Stalker) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2216, `VerifiedBuild`=54904 WHERE (`Entry`=77693 AND `DifficultyID`=0); -- 77693 (Auchenai Defender) - 

-- Creature Difficulty (Heroic)
DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=2 AND `Entry` IN (77832,79417));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `LevelScalingDeltaMin`, `LevelScalingDeltaMax`, `ContentTuningID`, `StaticFlags1`, `StaticFlags2`, `StaticFlags3`, `StaticFlags4`, `StaticFlags5`, `StaticFlags6`, `StaticFlags7`, `StaticFlags8`, `VerifiedBuild`) VALUES
(77832, 2, 0, 0, 2218, 536871168, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77832 (Soul Aegis) - Sessile, Floating
(79417, 2, 0, 0, 328, 536871168, 0, 0, 0, 0, 0, 0, 0, 54904); -- 79417 (Invisible Stalker) - Sessile; Floating

DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=2 AND `Entry` IN (77132,77130,77133,77131,77810,77948,77812,77952,77951,77950,77134,75839,77949,77694,77704,77695,79248,77696,77693,77947));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `HealthScalingExpansion`, `HealthModifier`, `ManaModifier`, `CreatureDifficultyID`, `TypeFlags`, `TypeFlags2`) VALUES
(77132, 2, 5, 5, 1, 79232, 270532680, 0), -- Sargerei Zealot
(77130, 2, 5, 5, 1, 79229, 2097224, 0), -- Sargerei Ritualist
(77133, 2, 5, 5, 1, 79228, 270532712, 0), -- Sargerei Hoplite
(77131, 2, 5, 8.5, 1, 79231, 2097224, 0), -- Sargerei Spirit-Tender
(77810, 2, 5, 26, 1, 79861, 2097256, 0), -- Soulbinder Nyami
(77948, 2, 5, 1, 1, 79867, 0, 0), -- Auchenai Hoplite
(77812, 2, 5, 8, 1, 79230, 2097224, 0), -- Sargerei Soulbinder
(77952, 2, 5, 1, 1, 79870, 0, 0), -- Auchenai Soulbinder
(77951, 2, 5, 1, 1, 79871, 0, 0), -- Auchenai Spirit-Tender
(77950, 2, 5, 1, 1, 79873, 0, 0), -- Auchenai Zealot
(77134, 2, 5, 10, 1, 79227, 2097224, 0), -- Sargerei Cleric
(75839, 2, 5, 35, 1, 79226, 270532712, 128), -- Vigilant Kaathar
(77949, 2, 5, 1, 1, 79869, 0, 0), -- Auchenai Ritualist
(77694, 2, 5, 1, 1, 79868, 0, 0), -- Auchenai Magus
(77704, 2, 5, 1, 1, 79872, 0, 0), -- Auchenai Warden
(77695, 2, 5, 1, 1, 79863, 0, 0), -- Auchenai Arbiter
(79248, 2, 5, 5.199999809265136718, 1, 79884, 32, 0), -- Soulbinder Tuulani
(77696, 2, 5, 1, 1, 79862, 0, 0), -- Auchenai Soulpriest
(77693, 2, 5, 1, 1, 79866, 0, 0), -- Auchenai Defender
(77947, 2, 5, 1, 1, 79864, 0, 0); -- Auchenai Cleric

UPDATE `creature_template_difficulty` SET `ContentTuningID`=2218, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77132 AND `DifficultyID`=2); -- 77132 (Sargerei Zealot) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2218, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77130 AND `DifficultyID`=2); -- 77130 (Sargerei Ritualist) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2218, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77133 AND `DifficultyID`=2); -- 77133 (Sargerei Hoplite) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2218, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77131 AND `DifficultyID`=2); -- 77131 (Sargerei Spirit-Tender) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2218, `StaticFlags1`=805306368, `VerifiedBuild`=54904 WHERE (`Entry`=77810 AND `DifficultyID`=2); -- 77810 (Soulbinder Nyami) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2218, `VerifiedBuild`=54904 WHERE (`Entry`=77948 AND `DifficultyID`=2); -- 77948 (Auchenai Hoplite) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2218, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77812 AND `DifficultyID`=2); -- 77812 (Sargerei Soulbinder) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2218, `VerifiedBuild`=54904 WHERE (`Entry`=77951 AND `DifficultyID`=2); -- 77951 (Auchenai Spirit-Tender) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2218, `VerifiedBuild`=54904 WHERE (`Entry`=77952 AND `DifficultyID`=2); -- 77952 (Auchenai Soulbinder) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2218, `VerifiedBuild`=54904 WHERE (`Entry`=77950 AND `DifficultyID`=2); -- 77950 (Auchenai Zealot) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2218, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=75839 AND `DifficultyID`=2); -- 75839 (Vigilant Kaathar) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2218, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77134 AND `DifficultyID`=2); -- 77134 (Sargerei Cleric) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2218, `VerifiedBuild`=54904 WHERE (`Entry`=77949 AND `DifficultyID`=2); -- 77949 (Auchenai Ritualist) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2218 WHERE (`Entry`=77947 AND `DifficultyID`=2); -- 77947 (Auchenai Cleric) -
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2218, `VerifiedBuild`=54904 WHERE (`Entry`=77694 AND `DifficultyID`=2); -- 77694 (Auchenai Magus) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2218, `VerifiedBuild`=54904 WHERE (`Entry`=77704 AND `DifficultyID`=2); -- 77704 (Auchenai Warden) --
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=9, `CreatureDifficultyID`=76970, `TypeFlags`=1024 WHERE (`Entry`=79417 AND `DifficultyID`=2); -- Invisible Stalker
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=5, `CreatureDifficultyID`=74871, `TypeFlags`=1024 WHERE (`Entry`=77832 AND `DifficultyID`=2); -- Soul Aegis
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=0, `LevelScalingDeltaMax`=0, `ContentTuningID`=2218 WHERE (`Entry`=79248 AND `DifficultyID`=2); -- 79248 (Soulbinder Tuulani) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2218 WHERE (`Entry`=77695 AND `DifficultyID`=2); -- 77695 (Auchenai Arbiter) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2218 WHERE (`DifficultyID`=2 AND `Entry` IN (82681,77819));
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2218 WHERE (`Entry`=77696 AND `DifficultyID`=2); -- 77696 (Auchenai Soulpriest) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=0, `LevelScalingDeltaMax`=0, `ContentTuningID`=2218 WHERE (`Entry`=77693 AND `DifficultyID`=2); -- 77693 (Auchenai Defender) - 

-- Creature Difficulty (Mythic)
DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=23 AND `Entry` IN (77132,77130,77133,77131,77812,77832,79417,77134,75839));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `HealthScalingExpansion`, `HealthModifier`, `ManaModifier`, `CreatureDifficultyID`, `TypeFlags`, `TypeFlags2`) VALUES
(77132, 23, 5, 5, 1, 93790, 270532680, 0), -- Sargerei Zealot
(77130, 23, 5, 5, 1, 93792, 2097224, 0), -- Sargerei Ritualist
(77133, 23, 5, 5, 1, 93789, 270532712, 0), -- Sargerei Hoplite
(77131, 23, 5, 8.5, 1, 93791, 2097224, 0), -- Sargerei Spirit-Tender
(77812, 23, 5, 8, 1, 93779, 2097224, 0), -- Sargerei Soulbinder
(77832, 23, 5, 1, 1, 74871, 1024, 0), -- Soul Aegis
(79417, 23, 9, 1, 1, 76970, 1024, 0), -- Invisible Stalker
(77134, 23, 5, 10, 1, 93788, 2097224, 0), -- Sargerei Cleric
(75839, 23, 5, 35, 1, 93888, 270532712, 128); -- Vigilant Kaathar

DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=23 AND `Entry` IN (77810,77952,77948,77951,77950,77949,77947,77704,77695,77694));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `LevelScalingDeltaMin`, `LevelScalingDeltaMax`, `ContentTuningID`, `StaticFlags1`, `StaticFlags2`, `StaticFlags3`, `StaticFlags4`, `StaticFlags5`, `StaticFlags6`, `StaticFlags7`, `StaticFlags8`, `VerifiedBuild`) VALUES
(77810, 23, 2, 2, 2219, 805306368, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77810 (Soulbinder Nyami) - CanSwim, Floating
(77952, 23, 1, 1, 2219, 0, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77952 (Auchenai Soulbinder) - 
(77948, 23, 0, 0, 2219, 0, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77948 (Auchenai Hoplite) - 
(77951, 23, 1, 1, 2219, 0, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77951 (Auchenai Spirit-Tender) - 
(77950, 23, 0, 0, 2219, 0, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77950 (Auchenai Zealot) - 
(77949, 23, 0, 0, 2219, 0, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77949 (Auchenai Ritualist) - 
(77947, 23, 1, 1, 2219, 0, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77947 (Auchenai Cleric) - 
(77704, 23, 0, 0, 2219, 0, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77704 (Auchenai Warden) - 
(77695, 23, 1, 1, 2219, 0, 0, 0, 0, 0, 0, 0, 0, 54904), -- 77695 (Auchenai Arbiter) - 
(77694, 23, 1, 1, 2219, 0, 0, 0, 0, 0, 0, 0, 0, 54904); -- 77694 (Auchenai Magus) - 

UPDATE `creature_template_difficulty` SET `ContentTuningID`=2219, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77132 AND `DifficultyID`=23); -- 77132 (Sargerei Zealot) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2219, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77130 AND `DifficultyID`=23); -- 77130 (Sargerei Ritualist) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2219, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77133 AND `DifficultyID`=23); -- 77133 (Sargerei Hoplite) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2219, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77131 AND `DifficultyID`=23); -- 77131 (Sargerei Spirit-Tender) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2219, `StaticFlags1`=536871168, `VerifiedBuild`=54904 WHERE (`Entry`=77832 AND `DifficultyID`=23); -- 77832 (Soul Aegis) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2219, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77812 AND `DifficultyID`=23); -- 77812 (Sargerei Soulbinder) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=328, `StaticFlags1`=536871168, `VerifiedBuild`=54904 WHERE (`Entry`=79417 AND `DifficultyID`=23); -- 79417 (Invisible Stalker) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2219, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=75839 AND `DifficultyID`=23); -- 75839 (Vigilant Kaathar) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2219, `StaticFlags1`=268435456, `VerifiedBuild`=54904 WHERE (`Entry`=77134 AND `DifficultyID`=23); -- 77134 (Sargerei Cleric) - CanSwim

-- Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=13 AND `SourceGroup` = 1 AND `SourceEntry` = 156920;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 156920, 0, 2, 51, 0, 5, 77832, 0, '', 0, 0, 0, '', 'Spell \'Void Beam\' can only hit \'Invisible Stalker\'');

-- SAI
DELETE FROM `smart_scripts` WHERE `entryorguid`=77695 AND `source_type`=0;
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (7769500, 7769501) AND `source_type`=9;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(77695, 0, 0, 0, '', 40, 0, 100, 0, 0, 7769500, 0, 0, 0, '', 80, (77695 * 100), 2, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Arbiter - On Waypoint reached 0 - Set Action List'),
(77695, 0, 1, 0, '', 40, 0, 100, 0, 1, 7769500, 0, 0, 0, '', 80, (77695 * 100), 2, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Arbiter - On Waypoint reached 1 - Set Action List'),
(77695, 0, 2, 0, '', 38, 0, 100, 0, 0, 1, 0, 0, 0, '', 80, (77695 * 100) + 1, 2, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Magus - On Data Set - Action List'),
(77695, 0, 3, 0, '', 40, 0, 100, 0, 0, 7769501, 0, 0, 0, '', 97, 14, 0, 0, 0, 0, 0, 0, NULL, 8, 0, 0, 0, 0, NULL, 1665.137, 2927.535, 34.76397, 0, 'Auchenai Arbiter - Self: Jump To Pos'),
(77695, 0, 4, 0, '', 40, 0, 100, 0, 0, 7769501, 0, 0, 0, '', 11, 155525, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Arbiter - Self: Cast Spell 155525'),
(77695, 0, 5, 0, '', 40, 0, 100, 0, 1, 7769501, 0, 0, 0, '', 97, 14, 0, 0, 0, 0, 0, 0, NULL, 8, 0, 0, 0, 0, NULL, 1655.913, 2979.347, 34.76414, 0, 'Auchenai Arbiter - Self: Jump To Pos'),
(77695, 0, 6, 0, '', 40, 0, 100, 0, 1, 7769501, 0, 0, 0, '', 11, 155525, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Arbiter - Self: Cast Spell 155525'),
(7769500, 9, 0, 0, '', 0, 0, 100, 0, 0, 0, 0, 0, 0, '', 66, 0, 0, 0, 0, 0, 0, 0, NULL, 19, 77693, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Arbiter - Face To Auchenai Defender'),
(7769500, 9, 1, 0, '', 0, 0, 100, 0, 0, 0, 0, 0, 0, '', 5, 22, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Arbiter - Play emote OneShotShot'),
(7769501, 9, 0, 0, '', 0, 0, 100, 0, 0, 0, 0, 0, 0, '', 53, 0, 7769501, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, 'EventRadiantFury', 0, 0, 0, 0, 'Auchenai Arbiter - Action List - Start Waypoint');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=22 AND `SourceGroup` = 3 AND `SourceEntry`=77695;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(22, 3, 77695, 0, 0, 58, 1, 0, 0, 0, 'EventRadiantFury', 0, 0, 0, '', 'Scripted creature has StringId \'EventRadiantFury\'');

-- SAI Sargerei Spirit-Tender
DELETE FROM `smart_scripts` WHERE `entryorguid`=77131 AND `source_type`=0;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(77131, 0, 0, 0, '', 63, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 156920, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Sargerei Spirit-Tender - On Just Created - Self Cast spell 156920');

-- SAI Sargerei Soulbinder
DELETE FROM `smart_scripts` WHERE `entryorguid`=77812 AND `source_type`=0;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(77812, 0, 0, 0, '', 63, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 156920, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Sargerei Soulbinder - On Just Created - Self Cast spell 156920');

-- SAI Sargerei Cleric
DELETE FROM `smart_scripts` WHERE `entryorguid`=77134 AND `source_type`=0;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(77134, 0, 0, 0, '', 63, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 156920, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Sargerei Cleric - On Just Created - Self Cast spell 156920');

-- SAI Soulbinder Nymia
DELETE FROM `smart_scripts` WHERE `entryorguid`=77810 AND `source_type`=0;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(77810, 0, 0, 0, '', 63, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 156920, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Soulbinder Nymia - On Just Created - Self Cast spell 156920');

-- SAI Auchenai Magus
DELETE FROM `smart_scripts` WHERE `entryorguid`=77694 AND `source_type`=0;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 7769400 AND `source_type`=9;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(77694, 0, 0, 0, '', 1, 0, 100, 0, 9000, 30000, 9000, 30000, 0, '', 11, 157652, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Magus (OOC) - Self: Cast spell 157652 on Self'),
(77694, 0, 1, 0, '', 40, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 157652, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Magus - Waypoint Reached - Cast spell 157652'),
(77694, 0, 2, 0, '', 38, 0, 100, 0, 0, 1, 0, 0, 0, '', 80, (77694 * 100), 2, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Magus - On Data Set - Action List'),
(7769400, 9, 0, 0, '', 0, 0, 100, 0, 0, 0, 0, 0, 0, '', 53, 0, 7769400, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, 'EventArcaneBomb', 0, 0, 0, 0, 'Auchenai Soulpriest - Action List - Start Waypoint');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=22 AND `SourceGroup` IN (1, 2, 3) AND `SourceEntry`=77694;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(22, 1, 77694, 0, 0, 58, 1, 0, 0, 0, 'CosmeticArcaneBomb', 0, 0, 0, '', 'Scripted creature has StringId \'CosmeticArcaneBomb\''),
(22, 2, 77694, 0, 0, 58, 1, 0, 0, 0, 'EventArcaneBomb', 0, 0, 0, '', 'Scripted creature has StringId \'EventArcaneBomb\''),
(22, 3, 77694, 0, 0, 58, 1, 0, 0, 0, 'EventArcaneBomb', 0, 0, 0, '', 'Scripted creature has StringId \'EventArcaneBomb\'');

-- Event at Nave of Eternal Rest
DELETE FROM `smart_scripts` WHERE `entryorguid`=77696 AND `source_type`=0;
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (7769600, 7769601, 7769602) AND `source_type`=9;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(77696, 0, 0, 0, '', 1, 0, 100, 0, 39000, 39000, 39000, 39000, 0, '', 87, 7769600, 7769601, 7769602, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Soulpriest (OOC) - Run Random Script'),
(77696, 0, 1, 0, '', 40, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 157762, 0, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Soulpriest (OOC) - Waypoint Reached - Cast spell 157762'),
(7769600, 9, 0, 0, '', 0, 0, 100, 0, 0, 0, 0, 0, 0, '', 53, 0, 7769600, 0, 0, 0, 0, 0, NULL, 1, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'Auchenai Soulpriest - Action List - Start Waypoint'),
(7769601, 9, 1, 0, '', 0, 0, 100, 0, 0, 0, 0, 0, 0, '', 45, 0, 1, 0, 0, 0, 0, 0, NULL, 19, 77694, 0, 0, 0, 'EventArcaneBomb', 0, 0, 0, 0, 'Auchenai Soulpriest - Action List - Set Data 0 1 to Auchenai Magus'),
(7769602, 9, 2, 0, '', 0, 0, 100, 0, 0, 0, 0, 0, 0, '', 45, 0, 1, 0, 0, 0, 0, 0, NULL, 19, 77695, 0, 0, 0, 'EventRadiantFury', 0, 0, 0, 0, 'Auchenai Soulpriest - Action List - Set Data 0 1 to Auchenai Arbiter');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=22 AND `SourceGroup` IN (1, 2) AND `SourceEntry`=77696;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(22, 1, 77696, 0, 0, 58, 1, 0, 0, 0, 'EventHalo', 0, 0, 0, '', 'Scripted creature has StringId \'EventHalo\''),
(22, 2, 77696, 0, 0, 58, 1, 0, 0, 0, 'EventHalo', 0, 0, 0, '', 'Scripted creature has StringId \'EventHalo\'');

-- Path for CGUID+16
SET @ENTRY := 77704;
SET @PATHOFFSET := 0;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 2, 'Auchenai Warden - Cosmetic Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, 1501.226, 2956.841, 35.23874, NULL, 16300),
(@PATH, 1, 1538.024, 2956.989, 35.23879, NULL, 0),
(@PATH, 2, 1538.042, 2949.358, 35.2387, NULL, 0),
(@PATH, 3, 1501.196, 2949.155, 35.22998, NULL, 15971),
(@PATH, 4, 1538.042, 2949.358, 35.2387, NULL, 0),
(@PATH, 5, 1538.024, 2956.989, 35.23879, NULL, 0);

UPDATE `creature` SET `position_x`= 1501.226, `position_y`= 2956.841, `position_z`= 35.23874, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+16;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+16;
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(@CGUID+16, @PATH, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '155647'); -- Auchenai Warden - 155647 - NPC Reaction

-- Path for CGUID+120
SET @ENTRY := 77704;
SET @PATHOFFSET := 1;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Auchenai Warden - Cosmetic Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, 1672.106, 3022.716, 35.16462, NULL, 10562),
(@PATH, 1, 1673.693, 3032.523, 35.23431, NULL, 16588);

UPDATE `creature` SET `position_x`= 1672.106, `position_y`= 3022.716, `position_z`= 35.16462, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+120;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+120;
INSERT INTO `creature_addon` (`guid`, `PathId`, `SheathState`) VALUES
(@CGUID+120, @PATH, 1);

-- Path for CGUID+137
SET @ENTRY := 77695;
SET @PATHOFFSET := 0;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Auchenai Arbiter - Cosmetic Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, 1709.325, 3099.063, 35.13764, NULL, 10193),
(@PATH, 1, 1714.448, 3104.976, 35.13763, NULL, 7779);

UPDATE `creature` SET `position_x`= 1709.325, `position_y`= 3099.063, `position_z`= 35.13764, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+137;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+137;
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(@CGUID+137, @PATH, 0, 0, 0, 0, 1, 0, 378, 0, 0, 0, 0, ''); -- Auchenai Arbiter

-- Path for Auchenai Arbiter
SET @ENTRY := 77695;
SET @PATHOFFSET := 1;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Auchenai Arbiter - Scripted Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, 1660.043, 2953.735, 34.27824, NULL, 284),
(@PATH, 1, 1663.869, 2930.622, 34.82322, NULL, 280),
(@PATH, 2, 1650.58, 2962.983, 35.05951, 5.50257110, 0);

-- Path for Auchenai Soulpriest
SET @ENTRY := 77696;
SET @PATHOFFSET := 0;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Auchenai Soulpriest - Scripted Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, 1661.759, 2953.042, 34.27822, NULL, 5126),
(@PATH, 1, 1674.719, 2953.108, 35.05952, 3.13579130, 0);

-- Path for Auchenai Magus
SET @ENTRY := 77694;
SET @PATHOFFSET := 0;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Auchenai Magus - Scripted Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, 1660.287, 2952.155, 34.27823, NULL, 11557),
(@PATH, 1, 1654.024, 2940.446, 35.05951, 1.07963836, 0);




SET @OGUID := 7000316;
SET @ATID := 69;
SET @ATCP := 56;
SET @ATIDSPAWN := 70;
SET @SPAWNGROUP := 1251;
SET @WORLDSAFELOCID := 100042;

-- GameObjects
DELETE FROM `gameobject` WHERE `guid` BETWEEN @OGUID+0 AND @OGUID+18;
INSERT INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `VerifiedBuild`, `size`, `visibility`) VALUES
(@OGUID+0, 276643, 1754, 9164, 9640, '1,2,23,8', 0, 0, -1563.5816650390625, -1020.588623046875, 76.08814239501953125, 2.35677337646484375, 0, 0, 0.923990249633789062, 0.382416039705276489, 7200, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Smuggler's Run - Difficulty: 0) CreateObject1
(@OGUID+1, 280662, 1771, 9327, 9327, '1,2,23,8', 0, 0, 79.2725677490234375, -2941.302001953125, -0.23841531574726104, 4.660104751586914062, 0, 0, -0.72534847259521484, 0.688381910324096679, 7200, 255, 1, 55142, 1, 100), -- Boat (Area: Tol Dagor - Difficulty: 0) CreateObject1
(@OGUID+2, 252245, 1771, 9327, 9327, '1,2,23,8', 0, 0, 79.109375, -2942.5244140625, 1.989225983619689941, 1.440670013427734375, 0, 0, 0.659636497497558593, 0.751584768295288085, 7200, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Tol Dagor - Difficulty: 0) CreateObject1
(@OGUID+3, 252245, 1643, 10015, 10015, '0', 0, 0, 33.43229293823242187, -2658.704833984375, 27.01893424987792968, 5.491595268249511718, 0, 0, -0.38554191589355468, 0.922690331935882568, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Tol Dagor - Difficulty: 0) CreateObject1
(@OGUID+4, 252245, 1643, 8721, 9561, '0', 0, 0, 793.57989501953125, 3371.655517578125, 235.0142669677734375, 0, 0, 0, 0, 1, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Waycrest Manor - Difficulty: 0) CreateObject1
(@OGUID+5, 252245, 1862, 9424, 9424, '1,23,2,8', 0, 0, -634.33160400390625, -259.921875, 238.173828125, 0, 0, 0, 0, 1, 7200, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Waycrest Manor - Difficulty: 0) CreateObject1
(@OGUID+6, 252248, 1643, 9042, 9767, '0', 0, 0, 4161.1318359375, -1140.779541015625, 164.7171478271484375, 1.737511277198791503, 0, 0, 0.763525962829589843, 0.64577716588973999, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Shrine of the Storm - Difficulty: 0) CreateObject1
(@OGUID+7, 252247, 1642, 8499, 8965, '0', 0, 0, -2647.25, 2388.3837890625, 10.04741477966308593, 3.517940521240234375, 0, 0, -0.98234748840332031, 0.187065258622169494, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Xibala - Difficulty: 0) CreateObject1
(@OGUID+8, 252247, 1594, 8064, 9729, '1,2,23,8', 0, 0, 630.34027099609375, -3620.70654296875, 3.562418222427368164, 5.49183511734008789, 0, 0, -0.38543128967285156, 0.922736525535583496, 7200, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Platinum Sands - Difficulty: 0) CreateObject1
(@OGUID+9, 252248, 1642, 8499, 8731, '0', 0, 0, -848.28472900390625, 2044.1492919921875, 728.180908203125, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Mount Mugamba - Difficulty: 0) CreateObject1
(@OGUID+10, 252237, 1642, 8499, 9404, '0', 0, 0, -848.59375, 2541.303955078125, 734.68096923828125, 4.717230796813964843, 0, 0, -0.70539283752441406, 0.7088165283203125, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Atal'Dazar - Difficulty: 0) CreateObject1
(@OGUID+11, 252247, 1642, 8500, 0, '0', 0, 0, 1257.55908203125, 747.90277099609375, -268.368682861328125, 0.762700915336608886, 0, 0, 0.372174263000488281, 0.928162872791290283, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: 0 - Difficulty: 0) CreateObject1
(@OGUID+12, 252247, 1642, 8501, 0, '0', 0, 0, 3205.748291015625, 3151.78125, 124.6168594360351562, 0, 0, 0, 0, 1, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: 0 - Difficulty: 0) CreateObject1
(@OGUID+13, 252247, 1877, 9527, 9527, '1,2,23,8', 0, 0, 3172.3203125, 3151, 125.0942001342773437, 0, 0, 0, 0, 1, 7200, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Temple of Sethraliss - Difficulty: 0) CreateObject1
(@OGUID+14, 252248, 1642, 8499, 8665, '0', 0, 0, -2001.7882080078125, 967.2569580078125, 8.890947341918945312, 1.561707258224487304, 0, 0, 0.703886032104492187, 0.710312962532043457, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Port of Zandalar - Difficulty: 0) CreateObject1
(@OGUID+15, 252247, 1594, 8064, 9729, '1,2,23,8', 0, 0, 616.23785400390625, -3637.442626953125, 6.78265237808227539, 6.111760616302490234, 0, 0, -0.08560752868652343, 0.996328949928283691, 7200, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Platinum Sands - Difficulty: 0) CreateObject1
(@OGUID+16, 252245, 1643, 8567, 9694, '0', 0, 0, -211.807296752929687, -1561.48095703125, 4.032770633697509765, 0.795597970485687255, 0, 0, 0.38739013671875, 0.921915888786315917, 120, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Plunder Harbor - Difficulty: 0) CreateObject1
(@OGUID+17, 252245, 1822, 9354, 9354, '23,8,2', 0, 0, 1078.1754150390625, -590.232666015625, 2.224796772003173828, 1.598357081413269042, 0, 0, 0.716783523559570312, 0.697295784950256347, 7200, 255, 1, 55142, 1, 100), -- Instance Portal (Area: Siege of Boralus - Difficulty: 0) CreateObject1
(@OGUID+18, 252245, 1822, 9354, 9354, '23,8,2', 0, 0, 1070.451416015625, -626.623291015625, 19.12912750244140625, 0, 0, 0, 0, 1, 86400, 255, 1, 55142, 1, 100); -- Instance Portal (Area: Siege of Boralus - Difficulty: Heroic) CreateObject1

-- Areatrigger
DELETE FROM `areatrigger_template` WHERE `Id` BETWEEN @ATID+0 AND @ATID+23;
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `VerifiedBuild`) VALUES 
(@ATID+0, 1, 1, 0),
(@ATID+1, 1, 1, 0),
(@ATID+2, 1, 1, 0),
(@ATID+3, 1, 1, 0),
(@ATID+4, 1, 1, 0),
(@ATID+5, 1, 1, 0),
(@ATID+6, 1, 1, 0),
(@ATID+7, 1, 1, 0),
(@ATID+8, 1, 1, 0),
(@ATID+9, 1, 1, 0),
(@ATID+10, 1, 1, 0),
(@ATID+11, 1, 1, 0),
(@ATID+12, 1, 1, 0),
(@ATID+13, 1, 1, 0),
(@ATID+14, 1, 1, 0),
(@ATID+15, 1, 1, 0),
(@ATID+16, 1, 1, 0),
(@ATID+17, 1, 1, 0),
(@ATID+18, 1, 1, 0),
(@ATID+19, 1, 1, 0),
(@ATID+20, 1, 1, 0),
(@ATID+21, 1, 1, 0),
(@ATID+22, 1, 1, 0),
(@ATID+23, 1, 1, 0);

DELETE FROM `areatrigger_template_actions` WHERE `AreaTriggerId` BETWEEN @ATID+0 AND @ATID+23;
INSERT INTO `areatrigger_template_actions` (`AreaTriggerId`, `IsCustom`, `ActionType`, `ActionParam`, `TargetType`) VALUES
(@ATID+0, 1, 2, @WORLDSAFELOCID+0, 5),
(@ATID+1, 1, 2, @WORLDSAFELOCID+1, 5),
(@ATID+2, 1, 2, @WORLDSAFELOCID+2, 5),
(@ATID+3, 1, 2, @WORLDSAFELOCID+3, 5),
(@ATID+4, 1, 2, @WORLDSAFELOCID+4, 5),
(@ATID+5, 1, 2, @WORLDSAFELOCID+5, 5),
(@ATID+6, 1, 2, @WORLDSAFELOCID+6, 5),
(@ATID+7, 1, 2, @WORLDSAFELOCID+7, 5),
(@ATID+8, 1, 2, @WORLDSAFELOCID+8, 5),
(@ATID+9, 1, 2, @WORLDSAFELOCID+9, 5),
(@ATID+10, 1, 2, @WORLDSAFELOCID+10, 5),
(@ATID+11, 1, 2, @WORLDSAFELOCID+11, 5),
(@ATID+12, 1, 2, @WORLDSAFELOCID+12, 5),
(@ATID+13, 1, 2, @WORLDSAFELOCID+13, 5),
(@ATID+14, 1, 2, @WORLDSAFELOCID+14, 5),
(@ATID+15, 1, 2, @WORLDSAFELOCID+15, 5),
(@ATID+16, 1, 2, @WORLDSAFELOCID+16, 5),
(@ATID+17, 1, 2, @WORLDSAFELOCID+17, 5),
(@ATID+18, 1, 2, @WORLDSAFELOCID+18, 5),
(@ATID+19, 1, 2, @WORLDSAFELOCID+19, 5),
(@ATID+20, 1, 2, @WORLDSAFELOCID+20, 5),
(@ATID+21, 1, 2, @WORLDSAFELOCID+21, 5),
(@ATID+22, 1, 2, @WORLDSAFELOCID+22, 5),
(@ATID+23, 1, 2, @WORLDSAFELOCID+23, 5);

DELETE FROM `areatrigger_create_properties` WHERE `Id` BETWEEN @ATCP+0 AND @ATCP+23;
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `TimeToTarget`, `TimeToTargetScale`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`) VALUES 
(@ATCP+0, 1, @ATID+0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 3, 4, 5, 3, 4, 5, 0, 0, '', 0), -- Siege of Boralus (A) entrance
(@ATCP+1, 1, @ATID+1, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 3, 3, 5, 3, 3, 5, 0, 0, '', 0), -- Siege of Boralus (A) exit
(@ATCP+2, 1, @ATID+2, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 5, 8, 8, 5, 8, 8, 0, 0, '', 0), -- Freehold entrance
(@ATCP+3, 1, @ATID+3, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 5, 10, 8, 5, 10, 8, 0, 0, '', 0), -- Freehold exit
(@ATCP+4, 1, @ATID+4, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 1, 2, 5, 1, 2, 5, 0, 0, '', 0), -- Tol Dagor entrance
(@ATCP+5, 1, @ATID+5, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 2, 2, 5, 2, 2, 5, 0, 0, '', 0), -- Tol Dagor exit
(@ATCP+6, 1, @ATID+6, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 1, 3, 5, 1, 3, 5, 0, 0, '', 0), -- Waycrest Manor entrance
(@ATCP+7, 1, @ATID+7, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 2, 4, 5, 2, 4, 5, 0, 0, '', 0), -- Waycrest Manor exit
(@ATCP+8, 1, @ATID+8, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 4, 4, 8, 4, 4, 8, 0, 0, '', 0), -- Shrine of Storms entrance
(@ATCP+9, 1, @ATID+9, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 4, 5, 8, 4, 5, 8, 0, 0, '', 0), -- Shrine of Storms exit
(@ATCP+10, 1, @ATID+10, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 4, 6, 10, 4, 6, 10, 0, 0, '', 0), -- Motherlode (A) entrance
(@ATCP+11, 1, @ATID+11, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 4, 8, 10, 4, 8, 10, 0, 0, '', 0), -- Motherlode (A) exit
(@ATCP+12, 1, @ATID+12, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 3, 6, 15, 3, 6, 15, 0, 0, '', 0), -- Atal Dazar entrance
(@ATCP+13, 1, @ATID+13, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 3, 6, 15, 3, 6, 15, 0, 0, '', 0), -- Atal Dazar exit
(@ATCP+14, 1, @ATID+14, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 5, 16, 20, 5, 16, 20, 0, 0, '', 0), -- KingsRest entrance
(@ATCP+15, 1, @ATID+15, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 5, 16, 15, 5, 16, 15, 0, 0, '', 0), -- KingsRest exit
(@ATCP+16, 1, @ATID+16, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 4, 18, 20, 4, 18, 20, 0, 0, '', 0), -- Underrot entrance
(@ATCP+17, 1, @ATID+17, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 4, 18, 20, 4, 18, 20, 0, 0, '', 0), -- Underrot exit
(@ATCP+18, 1, @ATID+18, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 4, 8, 20, 4, 8, 20, 0, 0, '', 0), -- Temple of Sethraliss entrance
(@ATCP+19, 1, @ATID+19, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 4, 8, 20, 4, 8, 20, 0, 0, '', 0), -- Temple of Sethraliss exit
(@ATCP+20, 1, @ATID+20, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 3, 6, 20, 3, 6, 20, 0, 0, '', 0), -- Motherlode (H) entrance
(@ATCP+21, 1, @ATID+21, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 4, 10, 10, 4, 10, 10, 0, 0, '', 0), -- Motherlode (H) exit
(@ATCP+22, 1, @ATID+22, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 2, 4, 15, 2, 4, 15, 0, 0, '', 0), -- Siege of Boralus (H) entrance
(@ATCP+23, 1, @ATID+23, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 3, 8, 10, 3, 8, 10, 0, 0, '', 0); -- Siege of Boralus (H) exit

DELETE FROM `areatrigger` WHERE `SpawnId` BETWEEN @ATIDSPAWN+0 AND @ATIDSPAWN+23;
INSERT INTO `areatrigger` (`SpawnId`, `AreaTriggerCreatePropertiesId`, `IsCustom`, `MapId`, `SpawnDifficulties`, `PosX`, `PosY`, `PosZ`, `Orientation`, `PhaseUseFlags`, `PhaseId`, `PhaseGroup`, `ScriptName`, `Comment`, `VerifiedBuild`) VALUES 
(@ATIDSPAWN+0, @ATCP+0, 1, 1643, '0', 1149.427, -554.867, 1.699, 1.375182, 0, 0, 0, '', '8.x Dungeon - Siege of Boralus - Entrance', 0),
(@ATIDSPAWN+1, @ATCP+1, 1, 1822, '23,8,2', 1068.079, -626.600, 17.993, 3.164860, 0, 0, 0, '', '8.x Dungeon - Siege of Boralus - Exit', 0),
(@ATIDSPAWN+2, @ATCP+2, 1, 1643, '0', -1587.766, -1274.649, 36.3525, 5.097314, 0, 0, 0, '', '8.x Dungeon - Freehold - Entrance', 0),
(@ATIDSPAWN+3, @ATCP+3, 1, 1754, '1,2,23,8', -1562.680, -1019.890, 73.518, 5.533423, 0, 0, 0, '', '8.x Dungeon - Freehold - Exit', 0),
(@ATIDSPAWN+4, @ATCP+4, 1, 1643, '0', 33.886, -2658.966, 26.349, 5.665448, 0, 0, 0, '', '8.x Dungeon - Tol Dagor - Entrance', 0),
(@ATIDSPAWN+5, @ATCP+5, 1, 1771, '1,2,23,8', 79.330, -2942.568, 0.385, 4.662903, 0, 0, 0, '', '8.x Dungeon - Tol Dagor - Exit', 0),
(@ATIDSPAWN+6, @ATCP+6, 1, 1643, '0', 794.152, 3371.817, 234.490, 6.174334, 0, 0, 0, '', '8.x Dungeon - Waycrest Manor - Entrance', 0),
(@ATIDSPAWN+7, @ATCP+7, 1, 1862, '1,2,23,8', -634.967, -259.9255, 236.156, 3.127803, 0, 0, 0, '', '8.x Dungeon - Waycrest Manor - Exit', 0),
(@ATIDSPAWN+8, @ATCP+8, 1, 1643, '0', 4161.641, -1142.632, 164.774, 4.822304, 0, 0, 0, '', '8.x Dungeon - Shrine of Storms - Entrance', 0),
(@ATIDSPAWN+9, @ATCP+9, 1, 1864, '1,2,23,8', 4167.192, -1220.847, 187.712, 1.577227, 0, 0, 0, '', '8.x Dungeon - Shrine of Storms - Exit', 0),
(@ATIDSPAWN+10, @ATCP+10, 1, 1642, '0', -2646.077, 2389.278, 8.7910, 0.205582, 0, 0, 0, '', '8.x Dungeon - Motherlode (Alliance) - Entrance', 0),
(@ATIDSPAWN+11, @ATCP+11, 1, 1594, '1,2,23,8', 626.649, -3619.271, -0.8169, 2.221698, 0, 0, 0, '', '8.x Dungeon - Motherlode (Alliance) - Exit', 0),
(@ATIDSPAWN+12, @ATCP+12, 1, 1642, '0', -848.366, 2044.2585, 726.1221, 1.564042, 0, 0, 0, '', '8.x Dungeon - Atal Dazar - Entrance', 0),
(@ATIDSPAWN+13, @ATCP+13, 1, 1763, '1,2,23,8', -848.193, 2043.196, 726.1222, 4.705334, 0, 0, 0, '', '8.x Dungeon - Atal Dazar - Exit', 0),
(@ATIDSPAWN+14, @ATCP+14, 1, 1642, '0', -848.042, 2542.115, 732.573, 1.573840, 0, 0, 0, '', '8.x Dungeon - KingsRest - Entrance', 0),
(@ATIDSPAWN+15, @ATCP+15, 1, 1762, '23,8,2', -945.247, 2521.6589, 833.5416, 4.739499, 0, 0, 0, '', '8.x Dungeon - KingsRest - Exit', 0),
(@ATIDSPAWN+16, @ATCP+16, 1, 1642, '0', 1258.614, 748.319, -273.2040, 0.756919, 0, 0, 0, '', '8.x Dungeon - Underrot - Entrance', 0),
(@ATIDSPAWN+17, @ATCP+17, 1, 1841, '1,2,23,8', 615.851, 1256.516, 100.362, 2.7162, 0, 0, 0, '', '8.x Dungeon - Underrot - Exit', 0),
(@ATIDSPAWN+18, @ATCP+18, 1, 1642, '0', 3208.812, 3151.754, 121.34663, 0.010269, 0, 0, 0, '', '8.x Dungeon - Temple of Sethraliss - Entrance', 0),
(@ATIDSPAWN+19, @ATCP+19, 1, 1877, '1,2,23,8', 3169.462, 3150.905, 121.6228, 3.082911, 0, 0, 0, '', '8.x Dungeon - Temple of Sethraliss - Exit', 0),
(@ATIDSPAWN+20, @ATCP+20, 1, 1642, '0', -2001.899, 967.272, 6.2339, 1.542327, 0, 0, 0, '', '8.x Dungeon - Motherlode (Horde) - Entrance', 0),
(@ATIDSPAWN+21, @ATCP+21, 1, 1594, '1,2,23,8', 614.331, -3637.275, 4.1750, 2.9619, 0, 0, 0, '', '8.x Dungeon - Motherlode (Horde) - Exit', 0),
(@ATIDSPAWN+22, @ATCP+22, 1, 1643, '0', -211.818, -1562.208, 2.8376, 3.846468, 0, 0, 0, '', '8.x Dungeon - Siege of Boralus (Horde) - Entrance', 0),
(@ATIDSPAWN+23, @ATCP+23, 1, 1822, '23,8,2', 1074.387, -588.369, 1.0748, 1.602863, 0, 0, 0, '', '8.x Dungeon - Siege of Boralus (Horde) - Exit', 0);

DELETE FROM `world_safe_locs` WHERE `ID` BETWEEN @WORLDSAFELOCID+0 AND @WORLDSAFELOCID+23;
INSERT INTO `world_safe_locs` (`ID`, `MapID`, `LocX`, `LocY`, `LocZ`, `Facing`, `COMMENT`) VALUES
(@WORLDSAFELOCID+0, 1822, 1084.48, -617.952, 17.5513, 0.42834568, '8.x Dungeon - Siege of Boralus - Entrance'),
(@WORLDSAFELOCID+1, 1643, 1138.77, -575.858, 1.1963, 4.4846063, '8.x Dungeon - Siege of Boralus - Exit'),
(@WORLDSAFELOCID+2, 1754, -1586.72, -1002.61, 73.4672, 2.866721, '8.x Dungeon - Freehold - Entrance'),
(@WORLDSAFELOCID+3, 1643, -1580.16, -1293.06, 34.3126, 5.385859, '8.x Dungeon - Freehold - Exit'),
(@WORLDSAFELOCID+4, 1771, 85.2528, -2933.52, 1.37269, 1.3391021, '8.x Dungeon - Tol Dagor - Entrance'),
(@WORLDSAFELOCID+5, 1643, 26.1921, -2650.23, 23.5238, 2.5878694, '8.x Dungeon - Tol Dagor - Exit'),
(@WORLDSAFELOCID+6, 1862, -626.408, -260.15103, 236.03317, 0.072306424, '8.x Dungeon - Waycrest Manor - Entrance'),
(@WORLDSAFELOCID+7, 1643, 789.7552, 3372.0774, 232.72673, 3.067398, '8.x Dungeon - Waycrest Manor - Exit'),
(@WORLDSAFELOCID+8, 1864, 4167.202, -1238.5278, 186.89357, 4.73095, '8.x Dungeon - Shrine of Storms - Entrance'),
(@WORLDSAFELOCID+9, 1643, 4151.3, -1097, 158.385, 1.7824349, '8.x Dungeon - Shrine of Storms - Exit'),
(@WORLDSAFELOCID+10, 1594, 630.774, -3640.81, 1.74989, 6.0063233, '8.x Dungeon - Motherlode (Alliance) - Entrance'),
(@WORLDSAFELOCID+11, 1642, -2653.96, 2387.93, 5.05817, 3.607927, '8.x Dungeon - Motherlode (Alliance) - Exit'),
(@WORLDSAFELOCID+12, 1763, -848.363, 2082.47, 725.146, 1.5707964, '8.x Dungeon - Atal Dazar - Entrance'),
(@WORLDSAFELOCID+13, 1642, -848.42, 2028.05, 726.19, 4.7006083, '8.x Dungeon - Atal Dazar - Exit'),
(@WORLDSAFELOCID+14, 1762, -945.114, 2544.25, 833.052, 1.578214, '8.x Dungeon - KingsRest - Entrance'),
(@WORLDSAFELOCID+15, 1642, -848.205, 2515.88, 730.225, 4.752863, '8.x Dungeon - KingsRest - Exit'),
(@WORLDSAFELOCID+16, 1841, 636.715, 1253.03, 98.588, 6.232135, '8.x Dungeon - Underrot - Entrance'),
(@WORLDSAFELOCID+17, 1642, 1242.41, 736.916, -271.521, 3.843372, '8.x Dungeon - Underrot - Exit'),
(@WORLDSAFELOCID+18, 1877, 3199.21, 3151.17, 121.316, 6.275331, '8.x Dungeon - Temple of Sethraliss - Entrance'),
(@WORLDSAFELOCID+19, 1642, 3196.15, 3151.61, 121.654, 3.1415577, '8.x Dungeon - Temple of Sethraliss - Exit'),
(@WORLDSAFELOCID+20, 1594, 630.774, -3640.81, 1.74989, 6.0063233, '8.x Dungeon - Motherlode (H) - Entrance'),
(@WORLDSAFELOCID+21, 1642, -2002.65, 959.031, 5.93225, 4.740838, '8.x Dungeon - Motherlode (H) - Exit'),
(@WORLDSAFELOCID+22, 1822, 1080.03, -604.543, 0.985657, 4.8422937, '8.x Dungeon - Siege of Boralus (H) - Entrance'),
(@WORLDSAFELOCID+23, 1643, -204.47, -1550.87, 2.95007, 0.7578639, '8.x Dungeon - Siege of Boralus (H) - Exit');

-- Spawngroups
DELETE FROM `spawn_group_template` WHERE `groupId` BETWEEN @SPAWNGROUP+0 AND @SPAWNGROUP+3;
INSERT INTO `spawn_group_template` (`groupId`, `groupName`, `groupFlags`) VALUES
(@SPAWNGROUP+0, 'Siege of Boralus - Alliance spawns', 32),
(@SPAWNGROUP+1, 'Siege of Boralus - Horde spawns', 32),
(@SPAWNGROUP+2, 'Motherlode - Alliance spawns', 32),
(@SPAWNGROUP+3, 'Motherlode - Horde spawns', 32);

DELETE FROM `spawn_group` WHERE `groupId` BETWEEN @SPAWNGROUP+0 AND @SPAWNGROUP+3 AND `spawnType`= 1;
INSERT INTO `spawn_group` (`groupId`, `spawnType`, `spawnId`) VALUES
(@SPAWNGROUP+0, 1, @OGUID+18),
(@SPAWNGROUP+1, 1, @OGUID+17),
(@SPAWNGROUP+2, 1, @OGUID+8),
(@SPAWNGROUP+3, 1, @OGUID+15);

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=33 AND `SourceGroup` = 0 AND `SourceEntry` BETWEEN @SPAWNGROUP+0 AND @SPAWNGROUP+3;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(33, 0, @SPAWNGROUP+0, 0, 0, 11, 0, 4485, 1, 0, '', 0, 0, 0, '', 'Siege of Boralus - Spawn Alliance NPCs if you are Alliance Player'),
(33, 0, @SPAWNGROUP+1, 0, 0, 11, 0, 4486, 1, 0, '', 0, 0, 0, '', 'Siege of Boralus - Spawn Horde NPCs if you are Horde Player'),
(33, 0, @SPAWNGROUP+2, 0, 0, 11, 0, 4485, 1, 0, '', 0, 0, 0, '', 'Motherlode - Spawn Alliance NPCs if you are Alliance Player'),
(33, 0, @SPAWNGROUP+3, 0, 0, 11, 0, 4486, 1, 0, '', 0, 0, 0, '', 'Motherlode - Spawn Horde NPCs if you are Horde Player');

-- Conditions
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 28 AND `SourceGroup` IN (@ATID+1, @ATID+23, @ATID+11, @ATID+21) AND `SourceEntry` = 1;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(28, @ATID+1, 1, 0, 0, 6, 0, 469, 0, 0, '', 0, 0, 0, '', 'Only trigger areatrigger when player is Alliance'),
(28, @ATID+23, 1, 0, 0, 6, 0, 67, 0, 0, '', 0, 0, 0, '', 'Only trigger areatrigger when player is Horde'),
(28, @ATID+11, 1, 0, 0, 6, 0, 469, 0, 0, '', 0, 0, 0, '', 'Only trigger areatrigger when player is Alliance'),
(28, @ATID+21, 1, 0, 0, 6, 0, 67, 0, 0, '', 0, 0, 0, '', 'Only trigger areatrigger when player is Horde');





SET @CGUID := 7001159;
SET @OGUID := 7000335;

-- Creature
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+190;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`, `size`) VALUES
(@CGUID+0, 137034, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -571.02606201171875, -237.493057250976562, 236.1141204833984375, 1.07134711742401123, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+1, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -569.02777099609375, -247.725692749023437, 236.115203857421875, 0.593174397945404052, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+2, 137034, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -589.04864501953125, -236.845489501953125, 236.1165008544921875, 3.132869243621826171, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+3, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -590.91143798828125, -232.553817749023437, 236.1165008544921875, 4.219472885131835937, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+4, 131585, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -587.209716796875, -217.019607543945312, 249.5423431396484375, 4.667870521545410156, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Enthralled Guard (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+5, 137034, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -567.50347900390625, -270.649322509765625, 236.115234375, 5.487166404724121093, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+6, 137034, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -569.87677001953125, -243.135421752929687, 235.71575927734375, 0.195613443851470947, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+7, 135359, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -572.7257080078125, -259.947906494140625, 236.112335205078125, 4.211675167083740234, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Sister Solena (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+8, 135358, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -572.34027099609375, -254.649307250976562, 236.36468505859375, 4.211675167083740234, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Sister Malady (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+9, 137034, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -568.39410400390625, -274.588531494140625, 236.114532470703125, 1.339905619621276855, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+10, 120652, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -600.4600830078125, -259.513885498046875, 236.1165008544921875, 3.138837814331054687, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Global Affix Stalker (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+11, 135360, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -572.029541015625, -264.859375, 236.477783203125, 3.085693836212158203, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Sister Briar (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+12, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -574.296875, -233.689239501953125, 236.11517333984375, 0.240302875638008117, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+13, 135240, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -588.46527099609375, -288.151031494140625, 236.1050872802734375, 2.932393789291381835, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Soul Essence (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+14, 131685, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -589.43927001953125, -219.397567749023437, 249.5423431396484375, 4.667777061462402343, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+15, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -592.93927001953125, -235.092010498046875, 236.1165008544921875, 5.983878135681152343, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+16, 135240, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -593.44097900390625, -284.28125, 236.1165008544921875, 4.793862342834472656, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Soul Essence (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+17, 137048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -564.6475830078125, -246.010421752929687, 236.1155242919921875, 3.550517797470092773, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Beguiled Villager (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 269121 - Cosmetic Beguiled)
(@CGUID+18, 135240, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -566.404541015625, -242.09375, 236.1143951416015625, 4.904598236083984375, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Soul Essence (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+19, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -571.18927001953125, -267.489593505859375, 189.703277587890625, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+20, 135240, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -570.81768798828125, -230.819442749023437, 236.11474609375, 4.607960700988769531, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Soul Essence (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+21, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -562.55035400390625, -270.970489501953125, 236.11614990234375, 3.373791933059692382, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+22, 137034, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -570.9774169921875, -287.258697509765625, 236.096588134765625, 0.87963569164276123, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+23, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -542.94964599609375, -254.725692749023437, 245.269439697265625, 4.352036952972412109, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+24, 135240, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -565.32464599609375, -277.258697509765625, 236.1145782470703125, 1.736617565155029296, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Soul Essence (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+25, 131585, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -566.89208984375, -195.686065673828125, 236.3025360107421875, 3.148679494857788085, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Enthralled Guard (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+26, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -540.951416015625, -260.102447509765625, 245.269439697265625, 3.221382379531860351, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+27, 135240, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -564.00518798828125, -268.166656494140625, 236.115875244140625, 4.22330474853515625, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Soul Essence (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+28, 135357, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -558.47222900390625, -259.2725830078125, 242.269927978515625, 3.109388589859008789, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Lady Waycrest (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 269118 - Cosmetic Missile Transform (Lady Waycrest))
(@CGUID+29, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -563.9444580078125, -274.401031494140625, 236.11541748046875, 2.378868579864501953, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+30, 137048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -570.75177001953125, -233.902786254882812, 236.114715576171875, 3.72335052490234375, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Beguiled Villager (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 269121 - Cosmetic Beguiled)
(@CGUID+31, 137034, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -541.9305419921875, -264.098968505859375, 245.269439697265625, 2.76560211181640625, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+32, 137048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -593.08856201171875, -288.59722900390625, 236.1165008544921875, 0.74304431676864624, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Beguiled Villager (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 269121 - Cosmetic Beguiled)
(@CGUID+33, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -557.592041015625, -282.107635498046875, 249.56805419921875, 3.109470605850219726, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+34, 137033, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -567.701416015625, -284.779510498046875, 236.115020751953125, 4.3520355224609375, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+35, 137034, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -560.77777099609375, -285.22222900390625, 249.5779876708984375, 2.070507049560546875, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 267413 - Cosmetic Missile Transform)
(@CGUID+36, 131587, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -566.87847900390625, -198.686050415039062, 236.208984375, 3.148666143417358398, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Bewitched Captain (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+37, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -570.2257080078125, -252.81597900390625, 190.171112060546875, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+38, 135240, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -562.64239501953125, -249.279510498046875, 236.11614990234375, 2.500848293304443359, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Soul Essence (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+39, 134041, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -537.1492919921875, -295.505218505859375, 236.111572265625, 5.501866340637207031, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Infected Peasant (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 266008 - Infected, 272860 - Cosmetic Thin Man Begging)
(@CGUID+40, 131666, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -533.65277099609375, -271.77777099609375, 235.934051513671875, 5.43984079360961914, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Coven Thornshaper (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+41, 131858, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -534.95489501953125, -278.836822509765625, 236.0850372314453125, 0.851217627525329589, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thornguard (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+42, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -536.26947021484375, -301.89923095703125, 236.136077880859375, 2.365589141845703125, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+43, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -563.00347900390625, -275.4132080078125, 190.0098419189453125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+44, 131666, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -527.0867919921875, -279.923614501953125, 236.102294921875, 3.193425655364990234, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Coven Thornshaper (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+45, 135052, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -609.9375, -161.411453247070312, 235.6681060791015625, 0.304306626319885253, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Blight Toad (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+46, 135052, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -605.11114501953125, -165.319442749023437, 235.6681060791015625, 2.890430927276611328, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Blight Toad (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+47, 135234, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -561.6805419921875, -330.630218505859375, 249.580474853515625, 0.99009937047958374, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Diseased Mastiff (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%, 265630 - Mastiff Disease)
(@CGUID+48, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -533.86737060546875, -297.502685546875, 236.1360931396484375, 1.104825735092163085, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%, 278431 - Parasitic)
(@CGUID+49, 135234, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -562.54339599609375, -330.036468505859375, 236.112518310546875, 4.502013206481933593, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Diseased Mastiff (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%, 265630 - Mastiff Disease)
(@CGUID+50, 131850, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -559.3663330078125, -331.619781494140625, 249.5794525146484375, 0.910719037055969238, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Maddened Survivalist (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+51, 131849, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -560.46875, -327.008697509765625, 236.1114349365234375, 3.695490598678588867, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Crazed Marksman (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+52, 131850, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -564.39581298828125, -326.55035400390625, 236.1113739013671875, 4.548031330108642578, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Maddened Survivalist (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+53, 131849, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -586.49652099609375, -347.73089599609375, 235.7375946044921875, 1.860623478889465332, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Crazed Marksman (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+54, 135052, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -608.5850830078125, -167.772567749023437, 235.6681060791015625, 0.863629043102264404, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Blight Toad (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+55, 137830, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -534.4674072265625, -299.823974609375, 236.1360931396484375, 3.135935783386230468, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Pallid Gorger (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+56, 135711, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -592.66143798828125, -326.586822509765625, 237.138458251953125, 4.473574161529541015, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Hunting Map (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+57, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -539.27264404296875, -297.227996826171875, 236.3379974365234375, 2.469692468643188476, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+58, 135474, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -610.25, -164.918411254882812, 235.6681060791015625, 3.295940637588500976, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 269151 - Cosmetic Witch Frog Usage)
(@CGUID+59, 131850, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -588.1319580078125, -345.708343505859375, 235.7375946044921875, 5.884096622467041015, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Maddened Survivalist (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+60, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -562.75518798828125, -245.34375, 190.081329345703125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+61, 135139, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -531.71875, -330.572906494140625, 236.111572265625, 0.031905733048915863, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dead Villager (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 265448 - Permanent Feign Death On Spawn)
(@CGUID+62, 131823, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -568.6319580078125, -150.71875, 235.2671356201171875, 4.215025901794433593, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Sister Malady (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 272625 - Override Energy Bar Color, 261266 - Runic Ward)
(@CGUID+63, 131545, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -517.25, -260.104156494140625, 184.0252227783203125, 6.152141571044921875, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Lady Waycrest (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+64, 131587, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -516.30206298828125, -190.197921752929687, 236.116485595703125, 5.574076175689697265, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Bewitched Captain (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+65, 135234, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -558.09893798828125, -333.958343505859375, 249.5802154541015625, 1.205202937126159667, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Diseased Mastiff (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%, 265630 - Mastiff Disease)
(@CGUID+66, 131685, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -609.685791015625, -155.954864501953125, 249.51531982421875, 5.710288047790527343, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+67, 131821, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -528.7725830078125, -335.376739501953125, 249.515350341796875, 2.098216772079467773, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Faceless Maiden (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+68, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -552.91143798828125, -276.984375, 189.8520355224609375, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+69, 131825, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -580.234375, -150.5625, 235.267547607421875, 5.491910934448242187, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Sister Briar (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 272627 - Override Energy Bar Color, 261265 - Ironbark Shield)
(@CGUID+70, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -532.68438720703125, -326.05157470703125, 236.1360931396484375, 0.751057803630828857, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+71, 135139, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -521.44793701171875, -302.170135498046875, 236.111572265625, 3.358336210250854492, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dead Villager (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 265448 - Permanent Feign Death On Spawn)
(@CGUID+72, 131824, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -574.73089599609375, -145.892364501953125, 235.2644805908203125, 4.778371334075927734, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Sister Solena (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 272626 - Override Energy Bar Color)
(@CGUID+73, 135048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -537.25457763671875, -350.3045654296875, 236.136077880859375, 1.65627753734588623, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Gorestained Piglet (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+74, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -518.5155029296875, -303.159576416015625, 236.1361083984375, 3.967284917831420898, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+75, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -529.19805908203125, -329.118072509765625, 236.1361083984375, 5.95155954360961914, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%, 278431 - Parasitic)
(@CGUID+76, 131685, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -538.53125, -322.052093505859375, 249.5301361083984375, 5.361541748046875, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+77, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -521.76043701171875, -299.975921630859375, 236.1360931396484375, 2.779886484146118164, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%, 278431 - Parasitic)
(@CGUID+78, 131527, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -518.54864501953125, -268.505218505859375, 182.782318115234375, 2.667675733566284179, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Lord Waycrest (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+79, 137830, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -531.5643310546875, -299.840576171875, 236.1361083984375, 3.135821342468261718, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Pallid Gorger (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+80, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -505.18798828125, -266.95013427734375, 234.60888671875, 5.964896678924560546, 7200, 3, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+81, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -552.6024169921875, -243.805557250976562, 190.20855712890625, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+82, 135049, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -605.9149169921875, -152.583328247070312, 249.51531982421875, 4.44603586196899414, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dreadwing Raven (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+83, 135048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -536.22430419921875, -313.56982421875, 236.454498291015625, 4.733040809631347656, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Gorestained Piglet (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+84, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -504.6319580078125, -252.536453247070312, 234.6051025390625, 6.14012002944946289, 7200, 3, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+85, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -521.75, -281.420135498046875, 186.580352783203125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+86, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -530.3836669921875, -332.695556640625, 236.1361083984375, 0.076816625893115997, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+87, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -525.72393798828125, -242.579864501953125, 184.405548095703125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+88, 131666, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -504.0142822265625, -260.8154296875, 234.60321044921875, 0.956529498100280761, 7200, 3, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Coven Thornshaper (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+89, 135139, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -537.02606201171875, -300.421875, 236.111572265625, 3.126610755920410156, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dead Villager (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 265448 - Permanent Feign Death On Spawn)
(@CGUID+90, 134303, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -450.630218505859375, -321.42535400390625, 236.111572265625, 4.163387775421142578, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Summoning Portal (Area: Waycrest Manor - Difficulty: Mythic) CreateObject2 (Auras: 266070 - Cosmetic Visual)
(@CGUID+91, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -502.5791015625, -296.699798583984375, 236.1361083984375, 3.151485204696655273, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%, 278431 - Parasitic)
(@CGUID+92, 135048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -514.404541015625, -319.395843505859375, 236.782196044921875, 5.541881561279296875, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Gorestained Piglet (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+93, 135234, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -575.22222900390625, -357.9913330078125, 235.7375946044921875, 5.594793319702148437, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Diseased Mastiff (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%, 265630 - Mastiff Disease)
(@CGUID+94, 135315, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -419.046875, -249.536453247070312, 246.3013763427734375, 4.576903820037841796, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Goliath Drain Target Female (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+95, 135714, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -523.95831298828125, -285.364593505859375, 227.90399169921875, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Vinegared Wine (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+96, 131821, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -528.576416015625, -320.9913330078125, 249.5301361083984375, 4.115458011627197265, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Faceless Maiden (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+97, 132209, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -385.607635498046875, -257.765625, 236.37640380859375, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Lightning Controller (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 260564 - Wildfire)
(@CGUID+98, 135713, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -511.4913330078125, -257.510406494140625, 236.73333740234375, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Vile Pod (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+99, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -497.547943115234375, -262.49212646484375, 234.6028900146484375, 0.137453705072402954, 7200, 3, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+100, 131585, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -519.2725830078125, -192.927078247070312, 236.1165008544921875, 6.031474113464355468, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Enthralled Guard (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+101, 132361, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -574.69097900390625, -151.559036254882812, 235.2676239013671875, 4.716972827911376953, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Focusing Iris (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 271079 - Cosmetic Focusing Iris Look)
(@CGUID+102, 131849, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -572.74481201171875, -359.986114501953125, 235.7375946044921875, 1.961302042007446289, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Crazed Marksman (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+103, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -498.031524658203125, -254.4959716796875, 234.6052703857421875, 5.341821670532226562, 7200, 3, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+104, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -521.6878662109375, -302.930908203125, 236.1361083984375, 3.723121166229248046, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+105, 135052, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -541.17706298828125, -150.880203247070312, 235.6681060791015625, 3.720450878143310546, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Blight Toad (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+106, 137830, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -515.37847900390625, -341.348968505859375, 236.79071044921875, 0.514622867107391357, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Pallid Gorger (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+107, 131667, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -415.598968505859375, -259.25347900390625, 236.1174774169921875, 3.168833017349243164, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Soulbound Goliath (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 261580 - Soul Harvest)
(@CGUID+108, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -495.857421875, -296.0693359375, 236.1361083984375, 2.022163867950439453, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+109, 135513, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -528.57989501953125, -188.428817749023437, 237.617919921875, 5.5980987548828125, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Ancient Tome (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+110, 134303, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -496.454864501953125, -198.105911254882812, 244.4434356689453125, 4.163387775421142578, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Summoning Portal (Area: Waycrest Manor - Difficulty: Mythic) CreateObject2 (Auras: 266070 - Cosmetic Visual)
(@CGUID+111, 135049, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -544.39239501953125, -158.20660400390625, 249.51531982421875, 5.187650203704833984, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dreadwing Raven (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+112, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -515.62677001953125, -275.661468505859375, 185.683502197265625, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+113, 135474, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -538.70831298828125, -155.852432250976562, 249.51531982421875, 3.939949512481689453, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+114, 135052, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -543.232666015625, -157.227432250976562, 235.671173095703125, 3.720450878143310546, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Blight Toad (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+115, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -539.381591796875, -366.20379638671875, 236.1360931396484375, 1.778770089149475097, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+116, 135474, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -540.842041015625, -154.08160400390625, 235.670135498046875, 0.101273626089096069, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thistle Acolyte (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 269151 - Cosmetic Witch Frog Usage)
(@CGUID+117, 135052, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -538.0382080078125, -155.892364501953125, 235.6739044189453125, 3.720450878143310546, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Blight Toad (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+118, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -499.5068359375, -299.24951171875, 236.1361083984375, 5.580564022064208984, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+119, 131685, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -542.61456298828125, -153.428817749023437, 249.515289306640625, 5.528810501098632812, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+120, 116633, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -514.8194580078125, -242.359375, 186.4376983642578125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Effect (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+121, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -532.5440673828125, -359.704498291015625, 236.1360931396484375, 1.847705960273742675, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+122, 135052, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -544.64581298828125, -153.272567749023437, 235.6681060791015625, 3.720450878143310546, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Blight Toad (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+123, 131585, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -493.507659912109375, -198.357635498046875, 236.0748748779296875, 0.002160806441679596, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Enthralled Guard (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+124, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -475.446441650390625, -246.423629760742187, 234.624786376953125, 6.208818912506103515, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+125, 135049, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -515.8507080078125, -161.274307250976562, 251.8550872802734375, 4.810708522796630859, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dreadwing Raven (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+126, 135048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -482.848968505859375, -300.873260498046875, 236.1360931396484375, 3.488520145416259765, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Gorestained Piglet (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+127, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -475.3857421875, -271.826812744140625, 234.6963653564453125, 3.017324209213256835, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+128, 135725, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -525.19964599609375, -342.869781494140625, 249.51483154296875, 2.40065169334411621, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Cursed Loom (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+129, 137830, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -494.7882080078125, -320.798614501953125, 235.70989990234375, 1.637995362281799316, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Pallid Gorger (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+130, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -472.66571044921875, -247.138961791992187, 234.60443115234375, 0.041875664144754409, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+131, 131666, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -485.196197509765625, -259.9600830078125, 235.2390594482421875, 0.585030436515808105, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Coven Thornshaper (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+132, 135048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -506.633697509765625, -340.185760498046875, 235.70989990234375, 5.806537151336669921, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Gorestained Piglet (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+133, 135139, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -536.44964599609375, -365.18402099609375, 236.111785888671875, 1.055417180061340332, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dead Villager (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 265448 - Permanent Feign Death On Spawn)
(@CGUID+134, 131587, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -493.513885498046875, -195.357635498046875, 236.015960693359375, 0.002079832134768366, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Bewitched Captain (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+135, 131586, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -505.515625, -342.102447509765625, 236.109527587890625, 1.590731143951416015, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Banquet Steward (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259716 - Enthralled)
(@CGUID+136, 131847, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -491.071197509765625, -321.02777099609375, 235.70989990234375, 2.982226848602294921, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Waycrest Reveler (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+137, 134041, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -531.39581298828125, -366.779510498046875, 236.1121063232421875, 1.961832404136657714, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Infected Peasant (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 266008 - Infected, 272860 - Cosmetic Thin Man Begging)
(@CGUID+138, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -474.291015625, -268.838653564453125, 234.6369171142578125, 3.326071023941040039, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+139, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -474.708343505859375, -249.30029296875, 234.60333251953125, 0.031052092090249061, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+140, 131858, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -476.541656494140625, -260.0225830078125, 235.2390594482421875, 3.538356542587280273, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Thornguard (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+141, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -473.839263916015625, -269.412567138671875, 234.6248931884765625, 3.442658424377441406, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+142, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -532.39166259765625, -364.468902587890625, 236.1360931396484375, 2.194856882095336914, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%, 278431 - Parasitic)
(@CGUID+143, 135139, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -499.201385498046875, -298.083343505859375, 236.111572265625, 1.566159248352050781, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dead Villager (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 265448 - Permanent Feign Death On Spawn)
(@CGUID+144, 131847, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -510.770843505859375, -339.87847900390625, 235.70989990234375, 4.05202341079711914, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Waycrest Reveler (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+145, 131847, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -471.552093505859375, -318.08160400390625, 236.109527587890625, 4.134391307830810546, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Waycrest Reveler (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+146, 135048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -476.517364501953125, -338.3507080078125, 235.70989990234375, 0.178709819912910461, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Gorestained Piglet (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+147, 131586, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -477.74322509765625, -334.44073486328125, 235.771881103515625, 0.293698012828826904, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Banquet Steward (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259716 - Enthralled)
(@CGUID+148, 131863, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -494.142364501953125, -345.82464599609375, 236.595184326171875, 1.550201058387756347, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Raal the Gluttonous (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+149, 135139, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -469.614593505859375, -300.338531494140625, 236.111572265625, 0.939325690269470214, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dead Villager (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 265448 - Permanent Feign Death On Spawn)
(@CGUID+150, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -472.357421875, -301.5068359375, 236.1360931396484375, 1.392820477485656738, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+151, 131847, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -475.03472900390625, -319.84375, 235.70989990234375, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Waycrest Reveler (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+152, 135048, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -474.692718505859375, -337.05035400390625, 235.70989990234375, 4.976558685302734375, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Gorestained Piglet (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+153, 131685, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -483.97222900390625, -190.142364501953125, 236.116241455078125, 1.025903940200805664, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Runic Disciple (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+154, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -470.95074462890625, -297.82073974609375, 236.136077880859375, 0.168031617999076843, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%, 278431 - Parasitic)
(@CGUID+155, 131847, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -471.0694580078125, -322.1007080078125, 235.70989990234375, 2.284619331359863281, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Waycrest Reveler (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+156, 131585, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -480.630218505859375, -189.435760498046875, 236.116241455078125, 3.543432235717773437, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Enthralled Guard (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+157, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -471.4898681640625, -267.6510009765625, 234.6194610595703125, 3.551302433013916015, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+158, 131666, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -450.4132080078125, -252.220489501953125, 234.6866607666015625, 3.658944606781005859, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Coven Thornshaper (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+159, 134041, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -467.48089599609375, -296.939239501953125, 236.111572265625, 3.697540283203125, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Infected Peasant (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 266008 - Infected, 272860 - Cosmetic Thin Man Begging)
(@CGUID+160, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -470.614532470703125, -296.327362060546875, 236.229736328125, 6.108142375946044921, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+161, 131585, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -483.17535400390625, -186.692703247070312, 236.116241455078125, 4.560091972351074218, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Enthralled Guard (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+162, 131666, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -450.98089599609375, -268.171875, 234.6866607666015625, 3.485479116439819335, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Coven Thornshaper (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+163, 135049, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -491.625, -161.680557250976562, 251.8558502197265625, 4.839976787567138671, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dreadwing Raven (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+164, 131586, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 1, -469.53125, -336.993072509765625, 236.109527587890625, 3.089185953140258789, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Banquet Steward (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259716 - Enthralled)
(@CGUID+165, 137830, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -473.625, -341.3975830078125, 236.8321380615234375, 2.023208379745483398, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Pallid Gorger (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+166, 135168, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -447.484375, -315.555572509765625, 237.645751953125, 4.420731067657470703, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dead Pig (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 265448 - Permanent Feign Death On Spawn)
(@CGUID+167, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -449.234954833984375, -334.523529052734375, 236.1947174072265625, 5.522914409637451171, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+168, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -440.701019287109375, -318.724884033203125, 236.165771484375, 4.048826217651367187, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+169, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -444.356353759765625, -335.0155029296875, 236.1985321044921875, 0.335822492837905883, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%, 278431 - Parasitic)
(@CGUID+170, 133435, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -480.46527099609375, -344.767364501953125, 151.316070556640625, 2.421268224716186523, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Drust Slaver Summoning Portal (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@CGUID+171, 135312, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -427.479156494140625, -269.53125, 243.176300048828125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Goliath Drain Target Male (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+172, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -431.9912109375, -254.112518310546875, 234.039886474609375, 0.054383471608161926, 7200, 3, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+173, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -455.37811279296875, -316.740386962890625, 236.18548583984375, 1.064016580581665039, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+174, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -438.1947021484375, -260.94293212890625, 233.82867431640625, 0.691592931747436523, 7200, 3, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+175, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -447.049530029296875, -330.407012939453125, 236.1361083984375, 2.802553892135620117, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+176, 135312, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -443.28472900390625, -267.958343505859375, 244.61761474609375, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Goliath Drain Target Male (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+177, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -449.2747802734375, -321.50439453125, 236.1361083984375, 1.350999355316162109, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+178, 131669, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -433.811492919921875, -271.0914306640625, 233.82867431640625, 4.203943252563476562, 7200, 3, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Jagged Hound (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 214567 - Mod Scale 90-110%)
(@CGUID+179, 135329, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -432.625, -259.493072509765625, 236.1174774169921875, 0.548182964324951171, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Matron Bryndle (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 265777 - Goliath Drain Flavor)
(@CGUID+180, 135168, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -446.56597900390625, -323.93402099609375, 237.645751953125, 1.603698015213012695, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Dead Pig (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 265448 - Permanent Feign Death On Spawn)
(@CGUID+181, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -440.796356201171875, -316.28143310546875, 236.176971435546875, 0.170766040682792663, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+182, 135315, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -438.380218505859375, -250.489578247070312, 242.6348724365234375, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Goliath Drain Target Female (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+183, 135312, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -428.817718505859375, -248.19097900390625, 243.373504638671875, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Goliath Drain Target Male (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+184, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -452.085296630859375, -314.97174072265625, 236.271514892578125, 2.876992702484130859, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+185, 135313, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -434.223968505859375, -270.263885498046875, 240.76904296875, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Goliath Drain Target Heavy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259716 - Enthralled)
(@CGUID+186, 134024, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -441.495208740234375, -320.460357666015625, 236.2048492431640625, 4.721146583557128906, 7200, 2, 0, 100, 1, NULL, NULL, NULL, NULL, 55165,-1), -- Devouring Maggot (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 202427 - Mod Scale 150%)
(@CGUID+187, 135312, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -433.90972900390625, -245.723953247070312, 241.0567169189453125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Goliath Drain Target Male (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+188, 137774, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -444.395843505859375, -331.390625, 180.5082244873046875, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Portal to Basement (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 271040 - Teleport to Basement Area Trigger)
(@CGUID+189, 135315, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -418.642364501953125, -269.967010498046875, 244.74169921875, 4.576903820037841796, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1), -- Goliath Drain Target Female (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259712 - Enthralled)
(@CGUID+190, 135313, 1862, 9424, 9424, '1,23,2,8', 0, 0, 0, 0, -415.26214599609375, -266.62152099609375, 244.74169921875, 4.576903820037841796, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55165,-1); -- Goliath Drain Target Heavy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1 (Auras: 259716 - Enthralled)
DELETE FROM `creature_addon` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+190;
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(@CGUID+48, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '202427 278431'), -- Devouring Maggot - 202427 - Mod Scale 150%, 278431 - Parasitic
(@CGUID+59, 0, 0, 0, 0, 0, 1, 0, 0, 15336, 0, 0, 0, ''), -- Maddened Survivalist
(@CGUID+75, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '202427 278431'), -- Devouring Maggot - 202427 - Mod Scale 150%, 278431 - Parasitic
(@CGUID+77, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '202427 278431'), -- Devouring Maggot - 202427 - Mod Scale 150%, 278431 - Parasitic
(@CGUID+82, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '214567'), -- Dreadwing Raven - 214567 - Mod Scale 90-110%
(@CGUID+91, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '202427 278431'), -- Devouring Maggot - 202427 - Mod Scale 150%, 278431 - Parasitic
(@CGUID+102, 0, 0, 0, 0, 0, 1, 0, 0, 15336, 0, 0, 0, ''), -- Crazed Marksman
(@CGUID+106, 0, 0, 0, 0, 0, 1, 0, 418, 0, 0, 0, 0, '214567'), -- Pallid Gorger - 214567 - Mod Scale 90-110%
(@CGUID+111, 0, 0, 0, 3, 0, 1, 0, 0, 1700, 0, 0, 0, '214567'), -- Dreadwing Raven - 214567 - Mod Scale 90-110%
(@CGUID+125, 0, 0, 0, 3, 0, 1, 0, 0, 1700, 0, 0, 0, '214567'), -- Dreadwing Raven - 214567 - Mod Scale 90-110%
(@CGUID+129, 0, 0, 0, 0, 0, 1, 0, 418, 0, 0, 0, 0, '214567'), -- Pallid Gorger - 214567 - Mod Scale 90-110%
(@CGUID+132, 0, 0, 0, 0, 0, 1, 0, 720, 0, 0, 0, 0, '214567'), -- Gorestained Piglet - 214567 - Mod Scale 90-110%
(@CGUID+135, 0, 0, 0, 0, 0, 1, 0, 720, 0, 0, 0, 0, '259716'), -- Banquet Steward - 259716 - Enthralled
(@CGUID+136, 0, 0, 0, 0, 0, 1, 0, 64, 0, 0, 0, 0, '259712'), -- Waycrest Reveler - 259712 - Enthralled
(@CGUID+142, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '202427 278431'), -- Devouring Maggot - 202427 - Mod Scale 150%, 278431 - Parasitic
(@CGUID+144, 0, 0, 0, 0, 0, 1, 0, 64, 0, 0, 0, 0, '259712'), -- Waycrest Reveler - 259712 - Enthralled
(@CGUID+145, 0, 0, 0, 0, 0, 1, 0, 400, 0, 0, 0, 0, '259712'), -- Waycrest Reveler - 259712 - Enthralled
(@CGUID+146, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '214567'), -- Gorestained Piglet - 214567 - Mod Scale 90-110%
(@CGUID+147, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '259716'), -- Banquet Steward - 259716 - Enthralled
(@CGUID+151, 0, 0, 0, 0, 0, 1, 0, 400, 0, 0, 0, 0, '259712'), -- Waycrest Reveler - 259712 - Enthralled
(@CGUID+152, 0, 0, 0, 0, 0, 1, 0, 418, 0, 0, 0, 0, '214567'), -- Gorestained Piglet - 214567 - Mod Scale 90-110%
(@CGUID+154, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '202427 278431'), -- Devouring Maggot - 202427 - Mod Scale 150%, 278431 - Parasitic
(@CGUID+155, 0, 0, 0, 0, 0, 1, 0, 400, 0, 0, 0, 0, '259712'), -- Waycrest Reveler - 259712 - Enthralled
(@CGUID+163, 0, 0, 0, 3, 0, 1, 0, 0, 1700, 0, 0, 0, '214567'), -- Dreadwing Raven - 214567 - Mod Scale 90-110%
(@CGUID+164, 0, 0, 0, 0, 0, 1, 0, 720, 0, 0, 0, 0, '259716'); -- Banquet Steward - 259716 - Enthralled

-- GameObjects
DELETE FROM `gameobject` WHERE `guid` BETWEEN @OGUID+0 AND @OGUID+25;
INSERT INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `VerifiedBuild`, `size`, `visibility`) VALUES
(@OGUID+0, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -454.484375, -299.807281494140625, 241.1402130126953125, 0.42129296064376831, 0, 0, 0.209092140197753906, 0.977895915508270263, 7200, 255, 1, 55165, 1, 100), -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+1, 282409, 1862, 9424, 9424, '1,23,2,8', 0, 0, -580.433349609375, -296.19244384765625, 235.97235107421875, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+2, 282408, 1862, 9424, 9424, '1,23,2,8', 0, 0, -580.4478759765625, -223.058517456054687, 236.01092529296875, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+3, 282767, 1862, 9424, 9424, '1,23,2,8', 0, 0, -588.95867919921875, -223.062652587890625, 249.4219207763671875, 4.712389945983886718, 0, 0, -0.70710659027099609, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+4, 282766, 1862, 9424, 9424, '1,23,2,8', 0, 0, -559.332275390625, -296.226593017578125, 249.4219207763671875, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+5, 282765, 1862, 9424, 9424, '1,23,2,8', 0, 0, -588.8980712890625, -296.226593017578125, 249.4219207763671875, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+6, 282768, 1862, 9424, 9424, '1,23,2,8', 0, 0, -559.3995361328125, -223.062652587890625, 249.4219207763671875, 4.712389945983886718, 0, 0, -0.70710659027099609, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+7, 282411, 1862, 9424, 9424, '1,23,2,8', 0, 0, -523.65533447265625, -233.675750732421875, 235.97235107421875, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+8, 282410, 1862, 9424, 9424, '1,23,2,8', 0, 0, -574.8778076171875, -183.705795288085937, 236.0064544677734375, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+9, 297809, 1862, 9424, 9424, '1,23,2,8', 0, 0, -544.79522705078125, -259.010162353515625, 182.44378662109375, 6.278062343597412109, 0, 0, -0.00256156921386718, 0.999996721744537353, 7200, 255, 1, 55165, 1, 100), -- Doodad_8FX_Drustvar_Witch_Waycrest_Organ_Barrier001 (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+10, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -584.34893798828125, -323.5225830078125, 253.1695404052734375, 6.24246072769165039, 0, 0, -0.02036094665527343, 0.999792695045471191, 7200, 255, 1, 55165, 1, 100), -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+11, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -555.171875, -218.791671752929687, 255.1493072509765625, 4.795436382293701171, 0, 0, -0.67714405059814453, 0.735850453376770019, 7200, 255, 1, 55165, 1, 100), -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+12, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -564.36981201171875, -323.7257080078125, 255.6175994873046875, 6.24246072769165039, 0, 0, -0.02036094665527343, 0.999792695045471191, 7200, 255, 1, 55165, 1, 100), -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+13, 282413, 1862, 9424, 9424, '1,23,2,8', 0, 0, -526.0927734375, -330.263916015625, 235.97235107421875, 3.141592741012573242, 0, 0, -1, 0, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+14, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -555.171875, -203.828125, 255.1493072509765625, 4.795436382293701171, 0, 0, -0.67714405059814453, 0.735850453376770019, 7200, 255, 1, 55165, 1, 100), -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+15, 282769, 1862, 9424, 9424, '1,23,2,8', 0, 0, -505.8912353515625, -309.13470458984375, 235.97235107421875, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+16, 290130, 1862, 9424, 9424, '1,23,2,8', 0, 0, -489.90728759765625, -208.030914306640625, 236.02386474609375, 0, 0, 0, 0, 1, 7200, 255, 1, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+17, 282412, 1862, 9424, 9424, '1,23,2,8', 0, 0, -484.003570556640625, -233.936126708984375, 235.97235107421875, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+18, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -523.58856201171875, -324.1944580078125, 255.366455078125, 4.575895309448242187, 0, 0, -0.75368118286132812, 0.657240211963653564, 7200, 255, 1, 55165, 1, 100), -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+19, 290702, 1862, 9424, 9424, '1,23,2,8', 0, 0, -481.664794921875, -287.277587890625, 235.97235107421875, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+20, 282770, 1862, 9424, 9424, '1,23,2,8', 0, 0, -481.628814697265625, -309.13470458984375, 235.97235107421875, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 7200, 255, 0, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+21, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -523.58856201171875, -336.135406494140625, 255.366455078125, 4.575895309448242187, 0, 0, -0.75368118286132812, 0.657240211963653564, 7200, 255, 1, 55165, 1, 100), -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+22, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -513.76043701171875, -183.970489501953125, 240.9019622802734375, 0.520980000495910644, 0, 0, 0.257554054260253906, 0.966263890266418457, 7200, 255, 1, 55165, 1, 100), -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+23, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -506.739593505859375, -179.840286254882812, 241.28717041015625, 0.325213253498077392, 0, 0, 0.161890983581542968, 0.986808657646179199, 7200, 255, 1, 55165, 1, 100), -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+24, 290131, 1862, 9424, 9424, '1,23,2,8', 0, 0, -464.10552978515625, -300.783203125, 226.5698089599609375, 0, 0, 0, 0, 1, 7200, 255, 1, 55165, 1, 100), -- Door (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
(@OGUID+25, 289699, 1862, 9424, 9424, '1,23,2,8', 0, 0, -502.682281494140625, -179.553817749023437, 241.28717041015625, 6.167336940765380859, 0, 0, -0.057891845703125, 0.998322844505310058, 7200, 255, 1, 55165, 1, 100); -- Drustvar Dungeon - Organ Energy (Area: Waycrest Manor - Difficulty: Mythic) CreateObject1
DELETE FROM `gameobject_addon` WHERE `guid` BETWEEN @OGUID+0 AND @OGUID+26;
INSERT INTO `gameobject_addon` (`guid`, `parent_rotation0`, `parent_rotation1`, `parent_rotation2`, `parent_rotation3`, `WorldEffectID`, `AIAnimKitID`) VALUES
(@OGUID+1, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+2, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+3, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+4, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+5, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+6, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+7, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+8, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+9, 0, 0, 1, -0.00000004371138828, 0, 0), -- Doodad_8FX_Drustvar_Witch_Waycrest_Organ_Barrier001
(@OGUID+13, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+15, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+16, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+17, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+19, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+20, 0, 0, 1, -0.00000004371138828, 0, 0), -- Door
(@OGUID+24, 0, 0, 1, -0.00000004371138828, 0, 0); -- Door

-- Template
UPDATE `creature_template` SET `unit_flags2`=0 WHERE `entry`=131667; -- Soulbound Goliath
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=137774; -- Portal to Basement
UPDATE `creature_template` SET `unit_flags`=33554688, `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=135168; -- Dead Pig
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=131595; -- Governess Tabitha
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=131863; -- Raal the Gluttonous
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=131615; -- Head Vintner Justin
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=131617; -- Groundskeeper Lilith
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=131619; -- Seamstress Johanna
UPDATE `creature_template` SET `unit_flags3`=4194304 WHERE `entry`=132361; -- Focusing Iris
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=135049; -- Dreadwing Raven
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=131527; -- Lord Waycrest
UPDATE `creature_template` SET `unit_flags2`=0 WHERE `entry`=131824; -- Sister Solena
UPDATE `creature_template` SET `unit_flags2`=0 WHERE `entry`=131825; -- Sister Briar
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry` IN (131545, 135357); -- Lady Waycrest
UPDATE `creature_template` SET `unit_flags2`=2048 WHERE `entry`=131853; -- Huntmaster Grath
UPDATE `creature_template` SET `unit_flags2`=0 WHERE `entry`=131823; -- Sister Malady
UPDATE `creature_template` SET `unit_flags`=33554688, `unit_flags2`=2048, `unit_flags3`=0 WHERE `entry`=135139; -- Dead Villager
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=135234; -- Diseased Mastiff
UPDATE `creature_template` SET `unit_flags3`=67108864 WHERE `entry`=135052; -- Blight Toad
UPDATE `creature_template` SET `unit_flags`=0 WHERE `entry`=137048; -- Beguiled Villager
UPDATE `creature_template` SET `unit_flags`=33554752 WHERE `entry`=135360; -- Sister Briar
UPDATE `creature_template` SET `unit_flags`=33554752 WHERE `entry`=135358; -- Sister Malady
UPDATE `creature_template` SET `unit_flags`=33554752 WHERE `entry`=135359; -- Sister Solena
UPDATE `creature_template` SET `unit_flags`=33554752 WHERE `entry`=137033; -- Runic Disciple
UPDATE `creature_template` SET `unit_flags`=33554752 WHERE `entry`=137034; -- Thistle Acolyte
UPDATE `creature_template` SET `flags_extra` = 128 WHERE `entry` = 132209;

-- Template Addon
DELETE FROM `creature_template_addon` WHERE `entry` IN (134303, 131667, 132209, 137774, 135313, 135315, 135329, 135312, 135168, 131863, 131847, 131586, 135513, 131849, 132361, 135713, 131669, 131527, 135048, 131824, 131825, 131545, 131823, 135139, 137830, 135234, 134024, 134041, 131587, 137048, 131585);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(134303, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '266070'), -- 134303 (Summoning Portal) - Cosmetic Visual
(131667, 0, 0, 0, 0, 0, 1, 0, 0, 13946, 0, 0, 3, '261580'), -- 131667 (Soulbound Goliath) - Soul Harvest
(132209, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '260564'), -- 132209 (Lightning Controller) - Wildfire
(137774, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '271040'), -- 137774 (Portal to Basement) - Teleport to Basement Area Trigger
(135313, 0, 0, 0, 0, 0, 1, 0, 0, 6270, 0, 0, 0, '259716'), -- 135313 (Goliath Drain Target Heavy) - Enthralled
(135315, 0, 0, 0, 3, 0, 1, 0, 0, 6270, 0, 0, 0, '259712'), -- 135315 (Goliath Drain Target Female) - Enthralled
(135329, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '265777'), -- 135329 (Matron Bryndle) - Goliath Drain Flavor
(135312, 0, 0, 0, 3, 0, 1, 0, 0, 6270, 0, 0, 0, '259712'), -- 135312 (Goliath Drain Target Male) - Enthralled
(135168, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '265448'), -- 135168 (Dead Pig) - Permanent Feign Death On Spawn
(131863, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 131863 (Raal the Gluttonous)
(131847, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '259712'), -- 131847 (Waycrest Reveler) - Enthralled
(131586, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '259716'), -- 131586 (Banquet Steward) - Enthralled
(135513, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 135513 (Ancient Tome)
(131849, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 131849 (Crazed Marksman)
(132361, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '271079'), -- 132361 (Focusing Iris) - Cosmetic Focusing Iris Look
(135713, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 135713 (Vile Pod)
(131669, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '214567'), -- 131669 (Jagged Hound) - Mod Scale 90-110%
(131527, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 131527 (Lord Waycrest)
(135048, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '214567'), -- 135048 (Gorestained Piglet) - Mod Scale 90-110%
(131824, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '272626'), -- 131824 (Sister Solena) - Override Energy Bar Color
(131825, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '272627 261265'), -- 131825 (Sister Briar) - Override Energy Bar Color, Ironbark Shield
(131545, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 131545 (Lady Waycrest)
(131823, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '272625 261266'), -- 131823 (Sister Malady) - Override Energy Bar Color, Runic Ward
(135139, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '265448'), -- 135139 (Dead Villager) - Permanent Feign Death On Spawn
(137830, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '214567'), -- 137830 (Pallid Gorger) - Mod Scale 90-110%
(135234, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '214567 265630'), -- 135234 (Diseased Mastiff) - Mod Scale 90-110%, Mastiff Disease
(134024, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '202427'), -- 134024 (Devouring Maggot) - Mod Scale 150%
(134041, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '266008 272860'), -- 134041 (Infected Peasant) - Infected, Cosmetic Thin Man Begging
(131587, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '259712'), -- 131587 (Bewitched Captain) - Enthralled
(137048, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '269121'), -- 137048 (Beguiled Villager) - Cosmetic Beguiled
(131585, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '259712'); -- 131585 (Enthralled Guard) - Enthralled

-- ScriptNames
DELETE FROM `spell_script_names` WHERE `spell_id` = 265448;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(265448, 'spell_gen_feign_death_all_flags');

-- Difficulty (Normal)
DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=1 AND `Entry` IN (144324,138761,135552,135556,135555,135553,139269,138762,131864,137806,135255,136302,135365,135341,131819,136330,134303,136094,136087,136086,136085,136084,137774,135315,135312,135168,132209,131667,135329,135313,133435,131863,131595,131847,131586,135725,135474,131615,131619,135513,131617,132361,131812,131824,135713,131823,131545,137830,136161,135714,135139,135049,131853,131825,131821,131527,135048,131669,134041,134024,135234,131849,131666,131858,131677,131850,135711,131587,135357,137048,218647,218646,218977,131585,131685,135360,135358,135359,137034,135240,137033));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `HealthScalingExpansion`, `HealthModifier`, `ManaModifier`, `CreatureDifficultyID`, `TypeFlags`, `TypeFlags2`) VALUES
(144324, 1, 7, 1, 1, 157025, 0, 0), -- Gorak Tul
(138761, 1, 7, 3, 1, 145524, 0, 0), -- Lucille Waycrest
(135552, 1, 7, 2, 1, 141758, 2101320, 128), -- Deathtouched Slaver
(135556, 1, 7, 3, 1, 141762, 4096, 0), -- Inquisitor Mace
(135555, 1, 7, 3, 1, 141761, 4096, 0), -- Inquisitor Sterntide
(135553, 1, 7, 3, 1, 141759, 4096, 0), -- Inquisitor Yorrick
(139269, 1, 7, 5, 1, 146062, 2097224, 0), -- Gloom Horror
(138762, 1, 7, 3, 1, 145525, 0, 0), -- Lucille Waycrest
(131864, 1, 7, 32, 1, 137784, 2097256, 128), -- Gorak Tul
(137806, 1, 7, 1, 1, 144353, 0, 0), -- Organ to Basement Traveler
(135255, 1, 7, 1, 1, 141404, 0, 0), -- Drain Target
(136302, 1, 7, 1, 1, 142596, 0, 0), -- Dark Etching Target
(135365, 1, 7, 12, 1, 141545, 2097224, 0), -- Matron Alma
(135341, 1, 7, 1, 1, 141509, 0, 0), -- Captured Villager
(131819, 1, 7, 4.5, 1, 137739, 2097224, 0), -- Coven Diviner
(136330, 1, 7, 1, 1, 142627, 0, 128), -- Soul Thorns
(134303, 1, 7, 1, 1, 140361, 0, 0), -- Summoning Portal
(136094, 1, 7, 3, 1, 142352, 0, 0), -- Inquisitor Notley
(136087, 1, 7, 3, 1, 142343, 0, 0), -- Inquisitor Sterntide
(136086, 1, 7, 3, 1, 142342, 0, 0), -- Inquisitor Mace
(136085, 1, 7, 3, 1, 142341, 0, 0), -- Inquisitor Yorrick
(136084, 1, 7, 3, 1, 142340, 0, 0), -- Lucille Waycrest
(137774, 1, 7, 1, 1, 144319, 1073741824, 6), -- Portal to Basement
(135315, 1, 7, 1, 1, 141476, 0, 0), -- Goliath Drain Target Female
(135312, 1, 7, 1, 1, 141473, 0, 0), -- Goliath Drain Target Male
(135168, 1, 7, 1, 1, 141306, 0, 0), -- Dead Pig
(132209, 1, 7, 1, 1, 138133, 0, 0), -- Lightning Controller
(131667, 1, 7, 35, 1, 137585, 2097256, 128), -- Soulbound Goliath
(135329, 1, 7, 12, 1, 141495, 2097224, 0), -- Matron Bryndle
(135313, 1, 7, 5, 1, 141474, 0, 0), -- Goliath Drain Target Heavy
(133435, 1, 7, 1, 1, 139464, 0, 0), -- Drust Slaver Summoning Portal
(131863, 1, 7, 34, 1, 137783, 2097256, 128), -- Raal the Gluttonous
(131595, 1, 7, 15, 1, 137513, 2097224, 0), -- Governess Tabitha
(131847, 1, 7, 3, 1, 137767, 2097224, 0), -- Waycrest Reveler
(131586, 1, 7, 5, 1, 137504, 2097224, 0), -- Banquet Steward
(135725, 1, 7, 1, 1, 141954, 1073741824, 6), -- Cursed Loom
(135474, 1, 7, 5, 1, 141669, 2097224, 0), -- Thistle Acolyte
(131615, 1, 7, 15, 1, 137533, 2097224, 0), -- Head Vintner Justin
(131619, 1, 7, 15, 1, 137537, 2097224, 0), -- Seamstress Johanna
(135513, 1, 7, 1, 1, 141715, 1073741824, 6), -- Ancient Tome
(131617, 1, 7, 12, 1, 137535, 2097224, 0), -- Groundskeeper Lilith
(132361, 1, 7, 1, 1, 138291, 0, 0), -- Focusing Iris
(131812, 1, 7, 8, 1, 137732, 2097224, 0), -- Heartsbane Soulcharmer
(131824, 1, 7, 9.5, 1, 137744, 2097256, 128), -- Sister Solena
(135713, 1, 7, 1, 1, 141942, 1073741824, 6), -- Vile Pod
(131823, 1, 7, 9.5, 1, 137743, 2097256, 128), -- Sister Malady
(131545, 1, 7, 100, 1, 137411, 2097256, 128), -- Lady Waycrest
(137830, 1, 7, 5, 1, 144377, 2097224, 0), -- Pallid Gorger
(136161, 1, 7, 32, 1, 142425, 2097224, 0), -- Gorak Tul
(135714, 1, 7, 1, 1, 141943, 1073741824, 6), -- Vinegared Wine
(135139, 1, 7, 1, 1, 141277, 0, 0), -- Dead Villager
(135049, 1, 7, 3, 1, 141177, 2097225, 0), -- Dreadwing Raven
(131853, 1, 7, 15, 1, 137773, 2097224, 0), -- Huntmaster Grath
(131825, 1, 7, 9.5, 1, 137745, 2097256, 128), -- Sister Briar
(131821, 1, 7, 4, 1, 137741, 2097224, 0), -- Faceless Maiden
(131527, 1, 7, 9, 1, 137391, 2097256, 128), -- Lord Waycrest
(135048, 1, 7, 2.5, 1, 141174, 2097225, 0), -- Gorestained Piglet
(131669, 1, 7, 1, 1, 137587, 2097224, 0), -- Jagged Hound
(134041, 1, 7, 5, 1, 140091, 2097224, 0), -- Infected Peasant
(134024, 1, 7, 1, 1, 140072, 2097224, 0), -- Devouring Maggot
(135234, 1, 7, 3, 1, 141375, 2097225, 0), -- Diseased Mastiff
(131849, 1, 7, 5, 1, 137769, 2097224, 0), -- Crazed Marksman
(131666, 1, 7, 5, 1, 137584, 2097224, 0), -- Coven Thornshaper
(131858, 1, 7, 5, 1, 137778, 2097224, 0), -- Thornguard
(131677, 1, 7, 8, 1, 137595, 2097224, 0), -- Heartsbane Runeweaver
(131850, 1, 7, 5, 1, 137770, 2097224, 0), -- Maddened Survivalist
(135711, 1, 7, 1, 1, 141940, 1073741824, 6), -- Hunting Map
(131587, 1, 7, 8, 1, 137505, 2097224, 0), -- Bewitched Captain
(135357, 1, 7, 100, 1, 141529, 2097224, 0), -- Lady Waycrest
(137048, 1, 7, 3, 1, 143491, 0, 0), -- Beguiled Villager
(131585, 1, 7, 5, 1, 137503, 2097224, 0), -- Enthralled Guard
(131685, 1, 7, 5, 1, 137603, 2097224, 0), -- Runic Disciple
(135360, 1, 7, 12, 1, 141538, 2097224, 0), -- Sister Briar
(135358, 1, 7, 12, 1, 141532, 2097224, 0), -- Sister Malady
(135359, 1, 7, 12, 1, 141535, 2097224, 0), -- Sister Solena
(137034, 1, 7, 5, 1, 143475, 2097224, 0), -- Thistle Acolyte
(135240, 1, 7, 3, 1, 141383, 2097224, 0), -- Soul Essence
(137033, 1, 7, 5, 1, 143472, 2097224, 0); -- Runic Disciple

UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `VerifiedBuild`=55165 WHERE (`Entry`=144324 AND `DifficultyID`=1); -- 144324 (Gorak Tul) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=138761 AND `DifficultyID`=1); -- 138761 (Lucille Waycrest) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135552 AND `DifficultyID`=1); -- 135552 (Deathtouched Slaver) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135556 AND `DifficultyID`=1); -- 135556 (Inquisitor Mace) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135553 AND `DifficultyID`=1); -- 135553 (Inquisitor Yorrick) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135555 AND `DifficultyID`=1); -- 135555 (Inquisitor Sterntide) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=139269 AND `DifficultyID`=1); -- 139269 (Gloom Horror) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=138762 AND `DifficultyID`=1); -- 138762 (Lucille Waycrest) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131864 AND `DifficultyID`=1); -- 131864 (Gorak Tul) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536870912, `VerifiedBuild`=55165 WHERE (`Entry`=137806 AND `DifficultyID`=1); -- 137806 (Organ to Basement Traveler) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=135255 AND `DifficultyID`=1); -- 135255 (Drain Target) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536871168, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=136302 AND `DifficultyID`=1); -- 136302 (Dark Etching Target) - Sessile, Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131819 AND `DifficultyID`=1); -- 131819 (Coven Diviner) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `VerifiedBuild`=55165 WHERE (`Entry`=135341 AND `DifficultyID`=1); -- 135341 (Captured Villager) - 
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135365 AND `DifficultyID`=1); -- 135365 (Matron Alma) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536871168, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=136330 AND `DifficultyID`=1); -- 136330 (Soul Thorns) - Sessile, Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536870912, `VerifiedBuild`=55165 WHERE (`Entry`=134303 AND `DifficultyID`=1); -- 134303 (Summoning Portal) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136094 AND `DifficultyID`=1); -- 136094 (Inquisitor Notley) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136084 AND `DifficultyID`=1); -- 136084 (Lucille Waycrest) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136085 AND `DifficultyID`=1); -- 136085 (Inquisitor Yorrick) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136086 AND `DifficultyID`=1); -- 136086 (Inquisitor Mace) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136087 AND `DifficultyID`=1); -- 136087 (Inquisitor Sterntide) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131667 AND `DifficultyID`=1); -- 131667 (Soulbound Goliath) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=132209 AND `DifficultyID`=1); -- 132209 (Lightning Controller) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=135312 AND `DifficultyID`=1); -- 135312 (Goliath Drain Target Male) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=135315 AND `DifficultyID`=1); -- 135315 (Goliath Drain Target Female) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=137774 AND `DifficultyID`=1); -- 137774 (Portal to Basement) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=256, `VerifiedBuild`=55165 WHERE (`Entry`=135168 AND `DifficultyID`=1); -- 135168 (Dead Pig) - Sessile
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135329 AND `DifficultyID`=1); -- 135329 (Matron Bryndle) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=133435 AND `DifficultyID`=1); -- 133435 (Drust Slaver Summoning Portal) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=135313 AND `DifficultyID`=1); -- 135313 (Goliath Drain Target Heavy) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131595 AND `DifficultyID`=1); -- 131595 (Governess Tabitha) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131863 AND `DifficultyID`=1); -- 131863 (Raal the Gluttonous) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131586 AND `DifficultyID`=1); -- 131586 (Banquet Steward) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131847 AND `DifficultyID`=1); -- 131847 (Waycrest Reveler) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135725 AND `DifficultyID`=1); -- 135725 (Cursed Loom) - CannotTurn
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131619 AND `DifficultyID`=1); -- 131619 (Seamstress Johanna) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131615 AND `DifficultyID`=1); -- 131615 (Head Vintner Justin) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135474 AND `DifficultyID`=1); -- 135474 (Thistle Acolyte) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131824 AND `DifficultyID`=1); -- 131824 (Sister Solena) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131617 AND `DifficultyID`=1); -- 131617 (Groundskeeper Lilith) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131545 AND `DifficultyID`=1); -- 131545 (Lady Waycrest) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536870912, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135513 AND `DifficultyID`=1); -- 135513 (Ancient Tome) - Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131823 AND `DifficultyID`=1); -- 131823 (Sister Malady) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131812 AND `DifficultyID`=1); -- 131812 (Heartsbane Soulcharmer) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `VerifiedBuild`=55165 WHERE (`Entry`=132361 AND `DifficultyID`=1); -- 132361 (Focusing Iris) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=536870912, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135713 AND `DifficultyID`=1); -- 135713 (Vile Pod) - Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135048 AND `DifficultyID`=1); -- 135048 (Gorestained Piglet) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131527 AND `DifficultyID`=1); -- 131527 (Lord Waycrest) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131825 AND `DifficultyID`=1); -- 131825 (Sister Briar) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131669 AND `DifficultyID`=1); -- 131669 (Jagged Hound) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=805306368, `VerifiedBuild`=55165 WHERE (`Entry`=135049 AND `DifficultyID`=1); -- 135049 (Dreadwing Raven) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131858 AND `DifficultyID`=1); -- 131858 (Thornguard) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131666 AND `DifficultyID`=1); -- 131666 (Coven Thornshaper) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131853 AND `DifficultyID`=1); -- 131853 (Huntmaster Grath) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131821 AND `DifficultyID`=1); -- 131821 (Faceless Maiden) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136161 AND `DifficultyID`=1); -- 136161 (Gorak Tul) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=134041 AND `DifficultyID`=1); -- 134041 (Infected Peasant) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=256, `VerifiedBuild`=55165 WHERE (`Entry`=135139 AND `DifficultyID`=1); -- 135139 (Dead Villager) - Sessile
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131849 AND `DifficultyID`=1); -- 131849 (Crazed Marksman) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135714 AND `DifficultyID`=1); -- 135714 (Vinegared Wine) - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131850 AND `DifficultyID`=1); -- 131850 (Maddened Survivalist) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131677 AND `DifficultyID`=1); -- 131677 (Heartsbane Runeweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=137830 AND `DifficultyID`=1); -- 137830 (Pallid Gorger) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135234 AND `DifficultyID`=1); -- 135234 (Diseased Mastiff) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=134024 AND `DifficultyID`=1); -- 134024 (Devouring Maggot) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=135357 AND `DifficultyID`=1); -- 135357 (Lady Waycrest) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131587 AND `DifficultyID`=1); -- 131587 (Bewitched Captain) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=256, `VerifiedBuild`=55165 WHERE (`Entry`=137048 AND `DifficultyID`=1); -- 137048 (Beguiled Villager) - Sessile
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135711 AND `DifficultyID`=1); -- 135711 (Hunting Map) - CannotTurn
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135360 AND `DifficultyID`=1); -- 135360 (Sister Briar) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135359 AND `DifficultyID`=1); -- 135359 (Sister Solena) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135358 AND `DifficultyID`=1); -- 135358 (Sister Malady) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131585 AND `DifficultyID`=1); -- 131585 (Enthralled Guard) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135240 AND `DifficultyID`=1); -- 135240 (Soul Essence) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=137033 AND `DifficultyID`=1); -- 137033 (Runic Disciple) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131685 AND `DifficultyID`=1); -- 131685 (Runic Disciple) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=137034 AND `DifficultyID`=1); -- 137034 (Thistle Acolyte) - CanSwim

-- Difficulty (Heroic)
DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=2 AND `Entry` IN (135552,139269,131864,135365,131819,136541,136330,131667,132209,135329,131863,131595,131586,131847,131615,137830,131619,131617,131812,131824,131823,131545,131527,131825,136161,135049,135048,131853,131821,135474,134041,131669,135234,135052,134024,131849,131666,131858,131677,131850,131587,135357,131685,131585,137048,135360,135358,135359,135240,137033,137034));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `HealthScalingExpansion`, `HealthModifier`, `ManaModifier`, `CreatureDifficultyID`, `TypeFlags`, `TypeFlags2`) VALUES
(135552, 2, 7, 2, 1, 141828, 2101320, 128), -- Deathtouched Slaver
(139269, 2, 7, 5, 1, 146072, 2097224, 0), -- Gloom Horror
(131864, 2, 7, 32, 1, 141826, 2097256, 128), -- Gorak Tul
(135365, 2, 7, 12, 1, 141546, 2097224, 0), -- Matron Alma
(131819, 2, 7, 4.5, 1, 138949, 2097224, 0), -- Coven Diviner
(136541, 2, 7, 0.384600013494491577, 1, 142860, 2097224, 128), -- Bile Oozeling
(136330, 2, 7, 1, 1, 142628, 0, 128), -- Soul Thorns
(131667, 2, 7, 35, 1, 138058, 2097256, 128), -- Soulbound Goliath
(132209, 2, 7, 1, 1, 148704, 0, 0), -- Lightning Controller
(135329, 2, 7, 12, 1, 141496, 2097224, 0), -- Matron Bryndle
(131863, 2, 7, 34, 1, 139377, 2097256, 128), -- Raal the Gluttonous
(131595, 2, 7, 15, 1, 138972, 2097224, 0), -- Governess Tabitha
(131586, 2, 7, 5, 1, 138935, 2097224, 0), -- Banquet Steward
(131847, 2, 7, 3, 1, 138945, 2097224, 0), -- Waycrest Reveler
(131615, 2, 7, 15, 1, 138964, 2097224, 0), -- Head Vintner Justin
(137830, 2, 7, 5, 1, 144378, 2097224, 0), -- Pallid Gorger
(131619, 2, 7, 15, 1, 138960, 2097224, 0), -- Seamstress Johanna
(131617, 2, 7, 12, 1, 138962, 2097224, 0), -- Groundskeeper Lilith
(131812, 2, 7, 8, 1, 138919, 2097224, 0), -- Heartsbane Soulcharmer
(131824, 2, 7, 9.5, 1, 138242, 2097256, 128), -- Sister Solena
(131823, 2, 7, 9.5, 1, 138244, 2097256, 128), -- Sister Malady
(131545, 2, 7, 100, 1, 137412, 2097256, 128), -- Lady Waycrest
(131527, 2, 7, 9, 1, 137392, 2097256, 128), -- Lord Waycrest
(131825, 2, 7, 9.5, 1, 138240, 2097256, 128), -- Sister Briar
(136161, 2, 7, 32, 1, 158828, 2097224, 0), -- Gorak Tul
(135049, 2, 7, 3, 1, 141182, 2097225, 0), -- Dreadwing Raven
(135048, 2, 7, 2.5, 1, 141175, 2097224, 0), -- Gorestained Piglet
(131853, 2, 7, 15, 1, 138958, 2097224, 0), -- Huntmaster Grath
(131821, 2, 7, 4, 1, 138947, 2097224, 0), -- Faceless Maiden
(135474, 2, 7, 5, 1, 141670, 2097224, 0), -- Thistle Acolyte
(134041, 2, 7, 5, 1, 141702, 2097224, 0), -- Infected Peasant
(131669, 2, 7, 1, 1, 138927, 2097224, 0), -- Jagged Hound
(135234, 2, 7, 3, 1, 141376, 2097225, 0), -- Diseased Mastiff
(135052, 2, 7, 1, 1, 141185, 2097225, 0), -- Blight Toad
(134024, 2, 7, 1, 1, 140074, 2097224, 0), -- Devouring Maggot
(131849, 2, 7, 5, 1, 138943, 2097224, 0), -- Crazed Marksman
(131666, 2, 7, 5, 1, 138929, 2097224, 0), -- Coven Thornshaper
(131858, 2, 7, 5, 1, 138939, 2097224, 0), -- Thornguard
(131677, 2, 7, 8, 1, 138923, 2097224, 0), -- Heartsbane Runeweaver
(131850, 2, 7, 5, 1, 138941, 2097224, 0), -- Maddened Survivalist
(131587, 2, 7, 8, 1, 138933, 2097224, 0), -- Bewitched Captain
(135357, 2, 7, 130, 1, 141530, 2097224, 0), -- Lady Waycrest
(131685, 2, 7, 5, 1, 138921, 2097224, 0), -- Runic Disciple
(131585, 2, 7, 5, 1, 138937, 2097224, 0), -- Enthralled Guard
(137048, 2, 7, 3.899999856948852539, 1, 143492, 0, 0), -- Beguiled Villager
(135360, 2, 7, 15.59999942779541015, 1, 141539, 2097224, 0), -- Sister Briar
(135358, 2, 7, 15.59999942779541015, 1, 141533, 2097224, 0), -- Sister Malady
(135359, 2, 7, 15.59999942779541015, 1, 141536, 2097224, 0), -- Sister Solena
(135240, 2, 7, 3, 1, 141776, 2097224, 0), -- Soul Essence
(137033, 2, 7, 6.5, 1, 143473, 2097224, 0), -- Runic Disciple
(137034, 2, 7, 6.5, 1, 143476, 2097224, 0); -- Thistle Acolyte

DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=2 AND `Entry` IN (144324,138761,135556,135553,135555,138762,137806,136302,135341,135255,134303,136094,136084,136085,136086,136087,135315,137774,133435,135168,135312,135313,135725,135513,135713,135714,132361,135139,135711));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `LevelScalingDeltaMin`, `LevelScalingDeltaMax`, `ContentTuningID`, `StaticFlags1`, `StaticFlags2`, `StaticFlags3`, `StaticFlags4`, `StaticFlags5`, `StaticFlags6`, `StaticFlags7`, `StaticFlags8`, `VerifiedBuild`) VALUES
(144324, 2, 0, 0, 501, 0, 0, 0, 0, 0, 0, 0, 0, 55165), -- 144324 (Gorak Tul) - 
(138761, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 138761 (Lucille Waycrest) - CanSwim
(135556, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135556 (Inquisitor Mace) - CanSwim
(135553, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135553 (Inquisitor Yorrick) - CanSwim
(135555, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135555 (Inquisitor Sterntide) - CanSwim
(138762, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 138762 (Lucille Waycrest) - CanSwim
(137806, 2, 0, 0, 501, 536870912, 0, 0, 0, 0, 0, 0, 0, 55165), -- 137806 (Organ to Basement Traveler) - Floating
(136302, 2, 0, 0, 501, 536871168, 0, 33554432, 0, 0, 0, 0, 0, 55165), -- 136302 (Dark Etching Target) - Sessile, Floating - CannotTurn
(135341, 2, 0, 0, 501, 0, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135341 (Captured Villager) - 
(135255, 2, 0, 0, 501, 536871168, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135255 (Drain Target) - Sessile, Floating
(134303, 2, 0, 0, 501, 536870912, 0, 0, 0, 0, 0, 0, 0, 55165), -- 134303 (Summoning Portal) - Floating
(136094, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 136094 (Inquisitor Notley) - CanSwim
(136084, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 136084 (Lucille Waycrest) - CanSwim
(136085, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 136085 (Inquisitor Yorrick) - CanSwim
(136086, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 136086 (Inquisitor Mace) - CanSwim
(136087, 2, 0, 0, 501, 268435456, 0, 0, 0, 0, 0, 0, 0, 55165), -- 136087 (Inquisitor Sterntide) - CanSwim
(135315, 2, 0, 0, 501, 536871168, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135315 (Goliath Drain Target Female) - Sessile, Floating
(137774, 2, 0, 0, 501, 536871168, 0, 0, 0, 0, 0, 0, 0, 55165), -- 137774 (Portal to Basement) - Sessile, Floating
(133435, 2, 0, 0, 501, 536871168, 0, 0, 0, 0, 0, 0, 0, 55165), -- 133435 (Drust Slaver Summoning Portal) - Sessile, Floating
(135168, 2, 0, 0, 501, 256, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135168 (Dead Pig) - Sessile
(135312, 2, 0, 0, 501, 536871168, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135312 (Goliath Drain Target Male) - Sessile, Floating
(135313, 2, 0, 0, 501, 536871168, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135313 (Goliath Drain Target Heavy) - Sessile, Floating
(135725, 2, 0, 0, 501, 0, 0, 33554432, 0, 0, 0, 0, 0, 55165), -- 135725 (Cursed Loom) - CannotTurn
(135513, 2, 0, 0, 501, 536870912, 0, 33554432, 0, 0, 0, 0, 0, 55165), -- 135513 (Ancient Tome) - Floating - CannotTurn
(135713, 2, 0, 0, 501, 536870912, 0, 33554432, 0, 0, 0, 0, 0, 55165), -- 135713 (Vile Pod) - Floating - CannotTurn
(135714, 2, 0, 0, 501, 0, 0, 33554432, 0, 0, 0, 0, 0, 55165), -- 135714 (Vinegared Wine) - CannotTurn
(132361, 2, 0, 0, 501, 0, 0, 0, 0, 0, 0, 0, 0, 55165), -- 132361 (Focusing Iris) - 
(135139, 2, 0, 0, 501, 256, 0, 0, 0, 0, 0, 0, 0, 55165), -- 135139 (Dead Villager) - Sessile
(135711, 2, 0, 0, 501, 0, 0, 33554432, 0, 0, 0, 0, 0, 55165); -- 135711 (Hunting Map) - CannotTurn

UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=157025 WHERE (`Entry`=144324 AND `DifficultyID`=2); -- Gorak Tul
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=145524 WHERE (`Entry`=138761 AND `DifficultyID`=2); -- Lucille Waycrest
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=141762, `TypeFlags`=4096 WHERE (`Entry`=135556 AND `DifficultyID`=2); -- Inquisitor Mace
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=141761, `TypeFlags`=4096 WHERE (`Entry`=135555 AND `DifficultyID`=2); -- Inquisitor Sterntide
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=141759, `TypeFlags`=4096 WHERE (`Entry`=135553 AND `DifficultyID`=2); -- Inquisitor Yorrick
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=145525 WHERE (`Entry`=138762 AND `DifficultyID`=2); -- Lucille Waycrest
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=144353 WHERE (`Entry`=137806 AND `DifficultyID`=2); -- Organ to Basement Traveler
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=142596 WHERE (`Entry`=136302 AND `DifficultyID`=2); -- Dark Etching Target
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141509 WHERE (`Entry`=135341 AND `DifficultyID`=2); -- Captured Villager
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141404 WHERE (`Entry`=135255 AND `DifficultyID`=2); -- Drain Target
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=140361 WHERE (`Entry`=134303 AND `DifficultyID`=2); -- Summoning Portal
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=142352 WHERE (`Entry`=136094 AND `DifficultyID`=2); -- Inquisitor Notley
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=142343 WHERE (`Entry`=136087 AND `DifficultyID`=2); -- Inquisitor Sterntide
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=142342 WHERE (`Entry`=136086 AND `DifficultyID`=2); -- Inquisitor Mace
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=142341 WHERE (`Entry`=136085 AND `DifficultyID`=2); -- Inquisitor Yorrick
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=3, `CreatureDifficultyID`=142340 WHERE (`Entry`=136084 AND `DifficultyID`=2); -- Lucille Waycrest
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141476 WHERE (`Entry`=135315 AND `DifficultyID`=2); -- Goliath Drain Target Female
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=144319, `TypeFlags`=1073741824, `TypeFlags2`=6 WHERE (`Entry`=137774 AND `DifficultyID`=2); -- Portal to Basement
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `HealthModifier`=5, `CreatureDifficultyID`=141474 WHERE (`Entry`=135313 AND `DifficultyID`=2); -- Goliath Drain Target Heavy
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141473 WHERE (`Entry`=135312 AND `DifficultyID`=2); -- Goliath Drain Target Male
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141306 WHERE (`Entry`=135168 AND `DifficultyID`=2); -- Dead Pig
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=139464 WHERE (`Entry`=133435 AND `DifficultyID`=2); -- Drust Slaver Summoning Portal
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141954, `TypeFlags`=1073741824, `TypeFlags2`=6 WHERE (`Entry`=135725 AND `DifficultyID`=2); -- Cursed Loom
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141715, `TypeFlags`=1073741824, `TypeFlags2`=6 WHERE (`Entry`=135513 AND `DifficultyID`=2); -- Ancient Tome
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=138291 WHERE (`Entry`=132361 AND `DifficultyID`=2); -- Focusing Iris
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141942, `TypeFlags`=1073741824, `TypeFlags2`=6 WHERE (`Entry`=135713 AND `DifficultyID`=2); -- Vile Pod
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141943, `TypeFlags`=1073741824, `TypeFlags2`=6 WHERE (`Entry`=135714 AND `DifficultyID`=2); -- Vinegared Wine
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141277 WHERE (`Entry`=135139 AND `DifficultyID`=2); -- Dead Villager
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=7, `CreatureDifficultyID`=141940, `TypeFlags`=1073741824, `TypeFlags2`=6 WHERE (`Entry`=135711 AND `DifficultyID`=2); -- Hunting Map
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=9, `CreatureDifficultyID`=120388 WHERE (`Entry`=116633 AND `DifficultyID`=2); -- Effect


UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135552 AND `DifficultyID`=2); -- 135552 (Deathtouched Slaver) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=139269 AND `DifficultyID`=2); -- 139269 (Gloom Horror) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131864 AND `DifficultyID`=2); -- 131864 (Gorak Tul) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135365 AND `DifficultyID`=2); -- 135365 (Matron Alma) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131819 AND `DifficultyID`=2); -- 131819 (Coven Diviner) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136541 AND `DifficultyID`=2); -- 136541 (Bile Oozeling) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=536871168, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=136330 AND `DifficultyID`=2); -- 136330 (Soul Thorns) - Sessile, Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131667 AND `DifficultyID`=2); -- 131667 (Soulbound Goliath) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=132209 AND `DifficultyID`=2); -- 132209 (Lightning Controller) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135329 AND `DifficultyID`=2); -- 135329 (Matron Bryndle) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131863 AND `DifficultyID`=2); -- 131863 (Raal the Gluttonous) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131595 AND `DifficultyID`=2); -- 131595 (Governess Tabitha) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131847 AND `DifficultyID`=2); -- 131847 (Waycrest Reveler) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131586 AND `DifficultyID`=2); -- 131586 (Banquet Steward) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=137830 AND `DifficultyID`=2); -- 137830 (Pallid Gorger) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131617 AND `DifficultyID`=2); -- 131617 (Groundskeeper Lilith) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131619 AND `DifficultyID`=2); -- 131619 (Seamstress Johanna) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131615 AND `DifficultyID`=2); -- 131615 (Head Vintner Justin) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131527 AND `DifficultyID`=2); -- 131527 (Lord Waycrest) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131545 AND `DifficultyID`=2); -- 131545 (Lady Waycrest) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131823 AND `DifficultyID`=2); -- 131823 (Sister Malady) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=805306368, `VerifiedBuild`=55165 WHERE (`Entry`=135049 AND `DifficultyID`=2); -- 135049 (Dreadwing Raven) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131821 AND `DifficultyID`=2); -- 131821 (Faceless Maiden) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135048 AND `DifficultyID`=2); -- 135048 (Gorestained Piglet) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131812 AND `DifficultyID`=2); -- 131812 (Heartsbane Soulcharmer) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131824 AND `DifficultyID`=2); -- 131824 (Sister Solena) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131825 AND `DifficultyID`=2); -- 131825 (Sister Briar) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136161 AND `DifficultyID`=2); -- 136161 (Gorak Tul) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131853 AND `DifficultyID`=2); -- 131853 (Huntmaster Grath) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135474 AND `DifficultyID`=2); -- 135474 (Thistle Acolyte) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131858 AND `DifficultyID`=2); -- 131858 (Thornguard) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131666 AND `DifficultyID`=2); -- 131666 (Coven Thornshaper) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135052 AND `DifficultyID`=2); -- 135052 (Blight Toad) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135234 AND `DifficultyID`=2); -- 135234 (Diseased Mastiff) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=134041 AND `DifficultyID`=2); -- 134041 (Infected Peasant) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131677 AND `DifficultyID`=2); -- 131677 (Heartsbane Runeweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=134024 AND `DifficultyID`=2); -- 134024 (Devouring Maggot) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131669 AND `DifficultyID`=2); -- 131669 (Jagged Hound) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131849 AND `DifficultyID`=2); -- 131849 (Crazed Marksman) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131685 AND `DifficultyID`=2); -- 131685 (Runic Disciple) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=256, `VerifiedBuild`=55165 WHERE (`Entry`=137048 AND `DifficultyID`=2); -- 137048 (Beguiled Villager) - Sessile
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131850 AND `DifficultyID`=2); -- 131850 (Maddened Survivalist) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=135357 AND `DifficultyID`=2); -- 135357 (Lady Waycrest) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131585 AND `DifficultyID`=2); -- 131585 (Enthralled Guard) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131587 AND `DifficultyID`=2); -- 131587 (Bewitched Captain) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135360 AND `DifficultyID`=2); -- 135360 (Sister Briar) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135358 AND `DifficultyID`=2); -- 135358 (Sister Malady) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135359 AND `DifficultyID`=2); -- 135359 (Sister Solena) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135240 AND `DifficultyID`=2); -- 135240 (Soul Essence) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=137033 AND `DifficultyID`=2); -- 137033 (Runic Disciple) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=501, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=137034 AND `DifficultyID`=2); -- 137034 (Thistle Acolyte) - CanSwim

-- Difficulty (Mythic)
DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=23 AND `Entry` IN (136330,136436,198489,134303,136094,136087,136086,136085,136084,131667,132209,137774,135329,135315,135313,135312,135168,133435,131863,131595,131586,135725,131847,131615,131619,135513,131617,132361,131812,137830,131824,135713,131823,131545,131527,131825,135714,136161,135139,135049,135048,131853,131821,131669,135474,131587,134041,134024,135234,135052,131849,131666,131858,131677,131850,135711,116633,131685,131585,135357,137048,135360,135358,135359,137034,137033,135240));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `HealthScalingExpansion`, `HealthModifier`, `ManaModifier`, `CreatureDifficultyID`, `TypeFlags`, `TypeFlags2`) VALUES
(136330, 23, 7, 1, 1, 142629, 0, 128), -- Soul Thorns
(136436, 23, 7, 1, 1, 142749, 8, 128), -- Burning Soul
(198489, 23, 9, 1, 2, 226868, 0, 0), -- Denizen of the Dream
(134303, 23, 7, 1, 1, 140361, 0, 0), -- Summoning Portal
(136094, 23, 7, 3, 1, 142352, 0, 0), -- Inquisitor Notley
(136087, 23, 7, 3, 1, 142343, 0, 0), -- Inquisitor Sterntide
(136086, 23, 7, 3, 1, 142342, 0, 0), -- Inquisitor Mace
(136085, 23, 7, 3, 1, 142341, 0, 0), -- Inquisitor Yorrick
(136084, 23, 7, 3, 1, 142340, 0, 0), -- Lucille Waycrest
(131667, 23, 7, 35, 1, 138059, 2097256, 128), -- Soulbound Goliath
(132209, 23, 7, 1, 1, 148705, 0, 0), -- Lightning Controller
(137774, 23, 7, 1, 1, 144319, 1073741824, 6), -- Portal to Basement
(135329, 23, 7, 12, 1, 141497, 2097224, 0), -- Matron Bryndle
(135315, 23, 7, 1, 1, 141476, 0, 0), -- Goliath Drain Target Female
(135313, 23, 7, 5, 1, 141474, 0, 0), -- Goliath Drain Target Heavy
(135312, 23, 7, 1, 1, 141473, 0, 0), -- Goliath Drain Target Male
(135168, 23, 7, 1, 1, 141306, 0, 0), -- Dead Pig
(133435, 23, 7, 1, 1, 139464, 0, 0), -- Drust Slaver Summoning Portal
(131863, 23, 7, 34, 1, 139378, 2097256, 128), -- Raal the Gluttonous
(131595, 23, 7, 15, 1, 138973, 2097224, 0), -- Governess Tabitha
(131586, 23, 7, 5, 1, 138936, 2097224, 0), -- Banquet Steward
(135725, 23, 7, 1, 1, 141954, 1073741824, 6), -- Cursed Loom
(131847, 23, 7, 3, 1, 138946, 2097224, 0), -- Waycrest Reveler
(131615, 23, 7, 15, 1, 138965, 2097224, 0), -- Head Vintner Justin
(131619, 23, 7, 15, 1, 138961, 2097224, 0), -- Seamstress Johanna
(135513, 23, 7, 1, 1, 141715, 1073741824, 6), -- Ancient Tome
(131617, 23, 7, 12, 1, 138963, 2097224, 0), -- Groundskeeper Lilith
(132361, 23, 7, 1, 1, 138291, 0, 0), -- Focusing Iris
(131812, 23, 7, 8, 1, 138920, 2097224, 0), -- Heartsbane Soulcharmer
(137830, 23, 7, 5, 1, 144379, 2097224, 0), -- Pallid Gorger
(131824, 23, 7, 9.5, 1, 138243, 2097256, 128), -- Sister Solena
(135713, 23, 7, 1, 1, 141942, 1073741824, 6), -- Vile Pod
(131823, 23, 7, 9.5, 1, 138245, 2097256, 128), -- Sister Malady
(131545, 23, 7, 100, 1, 137413, 2097256, 128), -- Lady Waycrest
(131527, 23, 7, 9, 1, 137393, 2097256, 128), -- Lord Waycrest
(131825, 23, 7, 9.5, 1, 138241, 2097256, 128), -- Sister Briar
(135714, 23, 7, 1, 1, 141943, 1073741824, 6), -- Vinegared Wine
(136161, 23, 7, 32, 1, 158829, 2097224, 0), -- Gorak Tul
(135139, 23, 7, 1, 1, 141277, 0, 0), -- Dead Villager
(135049, 23, 7, 3, 1, 141183, 2097225, 0), -- Dreadwing Raven
(135048, 23, 7, 2.5, 1, 141176, 2097224, 0), -- Gorestained Piglet
(131853, 23, 7, 15, 1, 138959, 2097224, 0), -- Huntmaster Grath
(131821, 23, 7, 4, 1, 138948, 2097224, 0), -- Faceless Maiden
(131669, 23, 7, 1, 1, 138928, 2097224, 0), -- Jagged Hound
(135474, 23, 7, 5, 1, 141671, 2097224, 0), -- Thistle Acolyte
(131587, 23, 7, 8, 1, 138934, 2097224, 0), -- Bewitched Captain
(134041, 23, 7, 5, 1, 141703, 2097224, 0), -- Infected Peasant
(134024, 23, 7, 1, 1, 140075, 2097224, 0), -- Devouring Maggot
(135234, 23, 7, 3, 1, 141377, 2097225, 0), -- Diseased Mastiff
(135052, 23, 7, 1, 1, 141186, 2097225, 0), -- Blight Toad
(131849, 23, 7, 5, 1, 138944, 2097224, 0), -- Crazed Marksman
(131666, 23, 7, 5, 1, 138930, 2097224, 0), -- Coven Thornshaper
(131858, 23, 7, 5, 1, 138940, 2097224, 0), -- Thornguard
(131677, 23, 7, 8, 1, 138924, 2097224, 0), -- Heartsbane Runeweaver
(131850, 23, 7, 5, 1, 138942, 2097224, 0), -- Maddened Survivalist
(135711, 23, 7, 1, 1, 141940, 1073741824, 6), -- Hunting Map
(116633, 23, 9, 1, 1, 120388, 0, 0), -- Effect
(131685, 23, 7, 5, 1, 138922, 2097224, 0), -- Runic Disciple
(131585, 23, 7, 5, 1, 138938, 2097224, 0), -- Enthralled Guard
(135357, 23, 7, 162.5, 1, 141531, 2097224, 0), -- Lady Waycrest
(137048, 23, 7, 4.875, 1, 143493, 0, 0), -- Beguiled Villager
(135360, 23, 7, 19.5, 1, 141540, 2097224, 0), -- Sister Briar
(135358, 23, 7, 19.5, 1, 141534, 2097224, 0), -- Sister Malady
(135359, 23, 7, 19.5, 1, 141537, 2097224, 0), -- Sister Solena
(137034, 23, 7, 8.125, 1, 143477, 2097224, 0), -- Thistle Acolyte
(137033, 23, 7, 8.125, 1, 143474, 2097224, 0), -- Runic Disciple
(135240, 23, 7, 3, 1, 141777, 2097224, 0); -- Soul Essence

UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536871168, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=136330 AND `DifficultyID`=23); -- 136330 (Soul Thorns) - Sessile, Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136436 AND `DifficultyID`=23); -- 136436 (Burning Soul) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=482, `StaticFlags3`=262144, `VerifiedBuild`=55165 WHERE (`Entry`=198489 AND `DifficultyID`=23); -- 198489 (Denizen of the Dream) - AllowInteractionWhileInCombat
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536870912, `VerifiedBuild`=55165 WHERE (`Entry`=134303 AND `DifficultyID`=23); -- 134303 (Summoning Portal) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136084 AND `DifficultyID`=23); -- 136084 (Lucille Waycrest) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136085 AND `DifficultyID`=23); -- 136085 (Inquisitor Yorrick) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136086 AND `DifficultyID`=23); -- 136086 (Inquisitor Mace) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136087 AND `DifficultyID`=23); -- 136087 (Inquisitor Sterntide) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136094 AND `DifficultyID`=23); -- 136094 (Inquisitor Notley) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131667 AND `DifficultyID`=23); -- 131667 (Soulbound Goliath) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=132209 AND `DifficultyID`=23); -- 132209 (Lightning Controller) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=256, `VerifiedBuild`=55165 WHERE (`Entry`=135168 AND `DifficultyID`=23); -- 135168 (Dead Pig) - Sessile
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135329 AND `DifficultyID`=23); -- 135329 (Matron Bryndle) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=135312 AND `DifficultyID`=23); -- 135312 (Goliath Drain Target Male) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=133435 AND `DifficultyID`=23); -- 133435 (Drust Slaver Summoning Portal) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=135313 AND `DifficultyID`=23); -- 135313 (Goliath Drain Target Heavy) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=135315 AND `DifficultyID`=23); -- 135315 (Goliath Drain Target Female) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=137774 AND `DifficultyID`=23); -- 137774 (Portal to Basement) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131595 AND `DifficultyID`=23); -- 131595 (Governess Tabitha) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131863 AND `DifficultyID`=23); -- 131863 (Raal the Gluttonous) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131586 AND `DifficultyID`=23); -- 131586 (Banquet Steward) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131615 AND `DifficultyID`=23); -- 131615 (Head Vintner Justin) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131847 AND `DifficultyID`=23); -- 131847 (Waycrest Reveler) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135725 AND `DifficultyID`=23); -- 135725 (Cursed Loom) - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536870912, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135513 AND `DifficultyID`=23); -- 135513 (Ancient Tome) - Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131619 AND `DifficultyID`=23); -- 131619 (Seamstress Johanna) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131812 AND `DifficultyID`=23); -- 131812 (Heartsbane Soulcharmer) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `VerifiedBuild`=55165 WHERE (`Entry`=132361 AND `DifficultyID`=23); -- 132361 (Focusing Iris) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=137830 AND `DifficultyID`=23); -- 137830 (Pallid Gorger) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131617 AND `DifficultyID`=23); -- 131617 (Groundskeeper Lilith) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=805306624, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=131545 AND `DifficultyID`=23); -- 131545 (Lady Waycrest) - Sessile, CanSwim, Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131853 AND `DifficultyID`=23); -- 131853 (Huntmaster Grath) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131823 AND `DifficultyID`=23); -- 131823 (Sister Malady) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131858 AND `DifficultyID`=23); -- 131858 (Thornguard) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=136161 AND `DifficultyID`=23); -- 136161 (Gorak Tul) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=134041 AND `DifficultyID`=23); -- 134041 (Infected Peasant) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=536870912, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135713 AND `DifficultyID`=23); -- 135713 (Vile Pod) - Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131849 AND `DifficultyID`=23); -- 131849 (Crazed Marksman) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135714 AND `DifficultyID`=23); -- 135714 (Vinegared Wine) - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135052 AND `DifficultyID`=23); -- 135052 (Blight Toad) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135474 AND `DifficultyID`=23); -- 135474 (Thistle Acolyte) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=805306368, `VerifiedBuild`=55165 WHERE (`Entry`=135049 AND `DifficultyID`=23); -- 135049 (Dreadwing Raven) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131677 AND `DifficultyID`=23); -- 131677 (Heartsbane Runeweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131527 AND `DifficultyID`=23); -- 131527 (Lord Waycrest) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131666 AND `DifficultyID`=23); -- 131666 (Coven Thornshaper) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131669 AND `DifficultyID`=23); -- 131669 (Jagged Hound) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131824 AND `DifficultyID`=23); -- 131824 (Sister Solena) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=256, `VerifiedBuild`=55165 WHERE (`Entry`=135139 AND `DifficultyID`=23); -- 135139 (Dead Villager) - Sessile
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131825 AND `DifficultyID`=23); -- 131825 (Sister Briar) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135048 AND `DifficultyID`=23); -- 135048 (Gorestained Piglet) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131821 AND `DifficultyID`=23); -- 131821 (Faceless Maiden) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=134024 AND `DifficultyID`=23); -- 134024 (Devouring Maggot) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135234 AND `DifficultyID`=23); -- 135234 (Diseased Mastiff) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131587 AND `DifficultyID`=23); -- 131587 (Bewitched Captain) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131850 AND `DifficultyID`=23); -- 131850 (Maddened Survivalist) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131685 AND `DifficultyID`=23); -- 131685 (Runic Disciple) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags3`=33554432, `VerifiedBuild`=55165 WHERE (`Entry`=135711 AND `DifficultyID`=23); -- 135711 (Hunting Map) - CannotTurn
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=256, `VerifiedBuild`=55165 WHERE (`Entry`=137048 AND `DifficultyID`=23); -- 137048 (Beguiled Villager) - Sessile
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=135357 AND `DifficultyID`=23); -- 135357 (Lady Waycrest) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=181, `StaticFlags1`=536871168, `VerifiedBuild`=55165 WHERE (`Entry`=116633 AND `DifficultyID`=23); -- 116633 (Effect) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=131585 AND `DifficultyID`=23); -- 131585 (Enthralled Guard) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135360 AND `DifficultyID`=23); -- 135360 (Sister Briar) - CanSwim
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=55165 WHERE (`Entry`=120652 AND `DifficultyID`=23);
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135358 AND `DifficultyID`=23); -- 135358 (Sister Malady) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135359 AND `DifficultyID`=23); -- 135359 (Sister Solena) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=137034 AND `DifficultyID`=23); -- 137034 (Thistle Acolyte) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=135240 AND `DifficultyID`=23); -- 135240 (Soul Essence) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=502, `StaticFlags1`=268435456, `VerifiedBuild`=55165 WHERE (`Entry`=137033 AND `DifficultyID`=23); -- 137033 (Runic Disciple) - CanSwim

-- Soul Essence smart ai
SET @ENTRY := 135240;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(@ENTRY, 0, 0, 0, '', 25, 0, 100, 0, 0, 0, 0, 0, 11, 267812, 0, 0, 0, 0, 0, 19, 137048, 6, 0, 0, 0, 0, 0, 'On reset - Self: Cast spell  267812 on Closest alive creature Beguiled Villager (137048) in 6 yards');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 22 AND `SourceGroup` = 1 AND `SourceEntry` = @ENTRY;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `Comment`) VALUES 
(22, 1, @ENTRY, 0, 0, 58, 1, 0, 0, 0, 'CosmeticDarkEtching', 0, 'Scripted creature has StringId ''CosmeticDarkEtching''');

UPDATE `creature` SET `StringId`='CosmeticDarkEtching' WHERE `guid` IN (@CGUID+16, @CGUID+13, @CGUID+38, @CGUID+18, @CGUID+20);

-- Thistle Acolyte smart ai
SET @ENTRY := 135474;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(@ENTRY, 0, 0, 0, '', 25, 0, 100, 0, 0, 0, 0, 0, 85, 269151, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On reset - Self: Cast spell 269151 on self');

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 22 AND `SourceGroup` = 1 AND `SourceEntry` = @ENTRY;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `Comment`) VALUES 
(22, 1, @ENTRY, 0, 0, 58, 1, 0, 0, 0, 'CosmeticWitchFrog', 0, 'Scripted creature has StringId ''CosmeticWitchFrog''');

UPDATE `creature` SET `StringId`='CosmeticWitchFrog' WHERE `guid` IN (@CGUID+58, @CGUID+116);

-- Infected Peasant smart ai
SET @ENTRY := 134041;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(@ENTRY, 0, 0, 0, '', 25, 0, 100, 0, 0, 0, 0, 0, 85, 272860, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On reset - Self: Cast spell 272860 on self');

-- Waypoints for CGUID+14
DELETE FROM `creature_formations` WHERE `leaderGUID` = @CGUID+14;
INSERT INTO `creature_formations` (`leaderGUID`, `memberGUID`, `dist`, `angle`, `groupAI`, `point_1`, `point_2`) VALUES
(@CGUID+14, @CGUID+14, 0, 0, 515, 0, 0),
(@CGUID+14, @CGUID+4, 2, 270, 515, 4, 10);

SET @ENTRY := 131685;
SET @PATHOFFSET := 0;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Runic Disciple - Formation Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, -559.2674, -191.4844, 249.5423, NULL, 0),
(@PATH, 1, -571.7257, -191.7691, 249.5423, NULL, 0),
(@PATH, 2, -588.0625, -191.5365, 249.5883, NULL, 0),
(@PATH, 3, -588.8386, -205.9427, 249.5423, NULL, 0),
(@PATH, 4, -589.4393, -219.3976, 249.5423, NULL, 4832),
(@PATH, 5, -588.8386, -205.9427, 249.5423, NULL, 0),
(@PATH, 6, -588.0625, -191.5365, 249.5883, NULL, 0),
(@PATH, 7, -571.7257, -191.7691, 249.5423, NULL, 0),
(@PATH, 8, -559.2674, -191.4844, 249.5423, NULL, 0),
(@PATH, 9, -559.9028, -207.7483, 249.5423, NULL, 0),
(@PATH, 10, -559.9011, -219.2951, 249.5423, NULL, 4808),
(@PATH, 11, -559.9028, -207.7483, 249.5423, NULL, 0);

UPDATE `creature` SET `position_x`= -559.2674, `position_y`= -191.4844, `position_z`= 249.5423, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+14;
UPDATE `creature` SET `position_x`= -559.2674, `position_y`= -191.4844, `position_z`= 249.5423 WHERE `guid`= @CGUID+4;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+14;
INSERT INTO `creature_addon` (`guid`, `PathId`, `SheathState`) VALUES
(@CGUID+14, @PATH, 1);

-- Waypoints for CGUID+36
DELETE FROM `creature_formations` WHERE `leaderGUID` = @CGUID+36;
INSERT INTO `creature_formations` (`leaderGUID`, `memberGUID`, `dist`, `angle`, `groupAI`, `point_1`, `point_2`) VALUES
(@CGUID+36, @CGUID+36, 0, 0, 515, 0, 0),
(@CGUID+36, @CGUID+25, 2, 90, 515, 1, 0);

SET @ENTRY := 131587;
SET @PATHOFFSET := 0;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Bewitched Captain - Formation Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, -579.8489, -198.7778, 236.1361, NULL, 0),
(@PATH, 1, -529.4012, -198.421, 236.1361, NULL, 0);

UPDATE `creature` SET `position_x`= -579.8489, `position_y`= -198.7778, `position_z`= 236.1361, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+36;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+36;
INSERT INTO `creature_addon` (`guid`, `PathId`, `SheathState`, `auras`) VALUES
(@CGUID+36, @PATH, 1, '259712');

-- Waypoints for CGUID+134
DELETE FROM `creature_formations` WHERE `leaderGUID` = @CGUID+134;
INSERT INTO `creature_formations` (`leaderGUID`, `memberGUID`, `dist`, `angle`, `groupAI`, `point_1`, `point_2`) VALUES
(@CGUID+134, @CGUID+134, 0, 0, 515, 0, 0),
(@CGUID+134, @CGUID+123, 2, 270, 515, 1, 9);

SET @ENTRY := 131587;
SET @PATHOFFSET := 1;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Bewitched Captain - Formation Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, -501.0644, -187.4605, 249.5423, NULL, 0),
(@PATH, 1, -504.3195, -168.5938, 249.6306, NULL, 4085),
(@PATH, 2, -501.1059, -187.4601, 249.5423, NULL, 0),
(@PATH, 3, -482.7951, -187.6458, 249.5424, NULL, 0),
(@PATH, 4, -482.0226, -207.5104, 249.6559, NULL, 0),
(@PATH, 5, -512.1129, -207.1337, 236.1001, NULL, 0),
(@PATH, 6, -520.0972, -206.0434, 236.0613, NULL, 0),
(@PATH, 7, -520.3889, -198.6684, 235.9686, NULL, 0),
(@PATH, 8, -511.9132, -195.3958, 236.0183, NULL, 0),
(@PATH, 9, -493.5139, -195.3576, 236.016, NULL, 4290),
(@PATH, 10, -511.9132, -195.3958, 236.0183, NULL, 0),
(@PATH, 11, -520.3889, -198.6684, 235.9686, NULL, 0),
(@PATH, 12, -520.0972, -206.0434, 236.0613, NULL, 0),
(@PATH, 13, -512.1129, -207.1337, 236.1001, NULL, 0),
(@PATH, 14, -482.0226, -207.5104, 249.6559, NULL, 0),
(@PATH, 15, -482.7951, -187.6458, 249.5424, NULL, 0);

UPDATE `creature` SET `position_x`= -501.0644, `position_y`= -187.4605, `position_z`= 249.5423, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+134;
UPDATE `creature` SET `position_x`= -501.0644, `position_y`= -187.4605, `position_z`= 249.5423  WHERE `guid`= @CGUID+123;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+134;
INSERT INTO `creature_addon` (`guid`, `PathId`, `SheathState`, `auras`) VALUES
(@CGUID+134, @PATH, 1, '259712');

-- Waypoints for CGUID+147
SET @ENTRY := 131586;
SET @PATHOFFSET := 0;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Banquet Steward - Cosmetic Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, -507.0243, -323.8368, 235.7298, NULL, 0),
(@PATH, 1, -508.8837, -337.5451, 235.7299, NULL, 0),
(@PATH, 2, -491.2813, -335.1823, 235.7298, NULL, 0),
(@PATH, 3, -483.691, -336.2396, 235.804, NULL, 0),
(@PATH, 4, -471.757, -332.6302, 235.7299, NULL, 0),
(@PATH, 5, -456.5712, -330.6181, 236.1361, NULL, 0),
(@PATH, 6, -448.8663, -330.7135, 236.1361, NULL, 0),
(@PATH, 7, -442.1163, -330.9254, 236.1646, NULL, 0),
(@PATH, 8, -440.5712, -319.132, 236.2429, NULL, 0),
(@PATH, 9, -441.1684, -310.6389, 236.2031, NULL, 0),
(@PATH, 10, -445.2066, -309.4757, 236.1361, NULL, 0),
(@PATH, 11, -451.6615, -311.2552, 236.1361, NULL, 0),
(@PATH, 12, -452.4757, -320.4201, 236.2586, NULL, 0),
(@PATH, 13, -457.7101, -328.9323, 236.1361, NULL, 0),
(@PATH, 14, -471.9149, -329.5903, 235.7299, NULL, 0),
(@PATH, 15, -476.408, -323.6146, 235.7299, NULL, 0),
(@PATH, 16, -490.1354, -323.309, 235.7298, NULL, 0);

UPDATE `creature` SET `position_x`= -507.0243, `position_y`= -323.8368, `position_z`= 235.7298, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+147;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+147;
INSERT INTO `creature_addon` (`guid`, `PathId`, `SheathState`, `auras`) VALUES
(@CGUID+147, @PATH, 1, '259712');

-- Waypoints for CGUID+55
DELETE FROM `creature_formations` WHERE `leaderGUID` = @CGUID+55;
INSERT INTO `creature_formations` (`leaderGUID`, `memberGUID`, `dist`, `angle`, `groupAI`, `point_1`, `point_2`) VALUES
(@CGUID+55, @CGUID+55, 0, 0, 515, 0, 0),
(@CGUID+55, @CGUID+79, 2, 0, 515, 0, 0);

SET @ENTRY := 137830;
SET @PATHOFFSET := 0;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 0, 0, 'Pallid Gorger - Formation Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, -515.6285, -299.9306, 236.1361, NULL, 0),
(@PATH, 1, -536.4983, -299.8125, 236.1361, NULL, 0),
(@PATH, 2, -535.1858, -330.0903, 236.1361, NULL, 0),
(@PATH, 3, -535.7465, -361.2517, 236.1361, NULL, 229),
(@PATH, 4, -535.1858, -330.0903, 236.1361, NULL, 0),
(@PATH, 5, -536.4983, -299.8125, 236.1361, NULL, 0),
(@PATH, 6, -515.6285, -299.9306, 236.1361, NULL, 0),
(@PATH, 7, -480.9271, -299.5625, 236.1361, NULL, 116);

UPDATE `creature` SET `position_x`= -515.6285, `position_y`= -299.9306, `position_z`= 236.1361, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+55;
UPDATE `creature` SET `position_x`= -515.6285, `position_y`= -299.9306, `position_z`= 236.1361 WHERE `guid`= @CGUID+79;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+55;
INSERT INTO `creature_addon` (`guid`, `PathId`, `SheathState`) VALUES
(@CGUID+55, @PATH, 1);

-- Waypoints for CGUIDS+127
DELETE FROM `creature_formations` WHERE `leaderGUID` = @CGUID+127;
INSERT INTO `creature_formations` (`leaderGUID`, `memberGUID`, `dist`, `angle`, `groupAI`, `point_1`, `point_2`) VALUES
(@CGUID+127, @CGUID+127, 0, 0, 515, 0, 0),
(@CGUID+127, @CGUID+141, 2, 310, 515, 0, 0),
(@CGUID+127, @CGUID+138, 4, 310, 515, 0, 0),
(@CGUID+127, @CGUID+157, 2, 50, 515, 0, 0);

SET @ENTRY := 131669;
SET @PATHOFFSET := 0;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 1, 0, 'Jagged Hound - Formation Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, -531.1788, -273.8333, 235.0007, NULL, 0),
(@PATH, 1, -531.1735, -247.0815, 235.3397, NULL, 0),
(@PATH, 2, -492.8507, -248.1146, 234.6055, NULL, 0),
(@PATH, 3, -463.3785, -246.8698, 234.6031, NULL, 0),
(@PATH, 4, -463.1719, -260.2639, 234.6033, NULL, 0),
(@PATH, 5, -463.8125, -272.916, 234.7457, NULL, 0),
(@PATH, 6, -496.3368, -273.5781, 234.7459, NULL, 0);

UPDATE `creature` SET `position_x`= -531.1788, `position_y`= -273.8333, `position_z`= 235.0007, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+127;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+127;
INSERT INTO `creature_addon` (`guid`, `PathId`, `SheathState`, `auras`) VALUES
(@CGUID+127, @PATH, 1, '214567');

-- Waypoints for CGUID+130
DELETE FROM `creature_formations` WHERE `leaderGUID` = @CGUID+130;
INSERT INTO `creature_formations` (`leaderGUID`, `memberGUID`, `dist`, `angle`, `groupAI`, `point_1`, `point_2`) VALUES
(@CGUID+130, @CGUID+130, 0, 0, 515, 0, 0),
(@CGUID+130, @CGUID+124, 2, 310, 515, 0, 0),
(@CGUID+130, @CGUID+139, 2, 50, 515, 0, 0);

SET @ENTRY := 131669;
SET @PATHOFFSET := 1;
SET @PATH := @ENTRY * 100 + @PATHOFFSET;
DELETE FROM `waypoint_path` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(@PATH, 1, 0, 'Jagged Hound - Formation Path');

DELETE FROM `waypoint_path_node` WHERE `PathId`= @PATH;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(@PATH, 0, -529.1511, -273.6962, 234.9343, NULL, 0),
(@PATH, 1, -529.5625, -247.0417, 234.8347, NULL, 0),
(@PATH, 2, -492.5938, -247.974, 234.6072, NULL, 0),
(@PATH, 3, -463.1754, -246.7413, 234.6031, NULL, 0),
(@PATH, 4, -463.8871, -272.916, 234.7459, NULL, 0),
(@PATH, 5, -496.684, -273.6823, 234.7502, NULL, 0);

UPDATE `creature` SET `position_x`= -529.1511, `position_y`= -273.6962, `position_z`= 234.9343, `orientation`= 0, `wander_distance`= 0, `MovementType`= 2 WHERE `guid`= @CGUID+130;
DELETE FROM `creature_addon` WHERE `guid`= @CGUID+130;
INSERT INTO `creature_addon` (`guid`, `PathId`, `SheathState`, `auras`) VALUES
(@CGUID+130, @PATH, 1, '214567');



-- Template
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_kings_rest_animated_guardian' WHERE `entry` = 133935;

-- Template Addon
UPDATE `creature_template_addon` SET `auras` = '276032' where `entry` = 133943;

-- Creature Text
DELETE FROM `creature_text` WHERE `CreatureID` = 133935;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(133935, 0, 0, 'THE IMPURE MAY NOT PASS INTO THE HALLS OF THE DEAD.', 12, 0, 100, 0, 0, 0, 153130, 0, 'Animated Guardian to Player'),
(133935, 0, 1, 'NONE SHALL ENTER.', 12, 0, 100, 0, 0, 0, 153128, 0, 'Animated Guardian to Player');

-- SpellScripts
DELETE FROM `spell_script_names` WHERE `spell_id` IN (270002, 269935, 276031);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(270002, 'spell_kings_rest_suppression_slam'),
(269935, 'spell_kings_rest_bound_by_shadow'),
(276031, 'spell_kings_rest_pit_of_despair');

-- Minion of Zul smart ai
SET @ENTRY := 133943;
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = @ENTRY;
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = @ENTRY;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(@ENTRY, 0, 0, 0, '', 63, 0, 100, 0, 0, 0, 0, 0, 85, 269935, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On just created - Self: Cast spell 269935 on self'),
(@ENTRY, 0, 1, 0, '', 4, 0, 100, 0, 0, 0, 0, 0, 85, 269936, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On aggro - Self: Cast spell 269936 on self');



-- Creature templates
UPDATE `creature_template` SET `faction`=1801, `npcflag`=1, `speed_run`=1.714285731315612792, `BaseAttackTime`=2000, `unit_flags2`=2048, `unit_flags3`=8388608 WHERE `entry`=221105; -- Huojin Sentry
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048, `unit_flags3`=16777216 WHERE `entry`=220526; -- Displaced Anomaly
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags2`=2048, `unit_flags3`=67108864 WHERE `entry`=220525; -- Young Albatross
UPDATE `creature_template` SET `faction`=2136, `BaseAttackTime`=2000, `unit_flags2`=2048, `unit_flags3`=67108864 WHERE `entry`=220524; -- Great Turtle Hatchling
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=33554432, `unit_flags2`=2048, `unit_flags3`=524289 WHERE `entry`=218391; -- Displaced Moment
UPDATE `creature_template` SET `faction`=1711, `speed_walk`=1, `speed_run`=1.142857193946838378, `BaseAttackTime`=2000, `unit_flags`=33554432, `unit_flags2`=2048 WHERE `entry`=218346; -- Infinite Ravager
UPDATE `creature_template` SET `faction`=1812, `BaseAttackTime`=2000, `unit_flags2`=2048, `unit_flags3`=8388608 WHERE `entry` IN (218148, 217058); -- Infinite Watcher
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048 WHERE `entry`=218117; -- Infinite Riftmender
UPDATE `creature_template` SET `faction`=35, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=33555200, `unit_flags2`=67110912, `unit_flags3`=16777216 WHERE `entry`=218068; -- [DNT] Example Anomaly
UPDATE `creature_template` SET `faction`=31, `speed_run`=0.857142865657806396, `BaseAttackTime`=2000, `unit_flags2`=2048 WHERE `entry`=218056; -- Time Flies
UPDATE `creature_template` SET `faction`=127, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048, `unit_flags3`=8388608 WHERE `entry`=217937; -- Infinite Proto-Drake
UPDATE `creature_template` SET `faction`=2105, `BaseAttackTime`=2000, `unit_flags`=256, `unit_flags2`=2048, `unit_flags3`=8388608 WHERE `entry`=217936; -- Infinite Watcher
UPDATE `creature_template` SET `faction`=2104, `BaseAttackTime`=2000, `unit_flags`=256, `unit_flags2`=2048, `unit_flags3`=8388608 WHERE `entry`=217934; -- Infinite Paradox
UPDATE `creature_template` SET `faction`=634, `BaseAttackTime`=2000, `unit_flags`=33554432, `unit_flags2`=67110912, `unit_flags3`=17301504 WHERE `entry`=217739; -- Timestorm
UPDATE `creature_template` SET `faction`=35, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048 WHERE `entry`=217668; -- Momentus
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=67110912, `unit_flags3`=16777216 WHERE `entry`=217666; -- Unstable Rift
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=33554432, `unit_flags2`=2048, `unit_flags3`=16777216 WHERE `entry`=217665; -- [DNT] Anvil Bunny
UPDATE `creature_template` SET `faction`=35, `npcflag`=129, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048 WHERE `entry`=217663; -- Erus
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags2`=2048 WHERE `entry`=217613; -- Displaced Moment
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags2`=2048 WHERE `entry`=217564; -- Archaios the Artificer
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=33555200, `unit_flags2`=2048, `unit_flags3`=16777216 WHERE `entry`=217560; -- [DNT] Dome Bunny
UPDATE `creature_template` SET `faction`=2576, `BaseAttackTime`=2000, `unit_flags2`=2048 WHERE `entry`=217558; -- Displaced Tick
UPDATE `creature_template` SET `faction`=1711, `BaseAttackTime`=2000, `unit_flags2`=2048 WHERE `entry`=217557; -- Infinite Ravager
UPDATE `creature_template` SET `faction`=16, `speed_walk`=1, `speed_run`=1.142857193946838378, `BaseAttackTime`=2000, `unit_flags`=64, `unit_flags2`=2048 WHERE `entry`=217190; -- Eratus, the Unwoven Paradox
UPDATE `creature_template` SET `faction`=1999, `BaseAttackTime`=2000, `unit_flags`=33554432, `unit_flags2`=2048 WHERE `entry`=217189; -- Displaced Moment
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048 WHERE `entry`=217060; -- Infinite Chronoweaver
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048 WHERE `entry` IN (217056, 217055); -- Infinite Scholar
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048 WHERE `entry`=217054; -- Infinite Acolyte
UPDATE `creature_template` SET `faction`=35, `npcflag`=129, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048 WHERE `entry`=217051; -- Horos
UPDATE `creature_template` SET `faction`=35, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048 WHERE `entry`=216594; -- Moratari
UPDATE `creature_template` SET `faction`=35, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=768, `unit_flags2`=2048 WHERE `entry`=216591; -- Eternus
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=256, `unit_flags2`=67110912, `unit_flags3`=17334272 WHERE `entry`=213680; -- Invisible Bunny

DELETE FROM `creature_template_movement` WHERE `CreatureId` IN (220526);
INSERT INTO `creature_template_movement` (`CreatureId`,`HoverInitiallyEnabled`,`Chase`,`Random`,`InteractionPauseTimer`) VALUES
(220526,1,0,0,NULL);

-- Equipments
DELETE FROM `creature_equip_template` WHERE (`ID`=1 AND `CreatureID` IN (221105,218346,218148,217936,217934,217668,217564,217557,217058,217051));
INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `AppearanceModID1`, `ItemVisual1`, `ItemID2`, `AppearanceModID2`, `ItemVisual2`, `ItemID3`, `AppearanceModID3`, `ItemVisual3`, `VerifiedBuild`) VALUES
(221105, 1, 88857, 0, 0, 0, 0, 0, 89875, 0, 0, 55142), -- Huojin Sentry
(218346, 1, 12993, 0, 0, 0, 0, 0, 0, 0, 0, 55142), -- Infinite Ravager
(218148, 1, 192148, 0, 0, 0, 0, 0, 0, 0, 0, 55142), -- Infinite Watcher
(217936, 1, 192148, 0, 0, 0, 0, 0, 0, 0, 0, 55142), -- Infinite Watcher
(217934, 1, 192148, 0, 0, 0, 0, 0, 0, 0, 0, 55142), -- Infinite Paradox
(217668, 1, 191228, 0, 0, 0, 0, 0, 0, 0, 0, 55142), -- Momentus
(217564, 1, 65653, 0, 0, 0, 0, 0, 0, 0, 0, 55142), -- Archaios the Artificer
(217557, 1, 12993, 0, 0, 0, 0, 0, 0, 0, 0, 55142), -- Infinite Ravager
(217058, 1, 192148, 0, 0, 0, 0, 0, 0, 0, 0, 55142), -- Infinite Watcher
(217051, 1, 78437, 0, 0, 0, 0, 0, 0, 0, 0, 55142); -- Horos

-- Difficulties
DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=0 AND `Entry` IN (217564,218346,218056,218148,217663,217558,218391,217666,217934,217189,217665,217190,217051,217739,216591,217560,217056,216594,217668,220524,218068,217557,217060,221105,218117,217058,217613,217937,217936,217054,220525,220526,217055));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `HealthScalingExpansion`, `HealthModifier`, `ManaModifier`, `CreatureDifficultyID`, `TypeFlags`, `TypeFlags2`, `StaticFlags1`, `StaticFlags2`) VALUES
(217564, 0, 4, 4, 1, 267057, 2147483720, 0, 0, 0), -- Archaios the Artificer
(218346, 0, 4, 2, 1, 267851, 0, 0, 0, 0), -- Infinite Ravager
(218056, 0, 9, 0.5, 1, 267551, 0, 0, 0, 0), -- Time Flies
(218148, 0, 4, 6, 1, 267643, 4096, 0, 0, 0), -- Infinite Watcher
(217663, 0, 4, 1, 1, 267156, 134217728, 16384, 0, 0), -- Erus
(217558, 0, 4, 1, 1, 267051, 0, 0, 0, 0), -- Displaced Tick
(218391, 0, 4, 0.300000011920928955, 1, 267897, 0, 0, 0, 0), -- Displaced Moment
(217666, 0, 4, 1, 1, 267159, 1610612752, 117440518, 0, 0), -- Unstable Rift
(217934, 0, 4, 6, 1, 267427, 0, 0, 0, 0), -- Infinite Paradox
(217189, 0, 4, 0.300000011920928955, 1, 266680, 0, 0, 0, 0), -- Displaced Moment
(217665, 0, 4, 1, 1, 267158, 0, 0, 0, 0), -- [DNT] Anvil Bunny
(217190, 0, 4, 8, 1, 266681, 2147483720, 0, 0, 0), -- Eratus, the Unwoven Paradox
(217051, 0, 4, 1, 1, 266541, 134217728, 16384, 0, 0), -- Horos
(217739, 0, 4, 1, 1, 267232, 1610612752, 100663302, 0, 0), -- Timestorm
(216591, 0, 4, 50, 2, 266081, 4, 32768, 0, 0), -- Eternus
(217560, 0, 4, 1, 1, 267053, 0, 0, 0, 0), -- [DNT] Dome Bunny
(217056, 0, 4, 1, 1, 266546, 0, 0, 0, 0), -- Infinite Scholar
(216594, 0, 4, 5, 2, 266084, 0, 32768, 0, 0), -- Moratari
(217668, 0, 4, 1, 1, 267161, 0, 16384, 0, 0), -- Momentus
(220524, 0, 4, 1, 1, 270133, 1, 0, 0, 0), -- Great Turtle Hatchling
(218068, 0, 4, 1, 1, 267563, 1610612752, 117440518, 0, 0), -- [DNT] Example Anomaly
(217557, 0, 4, 1, 1, 267050, 0, 0, 0, 0), -- Infinite Ravager
(217060, 0, 4, 1, 1, 266550, 0, 0, 0, 0), -- Infinite Chronoweaver
(221105, 0, 4, 20, 1, 270733, 0, 0, 0, 0), -- Huojin Sentry
(218117, 0, 4, 1, 1, 267612, 0, 0, 0, 0), -- Infinite Riftmender
(217058, 0, 4, 6, 1, 266548, 4096, 0, 0, 0), -- Infinite Watcher
(217613, 0, 4, 0.800000011920928955, 1, 267106, 0, 0, 0, 0), -- Displaced Moment
(217937, 0, 4, 10, 1, 267432, 4, 0, 0, 0), -- Infinite Proto-Drake
(217936, 0, 4, 6, 1, 267429, 0, 0, 0, 0), -- Infinite Watcher
(217054, 0, 4, 1, 1, 266544, 0, 0, 0, 0), -- Infinite Acolyte
(220525, 0, 4, 1, 1, 270134, 1, 0, 0, 0), -- Young Albatross
(220526, 0, 4, 0.800000011920928955, 1, 270135, 536870912, 67108864, 0, 0), -- Displaced Anomaly
(217055, 0, 4, 1, 1, 266545, 0, 0, 0, 0); -- Infinite Scholar

UPDATE `creature_template_difficulty` SET `ContentTuningID`=169, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=221105 AND `DifficultyID`=0); -- 221105 (Huojin Sentry) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=536871168, `VerifiedBuild`=55142 WHERE (`Entry`=220526 AND `DifficultyID`=0); -- 220526 (Displaced Anomaly) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=805306368, `VerifiedBuild`=55142 WHERE (`Entry`=220525 AND `DifficultyID`=0); -- 220525 (Young Albatross) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=220524 AND `DifficultyID`=0); -- 220524 (Great Turtle Hatchling) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=268435712, `VerifiedBuild`=55142 WHERE (`Entry`=218391 AND `DifficultyID`=0); -- 218391 (Displaced Moment) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `VerifiedBuild`=55142 WHERE (`Entry`=218346 AND `DifficultyID`=0); -- 218346 (Infinite Ravager) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2793, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=218148 AND `DifficultyID`=0); -- 218148 (Infinite Watcher) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2793, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=218117 AND `DifficultyID`=0); -- 218117 (Infinite Riftmender) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=536870912, `VerifiedBuild`=55142 WHERE (`Entry`=218068 AND `DifficultyID`=0); -- 218068 ([DNT] Example Anomaly) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=81, `StaticFlags1`=536870912, `VerifiedBuild`=55142 WHERE (`Entry`=218056 AND `DifficultyID`=0); -- 218056 (Time Flies) - Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2793, `StaticFlags1`=536870912, `VerifiedBuild`=55142 WHERE (`Entry`=217937 AND `DifficultyID`=0); -- 217937 (Infinite Proto-Drake) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2793, `VerifiedBuild`=55142 WHERE (`Entry`=217936 AND `DifficultyID`=0); -- 217936 (Infinite Watcher) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2793, `StaticFlags1`=536871168, `VerifiedBuild`=55142 WHERE (`Entry`=217934 AND `DifficultyID`=0); -- 217934 (Infinite Paradox) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `VerifiedBuild`=55142 WHERE (`Entry`=217739 AND `DifficultyID`=0); -- 217739 (Timestorm) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217668 AND `DifficultyID`=0); -- 217668 (Momentus) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=536871168, `VerifiedBuild`=55142 WHERE (`Entry`=217666 AND `DifficultyID`=0); -- 217666 (Unstable Rift) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=536870912, `VerifiedBuild`=55142 WHERE (`Entry`=217665 AND `DifficultyID`=0); -- 217665 ([DNT] Anvil Bunny) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2793, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217663 AND `DifficultyID`=0); -- 217663 (Erus) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `VerifiedBuild`=55142 WHERE (`Entry`=217613 AND `DifficultyID`=0); -- 217613 (Displaced Moment) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217564 AND `DifficultyID`=0); -- 217564 (Archaios the Artificer) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=536871168, `VerifiedBuild`=55142 WHERE (`Entry`=217560 AND `DifficultyID`=0); -- 217560 ([DNT] Dome Bunny) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=536870912, `VerifiedBuild`=55142 WHERE (`Entry`=217558 AND `DifficultyID`=0); -- 217558 (Displaced Tick) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `VerifiedBuild`=55142 WHERE (`Entry`=217557 AND `DifficultyID`=0); -- 217557 (Infinite Ravager) - 
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=536871168, `VerifiedBuild`=55142 WHERE (`Entry`=217190 AND `DifficultyID`=0); -- 217190 (Eratus, the Unwoven Paradox) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217189 AND `DifficultyID`=0); -- 217189 (Displaced Moment) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217060 AND `DifficultyID`=0); -- 217060 (Infinite Chronoweaver) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2851, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217058 AND `DifficultyID`=0); -- 217058 (Infinite Watcher) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2793, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217056 AND `DifficultyID`=0); -- 217056 (Infinite Scholar) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2793, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217055 AND `DifficultyID`=0); -- 217055 (Infinite Scholar) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2793, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217054 AND `DifficultyID`=0); -- 217054 (Infinite Acolyte) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=657, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=217051 AND `DifficultyID`=0); -- 217051 (Horos) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2793, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=216594 AND `DifficultyID`=0); -- 216594 (Moratari) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=2793, `StaticFlags1`=268435456, `VerifiedBuild`=55142 WHERE (`Entry`=216591 AND `DifficultyID`=0); -- 216591 (Eternus) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2793, `StaticFlags1`=536871168, `VerifiedBuild`=55142 WHERE (`Entry`=213680 AND `DifficultyID`=0); -- 213680 (Invisible Bunny) - Sessile, Floating

UPDATE `creature_questitem` SET `VerifiedBuild`=55142 WHERE (`CreatureEntry`=72767 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=72766 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=217557 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=72764 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=72764 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=73166 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=217564 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=72765 AND `DifficultyID`=0 AND `Idx`=1) OR (`CreatureEntry`=72765 AND `DifficultyID`=0 AND `Idx`=0) OR (`CreatureEntry`=72841 AND `DifficultyID`=0 AND `Idx`=0);
UPDATE `creature_quest_currency` SET `VerifiedBuild`=55142 WHERE (`CurrencyId`=738 AND `CreatureId` IN (72767,71986,71987,73279,72766,217557,218391,72764,73166,72765,220524,72841,217189,217558)) OR (`CurrencyId`=777 AND `CreatureId` IN (72767,71986,71987,73279,72766,72764,73166,72765,72841)) OR (`CurrencyId`=2778 AND `CreatureId` IN (72767,71986,71987,73279,72766,217557,218391,72764,73166,72765,220524,72841,217189,217558));

-- Vendors
DELETE FROM `npc_vendor` WHERE (`entry`=217663 AND `item`=224081 AND `ExtendedCost`=9038 AND `type`=1) OR (`entry`=217663 AND `item`=224078 AND `ExtendedCost`=9038 AND `type`=1) OR (`entry`=217663 AND `item`=224077 AND `ExtendedCost`=9038 AND `type`=1) OR (`entry`=217663 AND `item`=224076 AND `ExtendedCost`=9038 AND `type`=1) OR (`entry`=217663 AND `item`=224075 AND `ExtendedCost`=9038 AND `type`=1) OR (`entry`=217663 AND `item`=224080 AND `ExtendedCost`=9038 AND `type`=1) OR (`entry`=217663 AND `item`=224079 AND `ExtendedCost`=9038 AND `type`=1) OR (`entry`=217051 AND `item`=213598 AND `ExtendedCost`=8860 AND `type`=1) OR (`entry`=217051 AND `item`=213621 AND `ExtendedCost`=8859 AND `type`=1) OR (`entry`=217051 AND `item`=213624 AND `ExtendedCost`=8859 AND `type`=1) OR (`entry`=217051 AND `item`=213609 AND `ExtendedCost`=8558 AND `type`=1) OR (`entry`=217051 AND `item`=213604 AND `ExtendedCost`=8558 AND `type`=1) OR (`entry`=217051 AND `item`=213584 AND `ExtendedCost`=8558 AND `type`=1) OR (`entry`=217051 AND `item`=86588 AND `ExtendedCost`=8551 AND `type`=1) OR (`entry`=217051 AND `item`=86583 AND `ExtendedCost`=8553 AND `type`=1) OR (`entry`=217051 AND `item`=86589 AND `ExtendedCost`=8553 AND `type`=1);
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `ExtendedCost`, `type`, `PlayerConditionID`, `IgnoreFiltering`, `VerifiedBuild`, `OverrideGoldCost`) VALUES
(217663, 7, 224081, 0, 9038, 1, 0, 0, 55142, NULL), -- Timerunner's Staff
(217663, 6, 224078, 0, 9038, 1, 0, 0, 55142, NULL), -- Timerunner's Shield
(217663, 5, 224077, 0, 9038, 1, 0, 0, 55142, NULL), -- Timerunner's Dagger
(217663, 4, 224076, 0, 9038, 1, 0, 0, 55142, NULL), -- Timerunner's Bow
(217663, 3, 224075, 0, 9038, 1, 0, 0, 55142, NULL), -- Timerunner's Greatsword
(217663, 2, 224080, 0, 9038, 1, 0, 0, 55142, NULL), -- Timerunner's Sword
(217663, 1, 224079, 0, 9038, 1, 0, 0, 55142, NULL), -- Timerunner's Mace
(217051, 10, 213598, 0, 8860, 1, 0, 0, 55142, NULL), -- Reins of the Dashing Windsteed
(217051, 9, 213621, 0, 8859, 1, 0, 0, 55142, NULL), -- Reins of the Jade Pterrordax
(217051, 8, 213624, 0, 8859, 1, 0, 0, 55142, NULL), -- Cobalt Juggernaut
(217051, 7, 213609, 0, 8558, 1, 0, 0, 55142, NULL), -- Reins of the Little Red Riding Goat
(217051, 6, 213604, 0, 8558, 1, 0, 0, 55142, NULL), -- Reins of the Tropical Riding Crane
(217051, 5, 213584, 0, 8558, 1, 0, 0, 55142, NULL), -- Mogu Hazeblazer
(217051, 4, 86588, 0, 8551, 1, 0, 0, 55142, NULL), -- Pandaren Firework Launcher
(217051, 3, 86583, 0, 8553, 1, 0, 0, 55142, NULL), -- Salyin Battle Banner
(217051, 2, 86589, 0, 8553, 1, 0, 0, 55142, NULL); -- Ai-Li's Skymirror

-- Models
DELETE FROM `creature_model_info` WHERE `DisplayID` IN (117269, 79809, 113585, 117640, 115650, 117176, 115649, 113019, 113074, 112575, 112277, 112276, 116857, 116856, 116855, 116688, 112713);
INSERT INTO `creature_model_info` (`DisplayID`, `BoundingRadius`, `CombatReach`, `DisplayID_Other_Gender`, `VerifiedBuild`) VALUES
(117269, 0.872754871845245361, 0, 0, 55142),
(79809, 1.51990509033203125, 0, 0, 55142),
(113585, 1, 6, 0, 55142),
(117640, 0.699999988079071044, 0.699999988079071044, 0, 55142),
(115650, 1.168302655220031738, 1.5, 0, 55142),
(117176, 1.402945518493652343, 1.5, 0, 55142),
(115649, 1.404254555702209472, 2.40000009536743164, 0, 55142),
(113019, 1.168302655220031738, 1.5, 0, 55142),
(113074, 5.177106857299804687, 3.5, 0, 55142),
(112575, 1.132115364074707031, 1.5, 0, 55142),
(112277, 1.755318164825439453, 3, 0, 55142),
(112276, 1.755318164825439453, 3, 0, 55142),
(116857, 0.382999986410140991, 1.5, 0, 55142),
(116856, 0.305999994277954101, 1.5, 0, 55142),
(116855, 1.755318164825439453, 3, 0, 55142),
(116688, 0.382999986410140991, 1.5, 0, 55142),
(112713, 0.305999994277954101, 1.5, 0, 55142);

UPDATE `creature_model_info` SET `VerifiedBuild`=55142 WHERE `DisplayID` IN (51409, 44778, 56187, 20515, 51412, 11686, 51320, 51317, 51289, 51276, 51273, 42317, 37338, 50898, 50772, 50962, 51125, 51128, 49246, 25074, 25073, 25076, 25075, 12196, 12195, 49234, 49235, 49236, 49237, 51314);
UPDATE `creature_model_info` SET `BoundingRadius`=4.553055763244628906, `CombatReach`=4.375, `VerifiedBuild`=55142 WHERE `DisplayID`=19326;
UPDATE `creature_model_info` SET `BoundingRadius`=0.225000008940696716, `CombatReach`=0.75, `VerifiedBuild`=55142 WHERE `DisplayID`=74071;
UPDATE `creature_model_info` SET `BoundingRadius`=1, `VerifiedBuild`=55142 WHERE `DisplayID`=47677;
UPDATE `creature_model_info` SET `BoundingRadius`=5, `CombatReach`=6.25, `VerifiedBuild`=55142 WHERE `DisplayID`=51146;
UPDATE `creature_model_info` SET `BoundingRadius`=1.311424016952514648, `CombatReach`=1.5, `VerifiedBuild`=55142 WHERE `DisplayID`=51381;
UPDATE `creature_model_info` SET `BoundingRadius`=0.520871341228485107, `CombatReach`=0.375, `VerifiedBuild`=55142 WHERE `DisplayID`=61127;
UPDATE `creature_model_info` SET `BoundingRadius`=0.090000003576278686, `CombatReach`=0.300000011920928955, `VerifiedBuild`=55142 WHERE `DisplayID`=30402;

UPDATE `creature_template_model` SET `VerifiedBuild`=55142 WHERE (`Idx`=0 AND `CreatureID` IN (73166,217564,72841,165189,218346,71954,218056,60941,218148,69066,73385,217663,71955,64993,71939,73512,217558,71486,217666,72764,70356,218391,68288,73487,217934,217189,304,73531,217739,216591,305,217190,72765,40725,73403,221105,71952,218117,217058,25064,73398,73279,71988,220526,73303,32158,54879,217051,56921,73514,35362,59072,356,217665,54423,72780,71953,31698,18379,73362,217055,65011,40625,72767,71987,217937,217936,217054,220525,28363,217557,217060,28302,73353,217560,217056,14505,72766,217613,40165,71986,216594,217668,220524,218068,54741,47654)) OR (`Idx`=1 AND `CreatureID` IN (73512,73487,40725,217058,73514,71987,40165,71986)) OR (`Idx`=3 AND `CreatureID` IN (71987,71986)) OR (`Idx`=2 AND `CreatureID` IN (71987,71986));

-- Gameobject templates
DELETE FROM `gameobject_template` WHERE `entry` IN (423410 /*Anvil of Ephemeralities*/, 423343 /*Unstable Rift*/, 423103 /*Time Rift*/, 451226 /*Unraveling Sands*/, 424197 /*Bronze Barrier*/);
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`, `Data0`, `Data1`, `Data2`, `Data3`, `Data4`, `Data5`, `Data6`, `Data7`, `Data8`, `Data9`, `Data10`, `Data11`, `Data12`, `Data13`, `Data14`, `Data15`, `Data16`, `Data17`, `Data18`, `Data19`, `Data20`, `Data21`, `Data22`, `Data23`, `Data24`, `Data25`, `Data26`, `Data27`, `Data28`, `Data29`, `Data30`, `Data31`, `Data32`, `Data33`, `Data34`, `ContentTuningId`, `VerifiedBuild`) VALUES
(423410, 8, 87800, 'Anvil of Ephemeralities', '', '', '', 3, 1, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2851, 55142), -- Anvil of Ephemeralities
(423343, 10, 76234, 'Unstable Rift', 'inspect', 'Investigating', '', 3, 3492, 0, 0, 3000, 0, 0, 0, 0, 0, 0, 439809, 0, 0, 1, 193761, 0, 0, 0, 0, 0, 0, 0, 119615, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 55142), -- Unstable Rift
(423103, 5, 83572, 'Time Rift', '', '', '', 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2851, 55142), -- Time Rift
(451226, 48, 12628, 'Unraveling Sands', '', '', '', 0.349999994039535522, 2, 1, 0, 2055, 7, 500, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2793, 55142), -- Unraveling Sands
(424197, 0, 6391, 'Bronze Barrier', '', '', '', 3, 0, 0, 1, 0, 0, 0, 0, 21439, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 55142); -- Bronze Barrier

DELETE FROM `gameobject_template_addon` WHERE `entry` IN (423343 /*Unstable Rift*/, 423103 /*Time Rift*/, 451226 /*Unraveling Sands*/, 424197 /*Bronze Barrier*/);
INSERT INTO `gameobject_template_addon` (`entry`, `faction`, `flags`, `WorldEffectID`, `AIAnimKitID`) VALUES
(423343, 0, 32, 0, 5605), -- Unstable Rift
(423103, 0, 612368416, 0, 0), -- Time Rift
(451226, 0, 545259520, 11127, 6714), -- Unraveling Sands
(424197, 0, 32, 0, 0); -- Bronze Barrier

-- Quests
DELETE FROM `quest_template` WHERE `ID`=79432;
INSERT INTO `quest_template` (`ID`, `QuestType`, `QuestPackageID`, `ContentTuningID`, `QuestSortID`, `QuestInfoID`, `SuggestedGroupNum`, `RewardNextQuest`, `RewardXPDifficulty`, `RewardXPMultiplier`, `RewardMoneyDifficulty`, `RewardMoneyMultiplier`, `RewardSpell`, `RewardHonor`, `RewardKillHonor`, `StartItem`, `RewardArtifactXPDifficulty`, `RewardArtifactXPMultiplier`, `RewardArtifactCategoryID`, `Flags`, `FlagsEx`, `FlagsEx2`, `RewardSkillLineID`, `RewardNumSkillUps`, `PortraitGiver`, `PortraitGiverMount`, `PortraitGiverModelSceneID`, `PortraitTurnIn`, `RewardItem1`, `RewardItem2`, `RewardItem3`, `RewardItem4`, `RewardAmount1`, `RewardAmount2`, `RewardAmount3`, `RewardAmount4`, `ItemDrop1`, `ItemDrop2`, `ItemDrop3`, `ItemDrop4`, `ItemDropQuantity1`, `ItemDropQuantity2`, `ItemDropQuantity3`, `ItemDropQuantity4`, `RewardChoiceItemID1`, `RewardChoiceItemID2`, `RewardChoiceItemID3`, `RewardChoiceItemID4`, `RewardChoiceItemID5`, `RewardChoiceItemID6`, `RewardChoiceItemQuantity1`, `RewardChoiceItemQuantity2`, `RewardChoiceItemQuantity3`, `RewardChoiceItemQuantity4`, `RewardChoiceItemQuantity5`, `RewardChoiceItemQuantity6`, `RewardChoiceItemDisplayID1`, `RewardChoiceItemDisplayID2`, `RewardChoiceItemDisplayID3`, `RewardChoiceItemDisplayID4`, `RewardChoiceItemDisplayID5`, `RewardChoiceItemDisplayID6`, `POIContinent`, `POIx`, `POIy`, `POIPriority`, `RewardTitle`, `RewardArenaPoints`, `RewardFactionID1`, `RewardFactionID2`, `RewardFactionID3`, `RewardFactionID4`, `RewardFactionID5`, `RewardFactionValue1`, `RewardFactionValue2`, `RewardFactionValue3`, `RewardFactionValue4`, `RewardFactionValue5`, `RewardFactionCapIn1`, `RewardFactionCapIn2`, `RewardFactionCapIn3`, `RewardFactionCapIn4`, `RewardFactionCapIn5`, `RewardFactionOverride1`, `RewardFactionOverride2`, `RewardFactionOverride3`, `RewardFactionOverride4`, `RewardFactionOverride5`, `RewardFactionFlags`, `AreaGroupID`, `TimeAllowed`, `AllowableRaces`, `TreasurePickerID`, `Expansion`, `ManagedWorldStateID`, `QuestSessionBonus`, `LogTitle`, `LogDescription`, `QuestDescription`, `AreaDescription`, `QuestCompletionLog`, `RewardCurrencyID1`, `RewardCurrencyID2`, `RewardCurrencyID3`, `RewardCurrencyID4`, `RewardCurrencyQty1`, `RewardCurrencyQty2`, `RewardCurrencyQty3`, `RewardCurrencyQty4`, `PortraitGiverText`, `PortraitGiverName`, `PortraitTurnInText`, `PortraitTurnInName`, `AcceptedSoundKitID`, `CompleteSoundKitID`, `VerifiedBuild`) VALUES
(79432, 2, 0, 2793, -639, 282, 0, 79433, 5, 1, 5, 1, 0, 0, 0, 0, 0, 1, 0, 36700160, 0, 0, 0, 0, 19326, 0, 834, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 7, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 18446744073709551615, 0, 4, 0, 0, 'It\'s About Time', 'Slay Archaios the Artificer.', 'Your timing is serendipitous. I require your assistance.\n\nNot all infinites have seen what Nozdormu has shown me. Not all would agree even if they had. Archaios is one such detractor. He must be eliminated.', '', '', 0, 0, 0, 0, 0, 0, 0, 0, 'This lieutenant has been causing collateral damage to the timeline in his hunger for temporal artifacts.', 'Archaios the Artificer', '', '', 890, 878, 55142); -- It's About Time

UPDATE `quest_poi` SET `VerifiedBuild`=55142 WHERE (`QuestID`=79432 AND `BlobIndex`=0 AND `Idx1`=3) OR (`QuestID`=79432 AND `BlobIndex`=0 AND `Idx1`=2) OR (`QuestID`=79432 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=79432 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=53435 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=53435 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=32009 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=32009 AND `BlobIndex`=0 AND `Idx1`=0) OR (`QuestID`=32008 AND `BlobIndex`=0 AND `Idx1`=1) OR (`QuestID`=32008 AND `BlobIndex`=0 AND `Idx1`=0);

UPDATE `quest_poi_points` SET `VerifiedBuild`=55142 WHERE (`QuestID`=79432 AND `Idx1`=3 AND `Idx2`=0) OR (`QuestID`=79432 AND `Idx1`=2 AND `Idx2`=0) OR (`QuestID`=79432 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=79432 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=53435 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=53435 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=32009 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=32009 AND `Idx1`=0 AND `Idx2`=0) OR (`QuestID`=32008 AND `Idx1`=1 AND `Idx2`=0) OR (`QuestID`=32008 AND `Idx1`=0 AND `Idx2`=0);

DELETE FROM `quest_details` WHERE `ID`=79432;
INSERT INTO `quest_details` (`ID`, `Emote1`, `Emote2`, `Emote3`, `Emote4`, `EmoteDelay1`, `EmoteDelay2`, `EmoteDelay3`, `EmoteDelay4`, `VerifiedBuild`) VALUES
(79432, 0, 0, 0, 0, 0, 0, 0, 0, 55142); -- It's About Time

DELETE FROM `quest_request_items` WHERE `ID`=79432;
INSERT INTO `quest_request_items` (`ID`, `EmoteOnComplete`, `EmoteOnIncomplete`, `EmoteOnCompleteDelay`, `EmoteOnIncompleteDelay`, `CompletionText`, `VerifiedBuild`) VALUES
(79432, 396, 396, 0, 0, 'The other infinites jeopardize our mission. We cannot let them wreak havoc on the timeways.', 0); -- It's About Time

DELETE FROM `quest_objectives` WHERE `ID` IN (446716 /*446716*/, 446695 /*446695*/, 447709 /*447709*/);
INSERT INTO `quest_objectives` (`ID`, `QuestID`, `Type`, `Order`, `StorageIndex`, `ObjectID`, `Amount`, `Flags`, `Flags2`, `ProgressBarWeight`, `Description`, `VerifiedBuild`) VALUES
(446716, 79432, 1, 2, 1, 213631, 1, 2, 1, 0, 'Archaios\'s artifact taken', 55142), -- 446716
(446695, 79432, 0, 1, 0, 217564, 1, 1, 0, 0, '', 55142), -- 446695
(447709, 79432, 0, 0, 4, 219712, 1, 4, 0, 0, 'Investigate the Unstable Rift', 55142); -- 447709

DELETE FROM `quest_visual_effect` WHERE (`Index`=0 AND `ID` IN (446716,446695,447709));
INSERT INTO `quest_visual_effect` (`ID`, `Index`, `VisualEffect`, `VerifiedBuild`) VALUES
(446716, 0, 25088, 55142),
(446695, 0, 2101, 55142),
(447709, 0, 25456, 55142);

DELETE FROM `creature_queststarter` WHERE (`id`=216591 AND `quest`=79432);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(216591, 79432, 55142); -- It's About Time offered by Eternus

DELETE FROM `creature_questender` WHERE (`id`=216591 AND `quest`=79432);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES
(216591, 79432, 55142); -- It's About Time ended by Eternus




SET @CGUID := 9004166;
SET @NPCTEXTID := 590114;

-- Creature
DELETE FROM `creature` WHERE `guid` = @CGUID+0;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`, `size`) VALUES
(@CGUID+0, 216946, 2444, 13862, 13862, '0', 23362, 0, 0, 0, 63.30729293823242187, -904.3836669921875, 836.5286865234375, 2.599628686904907226, 120, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 55261, -1); -- Chromie (Area: Valdrakken - Difficulty: 0) CreateObject1

-- Template
UPDATE `creature_template` SET `faction`=35, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=256, `unit_flags2`=2048 WHERE `entry`=216946; -- Chromie

-- Gossip
DELETE FROM `creature_template_gossip` WHERE (`CreatureID`=216946 AND `MenuID` = 33614);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
(216946, 33614, 55261); -- Chromie

DELETE FROM `npc_text` WHERE `ID` BETWEEN @NPCTEXTID+0 AND @NPCTEXTID+1;
INSERT INTO `npc_text` (`ID`, `Probability0`, `Probability1`, `Probability2`, `Probability3`, `Probability4`, `Probability5`, `Probability6`, `Probability7`, `BroadcastTextId0`, `BroadcastTextId1`, `BroadcastTextId2`, `BroadcastTextId3`, `BroadcastTextId4`, `BroadcastTextId5`, `BroadcastTextId6`, `BroadcastTextId7`, `VerifiedBuild`) VALUES
(@NPCTEXTID+0, 1, 0, 0, 0, 0, 0, 0, 0, 261092, 0, 0, 0, 0, 0, 0, 0, 55261), -- 216946 (Chromie)
(@NPCTEXTID+1, 1, 0, 0, 0, 0, 0, 0, 0, 254760, 0, 0, 0, 0, 0, 0, 0, 55261); -- 216946 (Chromie)

DELETE FROM `gossip_menu` WHERE (`MenuID`=34417 AND `TextID`=@NPCTEXTID+0) OR (`MenuID`=33614 AND `TextID`=@NPCTEXTID+1);
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES
(34417, @NPCTEXTID+0, 55261), -- 216946 (Chromie)
(33614, @NPCTEXTID+1, 55261); -- 216946 (Chromie)

DELETE FROM `gossip_menu_option` WHERE (`OptionID`=0 AND `MenuID` IN (33614,34417));
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`, `ActionMenuID`, `ActionPoiID`, `GossipNpcOptionID`, `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextID`, `SpellID`, `OverrideIconID`, `VerifiedBuild`) VALUES
(33614, 121716, 0, 0, 'What was your Visage Day like?', 0, 0, 0, 34417, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 55261),
(34417, 122651, 0, 0, 'I want to ask about something else.', 149761, 0, 0, 33614, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 55261);

-- Phasing
DELETE FROM `phase_name` WHERE `ID` = 23362;
INSERT INTO `phase_name` (`ID`, `Name`) VALUES
(23362, 'Cosmetic - See Chromie in Valdrakken');

DELETE FROM `phase_area` WHERE (`PhaseId` = 23362 AND `AreaId` = 13862);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(13862, 23362, 'Cosmetic - See Chromie in Valdrakken');

DELETE FROM `conditions` WHERE (`SourceTypeOrReferenceId`=26 AND `SourceGroup` = 23362 AND `SourceEntry` = 0);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`) VALUES
(26, 23362, 0, 0, 0, 47, 0, 79511, 64, 0, 1, 'Apply Phase 23362 if Quest 79511 is not rewarded');

-- Quest
DELETE FROM `quest_offer_reward` WHERE `ID`=79511;
INSERT INTO `quest_offer_reward` (`ID`, `Emote1`, `Emote2`, `Emote3`, `Emote4`, `EmoteDelay1`, `EmoteDelay2`, `EmoteDelay3`, `EmoteDelay4`, `RewardText`, `VerifiedBuild`) VALUES
(79511, 1, 0, 0, 0, 0, 0, 0, 0, 'I take it Chromie sent you?', 55261); -- Identity Crisis

DELETE FROM `quest_details` WHERE `ID` IN (79512 /*Who Are You?*/, 79511 /*Identity Crisis*/);
INSERT INTO `quest_details` (`ID`, `Emote1`, `Emote2`, `Emote3`, `Emote4`, `EmoteDelay1`, `EmoteDelay2`, `EmoteDelay3`, `EmoteDelay4`, `VerifiedBuild`) VALUES
(79512, 1, 0, 0, 0, 0, 0, 0, 0, 55261), -- Who Are You?
(79511, 1, 0, 0, 0, 0, 0, 0, 0, 55261); -- Identity Crisis

DELETE FROM `creature_queststarter` WHERE (`id`=216945 AND `quest`=79512) OR (`id`=216946 AND `quest`=79511);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES
(216945, 79512, 55261), -- Who Are You? offered by Scalecommander Emberthal
(216946, 79511, 55261); -- Identity Crisis offered by Chromie

DELETE FROM `creature_questender` WHERE (`id`=216945 AND `quest`=79511);
INSERT INTO `creature_questender` (`id`, `quest`, `VerifiedBuild`) VALUES
(216945, 79511, 55261); -- Identity Crisis ended by Scalecommander Emberthal

-- Difficulty
DELETE FROM `creature_template_difficulty` WHERE `DifficultyID`=0 AND `Entry` = 216946;
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `HealthScalingExpansion`, `HealthModifier`, `ManaModifier`, `CreatureDifficultyID`, `TypeFlags`, `TypeFlags2`) VALUES
(216946, 0, 9, 50, 1, 266436, 4, 0); -- Chromie

UPDATE `creature_template_difficulty` SET `ContentTuningID`=2828, `StaticFlags1`=268435456, `VerifiedBuild`=55261 WHERE (`Entry`=216946 AND `DifficultyID`=0); -- 216946 (Chromie) - CanSwim

-- Model
DELETE FROM `creature_model_info` WHERE `DisplayID` IN (110931, 113618, 109539, 112097, 112098, 112099, 113211, 111286, 112498, 109269, 115816, 107355, 116193, 113871, 110679, 117912, 116140, 112141);
INSERT INTO `creature_model_info` (`DisplayID`, `BoundingRadius`, `CombatReach`, `DisplayID_Other_Gender`, `VerifiedBuild`) VALUES
(110931, 0.722000002861022949, 2, 0, 55261),
(113618, 0.305999994277954101, 1.5, 0, 55261),
(109539, 0.5, 1, 0, 55261),
(112097, 1.404254555702209472, 2.40000009536743164, 0, 55261),
(112098, 1.404254555702209472, 2.40000009536743164, 0, 55261),
(112099, 1.404254555702209472, 2.40000009536743164, 0, 55261),
(113211, 0.305999994277954101, 1.5, 0, 55261),
(111286, 0.382999986410140991, 1.5, 0, 55261),
(112498, 0.700812160968780517, 0.900000035762786865, 0, 55261),
(109269, 0.347000002861022949, 1.5, 0, 55261),
(115816, 0.347222000360488891, 1.5, 0, 55261),
(107355, 0.347000002861022949, 2, 0, 55261),
(116193, 0.305999994277954101, 1.5, 0, 55261),
(113871, 0.236000001430511474, 1.5, 0, 55261),
(110679, 1.930850028991699218, 3.300000190734863281, 0, 55261),
(117912, 0.372000008821487426, 1.5, 0, 55261),
(116140, 0.305999994277954101, 1.5, 0, 55261),
(112141, 0.722000002861022949, 2, 0, 55261);

UPDATE `creature_model_info` SET `CombatReach`=3.60000014305114746, `VerifiedBuild`=55261 WHERE `DisplayID` IN (104928, 104297, 104920, 104935, 104300, 104937, 106902, 104934, 106911, 103055, 104924, 104930, 106903, 104927, 104925, 104929, 104923, 104939, 104933, 106905, 104921);
UPDATE `creature_model_info` SET `VerifiedBuild`=55261 WHERE `DisplayID` IN (105893, 110847, 110484, 105526, 109518, 106214, 109519, 108179, 108178, 108919, 109251, 109522, 108384, 108622, 108466, 108506, 108482, 108498, 108524, 110511, 108583, 108002, 109302, 102031, 67362, 108157, 99952, 102033, 108402, 108464, 109253, 108992, 105828, 107950, 109005, 110241, 108283, 109383, 104555, 107270, 108995, 110350, 107065, 105394, 108164, 110647, 102647, 105851, 106110, 108377, 108267, 108074, 102140, 108135, 105529, 104178, 108158, 110240, 105530, 61527, 108249, 109602, 108657, 106705, 108656, 106707, 108988, 105806, 100847, 104451, 105104, 26375, 108418, 106449, 106946, 106416, 106945, 109473, 70287, 113531, 39211, 110344, 113479, 24746, 107069, 34533, 103825, 77405, 107700, 112292, 61892, 108433, 109669, 116648, 102365, 117262, 106489, 106487, 23439, 112587, 104171, 96559, 111423, 102777, 16412, 109442, 63703, 106310, 28282, 28111, 104323, 106253, 101210, 16431, 115766, 104949, 27823, 112578, 103479, 105218, 106426, 23767, 103828, 42722, 108832, 42720, 110930, 109849, 11686, 105601, 102137, 106367, 77687, 102202, 77406, 77690, 77407, 104308, 106494, 112165, 47166);
UPDATE `creature_model_info` SET `CombatReach`=3, `VerifiedBuild`=55261 WHERE `DisplayID` IN (110855, 108319, 108987);
UPDATE `creature_model_info` SET `CombatReach`=2.40000009536743164, `VerifiedBuild`=55261 WHERE `DisplayID` IN (107026, 106358, 108117, 107027, 107025, 107024, 107023);
UPDATE `creature_model_info` SET `BoundingRadius`=0.476598590612411499, `VerifiedBuild`=55261 WHERE `DisplayID`=107462;
UPDATE `creature_model_info` SET `BoundingRadius`=0.382999986410140991, `CombatReach`=1.5, `VerifiedBuild`=55261 WHERE `DisplayID` IN (109423, 105894);
UPDATE `creature_model_info` SET `BoundingRadius`=1.755318164825439453, `CombatReach`=3, `VerifiedBuild`=55261 WHERE `DisplayID`=108993;
UPDATE `creature_model_info` SET `BoundingRadius`=0.388999998569488525, `CombatReach`=1.5, `VerifiedBuild`=55261 WHERE `DisplayID`=106467;
UPDATE `creature_model_info` SET `BoundingRadius`=0.498713225126266479, `CombatReach`=0.5, `VerifiedBuild`=55261 WHERE `DisplayID`=102843;
UPDATE `creature_model_info` SET `BoundingRadius`=1, `CombatReach`=9, `VerifiedBuild`=55261 WHERE `DisplayID`=109286;
UPDATE `creature_model_info` SET `CombatReach`=3.300000190734863281, `VerifiedBuild`=55261 WHERE `DisplayID` IN (107394, 102218, 102181, 102746, 102940);
UPDATE `creature_model_info` SET `BoundingRadius`=1.014431953430175781, `CombatReach`=2.5, `VerifiedBuild`=55261 WHERE `DisplayID` IN (55233, 106811, 57521);
UPDATE `creature_model_info` SET `BoundingRadius`=1.014432311058044433, `CombatReach`=2.5, `VerifiedBuild`=55261 WHERE `DisplayID`=57522;
UPDATE `creature_model_info` SET `BoundingRadius`=1.755318164825439453, `CombatReach`=2.999999761581420898, `VerifiedBuild`=55261 WHERE `DisplayID` IN (107975, 106448);
UPDATE `creature_model_info` SET `BoundingRadius`=1.05910801887512207, `VerifiedBuild`=55261 WHERE `DisplayID`=100197;
UPDATE `creature_model_info` SET `BoundingRadius`=1.343548059463500976, `CombatReach`=4.024999618530273437, `VerifiedBuild`=55261 WHERE `DisplayID`=106908;
UPDATE `creature_model_info` SET `BoundingRadius`=1.141789793968200683, `VerifiedBuild`=55261 WHERE `DisplayID`=106788;
UPDATE `creature_model_info` SET `BoundingRadius`=0.819302201271057128, `CombatReach`=1, `VerifiedBuild`=55261 WHERE `DisplayID`=104133;
UPDATE `creature_model_info` SET `CombatReach`=2.699999809265136718, `VerifiedBuild`=55261 WHERE `DisplayID`=110235;
UPDATE `creature_model_info` SET `BoundingRadius`=1.755318164825439453, `CombatReach`=3.000000238418579101, `VerifiedBuild`=55261 WHERE `DisplayID` IN (106565, 106646);
UPDATE `creature_model_info` SET `BoundingRadius`=1.014432072639465332, `CombatReach`=2.5, `VerifiedBuild`=55261 WHERE `DisplayID`=55561;
UPDATE `creature_model_info` SET `BoundingRadius`=2.670000076293945312, `CombatReach`=7, `VerifiedBuild`=55261 WHERE `DisplayID`=107010;
UPDATE `creature_model_info` SET `BoundingRadius`=0.228800013661384582, `CombatReach`=1.649999976158142089, `VerifiedBuild`=55261 WHERE `DisplayID`=109761;
UPDATE `creature_model_info` SET `BoundingRadius`=0.509302020072937011, `VerifiedBuild`=55261 WHERE `DisplayID`=109141;
UPDATE `creature_model_info` SET `BoundingRadius`=0.500797629356384277, `CombatReach`=1, `VerifiedBuild`=55261 WHERE `DisplayID` IN (105600, 105602);
UPDATE `creature_model_info` SET `BoundingRadius`=25.37703704833984375, `VerifiedBuild`=55261 WHERE `DisplayID`=92146;
UPDATE `creature_model_info` SET `BoundingRadius`=0.305999994277954101, `CombatReach`=1.5, `VerifiedBuild`=55261 WHERE `DisplayID`=106481;
UPDATE `creature_model_info` SET `BoundingRadius`=22.45141220092773437, `VerifiedBuild`=55261 WHERE `DisplayID`=108926;
UPDATE `creature_model_info` SET `BoundingRadius`=1.343548059463500976, `CombatReach`=4.02500009536743164, `VerifiedBuild`=55261 WHERE `DisplayID`=106647;
UPDATE `creature_model_info` SET `BoundingRadius`=1.584961533546447753, `CombatReach`=2.10000014305114746, `VerifiedBuild`=55261 WHERE `DisplayID`=103508;

SET @CGUID := 7001350;
SET @SPAWNGROUPID := 1255;

-- Creature
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+16;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curHealthPct`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`, `size`) VALUES
(@CGUID+0, 134157, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1084.296875, 2623.63623046875, 810.16400146484375, 5.601494789123535156, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Warrior (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+1, 133943, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1069.2552490234375, 2659.318603515625, 811.0338134765625, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Minion of Zul (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: 269935 - Bound by Shadow)
(@CGUID+2, 134157, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1077.2100830078125, 2627.87158203125, 810.1683349609375, 5.369873523712158203, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Warrior (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+3, 134174, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1082.3853759765625, 2656.47216796875, 810.1707763671875, 5.919741153717041015, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Witch Doctor (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+4, 133943, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1070.8541259765625, 2661.4306640625, 810.9376220703125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Minion of Zul (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: 269935 - Bound by Shadow)
(@CGUID+5, 133943, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1070.96533203125, 2658.845458984375, 811.05706787109375, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Minion of Zul (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: 269935 - Bound by Shadow)
(@CGUID+6, 133943, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1073.2569580078125, 2661.0244140625, 810.9376220703125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Minion of Zul (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: 269935 - Bound by Shadow)
(@CGUID+7, 134158, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1040.140625, 2637.28564453125, 810.1707763671875, 4.446186065673828125, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Champion (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+8, 134157, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1080.8350830078125, 2669.678955078125, 810.1707763671875, 5.609979629516601562, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Warrior (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+9, 134174, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1055.0989990234375, 2663.8134765625, 810.1707763671875, 4.17470407485961914, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Witch Doctor (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+10, 134157, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1060.3385009765625, 2666.329833984375, 810.1707763671875, 4.488315582275390625, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Warrior (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+11, 133943, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1072.842041015625, 2659.14404296875, 810.9376220703125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Minion of Zul (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: 269935 - Bound by Shadow)
(@CGUID+12, 134158, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1082.53125, 2662.041748046875, 810.1707763671875, 6.203471660614013671, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Champion (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+13, 134157, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1086.7742919921875, 2650.109375, 810.1707763671875, 5.949019432067871093, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Warrior (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+14, 134157, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1049.03125, 2660.283935546875, 810.1707763671875, 3.988105535507202148, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Shadow-Borne Warrior (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )
(@CGUID+15, 133943, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1068.4930419921875, 2661.75341796875, 810.9376220703125, 0, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1), -- Minion of Zul (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: 269935 - Bound by Shadow)
(@CGUID+16, 134174, 1762, 9526, 9526, '23,8,2', 0, 0, 0, 0, -1046.3211669921875, 2635.2509765625, 810.1707763671875, 4.599164962768554687, 7200, 0, 0, 100, 0, NULL, NULL, NULL, NULL, 54988, -1); -- Shadow-Borne Witch Doctor (Area: Kings' Rest - Difficulty: Mythic) CreateObject2 (Auras: )

-- Update
UPDATE `gameobject_template` SET `AIName` = '', `ScriptName` = 'go_kings_rest_serpentine_seal' WHERE `entry` = 288466;

UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_kings_rest_shadow_of_zul' WHERE `entry` = 137020;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_kings_rest_shadow_borne_warrior' WHERE `entry` = 134157;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_kings_rest_shadow_borne_witch_doctor' WHERE `entry` = 134174;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_kings_rest_shadow_borne_champion' WHERE `entry` = 134158;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_kings_rest_minion_of_zul' WHERE `entry` = 133943;

UPDATE `creature` SET `StringId` = 'DontForceRespawn' WHERE `guid` IN (7000965, 7000970, 7000972, 7000976, 7000969, 7000967, 7000968, 7000981, 7000975, 7000966, 7000971, 7000982, 7000978, 7000979, 7000983, 7000973, 7000977, 7000980, 7000974, 7000993, 7001001, 7000997, 7000990, 7001002, 7000989, 7000998, 7000985);
UPDATE `creature` SET `StringId` = 'TempleEvent' WHERE `guid` IN (@CGUID+1, @CGUID+4, @CGUID+5, @CGUID+6, @CGUID+11, @CGUID+15);

-- Delete SAI Script (converted to C++)
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` = 133943;

-- Creature Text
DELETE FROM `creature_text` WHERE `CreatureID` = 137020 AND `GroupID` = 0;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(137020, 0, 0, 'Dis temple is under G\'huun\'s power!', 14, 0, 100, 0, 0, 106154, 151859, 0, 'Shadow of Zul to Player');

-- AreaTrigger
DELETE FROM `areatrigger_template` WHERE (`Id`=17933 AND `IsCustom`=0);
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `VerifiedBuild`) VALUES
(17933, 0, 0, 55165);

DELETE FROM `areatrigger_create_properties` WHERE (`Id`=13339 AND `IsCustom`=0);
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `TimeToTarget`, `TimeToTargetScale`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `VerifiedBuild`) VALUES
(13339, 0, 17933, 0, 1040, 0, 0, 0, 0, -1, 0, 0, 6283, 30000, 4, 1.5, 1.5, 5, 5, 1, 1, 0, 0, 55165); -- Spell: 269931 (Gust Slash)

DELETE FROM `areatrigger_create_properties_orbit` WHERE (`AreaTriggerCreatePropertiesId`=13339 AND `IsCustom`=0);
INSERT INTO `areatrigger_create_properties_orbit` (`AreaTriggerCreatePropertiesId`, `IsCustom`, `StartDelay`, `CircleRadius`, `BlendFromRadius`, `InitialAngle`, `ZOffset`, `CounterClockwise`, `CanLoop`, `VerifiedBuild`) VALUES
(13339, 0, 0, 5, 0, 0, 0, 0, 1, 55165); -- Spell: 269931 (Gust Slash)

UPDATE `areatrigger_create_properties` SET `ScriptName` = 'at_kings_rest_gust_slash' WHERE (`Id`=13339 AND `IsCustom`=0);

-- Spawngroups
DELETE FROM `spawn_group_template` WHERE `groupId` = @SPAWNGROUPID;
INSERT INTO `spawn_group_template` (`groupId`, `groupName`, `groupFlags`) VALUES
(@SPAWNGROUPID, 'Kings Rest - Zul activates temple spawns', 0x04);

DELETE FROM `spawn_group` WHERE `groupId` = @SPAWNGROUPID AND `spawnType`= 0;
INSERT INTO `spawn_group` (`groupId`, `spawnType`, `spawnId`) VALUES
(@SPAWNGROUPID, 0, @CGUID+0),
(@SPAWNGROUPID, 0, @CGUID+1),
(@SPAWNGROUPID, 0, @CGUID+2),
(@SPAWNGROUPID, 0, @CGUID+3),
(@SPAWNGROUPID, 0, @CGUID+4),
(@SPAWNGROUPID, 0, @CGUID+5),
(@SPAWNGROUPID, 0, @CGUID+6),
(@SPAWNGROUPID, 0, @CGUID+7),
(@SPAWNGROUPID, 0, @CGUID+8),
(@SPAWNGROUPID, 0, @CGUID+9),
(@SPAWNGROUPID, 0, @CGUID+10),
(@SPAWNGROUPID, 0, @CGUID+11),
(@SPAWNGROUPID, 0, @CGUID+12),
(@SPAWNGROUPID, 0, @CGUID+13),
(@SPAWNGROUPID, 0, @CGUID+14),
(@SPAWNGROUPID, 0, @CGUID+15),
(@SPAWNGROUPID, 0, @CGUID+16);

-- Condition (Targets)
DELETE FROM `conditions` WHERE (`SourceTypeOrReferenceId` = 13) AND (`SourceEntry` IN (270692));
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `Comment`) VALUES 
(13, 1, 270692, 0, 0, 31, 0, 3, 137652, 0, '', 0, 'Potential target of the spell is creature, entry is Untainted Guard Spirit (137652)'),
(13, 1, 270692, 0, 0, 58, 0, 0, 0, 0, 'DontForceRespawn', 1, 'Potential target of the spell is not DontForceRespawn'),
(13, 1, 270692, 0, 1, 31, 0, 3, 137650, 0, '', 0, 'Potential target of the spell is creature, entry is Untainted Spirit (137650)'),
(13, 1, 270692, 0, 1, 58, 0, 0, 0, 0, 'DontForceRespawn', 1, 'Potential target of the spell is not DontForceRespawn'),
(13, 1, 270692, 0, 2, 31, 0, 3, 137651, 0, '', 0, 'Potential target of the spell is creature, entry is Untainted Priest Spirit (137651)'),
(13, 1, 270692, 0, 2, 58, 0, 0, 0, 0, 'DontForceRespawn', 1, 'Potential target of the spell is not DontForceRespawn');

-- SAI
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` IN (137652, 137650, 137651);
DELETE FROM `smart_scripts` WHERE `source_type` = 0 AND `entryOrGuid` IN (137652, 137650, 137651);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(137652, 0, 0, 0, '', 8, 0, 100, 0, 270692, 0, 0, 0, 41, 3000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On spell  270692 hit - Self: Despawn in 3 s'),
(137650, 0, 0, 0, '', 8, 0, 100, 0, 270692, 0, 0, 0, 41, 3000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On spell  270692 hit - Self: Despawn in 3 s'),
(137651, 0, 0, 0, '', 8, 0, 100, 0, 270692, 0, 0, 0, 41, 3000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'On spell  270692 hit - Self: Despawn in 3 s');

-- Instance
DELETE FROM `instance_template` WHERE `map`=1862;
INSERT INTO `instance_template` (`map`, `parent`, `script`) VALUES
(1862, 0, 'instance_waycrest_manor');

-- Creature
DELETE FROM `creature` WHERE `guid`=7001260;

UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131824 AND `DifficultyID`=0); -- 131824 (Sister Solena) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131825 AND `DifficultyID`=0); -- 131825 (Sister Briar) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=2, `LevelScalingDeltaMax`=2, `ContentTuningID`=500, `StaticFlags1`=805306624, `VerifiedBuild`=55165 WHERE (`Entry`=131823 AND `DifficultyID`=0); -- 131823 (Sister Malady) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=500, `VerifiedBuild`=55165 WHERE (`Entry`=132361 AND `DifficultyID`=0); -- 132361 (Focusing Iris) - 

UPDATE `creature_template` SET `ScriptName` = 'boss_sister_briar' WHERE `entry` = 131825;
UPDATE `creature_template` SET `ScriptName` = 'boss_sister_malady' WHERE `entry` = 131823;
UPDATE `creature_template` SET `ScriptName` = 'boss_sister_solena' WHERE `entry` = 131824;

-- Summon Groups
DELETE FROM `creature_summon_groups` WHERE `summonerId`=131824;
INSERT INTO `creature_summon_groups` (`summonerId`, `summonerType`, `groupId`, `entry`, `position_x`, `position_y`, `position_z`, `orientation`, `summonType`, `summonTime`, `Comment`) VALUES
(131824, 0, 0, 132361, -574.691, -151.559, 235.268, 4.71697, 8, 0, 'Heartsbane Triad - Group 0 - Focusing Iris');

-- Texts
DELETE FROM `creature_text` WHERE `CreatureID` IN (131823, 131824, 131825);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
-- SAY AGGRO
(131823, 0, 0, 'I\'ll carve screams from your flesh!', 14, 0, 100, 0, 0, 97511, 144939, 0, 'Sister Malady to Player'),
(131824, 0, 0, 'You have no power here!', 14, 0, 100, 0, 0, 97519, 144931, 0, 'Sister Solena to Player'),
(131825, 0, 0, 'Such supple bones!', 14, 0, 100, 0, 0, 97503, 144951, 0, 'Sister Briar to Player'),
-- SAY_CLAIM_THE_IRIS
(131823, 1, 0, 'You\'ve had your turn!', 14, 0, 100, 0, 0, 97515, 144944, 0, 'Sister Malady'),
(131824, 1, 0, 'Give me the iris, you fool! Only I can focus its power!', 14, 0, 100, 0, 0, 97523, 144935, 0, 'Sister Solena'),
(131825, 1, 0, 'Release the iris to me, sister!', 14, 0, 100, 0, 0, 97509, 144962, 0, 'Sister Briar'),
-- SAY_SLAY
(131823, 3, 0, 'Such lovely agony!', 14, 0, 50, 0, 0, 97513, 144941, 0, 'Sister Malady'),
(131823, 3, 1, 'Your torment is far from over.', 14, 0, 50, 0, 0, 97514, 144942, 0, 'Sister Malady'),
(131824, 3, 0, 'Such a fragile soul!', 14, 0, 50, 0, 0, 97520, 144932, 0, 'Sister Solena'),
(131824, 3, 1, 'Your souls belong to me... forever.', 14, 0, 50, 0, 0, 97522, 144934, 0, 'Sister Solena'),
(131825, 3, 0, 'I love the sound of snapping limbs.', 14, 0, 50, 0, 0, 97504, 144955, 0, 'Sister Briar'),
(131825, 3, 1, 'Your skulls will make lovely ornaments.', 14, 0, 50, 0, 0, 97506, 144958, 0, 'Sister Briar'),
-- SAY_DEATH
(131823, 4, 0, 'My body cracks... but my curse... shall linger...', 14, 0, 100, 0, 0, 97512, 144940, 0, 'Sister Malady to Player'),
(131824, 4, 0, 'Death will not silence me...', 14, 0, 100, 0, 0, 97521, 144933, 0, 'Sister Solena to Player'),
(131825, 4, 0, 'My roots will sprout again...', 14, 0, 100, 0, 0, 97509, 144956, 0, 'Sister Briar to Player'),
-- SAY_DIRE_RITUAL_ALERT
(131823, 5, 0, '|TInterface\ICONS\SPELL_SHADOW_ANTISHADOW:20|t %s begins to cast |cFFFF0404|Hspell:260773|h[Dire Ritual]|h|r!', 41, 0, 100, 0, 0, 97526, 151946, 0, 'Sister Malady'),
(131824, 5, 0, '|TInterface\ICONS\SPELL_SHADOW_ANTISHADOW:20|t %s begins to cast |cFFFF0404|Hspell:260773|h[Dire Ritual]|h|r!', 41, 0, 100, 0, 0, 97526, 151946, 0, 'Sister Solena'),
(131825, 5, 0, '|TInterface\ICONS\SPELL_SHADOW_ANTISHADOW:20|t %s begins to cast |cFFFF0404|Hspell:260773|h[Dire Ritual]|h|r!', 41, 0, 100, 0, 0, 97526, 151946, 0, 'Sister Briar'),
-- SAY_DIRE_RITUAL
(131823, 6, 0, 'Rotted flesh and screams so dire, making music like a choir!', 14, 0, 100, 0, 0, 97517, 144946, 0, 'Sister Malady'),
(131824, 6, 0, 'Blackened soul, the pitch of night, lay to waste all those in sight!', 14, 0, 100, 0, 0, 97526, 144938, 0, 'Sister Solena'),
(131825, 6, 0, 'Powers high and spirits low, grant my thistles room to grow!', 14, 0, 100, 0, 0, 97515, 144944, 0, 'Sister Briar'),
-- Sister Malady
(131823, 2, 0, 'Your destruction is at hand!', 14, 0, 50, 0, 0, 97516, 144945, 0, 'Sister Malady'),
(131823, 2, 1, 'You bear the black mark!', 14, 0, 50, 0, 0, 97518, 144959, 0, 'Sister Malady'),
-- Sister Solena
(131824, 2, 0, 'Dance, puppet! Dance!', 14, 0, 50, 0, 0, 97524, 144936, 0, 'Sister Solena'),
(131824, 2, 1, 'You belong to me now!', 14, 0, 50, 0, 0, 97525, 144937, 0, 'Sister Solena'),
-- Sister Briar
(131825, 2, 0, 'My thorns will rip you to shreds!', 14, 0, 50, 0, 0, 97508, 144961, 0, 'Sister Briar to Player'),
(131825, 2, 1, 'The more you struggle, the more you\'ll bleed!', 14, 0, 50, 0, 0, 97507, 144960, 0, 'Sister Briar to Player');

-- Areatrigger
DELETE FROM `areatrigger_create_properties` WHERE (`IsCustom`=0 AND `Id` IN (13196, 13179, 13177));
INSERT INTO `areatrigger_create_properties` (`Id`, `IsCustom`, `AreaTriggerId`, `IsAreatriggerCustom`, `Flags`, `MoveCurveId`, `ScaleCurveId`, `MorphCurveId`, `FacingCurveId`, `AnimId`, `AnimKitId`, `DecalPropertiesId`, `TimeToTarget`, `TimeToTargetScale`, `Shape`, `ShapeData0`, `ShapeData1`, `ShapeData2`, `ShapeData3`, `ShapeData4`, `ShapeData5`, `ShapeData6`, `ShapeData7`, `ScriptName`, `VerifiedBuild`) VALUES
(13196, 0, 17807, 0, 4, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 200, 200, 0, 0, 0, 0, 0, 0, 'at_heartsbane_triad_aura_of_thorns', 52485), -- Spell: 268122 (Aura of Thorns)
(13179, 0, 17791, 0, 4, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 200, 200, 0, 0, 0, 0, 0, 0, 'at_heartsbane_triad_aura_of_dread', 52485), -- Spell: 268088 (Aura of Dread)
(13177, 0, 17789, 0, 4, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 200, 200, 0, 0, 0, 0, 0, 0, 'at_heartsbane_triad_aura_of_apathy', 52485); -- Spell: 268077 (Aura of Apathy)

DELETE FROM `areatrigger_template` WHERE (`IsCustom`=0 AND `Id` IN (17807, 17791, 17789));
INSERT INTO `areatrigger_template` (`Id`, `IsCustom`, `Flags`, `VerifiedBuild`) VALUES
(17807, 0, 0, 52485),
(17791, 0, 0, 52485),
(17789, 0, 0, 52485);

-- Spells
DELETE FROM `spell_script_names` WHERE `spell_id` IN (260741, 260852, 260854, 260907, 260923, 260703, 260773, 268077, 268088, 268085, 268122);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(260741, 'spell_heartsbane_triad_jagged_nettles'),
(260852, 'spell_heartsbane_triad_claim_the_iris'),
(260854, 'spell_heartsbane_triad_drop_the_iris'),
(260907, 'spell_heartsbane_triad_soul_manipulation_selector'),
(260923, 'spell_heartsbane_triad_soul_manipulation_periodic'),
(260703, 'spell_heartsbane_triad_unstable_runic_mark'),
(260773, 'spell_heartsbane_triad_dire_ritual'),
(268077, 'spell_heartsbane_triad_aura_of_apathy'),
(268088, 'spell_heartsbane_triad_aura_of_dread'),
(268085, 'spell_heartsbane_triad_aura_of_dread_movement_check'),
(268122, 'spell_heartsbane_triad_aura_of_thorns');

DELETE FROM `spell_proc` WHERE `SpellId` IN (77616);
INSERT INTO `spell_proc` (`SpellId`,`SchoolMask`,`SpellFamilyName`,`SpellFamilyMask0`,`SpellFamilyMask1`,`SpellFamilyMask2`,`SpellFamilyMask3`,`ProcFlags`,`ProcFlags2`,`SpellTypeMask`,`SpellPhaseMask`,`HitMask`,`AttributesMask`,`DisableEffectsMask`,`ProcsPerMinute`,`Chance`,`Cooldown`,`Charges`) VALUES
(77616,0x00,0,0x00000000,0x00000000,0x00000000,0x00000000,0x0,0x0,0x7,0x1,0x0,0x0,0x0,0,0,0,0); -- Dark Simulacrum

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dk_dark_simulacrum','spell_dk_dark_simulacrum_buff');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(77606,'spell_dk_dark_simulacrum'),
(77616,'spell_dk_dark_simulacrum_buff');