-- Wandering Isle fixes and changes

-- NPC: 54130 Amberleaf Scamp
-- NPC: 54131 Fe-Feng Hozen
-- NPC: 57164 Fe-Feng Leaper

-- NPC: 55477 Ji Firepaw at the farm
-- NPC: 55021 Jojo Ironbrow at first house
-- NPC: 57638 Jojo Ironbrow (spell spawn first house)
-- NPC: 55478 Jojo Ironbrow at farmstead
-- NPC: 57669 Jojo Ironbrow (spell spawn farmstead)
-- NPC: 55585 Jojo Ironbrow at Morning Breeze
-- NPC: 64939 Lamplighter Sunny
-- NPC: 55506 Raggis
-- NPC: 65467 Mesmerized Onlooker
-- NPC: 56394 Mesmerized Child
-- NPC: 56393 Mesmerized Onlooker
-- NPC: 55015 Whitefeather Crane
-- NPC: 57620 Whittler Dewei
-- NPC: 57779 Huo
-- NPC: 54787 Huo (spawned)
-- NPC: 54135 Master Li Fei
-- NPC: 54734 Master Li Fei (challenge npc)
-- NPC: 54958 Huo (spawned)
-- NPC: 57720 Ji Firepaw
-- NPC: 57721 Aysa Cloudsinger
-- NPC: 54786 Master Shang Xi
-- NPC: 57132 Wu-song Villager
-- NPC: 55019 Tushui Monk
-- NPC: 65468 Tushui Monk
-- NPC: 54993 Balance Pole
-- NPC: 57431 Balance Pole
-- NPC: 55083 Balance Pole
-- NPC: 56869 Bunny landing impact
-- NPC: 55292 Fang-she
-- NPC: 54976 Barbed Ray
-- NPC: 55020 Old Man Liang
-- NPC: 54975 Aysa Cloudsinger
-- NPC: 65493 Shu
-- NPC: 55213 Shu (spawned by spell)
-- NPC: 60488 Bunny water spout summon
-- NPC: 66941 Bunny water spout summon (Farmstead)
-- NPC: 57207 Spawned Yak
-- NPC: 57208 Spawned Cart
-- NPC: 57709 Nourished Yak
-- NPC: 57710 Cart
-- NPC: 57712 Cart Tender
-- NPC: 55539 Wugou
-- NPC: 60916 Wugou spawn
-- NPC: 55556 Shu at the Farmstead
-- NPC: 55558 Shu spawn at the Farmstead

-- GO: 209584 Ancient Clam
-- GO 209626 Break Gong

-- Spell: 108786 Summon Stack of Reeds
-- Spell: 108798 Jojo Headbash Stack of reeds impact
-- Spell: 108808 Summon Jojo Ironbrow
-- Spell: 102522 Fan The Flames
-- Spell: 103071 Forcecast Rock Jump A
-- Spell: 103069 Rock Jump A -- this gets the scriptname
-- Spell: 103070 Rock Jump B -- this gets the scriptname
-- Spell: 103072 Forcecast Rock Jump B
-- Spell: 103077 Rock Jump C -- this gets the scriptname
-- Spell: 103078 Forcecast Rock Jump C
-- Spell: 116810 Summon Bunny Water Spout -- I can't get this one to work so it is not used for now
-- Spell: 117033 Shu Jump to front right
-- Spell: 117034 Shu Jump to front left
-- Spell: 117035 Shu Jump to back right
-- Spell: 117036 Shu Jump to back left

-- Quest: 29662 Stronger Than Reeds
-- Quest: 29768 Missing Mallet
-- Quest: 29771 Stronger Than Wood
-- Quest: 29421 Only the Worthy Shall Pass
-- Quest: 29422 Huo, the Spirit of Fire
-- Quest: 29423 The Passion of Shen-zin Su
-- Quest: 29521 The Singing Pools
-- Quest: 29661 The Lesson of the Dry Fur
-- Quest: 29663 The Lesson of the Balanced Rock
-- Quest: 29676 Finding an Old Friend
-- Quest: 29677 The Sun Pearl 
-- Quest: 29678 Shu, the Spirit of Water
-- Quest: 29679 A new friend
-- Quest: 29680 The Source of Our Livelihood
-- Quest: 29774 Not In the Face!
-- Quest: 29772 Raucous Rousing
-- Quest: 29775 The Spirit and Body of Shen-zin Su
-- Quest: 29769 Rascals
-- Quest: 29770 Still Good!

DELETE FROM `creature_queststarter` WHERE `quest` IN (29768,29771);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29768', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55478', '29771', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29662, 29768, 29771);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES
(29662, 0, 0, 0, 0, 29676, -29661521, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'SmartQuest'),
('29768', '0', '0', '0', '29769', '29772', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29771', '0', '0', '0', '29769', '29772', '-29768', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `smart_scripts` WHERE `entryorguid`=29662 AND `source_type`=5;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param_string`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `action_param7`, `action_param_string`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_param_string`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(29662, 5, 0, 0, '', 50, 0, 100, 0, 0, 0, 0, 0, 0, '', 11, 108786, 0, 0, 0, 0, 0, 0, NULL, 7, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 'On Quest 29662 Reward - Cast on Player - Summon Stack of Reeds');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (108786, 108808, 108798);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(108786, 'spell_summon_stack_of_reeds'),
(108808, 'spell_summon_jojo_ironbrow'),
(108798, 'spell_jojo_headbash_filter');

-- Creature Texts
DELETE FROM `creature_text` WHERE `creatureID` IN (57669,57638);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('57669', '0', '0', 'Our crafters build the finest wooden planks.  They would weather the most brutal of storms.  But they are as water before the storm of my crushing skull.', '12', '0', '100', '1', '0', '0', '56343', '0', 'Jojo Ironbrow to Player'),
('57669', '1', '0', 'Nothing made by pandaren hands can withstand me.', '12', '0', '100', '2', '0', '0', '56344', '0', 'Jojo Ironbrow to Player'),
('57638', '0', '0', 'The reeds of the Singing Pools are the hardest in all of the land, but they are as air before my mighty brow.', '12', '0', '100', '1', '0', '0', '56333', '0', 'Jojo Ironbrow to Player'),
('57638', '1', '0', 'Many have tested my claim, and I yet stand proven.', '12', '0', '100', '2', '0', '0', '56334', '0', 'Jojo Ironbrow to Player');

-- Jojo Ironbrow Path

DELETE FROM `waypoint_path` WHERE `PathId`=5763800;
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Velocity`, `Comment`) VALUES 
(5763800, 0, 2, NULL, 'Jojo Ironbow - 57638');

DELETE FROM `waypoint_path_node` WHERE `PathId`=5763800;
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`) VALUES 
(5763800, 1, 1027.379, 3287.417, 126.2935),
(5763800, 2, 1017.188, 3284.896, 122.1512),
(5763800, 3, 1008.779, 3281.773, 126.0961),
(5763800, 4, 1002.936, 3280.243, 129.183),
(5763800, 5, 995.3472, 3277.767, 131.9249),
(5763800, 6, 987.7708, 3274.967, 132.621),
(5763800, 7, 980.6025, 3271.603, 132.3108),
(5763800, 8, 972.566, 3266.958, 129.9221),
(5763800, 9, 965.8785, 3262.287, 126.9234),
(5763800, 10, 957.291, 3258.333, 124.2717),
(5763800, 11, 954.6875, 3250.521, 123.0217),
(5763800, 12, 956.0166, 3239.251, 119.8172),
(5763800, 13, 958.2917, 3227.129, 119.1829),
(5763800, 14, 961.0208, 3215.984, 118.8171),
(5763800, 15, 965.2934, 3203.267, 119.1047),
(5763800, 16, 968.9957, 3193.558, 119.7131),
(5763800, 17, 965.625, 3186.459, 117.8083),
(5763800, 18, 960.5972, 3180.624, 117.5583),
(5763800, 19, 952.6045, 3169.271, 115.5682),
(5763800, 20, 945.833, 3161.459, 115.3083),
(5763800, 21, 938.7257, 3150.678, 116.1175),
(5763800, 22, 930.743, 3137.845, 113.3987),
(5763800, 23, 925.1771, 3127.699, 110.4666),
(5763800, 24, 921.0226, 3119.696, 110.343),
(5763800, 25, 917.9167, 3111.247, 110.0449),
(5763800, 26, 916.8212, 3100.016, 108.6972),
(5763800, 27, 918, 3090.027, 105.8014),
(5763800, 28, 919.632, 3082.332, 102.7485),
(5763800, 29, 922.191, 3071.392, 102.6767),
(5763800, 30, 925.3594, 3059.755, 102.7432),
(5763800, 31, 928.5191, 3048.86, 102.7789),
(5763800, 32, 931.0364, 3042.571, 102.7266),
(5763800, 33, 934.375, 3034.375, 102.8546),
(5763800, 34, 937.5, 3030.209, 103.7299),
(5763800, 35, 941.0504, 3025.699, 105.1759),
(5763800, 36, 944.791, 3022.917, 105.7299),
(5763800, 37, 958.5417, 3019.446, 105.591),
(5763800, 38, 965.6684, 3015.705, 105.6983),
(5763800, 39, 968.5799, 3009.252, 105.6935),
(5763800, 40, 966.8698, 3002.406, 105.0397);

DELETE FROM `npc_vendor` WHERE `entry`=57620;
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `ExtendedCost`, `type`, `PlayerConditionID`, `IgnoreFiltering`, `VerifiedBuild`) VALUES
(57620, 20, 39505, 0, 0, 1, 0, 0, 64978), -- Набор виртуозного начертателя
(57620, 19, 20815, 0, 0, 1, 0, 0, 64978), -- Инструменты ювелира
(57620, 18, 39354, 0, 0, 1, 0, 0, 64978), -- Тонкий пергамент
(57620, 17, 6260, 0, 0, 1, 0, 0, 64978), -- Синяя краска
(57620, 16, 2324, 0, 0, 1, 0, 0, 64978), -- Отбеливатель
(57620, 15, 2604, 0, 0, 1, 0, 0, 64978), -- Красная краска
(57620, 14, 6529, 0, 0, 1, 0, 0, 64978), -- Блесна
(57620, 13, 4289, 0, 0, 1, 0, 0, 64978), -- Соль
(57620, 12, 3371, 0, 0, 1, 0, 0, 64978), -- Хрустальная колба
(57620, 11, 2880, 0, 0, 1, 0, 0, 64978), -- Слабый плавень
(57620, 10, 2320, 0, 0, 1, 0, 0, 64978), -- Грубая нить
(57620, 9, 30817, 0, 0, 1, 0, 0, 64978), -- Простая мука
(57620, 8, 2678, 0, 0, 1, 0, 0, 64978), -- Пряные травы
(57620, 7, 6217, 0, 0, 1, 0, 0, 64978), -- Медный жезл
(57620, 6, 6256, 0, 0, 1, 0, 0, 64978), -- Удочка
(57620, 5, 5956, 0, 0, 1, 0, 0, 64978), -- Кузнечный молот
(57620, 4, 85663, 0, 0, 1, 0, 0, 64978), -- Лопата травника
(57620, 3, 2901, 0, 0, 1, 0, 0, 64978), -- Шахтерская кирка
(57620, 2, 7005, 0, 0, 1, 0, 0, 64978), -- Нож для снятия шкур
(57620, 1, 4470, 0, 0, 1, 0, 0, 64978); -- Простая древесина

-- Creature Templates

UPDATE `creature_template` SET `npcflag`=4289 WHERE `entry`=57620; -- Whittler Dewei
UPDATE `creature_template` SET `ScriptName`= 'npc_whitefeather_crane', `unit_flags3`=0x4000000 WHERE `entry`=55015; -- Whitefeather Crane
UPDATE `creature_template` SET `speed_run`=1, `BaseAttackTime`=2000, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry`=57636; -- Stack of Reeds
UPDATE `creature_template` SET `ScriptName`= 'npc_jojo_ironbrow_summon', `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=57638; -- Jojo Ironbrow

UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry` IN (57620, 55021, 65467, 56394, 56393, 55015, 57638) AND `DifficultyID`=0); -- CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=80, `StaticFlags1`=0x20000100, `VerifiedBuild`=65299 WHERE (`Entry`=57636 AND `DifficultyID`=0); -- 57636 (Stack of Reeds) - Sessile, Floating



-- Creature Spawns

-- Unnecessary spawn
DELETE FROM `gameobject` WHERE `guid` IN (300228, 300206, 300217, 300229, 300231, 300234, 300232, 300224, 300215, 300218, 300230);
DELETE FROM `creature` WHERE `guid` IN (450955, 450999, 450970, 450962, 450959, 450961, 450963, 450958, 450978, 451002, 450787, 450786, 450728, 451003);

-- Respawn creatures with the same guid
DELETE FROM `creature` WHERE `guid` IN (450755, 450745, 450759, 450741, 450752, 450747, 450742, 450760, 450744);
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`) VALUES
(450755, 57620, 860, 5736, 5826, '0', 0, 0, 0, 1, 1047.8802490234375, 3293.57470703125, 130.4282684326171875, 2.894826173782348632, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Whittler Dewei (Area: Поющие пруды - Difficulty: 0) CreateObject1 - !!! already present in database !!!
(450745, 55021, 860, 5736, 5826, '0', 0, 0, 0, 0, 1039.4913330078125, 3283.111083984375, 129.5230712890625, 1.815142393112182617, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Jojo Ironbrow (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 82343 - Generic Quest Invisibility 4) - !!! already present in database !!!

(450759, 65467, 860, 5736, 5826, '0', 0, 0, 0, 0, 1039.51220703125, 3293.630126953125, 129.3238372802734375, 4.677482128143310546, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Excited Onlooker (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8) - !!! already present in database !!!
(450741, 65467, 860, 5736, 5826, '0', 0, 0, 0, 0, 1041.9478759765625, 3291.673583984375, 129.422149658203125, 4.450589656829833984, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Excited Onlooker (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8)

(450752, 56394, 860, 5736, 5826, '0', 0, 0, 0, 0, 1038.0711669921875, 3291.072998046875, 129.069671630859375, 4.904375076293945312, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Mesmerized Child (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8)
(450747, 56394, 860, 5736, 5826, '0', 0, 0, 0, 0, 1040.79345703125, 3288.7734375, 129.3875732421875, 4.164384365081787109, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Mesmerized Child (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8) - !!! already present in database !!!
(450742, 56394, 860, 5736, 5826, '0', 0, 0, 0, 0, 1034.7535400390625, 3286.971435546875, 128.67218017578125, 5.917473793029785156, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Mesmerized Child (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8)
(450760, 56394, 860, 5736, 5826, '0', 0, 0, 0, 0, 1035.873291015625, 3287.873291015625, 128.843170166015625, 5.644624233245849609, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Mesmerized Child (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8) - !!! already present in database !!!

(450744, 56393, 860, 5736, 5826, '0', 0, 0, 0, 0, 1034.357666015625, 3290.73095703125, 128.7551116943359375, 5.323254108428955078, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978); -- Excited Onlooker (Area: Поющие пруды - Difficulty: 0) CreateObject1 (Auras: 78718 - Generic Quest Invisibility 8)

-- Creature Spawns Addons

DELETE FROM `creature_addon` WHERE `guid` IN (450745,450759,450741,450752,450747,450742,450760,450744,451158,451160,451170,451171,451173,451174,451178,451461,451457,451466,451456,451467,451463,451458);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(450745, 0, 0, 0, 0, 1, 1, 0, 461, 0, 0, 0, 0, '82343'), -- Jojo Ironbrow - 82343 - Generic Quest Invisibility 4 - !!! already present in database !!!
(450759, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Excited Onlooker - 78718 - Generic Quest Invisibility 8 - !!! already present in database !!!
(450741, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Excited Onlooker - 78718 - Generic Quest Invisibility 8
(450752, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Mesmerized Child - 78718 - Generic Quest Invisibility 8
(450747, 0, 0, 0, 0, 1, 1, 0, 0, 1507, 0, 0, 0, '78718'), -- Mesmerized Child - 78718 - Generic Quest Invisibility 8 - !!! already present in database !!!
(450742, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Mesmerized Child - 78718 - Generic Quest Invisibility 8
(450760, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Mesmerized Child - 78718 - Generic Quest Invisibility 8 - !!! already present in database !!!
(450744, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, '78718'), -- Excited Onlooker - 78718 - Generic Quest Invisibility 8

('451158', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'), -- farmstead jojo power quest
('451160', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451170', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451171', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451173', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451174', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),
('451178', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82343'),

('451461', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'), -- morning breeze jojo power quest
('451457', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451466', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451456', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451467', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451463', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852'),
('451458', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80852');


-- Fixes for Huo, the spirit of fire

UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000100, `VerifiedBuild`=64978 WHERE (`Entry`=54787 AND `DifficultyID` IN (0, 1)) OR (`Entry`=57779 AND `DifficultyID` IN (0, 1)); -- 54787, 57779 (Huo) - Sessile, CanSwim, Floating

UPDATE `creature` SET `PhaseId`= 632 WHERE `guid` IN (450608); 
UPDATE `creature` SET `PhaseId`= 631, `StringId` = 'Huo_Pre_Ignition' WHERE `guid` IN (450607); 

DELETE FROM `phase_area` WHERE `PhaseId` IN (631, 632);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(5849, 631, 'Cosmetic - Huo, Pre-Ignition'),
(5849, 632, 'Cosmetic - Huo, Post-Ignition');

DELETE FROM `conditions` WHERE (`SourceGroup` IN (631, 632)) AND (`SourceEntry` = 5849);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 631, 5849, 0, 0, 47, 0, 29422, 9, 0, '', 0, 0, 0, '', 'Allow phase 631 if quest 29422 state not taken / in progress'),

(26, 632, 5849, 0, 0, 47, 0, 29422, 66, 0, '', 0, 0, 0, '', 'Allow phase 632 if quest 29422 state completed / rewarded');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (102522, 109090, 109095, 109105, 109109);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES 
(102522, 'spell_fan_the_flames'),
(109090, 'spell_fan_the_flames_throw_wood_and_all_blow_air'),
(109095, 'spell_fan_the_flames_throw_wood_and_all_blow_air'),
(109105, 'spell_fan_the_flames_throw_wood_and_all_blow_air'),
(109109, 'spell_fan_the_flames_throw_wood_and_all_blow_air');

UPDATE `quest_template_addon` SET `ScriptName`= 'quest_29422_huo_the_spirit_of_fire', `NextQuestID`=29423 WHERE `ID` IN (29422);

-- 29423 The Passion of Shen-zin Su
SET @CGUID := 900000;

DELETE FROM `creature` WHERE `guid` IN (450021, @CGUID+741, @CGUID+742);
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`) VALUES
(@CGUID+741, 56479, 860, 5736, 5820, '0', '1323', 0, 0, 0, 942.28125, 3604.515625, 196.01605224609375, 6.248278617858886718, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978), -- Legacy of Liu Lang (Area: Храм Пяти Рассветов - Difficulty: 0) CreateObject1 (Auras: 132376 - GO_PA_TurtleShrine_01_Spell2_FireLoop) - !!! already present in database !!!
(@CGUID+742, 56479, 860, 5736, 5820, '0', '1324', 0, 0, 0, 942.28125, 3604.515625, 196.01605224609375, 6.248278617858886718, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 64978); -- Legacy of Liu Lang (Area: Храм Пяти Рассветов - Difficulty: 0) CreateObject1 (Auras: 132376 - GO_PA_TurtleShrine_01_Spell2_FireLoop) - !!! already present in database !!!

DELETE FROM `creature_addon` WHERE `guid` IN (450021, @CGUID+0, @CGUID+1);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(@CGUID+0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, ''), -- Legacy of Liu Lang - 132376 - 
(@CGUID+1, 0, 0, 0, 0, 0, 1, 0, 420, 2188, 0, 0, 5, '132376'); -- Legacy of Liu Lang - 132376 - GO_PA_TurtleShrine_01_Spell2_FireLoop - 

DELETE FROM `creature_template_gossip` WHERE (`CreatureID`=54786 AND `MenuID`=13158);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
(54786, 13158, 64978); -- Master Shang Xi

DELETE FROM `gossip_menu` WHERE (`MenuID`=13158 AND `TextID`=18536);
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES
(13158, 18536, 64978); -- 54786 (Master Shang Xi)

UPDATE `creature_template` SET `speed_run`=1.714285731315612792, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800, `ScriptName`= 'npc_huo_follower' WHERE `entry`=54958; -- Huo
UPDATE `creature` SET `ScriptName`= 'npc_chia_hui_autumnleaf', `StringId`= 'Chia_Hui_Talk_Event_Starter' WHERE `guid`=450386; 
UPDATE `creature` SET `StringId`= 'Brewer_Lin_Talk_Event' WHERE `guid`=450387; 
UPDATE `creature` SET `StringId`= 'Brewer_Zhen_Talk_Event' WHERE `guid`=450385; 
UPDATE `creature` SET `ScriptName`= 'npc_shanxi_quest', `StringId`= 'ShanXi_Talk' WHERE `guid`=450020; 
UPDATE `creature` SET `StringId`= 'Huo_Pre_Ignition' WHERE `guid`=450608; 

DELETE FROM `creature_template_addon` WHERE `entry` IN (54958);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(54958, 0, 0, 0, 3, 0, 1, 0, 0, 0, 3300, 0, 0, '102630'); -- 54958 (Huo) - Благословение Хо

UPDATE `creature_template_difficulty` SET `ContentTuningID`=80, `StaticFlags1`=0x30000000, `VerifiedBuild`=64978 WHERE (`Entry`=54958 AND `DifficultyID` IN (0, 1)); -- 54958 (Huo) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=80, `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry`=61127 AND `DifficultyID` IN (0, 1)); -- 61127 (Ji Firepaw) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=80, `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry`=61126 AND `DifficultyID` IN (0, 1)); -- 61126 (Aysa Cloudsinger) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry`=57619 AND `DifficultyID`=0); -- 57619 (Cheng Dawnscrive) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry`=54786 AND `DifficultyID`=0); -- 54786 (Master Shang Xi) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=64978 WHERE (`Entry`=57769 AND `DifficultyID`=0); -- 57769 (Шэнь-Цзынь Су) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry`=68986 AND `DifficultyID`=0); -- 68986 (Li the Tamer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry`=66298 AND `DifficultyID`=0); -- 66298 (Green Dragon Turtle) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=64978 WHERE (`Entry`=56479 AND `DifficultyID`=0); -- 56479 (Legacy of Liu Lang) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry`=57414 AND `DifficultyID`=0); -- 57414 (Temple Guard) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry`=65094 AND `DifficultyID`=0); -- 65094 (Priestess of the Dawn) - CanSwim

DELETE FROM `phase_area` WHERE `PhaseId` IN (1323, 1324);
INSERT INTO `phase_area` (`AreaId`, `PhaseId`, `Comment`) VALUES
(5736, 1323, 'Cosmetic - Central Statue, Pre-Fire'),
(5736, 1324, 'Cosmetic - Central Statue, Post-Fire');

DELETE FROM `conditions` WHERE (`SourceGroup` IN (1323, 1324)) AND (`SourceEntry` = 5736);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionStringValue1`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(26, 1323, 5736, 0, 0, 47, 0, 29423, 66, 0, '', 1, 0, 0, '', 'Allow phase 1323 if quest 29423 state not completed / not rewarded'),
(26, 1324, 5736, 0, 0, 47, 0, 29423, 66, 0, '', 0, 0, 0, '', 'Allow phase 1324 if quest 29423 state completed / rewarded');

DELETE FROM `creature_text` WHERE (`CreatureID` IN (54786, 60248, 60253, 61126, 61127));
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(54786, 0, 0, 'Welcome, Huo. The people have missed your warmth.', 12, 0, 100, 2, 0, 27788, 0, 60608, 0, 'Master Shang Xi to Player'),
(60248, 0, 0, 'Is that... is that Huo?', 12, 0, 100, 25, 0, 0, 0, 59751, 0, 'Chia-hui Autumnleaf to Player'),
(60253, 0, 0, 'It is! Well done, $p!', 12, 0, 100, 71, 0, 0, 0, 59752, 0, 'Brewer Lin to Player'),
(54786, 1, 0, 'You have conquered every challenge I put before you, $n. You have found Huo and brought him safely to the temple.', 12, 0, 100, 1, 0, 27789, 0, 60600, 0, 'Master Shang Xi to Player'),
(54786, 1, 1, 'Your training has paid off, young $c. You have found Huo and brought him safely to the temple.', 12, 0, 100, 1, 0, 0, 0, 55254, 0, 'Master Shang Xi to Player'),
(54786, 2, 0, 'There is a much larger problem we face now, my students. Shen-zin Su is in pain. If we do not act, the very land on which we stand could die, and all of us with it.', 12, 0, 100, 1, 0, 27790, 0, 60601, 0, 'Master Shang Xi to Player'),
(54786, 3, 0, 'We need to speak to Shen-zin Su and discover how to heal it. And to do that, we need the four elemental spirits returned. Huo was the first.', 12, 0, 100, 1, 0, 27791, 0, 60602, 0, 'Master Shang Xi to Player'),
(54786, 4, 0, 'Ji, I''d like you to go to the Dai-Lo Farmstead in search of Wugou, the spirit of earth.', 12, 0, 100, 1, 0, 27792, 0, 60603, 0, 'Master Shang Xi to Player'),
(54786, 5, 0, 'Aysa, I want you to go to the Singing Pools to find Shu, the spirit of water.', 12, 0, 100, 1, 0, 27793, 0, 60604, 0, 'Master Shang Xi to Player'),
(54786, 6, 0, 'And $n, you shall be the hand that guides us all. Speak with me for a moment before you join Aysa at the Singing Pools to the east.', 12, 0, 100, 1, 0, 27794, 0, 60605, 0, 'Master Shang Xi to Player'),
(61126, 0, 0, 'Yes master.', 12, 0, 100, 2, 0, 27406, 0, 0, 0, 'Aysa Cloudsinger to Player'),
(61127, 0, 0, 'On it!', 12, 0, 100, 0, 0, 27306, 0, 60606, 0, 'Ji Firepaw to Player');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (128700, 109178);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES 
(128700, 'spell_summon_fire_spirit'),
(109178, 'spell_despawn_fire_spirit');

DELETE FROM `quest_template_addon` WHERE `ID`=29423;
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES
(29423, 0, 0, 0, 0, 29521, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'quest_29423_the_passion_of_shen_zin_su');

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7750, 7835);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES 
(7750, 'at_talk_on_huo_follow_quest_29423'),
(7835, 'at_enter_temple_quest_29423');

DELETE FROM `waypoint_path` WHERE `PathId` IN (6112600);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Velocity`, `Comment`) VALUES
(6112600, 0, 1, NULL, '61126 (Aysa Cloudsinger)');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (6112600);
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`) VALUES
(6112600, 1, 969.36, 3601.2856, 196.06541, NULL),
(6112600, 2, 966.3715, 3602.764, 196.47968, NULL);

DELETE FROM `waypoint_path` WHERE `PathId` IN (6112700);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Velocity`, `Comment`) VALUES
(6112700, 0, 1, NULL, '61127 (Ji Firepaw)');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (6112700);
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`) VALUES
(6112700, 1, 971.5566, 3607.8015, 195.71495, NULL),
(6112700, 2, 966.1493, 3607.0894, 196.51373, NULL);

DELETE FROM `waypoint_path` WHERE `PathId` IN (6112701);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Velocity`, `Comment`) VALUES
(6112701, 0, 2, NULL, '61127 (Ji Firepaw)');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (6112701);
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(6112701, 1, 966.149, 3607.09, 196.514, NULL, 0),
(6112701, 2, 961.61, 3599.57, 196.419, NULL, 0),
(6112701, 3, 959.528, 3596.13, 196.313, NULL, 0),
(6112701, 4, 959.424, 3595.96, 196.351, NULL, 0),
(6112701, 5, 958.788, 3594.9, 196.421, NULL, 0),
(6112701, 6, 958.449, 3594.34, 196.443, NULL, 0),
(6112701, 7, 957.81, 3593.28, 196.62, NULL, 0),
(6112701, 8, 957.277, 3592.96, 196.604, NULL, 0),
(6112701, 9, 956.45, 3592.46, 196.599, NULL, 0),
(6112701, 10, 955.639, 3591.97, 195.728, NULL, 0),
(6112701, 11, 954.285, 3591.15, 195.732, NULL, 0),
(6112701, 12, 952.979, 3590.36, 195.712, NULL, 0),
(6112701, 13, 952.261, 3589.92, 196.578, NULL, 0),
(6112701, 14, 951.009, 3589.16, 196.59, NULL, 0),
(6112701, 15, 949.507, 3588.25, 196.58, NULL, 0),
(6112701, 16, 934.264, 3588.82, 196.584, NULL, 0),
(6112701, 17, 931.479, 3590.6, 196.603, NULL, 0),
(6112701, 18, 930.757, 3591.06, 195.712, NULL, 0),
(6112701, 19, 929.668, 3591.75, 195.726, NULL, 0),
(6112701, 20, 929.493, 3591.86, 195.731, NULL, 0),
(6112701, 21, 928.146, 3592.72, 195.745, NULL, 0),
(6112701, 22, 927.337, 3593.23, 196.657, NULL, 0),
(6112701, 23, 925.967, 3594.1, 196.594, NULL, 0),
(6112701, 24, 925.661, 3594.3, 196.569, NULL, 0),
(6112701, 25, 923.749, 3595.65, 196.407, NULL, 0),
(6112701, 26, 919.881, 3598.4, 196.619, NULL, 0),
(6112701, 27, 918.112, 3599.65, 196.525, NULL, 0),
(6112701, 28, 905.293, 3603.47, 193.129, NULL, 0),
(6112701, 29, 896.877, 3604.32, 193.098, NULL, 0),
(6112701, 30, 890.26, 3604.66, 192.259, NULL, 0),
(6112701, 31, 880.778, 3605.35, 192.204, NULL, 0),
(6112701, 32, 856.174, 3606.66, 173.901, NULL, 0);

DELETE FROM `waypoint_path` WHERE `PathId` IN (6112601);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Velocity`, `Comment`) VALUES
(6112601, 0, 2, NULL, '61126 (Aysa Cloudsinger)');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (6112601);
INSERT INTO `waypoint_path_node` (`PathId`, `NodeId`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `Delay`) VALUES
(6112601, 1, 966.372, 3602.76, 196.48, NULL, 0),
(6112601, 2, 953.724, 3586.03, 196.605, NULL, 0),
(6112601, 3, 946.795, 3581.76, 196.423, NULL, 0),
(6112601, 4, 942.691, 3571.45, 193.022, NULL, 0),
(6112601, 5, 940.927, 3558.86, 193.421, NULL, 0),
(6112601, 6, 941.162, 3564.98, 193.024, NULL, 0),
(6112601, 7, 941.02, 3559.48, 193.035, NULL, 0),
(6112601, 8, 941.266, 3558.73, 193.41, NULL, 0),
(6112601, 9, 940.927, 3558.86, 193.421, NULL, 0),
(6112601, 10, 940.895, 3558.66, 193.421, NULL, 0),
(6112601, 11, 940.85, 3556.92, 194.077, NULL, 0),
(6112601, 12, 940.91, 3555.48, 194.04, NULL, 0),
(6112601, 13, 940.902, 3555.33, 193.579, NULL, 0),
(6112601, 14, 940.881, 3553.84, 193.611, NULL, 0),
(6112601, 15, 941.021, 3552.36, 192.716, NULL, 0),
(6112601, 16, 941.067, 3551.87, 192.735, NULL, 0),
(6112601, 17, 941.796, 3544.16, 192.725, NULL, 0),
(6112601, 18, 941.84, 3543.69, 192.839, NULL, 0),
(6112601, 19, 942.205, 3532.94, 193.716, NULL, 0),
(6112601, 20, 941.934, 3523.57, 192.076, NULL, 0),
(6112601, 21, 941.339, 3500.11, 187.679, NULL, 0),
(6112601, 22, 943.224, 3486.15, 187.695, NULL, 0);

-- Quest: 29521 The Singing Pools
DELETE FROM `creature_addon` WHERE `guid`=450772;
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(450772, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, '49414'); -- Aysa Cloudsinger - 49414 - Generic Quest Invisibility 1 - !!! already present in database !!!

UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=-3, `LevelScalingDeltaMax`=-3, `ContentTuningID`=1723, `StaticFlags1`=0x10000000, `VerifiedBuild`=64978 WHERE (`Entry` IN (60250, 60249) AND `DifficultyID`=0); -- 60250 (Cai) - CanSwim

UPDATE `creature_template` SET `unit_flags3`=0x40000000 WHERE `entry`=54975; -- Aysa Cloudsinger
UPDATE `creature_template` SET `AIName`='', `ScriptName`='npc_cai', `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=60250; -- Cai
UPDATE `creature_template` SET `AIName`='', `ScriptName`='npc_deng', `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=60249; -- Deng

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7784);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES 
(7784, 'at_the_singing_pools_children_summon');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (116190, 116191);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES 
(116190, 'spell_summon_child_1'),
(116191, 'spell_summon_child_2');

DELETE FROM `creature_text` WHERE (`CreatureID` IN (60250, 60249));
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `comment`) VALUES
(60250, 0, 0, 'Hey there! Are you the one that brought back the fire spirit?', 12, 0, 100, 3, 0, 0, 0, 59875, 'Cai to Deng'),
(60250, 1, 0, 'Of course he was hot! Don''t be dumb, Deng.', 12, 0, 100, 5, 0, 0, 0, 59877, 'Cai to Deng'),
(60250, 2, 0, 'Nuh uh. Bet they could for sure kick you over that hill, though! Hah!', 12, 0, 100, 274, 0, 0, 0, 59880, 'Cai to Deng'),
(60250, 3, 0, 'Okay bye.', 12, 0, 100, 3, 0, 0, 0, 59878, 'Cai to Deng'),
(60249, 0, 0, 'Was he hot?  I bet he was hot!', 12, 0, 100, 6, 0, 0, 0, 59876, 'Deng to Cai'),
(60249, 1, 0, 'I bet you''re really strong, huh?  You could probably kick that bridge right in half if you wanted to!', 12, 0, 100, 6, 0, 0, 0, 59879, 'Deng to Cai'),
(60249, 2, 0, 'Bye!', 12, 0, 100, 3, 0, 0, 0, 59884, 'Deng to Cai');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29661, 29663, 29662);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES
(29661, 0, 0, 0, 29521, 29676, -29661521, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''),
(29663, 0, 0, 0, 29521, 29676, -29661521, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''),
(29662, 0, 0, 0, 0, 29676, -29661521, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '');

-- Quest: 29663 The Lesson of the Balanced Rock
-- Quest: 29661 The Lesson of the Dry Fur
DELETE FROM `quest_template_addon` WHERE `ID` IN (29521, 29661, 29663, 29676);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`) 
VALUES 
('29521', '0', '0', '0', '0', '29661', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('29661', '0', '0', '0', '29521', '29676', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('29663', '0', '0', '0', '29521', '29676', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'),
('29676', '0', '0', '0', '29663', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');

UPDATE `creature_template` SET `ScriptName` = 'npc_tushui_monk_on_pole' WHERE (`entry` = '55019');
UPDATE `creature_template` SET `ScriptName` = 'npc_tushui_monk_on_pole', `KillCredit1` = '55019' WHERE (`entry` = '65468');
UPDATE `creature_template` SET `ScriptName` = 'npc_balance_pole' WHERE `Entry` IN (54993, 57431, 55083);

DELETE FROM `vehicle_template` WHERE `creatureId` IN (54993, 57431, 55083, 56869);
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('54993', '0', '0');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('57431', '0', '0');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('55083', '0', '0');
INSERT INTO `vehicle_template` (`creatureId`, `despawnDelayMs`, `CustomFlags`) VALUES ('56869', '0', '0');

UPDATE `creature_template` SET `unit_flags3` = '524288' WHERE (`entry` = '54993');
UPDATE `creature_template` SET `unit_flags3` = '524288' WHERE (`entry` = '55083');
UPDATE `creature_template` SET `unit_flags3` = '524288' WHERE (`entry` = '57431');

UPDATE `creature_template` SET `npcflag` = '50331648' WHERE (`entry` = '54993');
UPDATE `creature_template` SET `npcflag` = '50331648' WHERE (`entry` = '55083');
UPDATE `creature_template` SET `npcflag` = '50331648' WHERE (`entry` = '57431');

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (55083, 57431,54993);
INSERT INTO `npc_spellclick_spells` VALUES
(54993, 102717, 1, 0),
(55083, 102717, 1, 0),
(57431, 102717, 1, 0);

DELETE FROM `conditions` WHERE `SourceEntry` = 107049;
insert  into `conditions`(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) values 
(13,1,107049,0,0,51,0,5,56869,0,0,0,0,'','Ride Vehicle target Balance Pole Landing Bunny'),
(17,0,107049,0,0,1,0,133381,0,0,1,30,0,'','Ride Vehicle when player has not aura'),
(17,0,107049,0,0,29,0,56869,8,0,0,30,0,'','Ride Vehicle when bunny within 8y'),
(13,1,107049,0,0,31,0,5,56869,0,0,0,0,'','Ride Vehicle target Balance Pole Landing Bunny');

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (8628); -- training bell trigger
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('8628', 'at_singing_pools_training_bell');
DELETE FROM `creature` WHERE `guid` IN (@CGUID+743, @CGUID+744, @CGUID+745, @CGUID+746);
INSERT INTO `creature` (guid, id, map, zoneId, areaId, spawnDifficulties, phaseUseFlags, PhaseId, PhaseGroup, terrainSwapMap, modelid, equipment_id, position_x, position_y, position_z, orientation, spawntimesecs, wander_distance, currentwaypoint, curHealthPct, MovementType, npcflag, unit_flags, unit_flags2, unit_flags3, ScriptName, StringId, VerifiedBuild) VALUES
(@CGUID+743, '54993', '860', '5736', '5826', '0', '0', '169', '0', '-1', '0', '0', '968.982', '3293.41', '117.685', '-0.50883', '300', '0', '0', '100', '0', NULL, '262144', NULL, NULL, '', NULL, '0'),
(@CGUID+744, '54993', '860', '5736', '5826', '0', '0', '169', '0', '-1', '0', '0', '941.046', '3299.72', '117.024', '-0.24147', '300', '0', '0', '100', '0', NULL, '262144', NULL, NULL, '', NULL, '0'),
(@CGUID+745, '54993', '860', '5736', '5826', '0', '0', '169', '0', '-1', '0', '0', '929.348', '3299.2', '117.429', '0.55515', '300', '0', '0', '100', '0', NULL, '262144', NULL, NULL, '', NULL, '0'),
(@CGUID+746, '54993', '860', '5736', '5826', '0', '0', '169', '0', '-1', '0', '0', '1011.65', '3299.14', '116.784', '3.13979', '300', '0', '0', '100', '0', NULL, '262144', NULL, NULL, '', NULL, '0');

-- Quest: 29677 The Sun Pearl 
UPDATE `creature_template` SET `ScriptName` = 'npc_fang_she' WHERE `entry` = 55292;
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` = 54976;
UPDATE `creature_template_difficulty` SET `StaticFlags1`=(0x10000000 | 0x00000080 | 0x00040000) WHERE `entry` IN (54976, 55292); -- swim/aquatic/loot
DELETE FROM `creature` WHERE `guid` IN ('451096', '451050', '451043'); -- remove duplicate Fang-She as not sure they are for different phases
UPDATE `creature` SET `wander_distance` = 5 AND `MovementType` = 1 WHERE id IN (54976); -- Barbed Rays should be moving
UPDATE `gameobject_template` SET `ContentTuningId` = 80 WHERE (`entry` = '209584');

DELETE FROM `smart_scripts` WHERE `entryorguid` = 54976 AND `source_type` = 0;
INSERT INTO `smart_scripts` (entryorguid, source_type, id, link, Difficulties,
    event_type, event_phase_mask, event_chance, event_flags,
    event_param1, event_param2, event_param3, event_param4, event_param5, event_param_string,
    action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, action_param7, action_param_string,
    target_type, target_param1, target_param2, target_param3, target_param4, target_param_string,
    target_x, target_y, target_z, target_o,
    comment) VALUES 
('54976', '0', '0', '0', '', '0', '0', '100', '0', '3000', '5000', '6000', '8000', '0', '', '11', '128407', '0', '0', '0', '0', '0', '0', NULL, '2', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Every 6 - 8 seconds (3 - 5s initially) (IC) - Self: Cast spell  128407 on Victim');

-- Quest: 29678 Shu The Spirit of Water
DELETE FROM `spell_script_names` WHERE `spell_id` IN (103069,103077,103070);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('103069', 'spell_rock_jump_a');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('103070', 'spell_rock_jump_b');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('103077', 'spell_rock_jump_c');

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7783);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('7783', 'at_pools_of_reflection');

-- Quest: 29679 A new Friend
-- Object required for the pool of reflection
SET @OGUID := 900000;
DELETE FROM `gameobject` WHERE `guid` = @OGUID+69;
INSERT INTO `gameobject` VALUES
(@OGUID+69, '209585', '860', '5736', '5862', '0', '0', '0', '0', '-1', '1106.2', '2860.34', '92.189', '0.918553', '-0', '-0', '-0.4433', '-0.896374', '300', '255', '1', '', NULL, '0');

UPDATE `creature_template` SET `AIName` = 'SmartAI', `unit_flags` = '33554944', `unit_flags2` = '2048' WHERE (`entry` = '60488');
UPDATE `creature_template` SET `ScriptName` = 'npc_shu_playing' WHERE (`entry` = '65493');

DELETE FROM `creature_template_addon` WHERE `entry` IN (60488);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('60488', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '89304 116695'); -- Water Spout Bunny at pool2 -- 85096 or 80797 ????? 
-- UPDATE `creature_template_addon` SET `auras` = '89304 116695' WHERE (`entry` = '60488');

-- these flags disable gravity for the bunny so it can stay above water
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '60488') and (`DifficultyID` = '0');

DELETE FROM `creature_addon` WHERE `guid` IN (451090);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('451090', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '89304'); -- Shu at pool2 -- 85096 or 80797 ????? 
-- UPDATE `creature_addon` SET `auras` = '89304' WHERE (`guid` = '451090'); -- not sure if this is correct or one of the above

DELETE FROM `spell_script_names` WHERE `spell_id` IN (117033,117034,117035,117036);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('117033', 'spell_jump_to_front_right');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('117034', 'spell_jump_to_front_left');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('117035', 'spell_jump_to_back_right');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('117036', 'spell_jump_to_back_left');

DELETE FROM `smart_scripts` WHERE `entryorguid` IN (60488,6048800); -- 15880 was the original model but not working
INSERT INTO `smart_scripts` VALUES -- we use model 21072 now taken from wowhead
('60488', '0', '0', '1', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '3', '0', '21072', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Morph'),
('60488', '0', '1', '0', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '6048800', '2', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Run Script'),
('6048800', '9', '0', '0', '', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '11', '117057', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Geyser Aura'),
('6048800', '9', '1', '0', '', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '11', '116696', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Burst'),
('6048800', '9', '2', '0', '', '0', '0', '100', '0', '0', '0', '3000', '3000', '0', '', '28', '116695', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Remove Aura'),
('6048800', '9', '3', '0', '', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '41', '0', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Despawn');

-- temp fix until quest accept works from auto completed quests and new offered quests
DELETE FROM `creature_queststarter` WHERE `quest` IN (29679, 29680);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('54975', '29679', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('54975', '29680', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29679,29680);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29679', '0', '0', '0', '29678', '29680', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'quest_29679_a_new_friend'),
('29680', '0', '0', '0', '29679', '29769', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (128588,128589);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('128588', 'spell_aysa_congrats_trigger_aura');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('128589', 'spell_aysa_congrats_timer');

DELETE FROM `creature_text` WHERE `CreatureID` = 54975;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `SoundPlayType`, `BroadcastTextId`, `TextRange`, `comment`) VALUES 
('54975', '0', '0', 'I have to admit, that looked pretty fun!', '12', '0', '100', '0', '0', '27394', '0', '66056', '1', 'Aysa Cloudsinger - quest A new friend'),
('54975', '1', '0', 'And it looks to me like you made a new friend.', '12', '0', '100', '0', '0', '27395', '0', '66057', '1', 'Aysa Cloudsinger - quest A new friend');

-- Quest: 29680 The Source of Our Livelihood
UPDATE `creature_template` SET `ScriptName` = 'npc_shu_follower' WHERE `Entry` IN (55213);
UPDATE `creature_template` SET `movementId` = '65' WHERE (`entry` = '55213');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55213,55539);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55213', '5521300', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', '103245'),
('55539', '0', '0', '0', '3', '0', '1', '0', '0', '0', '0', '0', '0', '0', '80797 42386');

DELETE FROM `waypoint_path` WHERE `PathId` IN (5521300);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(5521300,1,0,'Spawned Shu quest 29680');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (5521300);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('5521300', '0', '634.3', '3126.2', '88.7482'),
('5521300', '1', '642.205', '3124.58', '89.3339'),
('5521300', '2', '649.877', '3126.44', '89.808'),
('5521300', '3', '655.521', '3131.44', '89.3411'),
('5521300', '4', '660.047', '3135.83', '88.807'),
('5521300', '5', '665.788', '3136.77', '87.8995');

-- Quest: 29769 Rascals
-- Quest: 29770 Still Good!
DELETE FROM `creature_queststarter` WHERE `quest` IN (29769,29770);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29769', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55479', '29770', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29769);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29769', '0', '0', '0', '29680', '29768', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_text` WHERE `creatureID` IN (55477,55483,55504);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('55477', '0', '0', 'Wake... up... DANGIT!', '12', '0', '100', '15', '0', '27345', '60447', '0', 'Ji Firepaw'),
('55477', '1', '0', 'This is ridiculous...', '12', '0', '100', '274', '0', '27346', '60448', '0', 'Ji Firepaw'),
('55477', '2', '0', 'You sleep like a rock!', '12', '0', '100', '5', '0', '27347', '60449', '0', 'Ji Firepaw'),
('55477', '3', '0', 'There\'s no way you\'ll sleep through THIS.', '12', '0', '100', '274', '0', '27348', '60450', '0', 'Ji Firepaw'),
('55477', '4', '0', 'Seriously?', '12', '0', '100', '18', '0', '27349', '60451', '0', 'Ji Firepaw'),
('55477', '5', '0', 'Why...', '12', '0', '100', '507', '0', '27350', '60452', '0', 'Ji Firepaw'),
('55477', '6', '0', 'won\'t...', '12', '0', '100', '509', '0', '27351', '60453', '0', 'Ji Firepaw'),
('55477', '7', '0', 'you...', '12', '0', '100', '507', '0', '27352', '60454', '0', 'Ji Firepaw'),
('55477', '8', '0', 'just...', '12', '0', '100', '509', '0', '27353', '60455', '0', 'Ji Firepaw'),
('55477', '9', '0', 'WAKE UP?!', '12', '0', '100', '22', '0', '27354', '60456', '0', 'Ji Firepaw'),
('55477', '10', '0', 'I will break you, little rock man!', '12', '0', '100', '25', '0', '27355', '60457', '0', 'Ji Firepaw'),
('55477', '11', '0', '%s sighs.', '16', '0', '100', '0', '0', '0', '55942', '0', 'Ji Firepaw to Player'),
('55483', '0', '0', 'Gimme all your vegetables!', '12', '0', '100', '0', '0', '0', '54873', '0', 'Plump Virmen to Player'),
('55483', '0', '1', 'AIIIIEEEEEEE!', '12', '0', '100', '0', '0', '0', '54874', '0', 'Plump Virmen to Player'),
('55483', '0', '2', 'This virmen land!', '12', '0', '100', '0', '0', '0', '54876', '0', 'Plump Virmen to Player'),
('55483', '0', '3', 'You no take carrot! You take turnip instead!', '12', '0', '100', '0', '0', '0', '54877', '0', 'Plump Virmen to Player'),
('55483', '0', '4', 'We rowdy!', '12', '0', '100', '0', '0', '0', '54875', '0', 'Plump Virmen to Player'),
('55504', '0', '0', 'Gimme all your vegetables!', '12', '0', '100', '0', '0', '0', '54873', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '1', 'AIIIIEEEEEEE!', '12', '0', '100', '0', '0', '0', '54874', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '2', 'This virmen land!', '12', '0', '100', '0', '0', '0', '54876', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '3', 'You no take carrot! You take turnip instead!', '12', '0', '100', '0', '0', '0', '54877', '0', 'Plump Carrotcruncher to Player'),
('55504', '0', '4', 'We rowdy!', '12', '0', '100', '0', '0', '0', '54875', '0', 'Plump Carrotcruncher to Player');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55477,55483,55504);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55477,5547700,55483,55504);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55483', '0', '0', '0', '4', '0', '25', '0', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - On Aggro - Talk'),
('55483', '0', '1', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '30', '1', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - On Aggro - Set Random Phase'),
('55483', '0', '2', '0', '0', '0', '100', '0', '2000', '5000', '5000', '8000', '0', '', '11', '107936', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - Update IC - Cast Back Kick'),
('55483', '0', '3', '0', '0', '1', '100', '0', '5000', '8000', '8000', '12000', '0', '', '11', '107938', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - Update IC - Cast Tail Thrash'),
('55483', '0', '4', '0', '0', '2', '100', '0', '5000', '8000', '15000', '18000', '0', '', '11', '107944', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Virmen - Update IC - Cast Carrot Breath'),
('55504', '0', '0', '0', '4', '0', '25', '0', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - On Aggro - Talk'),
('55504', '0', '1', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '30', '1', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - On Aggro - Set Random Phase'),
('55504', '0', '2', '0', '0', '0', '100', '0', '2000', '5000', '5000', '8000', '0', '', '11', '107936', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - Update IC - Cast Back Kick'),
('55504', '0', '3', '0', '0', '1', '100', '0', '5000', '8000', '8000', '12000', '0', '', '11', '107938', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - Update IC - Cast Tail Thrash'),
('55504', '0', '4', '0', '0', '2', '100', '0', '5000', '8000', '15000', '20000', '0', '', '11', '107944', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Plump Carrotcruncher - Update IC - Cast Carrot Breath'),
('55477', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '59', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - On Respawn - Set Run'),
('55477', '0', '1', '0', '1', '0', '100', '0', '1000', '1000', '116000', '116000', '0', '', '80', '5547700', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Update OOC - Run Script'),
('5547700', '9', '0', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '1', '0', '0', '0', '100', '0', '16000', '16000', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '2', '0', '0', '0', '100', '0', '17000', '17000', '1000', '1000', '0', '', '1', '2', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '3', '0', '0', '0', '100', '0', '18000', '18000', '1000', '1000', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '628.734', '3138.05', '87.837', '0', 'Ji Firepaw - Move to Pos'),
('5547700', '9', '4', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '6.21337', 'Ji Firepaw - Set Orientation'),
('5547700', '9', '5', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '1', '3', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '6', '0', '0', '0', '100', '0', '4000', '4000', '1000', '1000', '0', '', '11', '118026', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Cast SpecialUnarmed w/ Gong Song'),
('5547700', '9', '7', '0', '0', '0', '100', '0', '3500', '3500', '1000', '1000', '0', '', '1', '4', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '8', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '69', '2', '0', '0', '0', '0', '0', '8', '0', '0', '0', '629.354', '3139.15', '87.837', '0', 'Ji Firepaw - Move to Pos'),
('5547700', '9', '9', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '0.558505', 'Ji Firepaw - Set Orientation'),
('5547700', '9', '10', '0', '0', '0', '100', '0', '4000', '4000', '1000', '1000', '0', '', '1', '5', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '11', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '17', '510', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Set Emote State'),
('5547700', '9', '12', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '6', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '13', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '7', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '14', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '8', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '15', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '17', '30', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Set Emote State'),
('5547700', '9', '16', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '9', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '17', '0', '0', '0', '100', '0', '20000', '20000', '1000', '1000', '0', '', '1', '10', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('5547700', '9', '18', '0', '0', '0', '100', '0', '13000', '13000', '1000', '1000', '0', '', '5', '20', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Play Emote');

UPDATE `creature_template_difficulty` SET `LootID` = '55483', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55483') and (`DifficultyID` = '0');
UPDATE `creature_template_difficulty` SET `LootID` = '55504', `GoldMin` = '7', `GoldMax` = '7' WHERE (`Entry` = '55504') and (`DifficultyID` = '0');

-- Quest: 29774 Not In the Face!
DELETE FROM `creature_queststarter` WHERE `quest` IN (29772,29774,29775);
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29772', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29774', '0');
INSERT INTO `creature_queststarter` (`id`, `quest`, `VerifiedBuild`) VALUES ('55477', '29775', '0');

DELETE FROM `quest_template_addon` WHERE `ID` IN (29772,29774,29775);
INSERT INTO `quest_template_addon` (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`, `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`, `ScriptName`) VALUES 
('29772', '0', '0', '0', '29768', '29774', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29774', '0', '0', '0', '29772', '29775', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ''),
('29775', '0', '0', '0', '29774', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '');

DELETE FROM `creature_text` WHERE `creatureID` IN (55539);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('55539', '0', '0', '%s snorts loudly, and continues sleeping.', '16', '0', '100', '0', '0', '0', '54991', '0', 'Wugou to Player');

DELETE FROM `creature_template_addon` WHERE `entry` IN (55558,60916);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55558', '8', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '105890'),
('60916', '8', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '105889 42386');

DELETE FROM `gossip_menu` WHERE `MenuID` = 13140;
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES ('13140', '18503', '0');

DELETE FROM `gossip_menu_option` WHERE `MenuID` = 13140 AND `GossipOptionID` = 18503;
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`) VALUES 
('13140', '18503', '0', '0', 'Shu, can you wake up Wuguo for me?', '54025', '0', '1');

UPDATE `creature_template` SET `AIName` = '' WHERE `Entry` IN (55556,55558);
UPDATE `creature_template` SET `ScriptName` = 'npc_shu_at_farmstead' WHERE `Entry` = 55556;

DELETE FROM `waypoint_path` WHERE `PathId` IN (5555800, 5555801,5555802);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(5555802,1,0,'Spawned Shu at the Farmstead after Wugou wakes up'),
(5555800,1,0,'Spawned Shu at the Farmstead quest'),
(5555801,1,0,'Spawned Shu at the Farmstead after quest');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (5555800, 5555801, 5555802);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('5555802', '0', '815.315', '3608.55', '173.987'),
('5555802', '1', '831.068', '3607.74', '175.247'),
('5555802', '2', '841.146', '3606.77', '175.188'),
('5555802', '3', '855.831', '3607.75', '173.619'),
('5555802', '4', '880.269', '3605.99', '192.267'),
('5555802', '5', '892.776', '3605.94', '192.93'),
('5555802', '6', '900.755', '3603.12', '193.08'),
('5555802', '7', '906.239', '3600.17', '193.1');
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('5555800', '0', '648.675', '3124.17', '90.2095'),
('5555800', '1', '639.615', '3120.11', '89.8218'),
('5555800', '2', '628.448', '3121.48', '88.5602'),
('5555800', '3', '622.969', '3125.62', '88.0585'),
('5555800', '4', '620.156', '3134.65', '87.837'),
('5555800', '5', '621.747', '3140.77', '87.837');
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
('5555801', '0', '614.498', '3126.25', '87.8195'),
('5555801', '1', '624.028', '3121.75', '87.9445'),
('5555801', '2', '634.573', '3125.93', '88.6816'),
('5555801', '3', '638.785', '3133.88', '88.1648'),
('5555801', '4', '639.109', '3140.55', '88.3733'),
('5555801', '5', '636.264', '3146.2', '88.5406'),
('5555801', '6', '630.757', '3147.61', '88.2409'),
('5555801', '7', '625.675', '3143.88', '87.837');

UPDATE `waypoint_path_node` SET `Orientation` = '6.21337' WHERE (`PathId` = 5555800 AND `NodeId` = 5);

DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55556,55558,5555800) AND `source_type` = 0;
-- Removed spell from object as i can't get it to work
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (209626) AND `source_type` = 1;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_chance`, `event_param1`, `action_type`, `action_param1`, `action_param2`, `target_type`, `target_param1`, `target_param2`, `comment`) VALUES 
('209626', '1', '0', '1', '70', '100', '2', '134', '118026', 0, '7', 0,0,'On Use Break Gong - Cast Gong Song'),
('209626', '1', '1', '2', '61', '100', '0', '45', '1', 1, '11', 55539, 20, 'Break Gong - Set Data Wugou'),
('209626', '1', '2', '0', '61', '100', '0', '45', '2', 2, '11', 55477, 20, 'Break Gong - Set Data Ji');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55539,55556,55558);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55539,55556,55558,5555800);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55477) AND `event_type` IN (38,61);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55539', '0', '0', '0', '38', '0', '100', '0', '1', '1', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Wugou - On Data Set - Talk'),
('55477', '0', '2', '3', '38', '0', '100', '0', '2', '2', '5000', '5000', '0', '', '1', '11', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Talk'),
('55477', '0', '3', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '33', '55546', '0', '0', '0', '0', '0', '18', '20', '0', '0', '0', '0', '0', '0', 'Ji Firepaw - Quest Credit'),
('55556', '0', '0', '0', '1', '0', '100', '0', '0', '5000', '8000', '12000', '0', '', '11', '118032', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Update OOC - Cast Water Spout'),
('55556', '0', '1', '2', '62', '0', '100', '0', '13140', '0', '0', '0', '0', '', '72', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Gossip Select - Close Gossip'),
('55556', '0', '2', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '85', '104017', '2', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Gossip Select - Invoker Cast Summon Spirit of Water'),
('55558', '0', '0', '1', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '53', '1', '8', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Just Spawned - Start WP'),
('55558', '0', '1', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '54', '2000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Just Spawned - Pause WP'),
('55558', '0', '2', '3', '58', '0', '100', '0', '5', '8', '0', '0', '0', '', '66', '0', '0', '0', '0', '0', '0', '8', '0', '0', '0', '0', '0', '0', '6.21337', 'Shu - On WP Ended - Set Orientation'),
('55558', '0', '3', '4', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '118027', '2', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Cast Shu\'s Water Splash'),
('55558', '0', '4', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '118034', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Cast Water Splash'),
('55558', '0', '5', '6', '8', '0', '100', '0', '118034', '0', '0', '0', '0', '', '11', '104023', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Cast Water Splash Credit'),
('55558', '0', '6', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5555800', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Run Script'),
('55558', '0', '7', '8', '31', '0', '100', '0', '118027', '0', '1000', '1000', '0', '', '28', '42386', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit Target - Remove Aura'),
('55558', '0', '8', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '64', '1', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit Target - Store Target'),
('55558', '0', '9', '10', '58', '0', '100', '0', '7', '9', '0', '0', '0', '', '11', '118035', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Cast Cast Water Spirit Laugh'),
('55558', '0', '10', '11', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '45', '1', '1', '0', '0', '0', '0', '12', '1', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Set Data'),
('55558', '0', '11', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '29', '0', '1', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Follow Summoner'),
('55558', '0', '12', '13', '8', '0', '100', '1', '106683', '0', '0', '0', '0', '', '59', '1', '15', '1', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Set Run'),
('55558', '0', '13', '14', '61', '0', '100', '1', '0', '0', '0', '0', '0', '', '53', '1', '9', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Start WP'),
('55558', '0', '14', '0', '61', '0', '100', '1', '0', '0', '0', '0', '0', '', '28', '104017', '0', '0', '0', '0', '0', '23', '0', '0', '0', '0', '0', '0', '0', 'Shu - On Spellhit - Remove Aura'),
('55558', '0', '15', '0', '58', '0', '100', '0', '7', '9', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - On WP Ended - Despawn'),
('5555800', '9', '0', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '11', '118035', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Cast Water Spirit Laugh'),
('5555800', '9', '1', '0', '0', '0', '100', '0', '2000', '2000', '1000', '1000', '0', '', '53', '1', '9', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Shu - Start WP');



UPDATE `gameobject_template` SET `AIName` = 'SmartGameObjectAI', `Data1` = '29772', `Data10` = '0' WHERE `Entry` = 209626; -- Data10 was 104012

DELETE FROM `spell_target_position` WHERE `ID` IN (108827,104450,108847,108858,102499,116191,116191,108786,115426,115426,115334,115336,115338,
116190,118500,126040,104571,131974,117615,117597,118499,115435,115495,115494,115493);
INSERT INTO spell_target_position VALUES
('108827', '0', '0', '860', '601.62', '3132.89', '88.0976', NULL, '0'),
('104450', '0', '0', '860', '909.137', '3610.38', '252.092', NULL, '0'),
('108847', '0', '0', '860', '1075.6', '4177.97', '204.63', NULL, '0'),
('108858', '0', '0', '860', '1075.54', '4177.9', '205.583', NULL, '0'),
('102499', '0', '0', '860', '1351.33', '3939.03', '109.324', NULL, '0'),
('116191', '0', '0', '860', '949.37', '3510', '187.715', NULL, '0'),
('108786', '0', '0', '860', '1038.55', '3286.39', '128.176', NULL, '0'),
('115426', '0', '0', '860', '695.26', '3600.99', '142.381', NULL, '20886'),
('115334', '0', '0', '860', '571.201', '3584.59', '94.8437', NULL, '0'),
('115336', '0', '0', '860', '573.84', '3577.94', '95.0413', NULL, '0'),
('115338', '0', '0', '860', '576.42', '3579.7', '94.9249', NULL, '0'),
('116190', '0', '0', '860', '933.509', '3511.01', '187.717', NULL, '0'),
('118500', '0', '0', '860', '992.108', '3604.55', '193.032', NULL, '0'),
('126040', '0', '0', '860', '756.201', '4168.14', '202.262', NULL, '0'),
('104571', '0', '0', '860', '666.514', '4218.27', '200.851', NULL, '0'),
('131974', '0', '0', '860', '248.988', '3941.08', '65.3511', NULL, '20886'),
('117615', '1', '0', '860', '250.757', '3932.67', '66.408', NULL, '20886'),
('117597', '0', '0', '860', '230.314', '4006.68', '87.2803', NULL, '0'),
('118499', '0', '0', '860', '992.003', '3600.76', '193.031', NULL, '0'),
('115435', '0', '0', '860', '566.524', '3583.47', '92.1576', NULL, '24742'),
('115495', '0', '0', '860', '418.16', '3630.88', '92.9404', NULL, '0'),
('115494', '0', '0', '860', '424.858', '3635.56', '92.6789', NULL, '0'),
('115493', '0', '0', '860', '422.292', '3633.7', '92.8522', NULL, '0');

-- Water spout bunny
UPDATE `creature_template` SET `AIName` = 'SmartAI', `unit_flags` = '33554944', `unit_flags2` = '2048' WHERE (`entry` = '66941');

DELETE FROM `creature_template_addon` WHERE `entry` IN (66941);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('66941', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '116695'); -- Water Spout Bunny at farmstead -- not sure of the invis aura ????? 

-- these flags disable gravity for the bunny so it can stay above water
UPDATE `creature_template_difficulty` SET `StaticFlags1` = '536870912' WHERE (`Entry` = '66941') and (`DifficultyID` = '0');

DELETE FROM `smart_scripts` WHERE `entryorguid` IN (66941,6694100); -- 15880 was the original model but not working
INSERT INTO `smart_scripts` VALUES -- we use model 21072 now taken from wowhead
('66941', '0', '0', '1', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '3', '0', '21072', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Morph'),
('66941', '0', '1', '0', '0', '61', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '6694100', '2', '0', '0', '0', '0', '0', '', '1', '0', '0', '0', '0', '', '0', '0', '0', '0', 'Water Spout - Just Spawned - Run Script'),
('6694100', '9', '0', '0', '', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '11', '117057', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Geyser Aura'),
('6694100', '9', '1', '0', '', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '11', '116696', '2', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Cast Water Spout Burst'),
('6694100', '9', '2', '0', '', '0', '0', '100', '0', '0', '0', '3000', '3000', '0', '', '28', '116695', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Remove Aura'),
('6694100', '9', '3', '0', '', '0', '0', '100', '0', '3000', '3000', '1000', '1000', '0', '', '41', '0', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Water Spout - Despawn');

-- Adding some creature text for Wo-son Villager
DELETE FROM `creature_text` WHERE `creatureID` IN (57132);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
('57132', '0', '0', 'I have had enough of this!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line'),
('57132', '0', '1', 'Just... go away!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line'),
('57132', '0', '2', 'You nasty... little creature!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line'),
('57132', '0', '3', 'Get off of me!', '12', '0', '100', '1', '0', '0', '0', '0', 'Wu-song Villager Random line');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (57132);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (57132);
INSERT INTO `smart_scripts` VALUES
('57132', '0', '0', '0', '', '0', '0', '100', '0', '20000', '20000', '40000', '65000', '0', '', '1', '0', '0', '0', '0', '0', '0', '0', NULL, '1', '0', '0', '0', '0', NULL, '0', '0', '0', '0', 'Wu-song Villager - In Combat - Say Line 0');

-- Fix Visibility and state addons for Jojos
DELETE FROM `creature_template_addon` WHERE `entry` IN (55585,55021);
INSERT INTO `creature_template_addon` 
(`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('55585', '0', '0', '0', '1', '0', '1', '0', '0', '461', '0', '0', '0', '0', '84886'),
('55021', '0', '0', '0', '1', '0', '1', '0', '0', '461', '0', '0', '0', '0', '82343');

-- Fix Visibility for Old Man Liang and Aysa during Singing Pools quests
UPDATE `creature_template_addon` SET `auras` = '0' WHERE (`entry` = '55020'); 

DELETE FROM `creature_addon` WHERE `guid` IN (451049,451022,450772,451092,451042,451091);
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 

('451022', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'), -- Old Man Liang at his house
('450772', '0', '0', '0', '1', '0', '1', '1', '0', '0', '0', '0', '0', '0', '80797'), -- Aysa at the pool

('451092', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '85096'), -- Old Man Liang at the pool2
('451091', '0', '0', '0', '1', '0', '1', '1', '0', '0', '0', '0', '0', '0', '85096'), -- Aysa at the pool2

('451049', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82358'), -- Old Man Liang at the tower
('451042', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '82358'); -- Aysa at the tower

-- Fix invalid scripts
UPDATE `smart_scripts` SET `event_param3` = '120000', `event_param4` = '120000' WHERE (`entryorguid` = '-450361') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');
UPDATE `smart_scripts` SET `event_param3` = '120000', `event_param4` = '120000' WHERE (`entryorguid` = '-450358') and (`source_type` = '0') and (`id` = '0') and (`link` = '0');

-- locations for cart and tender dude talk
DELETE FROM `areatrigger_scripts` WHERE `entry` IN (7258, 7822);
INSERT INTO `areatrigger_scripts` VALUES
(7258, 'at_cart_locations'),
(7822, 'at_cart_locations');

DELETE FROM `creature_text` WHERE `CreatureID` = 57712;
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`BroadcastTextId`,`TextRange`,`comment`) VALUES 
(57712,0,0,'Hello friend!  You\'re welcome to use my cart if you like.  It will take you to the Dai-Lo Farmstead.',12,0,100,3,0,0,56406,0,'Delivery Cart Tender to Player'),
(57712,1,0,'Hello friend!  You\'re welcome to use my cart if you like.  It will take you to the Temple of Five Dawns.',12,0,100,3,0,0,56415,0,'Delivery Cart Tender to Player');

-- Yak & cart & tender
UPDATE `creature_template` SET `VehicleId` = '1944' WHERE (`entry` = '57208');
UPDATE `creature_template` SET `movementId` = 132 WHERE `Entry` = 57207;
UPDATE `creature_template` SET `ScriptName` = 'npc_ox_cart' WHERE `Entry` IN (57208,57207);

DELETE FROM `vehicle_template` WHERE `creatureId` = 57208;
INSERT INTO `vehicle_template` VALUES (57208, 0, NULL,0);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (57710,59497);
INSERT INTO `npc_spellclick_spells` VALUES
('57710', '107784', '3', '0'),
('57710', '115904', '1', '0'),
('59497', '114453', '3', '0'),
('59497', '115904', '1', '0');

DELETE FROM `creature_template_addon` WHERE `entry` IN (57208,57207,57710,57709);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('57207', '5720700', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', ''), -- 
('57208', '5720800', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', '108692'),
('57709', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '84886 94570'),
('57710', '0', '0', '0', '0', '0', '1', '1', '0', '0', '0', '0', '0', '0', '108692 94570 84886');

DELETE FROM `waypoint_path` WHERE `PathId` IN (5720700,5720800);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(5720700,1,0,'The Wandering Isle Cart & Ox'),
(5720800,1,0,'The Wandering Isle Cart & Ox');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (5720700,5720800);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ) values
(5720700, '0', '981.766', '2857.77', '88.7352'),
(5720700, '1', '978.016', '2848.52', '87.2352'),
(5720700, '2', '948.266', '2835.52', '87.2352'),
(5720700, '3', '919.016', '2825.02', '87.2352'),
(5720700, '4', '895.766', '2811.27', '86.9852'),
(5720700, '5', '880.766', '2807.02', '83.9852'),
(5720700, '6', '853.516', '2799.02', '83.7352'),
(5720700, '7', '825.516', '2790.77', '78.7352'),
(5720700, '8', '802.266', '2785.27', '76.7352'),
(5720700, '9', '790.266', '2792.77', '75.7352'),
(5720700, '10', '763.016', '2815.52', '76.2352'),
(5720700, '11', '746.766', '2834.77', '75.7352'),
(5720700, '12', '746.266', '2851.77', '75.9852'),
(5720700, '13', '755.516', '2869.77', '75.2352'),
(5720700, '14', '753.516', '2890.02', '74.9852'),
(5720700, '15', '732.766', '2915.02', '74.7352'),
(5720700, '16', '703.766', '2939.02', '74.7352'),
(5720700, '17', '683.766', '2954.77', '75.2352'),
(5720700, '18', '674.016', '2975.52', '74.9852'),
(5720700, '19', '667.266', '2983.27', '75.2352'),
(5720700, '20', '660.516', '2990.77', '79.9852'),
(5720700, '21', '652.266', '2999.52', '74.9852'),
(5720700, '22', '645.016', '3006.27', '74.7352'),
(5720700, '23', '629.016', '3016.52', '75.4852'),
(5720700, '24', '616.016', '3035.52', '76.7352'),
(5720700, '25', '612.766', '3061.27', '80.4852'),
(5720700, '26', '612.766', '3080.27', '84.4852'),
(5720700, '27', '618.01', '3107.21', '87.5104'),
(5720700, '28', '617.01', '3131.96', '87.7604'),
(5720700, '29', '607.01', '3143.96', '88.0104'),
(5720700, '30', '591.51', '3146.71', '88.0104'),
(5720700, '31', '574.51', '3148.71', '87.0104'),
(5720700, '32', '555.01', '3159.21', '78.0104'),
(5720700, '33', '547.26', '3174.96', '77.5104'),
(5720800, '0', '981.864', '2857.8', '88.8159'),
(5720800, '1', '978.114', '2848.55', '87.3159'),
(5720800, '2', '948.364', '2835.55', '87.3159'),
(5720800, '3', '919.114', '2825.05', '87.3159'),
(5720800, '4', '895.614', '2811.3', '87.0659'),
(5720800, '5', '880.614', '2807.05', '83.8159'),
(5720800, '6', '853.364', '2799.05', '83.8159'),
(5720800, '7', '825.614', '2790.8', '78.5659'),
(5720800, '8', '802.364', '2785.3', '76.8159'),
(5720800, '9', '790.364', '2792.8', '75.8159'),
(5720800, '10', '762.864', '2815.55', '76.0659'),
(5720800, '11', '746.614', '2834.8', '75.8159'),
(5720800, '12', '746.364', '2851.8', '76.0659'),
(5720800, '13', '755.614', '2869.8', '75.3159'),
(5720800, '14', '753.364', '2890.05', '75.0659'),
(5720800, '15', '732.864', '2915.05', '74.5659'),
(5720800, '16', '703.614', '2939.05', '74.5659'),
(5720800, '17', '683.864', '2954.8', '75.3159'),
(5720800, '18', '674.114', '2975.55', '75.0659'),
(5720800, '19', '667.364', '2983.3', '75.3159'),
(5720800, '20', '660.614', '2990.55', '79.8159'),
(5720800, '21', '652.364', '2999.3', '75.0659'),
(5720800, '22', '645.114', '3006.3', '74.8159'),
(5720800, '23', '629.114', '3016.55', '75.3159'),
(5720800, '24', '616.114', '3035.55', '76.5659'),
(5720800, '25', '612.864', '3061.3', '80.5659'),
(5720800, '26', '612.864', '3080.3', '84.3159'),
(5720800, '27', '618.052', '3107.17', '87.5442'),
(5720800, '28', '617.052', '3131.92', '87.7942'),
(5720800, '29', '607.052', '3143.92', '87.7942'),
(5720800, '30', '591.302', '3146.67', '88.0442'),
(5720800, '31', '574.552', '3148.67', '87.0442'),
(5720800, '32', '555.052', '3159.17', '78.0442'),
(5720800, '33', '547.302', '3174.92', '77.2942');

-- Misc mobs fixes
-- Remove SmartAI from 54130 since there is scripting in place
UPDATE `creature_template` SET `AIName` = '' WHERE `Entry` = 54130;
-- Added smart scripts missing for 54131 and 57164
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (54131,57164) AND `source_type` = 0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('54131', '0', '0', '0', '11', '0', '100', '0', '0', '0', '0', '0', '0', '', '8', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - On Respawn - Set React State'),
('54131', '0', '1', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '11', '56739', '1', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - On Aggro - Despawn Vehicle'),
('54131', '0', '2', '0', '7', '0', '100', '0', '0', '0', '0', '0', '0', '', '41', '2000', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - On Evade - Despawn'),
('54131', '0', '3', '0', '0', '0', '100', '0', '2000', '8000', '4000', '8000', '0', '', '11', '121484', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - IC - Cast Ook\'em'),
('54131', '0', '4', '0', '0', '0', '50', '0', '5000', '10000', '5000', '10000', '0', '', '11', '115006', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Hozen - IC - Cast Hozen Rage'),
('57164', '0', '0', '0', '4', '0', '100', '0', '0', '0', '0', '0', '0', '', '11', '107526', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Fe-Feng Leaper - On Aggro - Cast Reckless Leap');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (55506);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (55506) AND `source_type` = 0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('55506', '0', '0', '0', '0', '0', '100', '0', '8000', '12000', '35000', '40000', '0', '', '11', '125383', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Raggis - Update IC - Cast Burrow');

-- Jojo at Farmstead
DELETE FROM `waypoint_path` WHERE `PathId` IN (5766900);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(5766900,0,0,'Jojo Ironbrow (spell spawn farmstead)');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (5766900);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ, Orientation,Delay) values
(5766900, '0', '599.929', '3140.35', '88.2049', '0', '0'),
(5766900, '1', '594.019', '3144.98', '87.8352', '0', '0'),
(5766900, '2', '585.708', '3147.41', '87.8352', '0', '0'),
(5766900, '3', '578.502', '3150.31', '87.6758', '0', '0'),
(5766900, '4', '568.241', '3155.38', '84.0477', '0', '0'),
(5766900, '5', '556.214', '3163.61', '77.2933', '0', '0'),
(5766900, '6', '550.847', '3172.42', '77.0957', '0', '0'),
(5766900, '7', '549.276', '3180.51', '77.0957', '0', '0'),
(5766900, '8', '548.031', '3191.97', '77.0957', '0', '0'),
(5766900, '9', '546.122', '3202.98', '76.5399', '0', '0'),
(5766900, '10', '537.189', '3213.91', '75.7559', '0', '0'),
(5766900, '11', '527.2', '3219.97', '74.6265', '0', '0'),
(5766900, '12', '516.071', '3230.63', '74.093', '0', '0'),
(5766900, '13', '511.005', '3245.36', '76.2209', '0', '0'),
(5766900, '14', '510.042', '3258.1', '77.5846', '0', '0'),
(5766900, '15', '511.365', '3274.88', '76.3656', '0', '0'),
(5766900, '16', '515.174', '3293.1', '74.0391', '0', '0'),
(5766900, '17', '517.49', '3302.16', '73.4592', '0', '0'),
(5766900, '18', '522.252', '3313.48', '73.2886', '0', '0'),
(5766900, '19', '524.229', '3319.17', '74.429', '0', '0'),
(5766900, '20', '527.083', '3323.94', '77.9471', '0', '0'),
(5766900, '21', '529.615', '3328.26', '77.2828', '0', '0'),
(5766900, '22', '532.233', '3332.62', '73.9326', '0', '0'),
(5766900, '23', '536.986', '3339.77', '74.2679', '0', '0'),
(5766900, '24', '541.793', '3346.9', '75.5099', '0', '0'),
(5766900, '25', '545.04', '3354.28', '76.3512', '0', '0'),
(5766900, '26', '551.722', '3360.92', '77.6802', '0', '0'),
(5766900, '27', '562.785', '3369.31', '79.3041', '0', '0'),
(5766900, '28', '573.752', '3374.6', '81.1686', '0', '0'),
(5766900, '29', '584.516', '3376.84', '83.0058', '0', '0'),
(5766900, '30', '602.658', '3380.54', '86.0039', '0', '0'),
(5766900, '31', '616.135', '3383.62', '88.2383', '0', '0'),
(5766900, '32', '629.408', '3388.19', '91.4238', '0', '0'),
(5766900, '33', '641.904', '3394.52', '94.8907', '0', '0'),
(5766900, '34', '656.509', '3405.72', '99.4795', '0', '0'),
(5766900, '35', '669.83', '3419.28', '104.06', '0', '0'),
(5766900, '36', '680.092', '3431.23', '108.153', '0', '0'),
(5766900, '37', '685.416', '3441.67', '111.014', '0', '0'),
(5766900, '38', '689.628', '3453.92', '115.057', '0', '0'),
(5766900, '39', '694.601', '3465.64', '117.579', '0', '0'),
(5766900, '40', '705.93', '3473.97', '119.103', '0', '0'),
(5766900, '41', '719.997', '3481.66', '123.177', '0', '0'),
(5766900, '42', '730.43', '3485.82', '128.295', '0', '0'),
(5766900, '43', '740.32', '3491.29', '132.969', '0', '0'),
(5766900, '44', '747.703', '3497.73', '136.24', '0', '0'),
(5766900, '45', '754.712', '3506.09', '137.961', '0', '0'),
(5766900, '46', '762.233', '3509.7', '140.319', '0', '0'),
(5766900, '47', '770.802', '3509.02', '140.395', '0', '0'),
(5766900, '48', '781.087', '3506.4', '141.267', '0', '0'),
(5766900, '49', '783.833', '3502.92', '141.293', '0', '0'),
(5766900, '50', '782.533', '3499.91', '141.293', '0', '0');

UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `Entry` IN (57669);
DELETE FROM `smart_scripts` WHERE `entryorguid` IN (57669,5766900);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param_string`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
('57669', '0', '0', '0', '54', '0', '100', '0', '0', '0', '0', '0', '0', '', '80', '5766900', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Just Spawned - Run Script'),
('57669', '0', '1', '0', '58', '0', '100', '0', '50', '6', '0', '0', '0', '', '41', '0', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - On WP Ended - Despawn'),
('5766900', '9', '0', '0', '0', '0', '100', '0', '1000', '1000', '1000', '1000', '0', '', '1', '0', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5766900', '9', '1', '0', '0', '0', '100', '0', '7000', '7000', '1000', '1000', '0', '', '128', '1', '1', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set AI AnimKit'),
('5766900', '9', '2', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '69', '1', '0', '0', '0', '0', '0', '8', '0', '0', '0', '599.215', '3132.27', '89.0657', '0', 'Jojo Ironbrow - Move to Pos'),
('5766900', '9', '3', '0', '0', '0', '100', '0', '5000', '5000', '1000', '1000', '0', '', '11', '129293', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Cast Jojo Headbash, Planks Cast'),
('5766900', '9', '4', '0', '0', '0', '100', '0', '2500', '2500', '1000', '1000', '0', '', '1', '1', '0', '0', '0', '0', '0', '7', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Talk'),
('5766900', '9', '5', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108831', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Auras'),
('5766900', '9', '6', '0', '0', '0', '100', '0', '100', '100', '0', '0', '0', '', '28', '108831', '0', '0', '0', '0', '0', '18', '50', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Remove Player Auras'),
('5766900', '9', '7', '0', '0', '0', '100', '0', '0', '0', '1000', '1000', '0', '', '45', '1', '1', '0', '0', '0', '0', '11', '0', '10', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Set Data'),
('5766900', '9', '8', '0', '0', '0', '100', '0', '8000', '8000', '1000', '1000', '0', '', '53', '0', '6', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', 'Jojo Ironbrow - Start WP');

-- Lamplighter Sunny
DELETE FROM `creature_template_addon` WHERE `entry` IN (64939,55478,57669);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `MountCreatureID`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvPFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES 
('64939', '6493900', '0', '0', '0', '0', '0', '1', '0', '0', '0', '0', '0', '0', '126986'),
('55478', '0', '0', '0', '1', '0', '1', '1', '0', '461', '0', '0', '0', '0', '60921'),
('57669', '5766900', '0', '0', '0', '0', '0', '1', '0', '0', '2935', '0', '0', '0', '');


-- Lamp Lighter needs waypoint movement
UPDATE `creature` SET `zoneId` = '5736', `areaId` = '5881', `wander_distance` = '0', `MovementType` = '2' WHERE (`guid` = '451131');

DELETE FROM `waypoint_path` WHERE `PathId` IN (6493900);
INSERT INTO `waypoint_path` (`PathId`, `MoveType`, `Flags`, `Comment`) VALUES
(6493900,0,0,'Lamplighter Sunny');

DELETE FROM `waypoint_path_node` WHERE `PathId` IN (6493900);
INSERT INTO `waypoint_path_node` (PathId,NodeId,PositionX, PositionY,PositionZ, Orientation,Delay) values
(6493900, '0', '612.982', '3096.65', '86.6167', '0', '0'),
(6493900, '1', '612.482', '3088.15', '85.3667', '0', '0'),
(6493900, '2', '610.982', '3079.9', '84.1167', '0', '0'),
(6493900, '3', '611.232', '3067.15', '81.6167', '0', '0'),
(6493900, '4', '611.482', '3050.65', '78.1167', '0', '0'),
(6493900, '5', '612.732', '3041.15', '76.8667', '0', '0'),
(6493900, '6', '619.232', '3025.4', '75.8667', '0', '0'),
(6493900, '7', '624.71', '3015.89', '75.1487', '0', '7000'),
(6493900, '8', '622.442', '3021.02', '75.5442', '0', '0'),
(6493900, '9', '617.692', '3028.02', '76.0442', '0', '0'),
(6493900, '10', '613.942', '3037.02', '76.7942', '0', '0'),
(6493900, '11', '611.692', '3045.27', '77.5442', '0', '0'),
(6493900, '12', '611.442', '3053.52', '79.0442', '0', '0'),
(6493900, '13', '611.692', '3062.27', '80.5442', '0', '0'),
(6493900, '14', '611.942', '3071.02', '82.2942', '0', '0'),
(6493900, '15', '614.692', '3080.27', '84.0442', '0', '0'),
(6493900, '16', '620.674', '3085.16', '85.9397', '0', '7000'),
(6493900, '17', '619.254', '3091.48', '86.6491', '0', '0'),
(6493900, '18', '618.004', '3101.98', '87.1491', '0', '0'),
(6493900, '19', '616.504', '3112.48', '87.6491', '0', '0'),
(6493900, '20', '615.004', '3123.23', '87.8991', '0', '0'),
(6493900, '21', '617.004', '3137.73', '87.8991', '0', '0'),
(6493900, '22', '618.335', '3147.79', '87.8586', '0', '7000'),
(6493900, '23', '611.154', '3148.1', '88.101', '0', '0'),
(6493900, '24', '601.904', '3149.35', '87.851', '0', '0'),
(6493900, '25', '597.154', '3148.85', '87.851', '0', '0'),
(6493900, '26', '592.472', '3143.41', '87.8434', '0', '7000'),
(6493900, '27', '587.675', '3146.72', '88.0353', '0', '0'),
(6493900, '28', '580.925', '3146.47', '87.7853', '0', '0'),
(6493900, '29', '572.175', '3147.72', '86.5353', '0', '0'),
(6493900, '30', '564.175', '3150.47', '83.0353', '0', '0'),
(6493900, '31', '555.925', '3155.72', '78.7853', '0', '0'),
(6493900, '32', '549.175', '3161.97', '77.5353', '0', '0'),
(6493900, '33', '545.675', '3169.72', '77.2853', '0', '0'),
(6493900, '34', '544.425', '3177.97', '77.2853', '0', '0'),
(6493900, '35', '543.175', '3186.72', '77.2853', '0', '0'),
(6493900, '36', '542.425', '3194.22', '77.5353', '0', '0'),
(6493900, '37', '539.675', '3202.47', '77.0353', '0', '0'),
(6493900, '38', '535.878', '3207.53', '76.2272', '0', '7000'),
(6493900, '39', '522.16', '3220.36', '74.4621', '0', '0'),
(6493900, '40', '514.915', '3231.21', '74.093', '0', '0'),
(6493900, '41', '512.849', '3240.09', '75.0834', '0', '0'),
(6493900, '42', '511.016', '3248.97', '76.8458', '0', '0'),
(6493900, '43', '511.754', '3261.91', '77.5846', '2.98451', '12000'),
(6493900, '44', '508.482', '3259.15', '77.6167', '0', '0'),
(6493900, '45', '505.232', '3251.15', '77.3667', '0', '0'),
(6493900, '46', '504.982', '3244.65', '76.3667', '0', '0'),
(6493900, '47', '507.232', '3236.65', '75.1167', '0', '0'),
(6493900, '48', '511.232', '3228.65', '74.3667', '0', '0'),
(6493900, '49', '518.482', '3220.4', '74.6167', '0', '0'),
(6493900, '50', '528.982', '3213.65', '75.8667', '0', '0'),
(6493900, '51', '536.232', '3209.4', '76.3667', '0', '0'),
(6493900, '52', '540.482', '3201.4', '76.8667', '0', '0'),
(6493900, '53', '543.732', '3190.15', '77.1167', '0', '0'),
(6493900, '54', '544.732', '3177.4', '77.1167', '0', '0'),
(6493900, '55', '547.982', '3165.4', '77.1167', '0', '0'),
(6493900, '56', '554.232', '3157.15', '77.8667', '0', '0'),
(6493900, '57', '561.482', '3152.65', '81.1167', '0', '0'),
(6493900, '58', '567.232', '3149.4', '84.3667', '0', '0'),
(6493900, '59', '574.732', '3147.15', '87.3667', '0', '0'),
(6493900, '60', '583.982', '3145.65', '87.8667', '0', '0'),
(6493900, '61', '593.232', '3144.4', '87.8667', '0', '0'),
(6493900, '62', '599.982', '3142.9', '88.1167', '0', '0'),
(6493900, '63', '606.482', '3141.4', '87.8667', '0', '0'),
(6493900, '64', '615.232', '3137.4', '87.8667', '0', '0'),
(6493900, '65', '619.982', '3130.4', '87.8667', '0', '0'),
(6493900, '66', '621.482', '3123.4', '87.8667', '0', '0'),
(6493900, '67', '619.732', '3112.4', '87.8667', '0', '0'),
(6493900, '68', '616.982', '3104.65', '87.3667', '0', '0');