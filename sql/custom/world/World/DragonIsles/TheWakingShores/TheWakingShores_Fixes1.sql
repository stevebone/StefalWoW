-- Update to match your DB
SET @NPCTEXTID := 610033;

DELETE FROM `creature_equip_template` WHERE (`ID`=1 AND `CreatureID` IN (185589,186110,198163,196556,187609,188693,187272,192344,187598,191010,192371)) OR (`ID`=3 AND `CreatureID` IN (186239,187602)) OR (`ID`=4 AND `CreatureID`=187602) OR (`ID`=2 AND `CreatureID` IN (187541,187602));
INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `AppearanceModID1`, `ItemVisual1`, `ItemID2`, `AppearanceModID2`, `ItemVisual2`, `ItemID3`, `AppearanceModID3`, `ItemVisual3`, `VerifiedBuild`) VALUES
(185589, 1, 197830, 0, 0, 192133, 0, 0, 0, 0, 0, 67451), -- Worldbreaker Brute
(186110, 1, 197819, 0, 0, 0, 0, 0, 0, 0, 0, 67451), -- Djaradin Crustshaper
(198163, 1, 112097, 0, 0, 0, 0, 0, 0, 0, 0, 67451), -- Mammoth-Tamer Tavok
(196556, 1, 192046, 0, 0, 192142, 0, 0, 0, 0, 0, 67451), -- Proud Dragonhunter
(187609, 1, 39763, 0, 0, 115083, 0, 0, 0, 0, 0, 67451), -- Earthcaller Yevaa
(186239, 3, 163608, 0, 0, 0, 0, 0, 0, 0, 0, 67451), -- Qalashi Skullhauler
(187602, 4, 124628, 0, 0, 192069, 0, 0, 0, 0, 0, 67451), -- Qalashi Scaleripper
(187602, 3, 163608, 0, 0, 192069, 0, 0, 0, 0, 0, 67451), -- Qalashi Scaleripper
(188693, 1, 107134, 0, 0, 0, 0, 0, 0, 0, 0, 67451), -- Boneshaper Jardak
(187272, 1, 189688, 0, 0, 0, 0, 0, 0, 0, 0, 67451), -- Forgemaster Bazentus
(192344, 1, 191870, 0, 0, 0, 0, 0, 0, 0, 0, 67451), -- Ashbinder Tornecha
(187541, 2, 189135, 0, 0, 0, 0, 0, 0, 0, 0, 67451), -- Obsidian Protector
(187598, 1, 192122, 0, 0, 0, 0, 0, 0, 0, 0, 67451), -- Rohzor Forgesmash
(187602, 2, 192122, 0, 0, 192069, 0, 0, 0, 0, 0, 67451), -- Qalashi Scaleripper
(191010, 1, 189688, 0, 0, 0, 0, 0, 0, 0, 0, 67451), -- Scalesmith Mita
(192371, 1, 116640, 0, 0, 0, 0, 0, 0, 0, 0, 67451); -- Qalashi Dusttwister

UPDATE `creature_equip_template` SET `VerifiedBuild`=67451 WHERE (`ID`=1 AND `CreatureID` IN (184657,180393,182725,182727,192387,185882,191058,196663,186511,190983,187813,199298,186451,191014,199020,187600,187908,186109,189557,186483,186638,186239,191524,197788,186452,189604,191525,186609,191476,189472,189467,189227,189464,189265,189471,193293,193944,189478,189901,191498,189340,189466,192788,192787,192786,189235,193377,189643,191940,193967,191876,187599,187602,187541,189199,192164,195957,188380,192121,186324,187533,198877,188443,191711,191710,188351,189202,188625,188344,188623,188179,193456,188336,213636,209065,205781)) OR (`ID`=2 AND `CreatureID` IN (186483,186239));

DELETE FROM `gameobject_template_addon` WHERE `entry` IN (380540 /*Ornamented Statue*/, 324067 /*Instance Portal*/, 376645 /*Rock Wall*/, 386655 /*Barrier (Wide)*/, 382287 /*Blooming Fangtooth*/, 376121 /*The Black Locus*/, 375244 /*Frigid Bubble Poppy*/, 376338 /*Wurmling Bone Pile*/, 381521 /*Drajardin Grindstone*/, 379263 /*Rich Draconium Deposit*/, 380840 /*Torn Riding Pack*/, 381671 /*Archivists' Request*/, 381664 /*Archivists' Request*/, 381483 /*Marmoni's Prize*/, 376454 /*Bag*/, 376453 /*Camera*/, 376711 /*Reliquary Book Stack*/, 378169 /*Reliquary Compass*/, 378168 /*Expedition Scroll*/, 378171 /*Reliquary Magnifying Glass*/, 378167 /*Dragon Scroll Stack*/, 378166 /*Reliquary Book Stack*/, 376104 /*Siege Arbalest*/, 376151 /*Drajardin Grindstone*/, 380826 /*Focusing Crystal*/, 380557 /*Igneous Geode*/, 380606 /*Focusing Crystal*/, 380825 /*Focusing Crystal*/, 381420 /*Dragonsteel Axe*/, 380648 /*Odd Book*/, 382241 /*Earth-Warder's Forge*/, 381336 /*One Small Part*/, 381340 /*Neltharus Stands*/, 376920 /*Vault Barrier*/, 379262 /*10DU_Neltharus_WallCoffer01*/, 379256 /*10DU_Neltharus_WallCoffer01*/, 379257 /*10DU_Neltharus_WallCoffer01*/, 379255 /*10DU_Neltharus_WallCoffer01*/, 379258 /*10DU_Neltharus_WallCoffer01*/, 379259 /*10DU_Neltharus_WallCoffer01*/, 379260 /*10DU_Neltharus_WallCoffer01*/, 379261 /*10DU_Neltharus_WallCoffer01*/, 384317 /*Crumpled Schematic*/, 377496 /*10DU_Neltharus_Door01 - C*/, 377495 /*10DU_Neltharus_Door01 - A*/, 377494 /*10DU_Neltharus_Door01 - B*/, 377493 /*Stairwell Door*/, 377966 /*10DU_Neltharus_DragonHead_FX02*/);
INSERT INTO `gameobject_template_addon` (`entry`, `faction`, `flags`, `WorldEffectID`, `AIAnimKitID`) VALUES
(380540, 0, 0x24, 0, 0), -- Ornamented Statue
(324067, 0, 0x20, 0, 0), -- Instance Portal
(376645, 0, 0x10020, 0, 0), -- Rock Wall
(386655, 0, 0x20, 0, 0), -- Barrier (Wide)
(382287, 0, 0x44000, 0, 0), -- Blooming Fangtooth
(376121, 0, 0x40020, 0, 0), -- The Black Locus
(375244, 0, 0x44000, 0, 0), -- Frigid Bubble Poppy
(376338, 35, 0x10, 2099, 0), -- Wurmling Bone Pile
(381521, 0, 0x0, 0, 13683), -- Drajardin Grindstone
(379263, 94, 0x44000, 0, 0), -- Rich Draconium Deposit
(380840, 0, 0x0, 2437, 0), -- Torn Riding Pack
(381671, 0, 0x4, 0, 0), -- Archivists' Request
(381664, 0, 0x4, 0, 0), -- Archivists' Request
(381483, 0, 0x204000, 0, 0), -- Marmoni's Prize
(376454, 190, 0x0, 0, 0), -- Bag
(376453, 190, 0x0, 0, 0), -- Camera
(376711, 190, 0x0, 0, 0), -- Reliquary Book Stack
(378169, 190, 0x0, 0, 0), -- Reliquary Compass
(378168, 190, 0x0, 0, 0), -- Expedition Scroll
(378171, 190, 0x0, 0, 0), -- Reliquary Magnifying Glass
(378167, 190, 0x0, 0, 0), -- Dragon Scroll Stack
(378166, 190, 0x0, 0, 0), -- Reliquary Book Stack
(376104, 0, 0x42000, 0, 0), -- Siege Arbalest
(376151, 0, 0x0, 0, 13683), -- Drajardin Grindstone
(380826, 0, 0x40000, 0, 0), -- Focusing Crystal
(380557, 0, 0x4, 0, 0), -- Igneous Geode
(380606, 0, 0x40000, 0, 0), -- Focusing Crystal
(380825, 0, 0x40000, 0, 0), -- Focusing Crystal
(381420, 0, 0x0, 2100, 0), -- Dragonsteel Axe
(380648, 0, 0x40000, 2100, 0), -- Odd Book
(382241, 0, 0x10000, 0, 0), -- Earth-Warder's Forge
(381336, 0, 0x42000, 0, 0), -- One Small Part
(381340, 0, 0x42000, 0, 0), -- Neltharus Stands
(376920, 114, 0x0, 0, 0), -- Vault Barrier
(379262, 0, 0x10, 0, 0), -- 10DU_Neltharus_WallCoffer01
(379256, 0, 0x10, 0, 0), -- 10DU_Neltharus_WallCoffer01
(379257, 0, 0x10, 0, 0), -- 10DU_Neltharus_WallCoffer01
(379255, 0, 0x10, 0, 0), -- 10DU_Neltharus_WallCoffer01
(379258, 0, 0x10, 0, 0), -- 10DU_Neltharus_WallCoffer01
(379259, 0, 0x10, 0, 0), -- 10DU_Neltharus_WallCoffer01
(379260, 0, 0x10, 0, 0), -- 10DU_Neltharus_WallCoffer01
(379261, 0, 0x10, 0, 0), -- 10DU_Neltharus_WallCoffer01
(384317, 0, 0x0, 18570, 0), -- Crumpled Schematic
(377496, 0, 0x30, 0, 0), -- 10DU_Neltharus_Door01 - C
(377495, 0, 0x30, 0, 0), -- 10DU_Neltharus_Door01 - A
(377494, 0, 0x30, 0, 0), -- 10DU_Neltharus_Door01 - B
(377493, 0, 0x20, 0, 0), -- Stairwell Door
(377966, 0, 0x30, 0, 0); -- 10DU_Neltharus_DragonHead_FX02

UPDATE `gameobject_template_addon` SET `faction`=3273 WHERE `entry`=268888; -- Fishing Bobber
UPDATE `gameobject_template_addon` SET `flags`=0x44000 WHERE `entry`=381102; -- Serevite Deposit
UPDATE `gameobject_template_addon` SET `flags`=0x44000 WHERE `entry`=376580; -- Djaradin Cache
UPDATE `gameobject_template_addon` SET `flags`=0x10020 WHERE `entry`=376646; -- Rock Wall


DELETE FROM `creature_template_addon` WHERE `entry` IN (190132 /*190132 (Shikaar Ohuna)*/, 184656 /*184656 (Loyal Bakar)*/, 184657 /*184657 (Shikaar Hunter) - Conversation Aura: Talk/Salute/Cheer/Laugh [DNT]*/, 192387 /*192387 (Shikaar Scout) - [DNT] Generic - Centaur - Falconeer Aura*/, 192652 /*192652 (Clearwater Bull) - Permanent Feign Death*/, 190530 /*190530 (Wild Proto-Drake)*/, 192854 /*192854 (Clearwater Ottuk)*/, 187799 /*187799 (Reed Skimmer)*/, 185882 /*185882 (Scout Tomul)*/, 191478 /*191478 (Meadowhoof Calf) - Permanent Feign Death*/, 190909 /*190909 (Swift Hornstrider) - Hearty*/, 190908 /*190908 (Plainswalker Calf) - Hearty*/, 198670 /*198670 (Tracker Dragon Glyph)*/, 190905 /*190905 (Meadowhoof Musken) - Hearty*/, 188088 /*188088 (Windblessed Ohuna)*/, 198458 /*198458 (Dragon Glyph) - Visual*/, 192347 /*192347 (Prairie Buzzwing)*/, 189066 /*189066 (Thaelin Darkanvil)*/, 191788 /*191788 (Wild Proto-Drake) - Cosmetic - Sleep Zzz (With Aggro Change) - Head (Scale 4,  Down 1)*/, 191058 /*191058 (Shikaar Fighter) - Wounded Hunter*/, 196663 /*196663 (Ruby Scaleguard)*/, 194633 /*194633 (Etched Ancient Monolith) - Desaturate*/, 190503 /*190503 (Bronze Timekeeper) - Cosmetic Race Flag Overhead*/, 196673 /*196673 (Vacationing Tender)*/, 196668 /*196668 (Wistful Watcher)*/, 194092 /*194092 (Wild Proto-Drake) - Conversation Aura: Talk/Roar/Laugh/Threaten [DNT]*/, 191180 /*191180 (Wild Proto-Drake) - Cosmetic - Sleep Zzz (With Aggro Change) - Head (Scale 4,  Down 1)*/, 191395 /*191395 (Wild Proto-Drake) - Conversation Aura: Talk/Roar/Laugh/Threaten [DNT]*/, 190812 /*190812 (Wild Proto-Drake) - Sparring Aura: AttackUnarmed/DragonSpit [DNT]*/, 189486 /*189486 (Captured Proto-Drake) - Cosmetic: Wind Vortex [DNT]*/, 189515 /*189515 (Infused Proto-Drake)*/, 191413 /*191413 (Wild Proto-Drake) - Cosmetic - Sleep Zzz (With Aggro Change) - Head (Scale 4,  Down 1)*/, 189823 /*189823 (Ravenous Nestling) - Hearty*/, 186110 /*186110 (Djaradin Crustshaper) - Permanent Feign Death (Flies)*/, 181764 /*181764 (Lava Phoenix) - From the Ashes*/, 198163 /*198163 (Mammoth-Tamer Tavok) - Cosmetic - SitGround - Lower Body Anim Kit - Breakable*/, 198200 /*198200 (Stonescale Proto-Dragon)*/, 196556 /*196556 (Proud Dragonhunter) - Channel: Sharpen Sword (T3) [DNT]*/, 181029 /*181029 (Generic - Empty Bunny) - Cosmetic - Black Smoke + Fire*/, 186509 /*186509 (Modak Flamespit) - Channel: Lava Orb (Can Move) [DNT]*/, 191479 /*191479 (Charred Hornspike) - Permanent Feign Death (Flies)*/, 191476 /*191476 (Searing Flame Harchek) - [DNT] Searing Meat, [DNT]Visual*/, 196605 /*196605 (Invis Bunny Template - Gigantic AOI)*/, 186109 /*186109 (Qalashi Necksnapper)*/, 191679 /*191679 (Magmammoth Bull) - Hearty*/, 189557 /*189557 (Qalashi Boltthrower) - Carry Arbalist Ammo*/, 191708 /*191708 (Wild Proto-Drake)*/, 191043 /*191043 (Itchy Sparkwobble) - Poisoned Blades*/, 190630 /*190630 (Lava Tentacles) - Ride Vehicle*/, 190023 /*190023 (Lava Tentacles) - Ride Vehicle*/, 191498 /*191498 (Qalashi Mammoth Trainer)*/, 189235 /*189235 (Overseer Lahar) - Imbued Magma, -Unknown-*/, 192781 /*192781 (Ore Elemental) - Granite Shell*/, 187073 /*187073 (Ruby Sitter) - Egg Sparkles [DNT]*/, 194088 /*194088 (Wild Proto-Drake) - Cosmetic - Sleep Zzz (With Aggro Change) - Head (Scale 4,  Down 1)*/, 187709 /*187709 (Handhold)*/, 187710 /*187710 (Handhold)*/, 187708 /*187708 (Handhold)*/, 187711 /*187711 (Handhold)*/, 191704 /*191704 (Apex Proto-Dragon)*/, 195160 /*195160 (Igneous Wanderer)*/, 188234 /*188234 (Earthen Ward) - Elemental Ward*/, 186638 /*186638 (Qalashi Crustshaper)*/, 194915 /*194915 (Invis Bunny) - Slime Spawn Aura (Dummy)*/, 189942 /*189942 (Wrathion) - Cosmetic - Alpha State 0%*/, 186261 /*186261 (Restless Lava) - Lava Pool*/, 189581 /*189581 (Lava Fledgling) - Hearty*/, 197016 /*197016 (Fishing Net) - Loc 5 Net 1 Controller Aura [DNT]*/, 186483 /*186483 (Qalashi Steelcrafter) - Sharpen Weapon*/, 197645 /*197645 (Daring Fisher) - Fishing*/, 193967 /*193967 (Dragonbane Wingshredder) - Air Guard, [DNT] Anti-Air Marker*/, 191525 /*191525 (Qalashi Wallcrasher) - Permanent Feign Death (Flies)*/, 191876 /*191876 (Goruk Steelwall) - Cosmetic - Sleep Zzz Breakable (Djaradin)*/, 191940 /*191940 (Qalashi Necksnapper) - Cosmetic - Sleep Zzz Breakable (Djaradin)*/, 186239 /*186239 (Qalashi Skullhauler) - Permanent Feign Death (Flies)*/, 189643 /*189643 (Qalashi Crustshaper) - Permanent Feign Death (Flies)*/, 181763 /*181763 (Lava Phoenix)*/, 195128 /*195128 (Invisible Bunny)*/, 195001 /*195001 (Invisible Bunny)*/, 195009 /*195009 (Invisible Bunny)*/, 195127 /*195127 (Invisible Bunny)*/, 187868 /*187868 (Molten Uprising) - Lava Pool*/, 188141 /*188141 (Malsiran)*/, 196783 /*196783 (Generic)*/, 199298 /*199298 (Qalashi Skullhauler) - Channel: Carry Bone Bin [DNT]*/, 187867 /*187867 (Qalashi Magmacrafter) - Channel: Lava (CustomSpell03) [DNT]*/, 187366 /*187366 (Worldcarver Wurmling) - Hearty*/, 196772 /*196772 (Battleseared Magmatusk) - AI Cast AoE*/, 196858 /*196858 (Fyranian)*/, 198737 /*198737 (Firewing)*/, 189199 /*189199 (Vadrya Valte)*/, 192164 /*192164 (Gringot Coldsteel)*/, 195957 /*195957 (Blacktalon Strategist) - Read Scroll*/, 192121 /*192121 (Falron Greygold)*/, 192155 /*192155 (Felona Oregrab)*/, 186324 /*186324 (Blacktalon Schemer)*/, 198847 /*198847 (Swart)*/, 190910 /*190910 (Evorian) - Sabellian Transform, Wrathion Transform*/, 191710 /*191710 (Left)*/, 188351 /*188351 (Stabitha Sharptwist) - Avoidance, AI Regen / Crit, General Defensive Passive - Leather*/, 188415 /*188415 (Jovax the Angry)*/, 186451 /*186451 (Blacktalon Avenger)*/, 189202 /*189202 (Uirreg Stoutbrow)*/, 198877 /*198877 (Blacktalon Sentry)*/, 188440 /*188440 (Djaradin Prisoner)*/, 198854 /*198854 (Obsidian Outfitter) - Carrying Crate*/, 187524 /*187524 (Obsidian Bladewing)*/, 192356 /*192356 (Enraged Embers) - Animated Ash*/, 192344 /*192344 (Ashbinder Tornecha) - Channel: Earth Pool [DNT]*/, 187541 /*187541 (Obsidian Protector)*/, 187602 /*187602 (Qalashi Scaleripper) - Jagged Cuts*/, 213636 /*213636 (Captain Garrick) - -Unknown-, Adaptation, -Unknown-, Proc, -Unknown-, AI Buff Watcher, Devotion Aura, Devotion Aura*/, 190983 /*190983 (Qalashi Pillager) - Supply-Laden*/, 189825 /*189825 (Ravenous Nestling) - Ride Vehicle Hardcoded*/, 196336 /*196336 (Qalashi Flameslinger)*/, 187791 /*187791 (Earthen Ward) - Increased Threat (SERVERSIDE), Avoidance, Elemental Ward*/, 187599 /*187599 (Qalashi Bonebreaker)*/, 188135 /*188135 (Zepharion)*/, 192323 /*192323 (Trapped Whelpling)*/, 190314 /*190314 (Broodmother Dracasia)*/, 198630 /*198630 (Tracker Dragon Glyph)*/, 188155 /*188155 (Osoria) - Sabellian Transform, Growth*/, 188158 /*188158 (Baskilan)*/, 194984 /*194984 (Dragon Glyph) - Visual*/, 187919 /*187919 (Caldera Stomper)*/, 188140 /*188140 (Vazallia) - Sabellian Transform, Wrathion Transform*/, 192371 /*192371 (Qalashi Dusttwister) - Supply-Laden, Channel: Earth Pool [DNT]*/, 187600 /*187600 (Qalashi Stonemender)*/, 189826 /*189826 (Hanging Spider Vehicle)*/, 192564 /*192564 (Myridian) - Wrathion Transform, Sabellian Transform, Growth*/, 187908 /*187908 (Qalashi Scaleripper) - Jagged Cuts*/, 196791 /*196791 (Kixandria)*/, 190118 /*190118 (Winds of the Isles) - Visual*/, 190085 /*190085 (Grounding Spear) - Grounding Spear*/, 189669 /*189669 (Binding Spear) - Binding Spear*/, 194816 /*194816 (Forgewrought Monstrosity) - Forgewrought Fury*/, 189786 /*189786 (Blazing Aegis) - Blazing Aegis*/, 189265 /*189265 (Qalashi Bonetender) - Channel: Lava (ChannelCastOmni) [DNT]*/, 189478 /*189478 (Forgemaster Gorek) - -Unknown-*/, 193291 /*193291 (Apex Blazewing)*/, 189901 /*189901 (Warlord Sargha) - -Unknown-*/, 193293 /*193293 (Qalashi Warden) - Invisibility and Stealth Detection*/, 189340 /*189340 (Chargath, Bane of Scales) - Cosmetic Spear Stack, -Unknown-*/, 189466 /*189466 (Irontorch Commander) - -Unknown-, Invisibility and Stealth Detection*/, 194883 /*194883 (Cosmetic Flavor[DNT]) - Cosmetic Flavor[DNT]*/, 189474 /*189474 (The Scorching Forge)*/, 181861 /*181861 (Magmatusk) - -Unknown-*/, 197709 /*197709 (Draconic Tincture) - Draconic Tincture*/, 197337 /*197337 (Burning 3 Books) - Burning Books*/, 197333 /*197333 (Burning 1 Book) - Burning Book*/, 189472 /*189472 (Qalashi Lavabearer) - Lava Drip, Invisibility and Stealth Detection*/, 189266 /*189266 (Qalashi Trainee) - Dual Wield*/, 192788 /*192788 (Qalashi Thaumaturge)*/, 192786 /*192786 (Qalashi Plunderer)*/, 209068 /*209068 (Ash) - Predator's Thirst, Pet Check, -Unknown-, Pet Active*/, 189227 /*189227 (Qalashi Hunter) - Cosmetic Spear Stack*/, 209065 /*209065 (Austin Huxworth) - -Unknown-, -Unknown-, Adaptation, General Defensive Passive - Mail, Proc, -Unknown-, Override Energy Bar Color, Predator's Thirst, AI Buff Watcher, -Unknown-, -Unknown-*/, 234222 /*234222 (Helper) - -Unknown-*/, 209069 /*209069 (Nyx) - Predator's Thirst, Pet Check, -Unknown-*/, 189247 /*189247 (Tamed Phoenix)*/);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(190132, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 190132 (Shikaar Ohuna)
(184656, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 184656 (Loyal Bakar)
(184657, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '376147'), -- 184657 (Shikaar Hunter) - Conversation Aura: Talk/Salute/Cheer/Laugh [DNT]
(192387, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '361360'), -- 192387 (Shikaar Scout) - [DNT] Generic - Centaur - Falconeer Aura
(192652, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '130966'), -- 192652 (Clearwater Bull) - Permanent Feign Death
(190530, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 190530 (Wild Proto-Drake)
(192854, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 192854 (Clearwater Ottuk)
(187799, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 187799 (Reed Skimmer)
(185882, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, ''), -- 185882 (Scout Tomul)
(191478, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '299698'), -- 191478 (Meadowhoof Calf) - Permanent Feign Death
(190909, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '391254'), -- 190909 (Swift Hornstrider) - Hearty
(190908, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '391254'), -- 190908 (Plainswalker Calf) - Hearty
(198670, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 4, ''), -- 198670 (Tracker Dragon Glyph)
(190905, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '391254'), -- 190905 (Meadowhoof Musken) - Hearty
(188088, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 188088 (Windblessed Ohuna)
(198458, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 4, '393932'), -- 198458 (Dragon Glyph) - Visual
(192347, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 192347 (Prairie Buzzwing)
(189066, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 189066 (Thaelin Darkanvil)
(191788, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '371171'), -- 191788 (Wild Proto-Drake) - Cosmetic - Sleep Zzz (With Aggro Change) - Head (Scale 4,  Down 1)
(191058, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '376468'), -- 191058 (Shikaar Fighter) - Wounded Hunter
(196663, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 196663 (Ruby Scaleguard)
(194633, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '164429'), -- 194633 (Etched Ancient Monolith) - Desaturate
(190503, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, '370243'), -- 190503 (Bronze Timekeeper) - Cosmetic Race Flag Overhead
(196673, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 196673 (Vacationing Tender)
(196668, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 196668 (Wistful Watcher)
(194092, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '376376'), -- 194092 (Wild Proto-Drake) - Conversation Aura: Talk/Roar/Laugh/Threaten [DNT]
(191180, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '371171'), -- 191180 (Wild Proto-Drake) - Cosmetic - Sleep Zzz (With Aggro Change) - Head (Scale 4,  Down 1)
(191395, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '376376'), -- 191395 (Wild Proto-Drake) - Conversation Aura: Talk/Roar/Laugh/Threaten [DNT]
(190812, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '373362'), -- 190812 (Wild Proto-Drake) - Sparring Aura: AttackUnarmed/DragonSpit [DNT]
(189486, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '382487'), -- 189486 (Captured Proto-Drake) - Cosmetic: Wind Vortex [DNT]
(189515, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 189515 (Infused Proto-Drake)
(191413, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '371171'), -- 191413 (Wild Proto-Drake) - Cosmetic - Sleep Zzz (With Aggro Change) - Head (Scale 4,  Down 1)
(189823, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '391254'), -- 189823 (Ravenous Nestling) - Hearty
(186110, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '154470'), -- 186110 (Djaradin Crustshaper) - Permanent Feign Death (Flies)
(181764, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '377929'), -- 181764 (Lava Phoenix) - From the Ashes
(198163, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '261943'), -- 198163 (Mammoth-Tamer Tavok) - Cosmetic - SitGround - Lower Body Anim Kit - Breakable
(198200, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 198200 (Stonescale Proto-Dragon)
(196556, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '388666'), -- 196556 (Proud Dragonhunter) - Channel: Sharpen Sword (T3) [DNT]
(181029, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '286730'), -- 181029 (Generic - Empty Bunny) - Cosmetic - Black Smoke + Fire
(186509, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '378432'), -- 186509 (Modak Flamespit) - Channel: Lava Orb (Can Move) [DNT]
(191479, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '154470'), -- 191479 (Charred Hornspike) - Permanent Feign Death (Flies)
(191476, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '394916 375987'), -- 191476 (Searing Flame Harchek) - [DNT] Searing Meat, [DNT]Visual
(196605, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, ''), -- 196605 (Invis Bunny Template - Gigantic AOI)
(186109, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 186109 (Qalashi Necksnapper)
(191679, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '391254'), -- 191679 (Magmammoth Bull) - Hearty
(189557, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '369262'), -- 189557 (Qalashi Boltthrower) - Carry Arbalist Ammo
(191708, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 191708 (Wild Proto-Drake)
(191043, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '389243'), -- 191043 (Itchy Sparkwobble) - Poisoned Blades
(190630, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '43671'), -- 190630 (Lava Tentacles) - Ride Vehicle
(190023, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '43671'), -- 190023 (Lava Tentacles) - Ride Vehicle
(191498, 0, 0, 0, 0, 0, 1, 0, 0, 24787, 0, 0, 0, ''), -- 191498 (Qalashi Mammoth Trainer)
(189235, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '372472 422354'), -- 189235 (Overseer Lahar) - Imbued Magma, -Unknown-
(192781, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '378149'), -- 192781 (Ore Elemental) - Granite Shell
(187073, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '382158'), -- 187073 (Ruby Sitter) - Egg Sparkles [DNT]
(194088, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '371171'), -- 194088 (Wild Proto-Drake) - Cosmetic - Sleep Zzz (With Aggro Change) - Head (Scale 4,  Down 1)
(187709, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 187709 (Handhold)
(187710, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 187710 (Handhold)
(187708, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 187708 (Handhold)
(187711, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 187711 (Handhold)
(191704, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 191704 (Apex Proto-Dragon)
(195160, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 195160 (Igneous Wanderer)
(188234, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '370634'), -- 188234 (Earthen Ward) - Elemental Ward
(186638, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 186638 (Qalashi Crustshaper)
(194915, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '384950'), -- 194915 (Invis Bunny) - Slime Spawn Aura (Dummy)
(189942, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, '80808'), -- 189942 (Wrathion) - Cosmetic - Alpha State 0%
(186261, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '370034'), -- 186261 (Restless Lava) - Lava Pool
(189581, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '391254'), -- 189581 (Lava Fledgling) - Hearty
(197016, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '390701'), -- 197016 (Fishing Net) - Loc 5 Net 1 Controller Aura [DNT]
(186483, 0, 0, 0, 0, 0, 1, 0, 0, 16024, 0, 0, 0, '369521'), -- 186483 (Qalashi Steelcrafter) - Sharpen Weapon
(197645, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '386041'), -- 197645 (Daring Fisher) - Fishing
(193967, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, '382734 382774'), -- 193967 (Dragonbane Wingshredder) - Air Guard, [DNT] Anti-Air Marker
(191525, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '154470'), -- 191525 (Qalashi Wallcrasher) - Permanent Feign Death (Flies)
(191876, 0, 0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, '376792'), -- 191876 (Goruk Steelwall) - Cosmetic - Sleep Zzz Breakable (Djaradin)
(191940, 0, 0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, '376792'), -- 191940 (Qalashi Necksnapper) - Cosmetic - Sleep Zzz Breakable (Djaradin)
(186239, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '154470'), -- 186239 (Qalashi Skullhauler) - Permanent Feign Death (Flies)
(189643, 0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0, '154470'), -- 189643 (Qalashi Crustshaper) - Permanent Feign Death (Flies)
(181763, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 181763 (Lava Phoenix)
(195128, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 195128 (Invisible Bunny)
(195001, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 195001 (Invisible Bunny)
(195009, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 195009 (Invisible Bunny)
(195127, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 195127 (Invisible Bunny)
(187868, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '370034'), -- 187868 (Molten Uprising) - Lava Pool
(188141, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 188141 (Malsiran)
(196783, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 196783 (Generic)
(199298, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '366385'), -- 199298 (Qalashi Skullhauler) - Channel: Carry Bone Bin [DNT]
(187867, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '358413'), -- 187867 (Qalashi Magmacrafter) - Channel: Lava (CustomSpell03) [DNT]
(187366, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '391254'), -- 187366 (Worldcarver Wurmling) - Hearty
(196772, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '280235'), -- 196772 (Battleseared Magmatusk) - AI Cast AoE
(196858, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 196858 (Fyranian)
(198737, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 198737 (Firewing)
(189199, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 189199 (Vadrya Valte)
(192164, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 192164 (Gringot Coldsteel)
(195957, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '212171'), -- 195957 (Blacktalon Strategist) - Read Scroll
(192121, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 192121 (Falron Greygold)
(192155, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 192155 (Felona Oregrab)
(186324, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 186324 (Blacktalon Schemer)
(198847, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 198847 (Swart)
(190910, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '374012 373400'), -- 190910 (Evorian) - Sabellian Transform, Wrathion Transform
(191710, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 191710 (Left)
(188351, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '269360 264071 263843'), -- 188351 (Stabitha Sharptwist) - Avoidance, AI Regen / Crit, General Defensive Passive - Leather
(188415, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 188415 (Jovax the Angry)
(186451, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 186451 (Blacktalon Avenger)
(189202, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 189202 (Uirreg Stoutbrow)
(198877, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 198877 (Blacktalon Sentry)
(188440, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 188440 (Djaradin Prisoner)
(198854, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '382644'), -- 198854 (Obsidian Outfitter) - Carrying Crate
(187524, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 187524 (Obsidian Bladewing)
(192356, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '377399'), -- 192356 (Enraged Embers) - Animated Ash
(192344, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '364682'), -- 192344 (Ashbinder Tornecha) - Channel: Earth Pool [DNT]
(187541, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, ''), -- 187541 (Obsidian Protector)
(187602, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '395824'), -- 187602 (Qalashi Scaleripper) - Jagged Cuts
(213636, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '420130 214027 413766 329535 431605 285707 465 344218'), -- 213636 (Captain Garrick) - -Unknown-, Adaptation, -Unknown-, Proc, -Unknown-, AI Buff Watcher, Devotion Aura, Devotion Aura
(190983, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '391245'), -- 190983 (Qalashi Pillager) - Supply-Laden
(189825, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '46598'), -- 189825 (Ravenous Nestling) - Ride Vehicle Hardcoded
(196336, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 196336 (Qalashi Flameslinger)
(187791, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '35773 65220 370634'), -- 187791 (Earthen Ward) - Increased Threat (SERVERSIDE), Avoidance, Elemental Ward
(187599, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ''), -- 187599 (Qalashi Bonebreaker)
(188135, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 188135 (Zepharion)
(192323, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 192323 (Trapped Whelpling)
(190314, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 190314 (Broodmother Dracasia)
(198630, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 4, ''), -- 198630 (Tracker Dragon Glyph)
(188155, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '374012 374015'), -- 188155 (Osoria) - Sabellian Transform, Growth
(188158, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 188158 (Baskilan)
(194984, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 4, '385045'), -- 194984 (Dragon Glyph) - Visual
(187919, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 187919 (Caldera Stomper)
(188140, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '374012 373400'), -- 188140 (Vazallia) - Sabellian Transform, Wrathion Transform
(192371, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '391245 364682'), -- 192371 (Qalashi Dusttwister) - Supply-Laden, Channel: Earth Pool [DNT]
(187600, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 187600 (Qalashi Stonemender)
(189826, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 189826 (Hanging Spider Vehicle)
(192564, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, '373400 374012 374015'), -- 192564 (Myridian) - Wrathion Transform, Sabellian Transform, Growth
(187908, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '395824'), -- 187908 (Qalashi Scaleripper) - Jagged Cuts
(196791, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 3, ''), -- 196791 (Kixandria)
(190118, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 4, '373591'), -- 190118 (Winds of the Isles) - Visual
(190085, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '373652'), -- 190085 (Grounding Spear) - Grounding Spear
(189669, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '372605'), -- 189669 (Binding Spear) - Binding Spear
(194816, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '384663'), -- 194816 (Forgewrought Monstrosity) - Forgewrought Fury
(189786, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '377165'), -- 189786 (Blazing Aegis) - Blazing Aegis
(189265, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '364588'), -- 189265 (Qalashi Bonetender) - Channel: Lava (ChannelCastOmni) [DNT]
(189478, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '422359'), -- 189478 (Forgemaster Gorek) - -Unknown-
(193291, 0, 0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 193291 (Apex Blazewing)
(189901, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '423679'), -- 189901 (Warlord Sargha) - -Unknown-
(193293, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '18950'), -- 193293 (Qalashi Warden) - Invisibility and Stealth Detection
(189340, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '384484 422356'), -- 189340 (Chargath, Bane of Scales) - Cosmetic Spear Stack, -Unknown-
(189466, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '422357 18950'), -- 189466 (Irontorch Commander) - -Unknown-, Invisibility and Stealth Detection
(194883, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '384835'), -- 194883 (Cosmetic Flavor[DNT]) - Cosmetic Flavor[DNT]
(189474, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, ''), -- 189474 (The Scorching Forge)
(181861, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, '422362'), -- 181861 (Magmatusk) - -Unknown-
(197709, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '391574'), -- 197709 (Draconic Tincture) - Draconic Tincture
(197337, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '390521'), -- 197337 (Burning 3 Books) - Burning Books
(197333, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '382248'), -- 197333 (Burning 1 Book) - Burning Book
(189472, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '372211 18950'), -- 189472 (Qalashi Lavabearer) - Lava Drip, Invisibility and Stealth Detection
(189266, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, '323486'), -- 189266 (Qalashi Trainee) - Dual Wield
(192788, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 192788 (Qalashi Thaumaturge)
(192786, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 192786 (Qalashi Plunderer)
(209068, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '284301 289481 431605 166615'), -- 209068 (Ash) - Predator's Thirst, Pet Check, -Unknown-, Pet Active
(189227, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '384488'), -- 189227 (Qalashi Hunter) - Cosmetic Spear Stack
(209065, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, '430678 420130 214027 284216 329535 431605 284225 284301 285707 413569 1218374'), -- 209065 (Austin Huxworth) - -Unknown-, -Unknown-, Adaptation, General Defensive Passive - Mail, Proc, -Unknown-, Override Energy Bar Color, Predator's Thirst, AI Buff Watcher, -Unknown-, -Unknown-
(234222, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '472701'), -- 234222 (Helper) - -Unknown-
(209069, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '284301 289481 431605'), -- 209069 (Nyx) - Predator's Thirst, Pet Check, -Unknown-
(189247, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''); -- 189247 (Tamed Phoenix)

UPDATE `creature_template_addon` SET `auras`='371800 371801' WHERE `entry`=184812; -- 184812 (Restless Explorer) - Aura: Dirt Climber's Gear [DNT], Holding On
UPDATE `creature_template_addon` SET `auras`='395824' WHERE `entry`=196553; -- 196553 (Proud Dragonhunter) - Jagged Cuts
UPDATE `creature_template_addon` SET `AnimTier`=3, `auras`='373397' WHERE `entry`=192284; -- 192284 (Kandrostrasz) - Wrathion Transform
UPDATE `creature_template_addon` SET `AnimTier`=0 WHERE `entry`=190069; -- 190069 (Rumbling Proto-Dragon) - Channel: Perch [DNT]
UPDATE `creature_template_addon` SET `auras`='391254' WHERE `entry`=191639; -- 191639 (Vibrant Butterfly) - Hearty
UPDATE `creature_template_addon` SET `AnimTier`=3, `auras`='371800 371801' WHERE `entry`=187194; -- 187194 (Restless Explorer) - Aura: Dirt Climber's Gear [DNT], Holding On
UPDATE `creature_template_addon` SET `SheathState`=0, `aiAnimKit`=15430, `auras`='' WHERE `entry`=186765; -- 186765 (Blacktalon Provisioner)
UPDATE `creature_template_addon` SET `auras`='430678 420130 214027 456227 329535 431605 285707 413569 1218374' WHERE `entry`=214390; -- 214390 (Shuja Grimaxe) - -Unknown-, -Unknown-, Adaptation, -Unknown-, Proc, -Unknown-, AI Buff Watcher, -Unknown-, -Unknown-
UPDATE `creature_template_addon` SET `auras`='430678 420130 214027 456226 329535 431605 285707 452978 413569 1218374' WHERE `entry`=209072; -- 209072 (Crenna Earth-Daughter) - -Unknown-, -Unknown-, Adaptation, -Unknown-, Proc, -Unknown-, AI Buff Watcher, -Unknown-, -Unknown-, -Unknown-
UPDATE `creature_template_addon` SET `auras`='430678 420130 214027 413766 329535 431605 285707 413569 465 344218 1218374' WHERE `entry`=209057; -- 209057 (Captain Garrick) - -Unknown-, -Unknown-, Adaptation, -Unknown-, Proc, -Unknown-, AI Buff Watcher, -Unknown-, Devotion Aura, Devotion Aura, -Unknown-

DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=205 AND `Entry` IN (190630,190023,191498,189905,192787,189235,192781,192464,194389,120271,190085,189669,194816,189786,189219,189478,193291,189471,189901,193944,193293,189340,189466,194883,189474,181861,197709,197337,197333,189464,189265,211204,210933,189472,189467,211227,189470,194773,192788,192134,189266,15716,23460,18363,15714,18379,29582,23458,70302,20030,20222,50269,28302,304,29929,23459,60941,26131,18364,31695,23455,15666,47353,31694,15715,20029,23457,64993,31717,32640,51195,71486,14505,31698,31778,65011,23456,18365,32158,73780,53276,64992,76084,33030,19280,305,189247,192786,189342,189227,189361,234222,205781,209069,209068,209065));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `HealthScalingExpansion`, `HealthModifier`, `ManaModifier`, `CreatureDifficultyID`, `TypeFlags`, `TypeFlags2`, `TypeFlags3`) VALUES
(190630, 205, 9, 1, 1, 218826, 0x200048, 0, 0), -- Lava Tentacles
(190023, 205, 9, 1, 1, 216712, 0x200048, 0, 0), -- Lava Tentacles
(191498, 205, 9, 6, 1, 219703, 0x200048, 0, 0), -- Qalashi Mammoth Trainer
(189905, 205, 9, 40, 1, 216594, 0x0, 0, 0), -- High Thaumaturge Fural
(192787, 205, 9, 7, 1, 221000, 0x200048, 0, 0), -- Qalashi Spinecrusher
(189235, 205, 9, 12, 1, 215921, 0x200048, 2147483648, 0), -- Overseer Lahar
(192781, 205, 9, 4, 1, 220993, 0x200048, 0, 0), -- Ore Elemental
(192464, 205, 9, 2, 1, 226392, 0x200048, 128, 0), -- Raging Ember
(194389, 205, 9, 2, 1, 222618, 0x200048, 0, 0), -- Lava Spawn
(120271, 205, 11, 1, 1, 124532, 0x400, 0, 0), -- Invisible Bunny (IGC)
(190085, 205, 9, 8.199999809265136718, 1, 216774, 0x40200058, 128, 0), -- Grounding Spear
(189669, 205, 9, 1, 1, 216354, 0x200058, 67108866, 0), -- Binding Spear
(194816, 205, 9, 6, 1, 223049, 0x200048, 2147483648, 0), -- Forgewrought Monstrosity
(189786, 205, 9, 1000, 1, 216471, 0x60000010, 67108870, 0), -- Blazing Aegis
(189219, 205, 9, 1000, 1, 215905, 0x60000010, 1030, 0), -- Qalashi Goulash
(189478, 205, 9, 31.5, 1, 277023, 0x200068, 128, 0), -- Forgemaster Gorek
(193291, 205, 9, 8, 1, 221507, 0x200048, 2147483648, 0), -- Apex Blazewing
(189471, 205, 9, 7, 1, 216155, 0x200048, 0, 0), -- Qalashi Blacksmith
(189901, 205, 9, 36, 1, 216590, 0x200068, 128, 0), -- Warlord Sargha
(193944, 205, 9, 7, 1, 222173, 0x200048, 2147483648, 0), -- Qalashi Lavamancer
(193293, 205, 9, 8, 1, 221509, 0x200040, 2147487744, 0), -- Qalashi Warden
(189340, 205, 9, 40, 1, 216024, 0x200068, 128, 0), -- Chargath, Bane of Scales
(189466, 205, 9, 11, 1, 216150, 0x200068, 2147483648, 0), -- Irontorch Commander
(194883, 205, 9, 1, 1, 223116, 0x60200058, 117440518, 0), -- Cosmetic Flavor[DNT]
(189474, 205, 9, 1, 1, 216158, 0x20, 0, 0), -- The Scorching Forge
(181861, 205, 9, 34.65000152587890625, 1, 277032, 0x200049, 128, 0), -- Magmatusk
(197709, 205, 9, 1, 1, 226059, 0x40200048, 0, 0), -- Draconic Tincture
(197337, 205, 9, 1000, 1, 225675, 0x0, 0, 0), -- Burning 3 Books
(197333, 205, 9, 1000, 1, 225671, 0x0, 0, 0), -- Burning 1 Book
(189464, 205, 9, 7, 1, 216148, 0x200048, 0, 0), -- Qalashi Irontorch
(189265, 205, 9, 6, 1, 215951, 0x200048, 0, 0), -- Qalashi Bonetender
(211204, 205, 11, 1, 1, 260555, 0x40200048, 34, 0), -- Path Helper
(210933, 205, 9, 1, 1, 260282, 0x40200048, 34, 0), -- Path Helper
(189472, 205, 9, 8, 1, 216156, 0x200048, 2147483648, 0), -- Qalashi Lavabearer
(189467, 205, 9, 6.5, 1, 216151, 0x200048, 0, 0), -- Qalashi Bonesplitter
(211227, 205, 11, 1, 1, 260578, 0x40200048, 34, 0), -- Path Helper
(189470, 205, 9, 3.20000004768371582, 1, 216154, 0x200048, 0, 0), -- Lava Flare
(194773, 205, 11, 0.200000002980232238, 1, 223006, 0x0, 0, 0), -- Lava Slime
(192788, 205, 9, 6, 1, 277027, 0x200048, 0, 0), -- Qalashi Thaumaturge
(192134, 205, 11, 0.200000002980232238, 1, 220343, 0x0, 0, 0), -- Lava Snail
(189266, 205, 9, 5, 1, 215952, 0x200048, 0, 0), -- Qalashi Trainee
(15716, 205, 11, 1, 1, 10962, 0x0, 0, 0), -- Red Qiraji Battle Tank
(23460, 205, 1, 1, 1, 19808, 0x0, 0, 0), -- Cobalt Netherwing Drake
(18363, 205, 1, 1, 1, 14068, 0x0, 0, 0), -- Tawny Wind Rider
(15714, 205, 11, 1, 1, 10960, 0x0, 0, 0), -- Yellow Qiraji Battle Tank
(18379, 205, 2, 1, 1, 14092, 0x0, 0, 0), -- Swift Purple Wind Rider
(29582, 205, 11, 1, 1, 32792, 0x0, 0, 0), -- Winged Steed of the Ebon Blade
(23458, 205, 1, 1, 1, 19804, 0x0, 0, 0), -- Purple Netherwing Drake
(70302, 205, 11, 1, 1, 64953, 0x0, 0, 0), -- Clutch of Ji-Kun
(20030, 205, 11, 1, 1, 15882, 0x0, 0, 0), -- Thalassian Charger
(20222, 205, 11, 1, 1, 16049, 0x0, 0, 0), -- Black Hawkstrider
(50269, 205, 11, 1, 1, 38458, 0x0, 0, 0), -- Sandstone Drake
(28302, 205, 11, 1, 1, 29862, 0x0, 0, 0), -- Acherus Deathcharger
(304, 205, 11, 1, 1, 184, 0x0, 0, 0), -- Felsteed
(29929, 205, 11, 1, 1, 33463, 0x0, 0, 0), -- Mechano-Hog
(23459, 205, 1, 1, 1, 19806, 0x0, 0, 0), -- Violet Netherwing Drake
(60941, 205, 11, 1, 1, 20490, 0x0, 0, 0), -- Azure Water Strider
(26131, 205, 11, 1, 1, 25220, 0x0, 0, 0), -- Swift White Hawkstrider
(18364, 205, 1, 1, 1, 14069, 0x0, 0, 0), -- Blue Wind Rider
(31695, 205, 11, 25, 10, 36290, 0x0, 0, 0), -- Blue Drake Mount
(23455, 205, 1, 1, 1, 19798, 0x0, 0, 0), -- Onyx Netherwing Drake
(15666, 205, 11, 1, 1, 10917, 0x0, 0, 0), -- Blue Qiraji Battle Tank
(47353, 205, 11, 4, 1, 43580, 0x0, 0, 0), -- Drake of the East Wind
(31694, 205, 6, 4, 1, 36288, 0x0, 0, 0), -- Azure Drake Mount
(15715, 205, 11, 1, 1, 10961, 0x0, 0, 0), -- Green Qiraji Battle Tank
(20029, 205, 11, 1, 1, 15881, 0x0, 0, 0), -- Thalassian Warhorse
(23457, 205, 1, 1, 1, 19802, 0x0, 0, 0), -- Veridian Netherwing Drake
(64993, 205, 11, 1, 1, 58391, 0x0, 0, 0), -- Heavenly Azure Cloud Serpent
(31717, 205, 11, 5.952380180358886718, 1, 36329, 0x0, 0, 0), -- Bronze Drake Mount
(32640, 205, 11, 1, 1, 37546, 0x0, 0, 0), -- Traveler's Tundra Mammoth
(51195, 205, 11, 1, 1, 36686, 0x0, 0, 0), -- Kor'kron Annihilator
(71486, 205, 11, 1, 1, 66599, 0x0, 0, 0), -- Hearthsteed
(14505, 205, 11, 1, 1, 9906, 0x0, 0, 0), -- Dreadsteed
(31698, 205, 3, 1, 1, 36294, 0x0, 0, 0), -- Twilight Drake
(31778, 205, 2, 1.25, 1, 36427, 0x0, 0, 0), -- Black Drake Mount
(65011, 205, 11, 1, 1, 58409, 0x0, 0, 0), -- Albino Riding Crane
(23456, 205, 1, 1, 1, 19800, 0x0, 0, 0), -- Azure Netherwing Drake
(18365, 205, 1, 1, 1, 14070, 0x0, 0, 0), -- Green Wind Rider
(32158, 205, 11, 5.952380180358886718, 1, 36649, 0x0, 0, 0), -- Albino Drake
(73780, 205, 11, 1, 1, 69879, 0x0, 0, 0), -- Prideful Gladiator's Cloud Serpent
(53276, 205, 3, 0.699999988079071044, 1, 32926, 0x0, 0, 0), -- Amani Battle Bear
(64992, 205, 11, 1, 1, 58390, 0x0, 0, 0), -- Heavenly Jade Cloud Serpent
(76084, 205, 11, 1, 1, 72529, 0x0, 0, 0), -- Iron Skyreaver
(33030, 205, 11, 1, 1, 38324, 0x0, 0, 0), -- Magnificent Flying Carpet
(19280, 205, 11, 1, 1, 15105, 0x0, 0, 0), -- Red Hawkstrider
(305, 205, 11, 1, 1, 185, 0x0, 0, 0), -- White Stallion
(189247, 205, 9, 2, 1, 215933, 0x200048, 0, 0), -- Tamed Phoenix
(192786, 205, 9, 6.5, 1, 220999, 0x200048, 0, 0), -- Qalashi Plunderer
(189342, 205, 9, 1000, 1, 216026, 0x60000010, 6, 0), -- Burning Chain
(189227, 205, 9, 6, 1, 215913, 0x200048, 0, 0), -- Qalashi Hunter
(189361, 205, 11, 0.200000002980232238, 1, 216045, 0x0, 0, 0), -- Lava Pincer
(234222, 205, 9, 1, 1, 285921, 0x40200048, 34, 0), -- Helper
(205781, 205, 9, 10, 1, 234973, 0x0, 0, 0), -- Archivist Edress
(209069, 205, 11, 1.5, 1, 238310, 0x1000, 1, 0), -- Nyx
(209068, 205, 11, 1.5, 1, 238309, 0x1000, 1, 0), -- Ash
(209065, 205, 11, 1.549999952316284179, 1, 238306, 0x1000, 3145728, 0); -- Austin Huxworth

UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=11, `HealthModifier`=2.20000004768371582, `ManaModifier`=100, `CreatureDifficultyID`=238300, `TypeFlags`=0x1000, `TypeFlags2`=3145728 WHERE (`Entry`=209059 AND `DifficultyID`=205); -- Meredy Huntswell
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=11, `HealthModifier`=0.050000000745058059, `CreatureDifficultyID`=261706 WHERE (`Entry`=212352 AND `DifficultyID`=205); -- Healing Stream Totem
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=11, `CreatureDifficultyID`=51639, `TypeFlags`=0x400, `TypeFlags2`=16 WHERE (`Entry`=43499 AND `DifficultyID`=205); -- Consecration
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=9, `CreatureDifficultyID`=260280, `TypeFlags`=0x40200048, `TypeFlags2`=34 WHERE (`Entry`=210931 AND `DifficultyID`=205); -- Path Helper
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=9, `CreatureDifficultyID`=260281, `TypeFlags`=0x40200048, `TypeFlags2`=34 WHERE (`Entry`=210932 AND `DifficultyID`=205); -- Path Helper
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=9, `CreatureDifficultyID`=260279, `TypeFlags`=0x40200048, `TypeFlags2`=34 WHERE (`Entry`=210930 AND `DifficultyID`=205); -- Path Helper
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=11, `CreatureDifficultyID`=263248, `TypeFlags`=0x40200048, `TypeFlags2`=34 WHERE (`Entry`=213839 AND `DifficultyID`=205); -- Path Helper
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=11, `HealthModifier`=1.5, `ManaModifier`=100, `CreatureDifficultyID`=238313, `TypeFlags`=0x1000, `TypeFlags2`=3145728 WHERE (`Entry`=209072 AND `DifficultyID`=205); -- Crenna Earth-Daughter
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=11, `HealthModifier`=2.5, `CreatureDifficultyID`=238298, `TypeFlags`=0x1000, `TypeFlags2`=3145728 WHERE (`Entry`=209057 AND `DifficultyID`=205); -- Captain Garrick
UPDATE `creature_template_difficulty` SET `HealthScalingExpansion`=11, `HealthModifier`=1.60000002384185791, `CreatureDifficultyID`=263832, `TypeFlags`=0x1000, `TypeFlags2`=3145728 WHERE (`Entry`=214390 AND `DifficultyID`=205); -- Shuja Grimaxe

DELETE FROM `creature_template_difficulty` WHERE (`DifficultyID`=205 AND `Entry` IN (190630,190023,191498,213636,194916,187813,199298,189825,190971,196772,187791,188135,192323,190314,196336,198630,188155,188158,189811,194984,187919,188140,196814,187599,192325,182815,187600,189826,192371,192564,187908,196791,196758,190118,186241,190983,192464,194389,120271,190085,189669,194816,189786,189219,189478,193291,189471,189901,193293,193944,189340,189466,194883,189474,181861,197709,197337,189265,197333,189464,189472,211204,211227,189467,210933,189470,189266,192134,194773,192788,192786,209068,189227,209065,205781,189342,234222,209069,189247,189361));
INSERT INTO `creature_template_difficulty` (`Entry`, `DifficultyID`, `LevelScalingDeltaMin`, `LevelScalingDeltaMax`, `ContentTuningID`, `StaticFlags1`, `StaticFlags2`, `StaticFlags3`, `StaticFlags4`, `StaticFlags5`, `StaticFlags6`, `StaticFlags7`, `StaticFlags8`, `VerifiedBuild`) VALUES
(190630, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 190630 (Lava Tentacles) - CanSwim
(190023, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 190023 (Lava Tentacles) - CanSwim
(191498, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 191498 (Qalashi Mammoth Trainer) - CanSwim
(213636, 205, 0, 0, 2702, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 213636 (Captain Garrick) - CanSwim
(194916, 205, 0, 0, 2151, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 194916 (Zakastia) - CanSwim
(187813, 205, 0, 0, 2305, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 187813 (Qalashi Wallcrasher) - CanSwim
(199298, 205, 0, 0, 2305, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 199298 (Qalashi Skullhauler) - CanSwim
(189825, 205, 0, 0, 2287, 0x30000100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189825 (Ravenous Nestling) - Sessile, CanSwim, Floating
(190971, 205, 2, 2, 2287, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 190971 (Shas'ith) - 
(196772, 205, 0, 0, 2287, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 196772 (Battleseared Magmatusk) - CanSwim
(187791, 205, 0, 0, 2151, 0x30000100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 187791 (Earthen Ward) - Sessile, CanSwim, Floating
(188135, 205, 1, 1, 2078, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 188135 (Zepharion) - CanSwim
(192323, 205, 0, 0, 2150, 0x20000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 192323 (Trapped Whelpling) - Floating
(190314, 205, 1, 1, 2078, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 190314 (Broodmother Dracasia) - CanSwim
(196336, 205, 0, 0, 2287, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 196336 (Qalashi Flameslinger) - CanSwim
(198630, 205, 0, 0, 2078, 0x20000100, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 198630 (Tracker Dragon Glyph) - Sessile, Floating - UntargetableByClient
(188155, 205, 1, 1, 2078, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 188155 (Osoria) - CanSwim
(188158, 205, 1, 1, 2078, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 188158 (Baskilan) - CanSwim
(189811, 205, 0, 0, 2289, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189811 (Agitated Keystone) - CanSwim
(194984, 205, 0, 0, 2078, 0x20000100, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 194984 (Dragon Glyph) - Sessile, Floating - UntargetableByClient
(187919, 205, 1, 1, 2305, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 187919 (Caldera Stomper) - CanSwim
(188140, 205, 2, 2, 2078, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 188140 (Vazallia) - CanSwim
(196814, 205, 0, 0, 2078, 0x20000100, 0x0, 0x0, 0x2000000, 0x1, 0x0, 0x0, 0x0, 67451), -- 196814 (Neltharus) - Sessile, Floating - HideInCombatLog - UntargetableByClient
(187599, 205, 0, 0, 2287, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 187599 (Qalashi Bonebreaker) - CanSwim
(192325, 205, 0, 0, 2287, 0x30000100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 192325 (Stone Shackles) - Sessile, CanSwim, Floating
(182815, 205, 0, 0, 2287, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 182815 (Summoned Lava Elemental) - CanSwim
(187600, 205, 0, 0, 2287, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 187600 (Qalashi Stonemender) - CanSwim
(189826, 205, 0, 0, 2287, 0x20000100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189826 (Hanging Spider Vehicle) - Sessile, Floating
(192371, 205, 0, 0, 2287, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 192371 (Qalashi Dusttwister) - CanSwim
(192564, 205, 2, 2, 2078, 0x30000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 192564 (Myridian) - CanSwim, Floating
(187908, 205, 0, 0, 2287, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 187908 (Qalashi Scaleripper) - CanSwim
(196791, 205, 2, 2, 2078, 0x30000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 196791 (Kixandria) - CanSwim, Floating
(196758, 205, 0, 0, 2287, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 196758 (Magma Skitter) - CanSwim
(190118, 205, 0, 0, 2424, 0x20000100, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 190118 (Winds of the Isles) - Sessile, Floating - UntargetableByClient
(186241, 205, 0, 0, 2151, 0x20000100, 0x0, 0x2000000, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 186241 (Generic - Empty Bunny) - Sessile, Floating - CannotTurn
(190983, 205, 0, 0, 2287, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 190983 (Qalashi Pillager) - CanSwim
(192464, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 192464 (Raging Ember) - CanSwim
(194389, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 194389 (Lava Spawn) - CanSwim
(120271, 205, 0, 0, 328, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 120271 (Invisible Bunny (IGC)) - 
(190085, 205, 0, 0, 2955, 0x20000100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 190085 (Grounding Spear) - Sessile, Floating
(189669, 205, 0, 0, 2955, 0x20000100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189669 (Binding Spear) - Sessile, Floating
(194816, 205, 1, 1, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 194816 (Forgewrought Monstrosity) - CanSwim
(189786, 205, 0, 0, 2955, 0x20000100, 0x0, 0x2000000, 0x0, 0x2001, 0x0, 0x0, 0x0, 67451), -- 189786 (Blazing Aegis) - Sessile, Floating - CannotTurn - UntargetableByClient, SuppressHighlightWhenTargetedOrMousedOver
(189219, 205, 0, 0, 2955, 0x20000100, 0x0, 0x0, 0x0, 0x2001, 0x0, 0x0, 0x0, 67451), -- 189219 (Qalashi Goulash) - Sessile, Floating - UntargetableByClient, SuppressHighlightWhenTargetedOrMousedOver
(189478, 205, 2, 2, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189478 (Forgemaster Gorek) - CanSwim
(193291, 205, 1, 1, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 193291 (Apex Blazewing) - CanSwim
(189471, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189471 (Qalashi Blacksmith) - CanSwim
(189901, 205, 2, 2, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189901 (Warlord Sargha) - CanSwim
(193293, 205, 1, 1, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 193293 (Qalashi Warden) - CanSwim
(193944, 205, 1, 1, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 193944 (Qalashi Lavamancer) - CanSwim
(189340, 205, 2, 2, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189340 (Chargath, Bane of Scales) - CanSwim
(189466, 205, 1, 1, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189466 (Irontorch Commander) - CanSwim
(194883, 205, 0, 0, 2955, 0x30000100, 0x0, 0x2000000, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 194883 (Cosmetic Flavor[DNT]) - Sessile, CanSwim, Floating - CannotTurn - UntargetableByClient
(189474, 205, 0, 0, 2955, 0x20000100, 0x0, 0x2000000, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 189474 (The Scorching Forge) - Sessile, Floating - CannotTurn - UntargetableByClient
(181861, 205, 2, 2, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 181861 (Magmatusk) - CanSwim
(197709, 205, 0, 0, 2955, 0x30000100, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 197709 (Draconic Tincture) - Sessile, CanSwim, Floating - UntargetableByClient
(197337, 205, 0, 0, 2955, 0x20000100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 197337 (Burning 3 Books) - Sessile, Floating
(189265, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189265 (Qalashi Bonetender) - CanSwim
(197333, 205, 0, 0, 2955, 0x20000100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 197333 (Burning 1 Book) - Sessile, Floating
(189464, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189464 (Qalashi Irontorch) - CanSwim
(189472, 205, 1, 1, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189472 (Qalashi Lavabearer) - CanSwim
(211204, 205, 0, 0, 2702, 0x30000100, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 211204 (Path Helper) - Sessile, CanSwim, Floating - UntargetableByClient
(211227, 205, 0, 0, 2702, 0x30000100, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 211227 (Path Helper) - Sessile, CanSwim, Floating - UntargetableByClient
(189467, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189467 (Qalashi Bonesplitter) - CanSwim
(210933, 205, 0, 0, 2958, 0x30000100, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 210933 (Path Helper) - Sessile, CanSwim, Floating - UntargetableByClient
(189470, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189470 (Lava Flare) - CanSwim
(189266, 205, 0, 0, 2955, 0x10000000, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189266 (Qalashi Trainee) - CanSwim - DoNotFadeIn
(192134, 205, 0, 0, 81, 0x10000000, 0x0, 0x0, 0x1000000, 0x0, 0x0, 0x0, 0x0, 67451), -- 192134 (Lava Snail) - CanSwim - PreventSwim
(194773, 205, 0, 0, 81, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 194773 (Lava Slime) - CanSwim
(192788, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 192788 (Qalashi Thaumaturge) - CanSwim
(192786, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 192786 (Qalashi Plunderer) - CanSwim
(209068, 205, 0, 0, 2702, 0x10000000, 0x0, 0x40000, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 209068 (Ash) - CanSwim - AllowInteractionWhileInCombat
(189227, 205, 0, 0, 2955, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189227 (Qalashi Hunter) - CanSwim
(209065, 205, 0, 0, 2702, 0x10000000, 0x0, 0x0, 0x8000, 0x0, 0x0, 0x0, 0x0, 67451), -- 209065 (Austin Huxworth) - CanSwim - TreatAsRaidUnitForHelpfulSpells
(205781, 205, 0, 0, 2151, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 205781 (Archivist Edress) - CanSwim
(189342, 205, 0, 0, 2955, 0x20000100, 0x0, 0x0, 0x0, 0x2001, 0x0, 0x0, 0x0, 67451), -- 189342 (Burning Chain) - Sessile, Floating - UntargetableByClient, SuppressHighlightWhenTargetedOrMousedOver
(234222, 205, 0, 0, 2955, 0x30000100, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 67451), -- 234222 (Helper) - Sessile, CanSwim, Floating - UntargetableByClient
(209069, 205, 0, 0, 2702, 0x10000000, 0x0, 0x40000, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 209069 (Nyx) - CanSwim - AllowInteractionWhileInCombat
(189247, 205, 0, 0, 2955, 0x30000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451), -- 189247 (Tamed Phoenix) - CanSwim, Floating
(189361, 205, 0, 0, 81, 0x10000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 67451); -- 189361 (Lava Pincer) - CanSwim

UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=190132 AND `DifficultyID`=0); -- 190132 (Shikaar Ohuna) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=180393 AND `DifficultyID`=0); -- 180393 (Shikaar Ranger) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=182725 AND `DifficultyID`=0); -- 182725 (Shikaar Preyseeker) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=184656 AND `DifficultyID`=0); -- 184656 (Loyal Bakar) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=184657 AND `DifficultyID`=0); -- 184657 (Shikaar Hunter) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=182727 AND `DifficultyID`=0); -- 182727 (Shikaar Beastmaster) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=185883 AND `DifficultyID`=0); -- 185883 (Blazing Proto-Dragon) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192387 AND `DifficultyID`=0); -- 192387 (Shikaar Scout) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=187937 AND `DifficultyID`=0); -- 187937 (Plainswalker Bull) - CanSwim
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=67451 WHERE (`DifficultyID`=0 AND `Entry` IN (192856,187981,192345,195160,197772,190910,191711,191710,188351));
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192255 AND `DifficultyID`=0); -- 192255 (Stoneshell) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=190530 AND `DifficultyID`=0); -- 190530 (Wild Proto-Drake) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192382 AND `DifficultyID`=0); -- 192382 (Mudlopper) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192652 AND `DifficultyID`=0); -- 192652 (Clearwater Bull) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192654 AND `DifficultyID`=0); -- 192654 (Clearwater Calf) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192854 AND `DifficultyID`=0); -- 192854 (Clearwater Ottuk) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=187799 AND `DifficultyID`=0); -- 187799 (Reed Skimmer) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192651 AND `DifficultyID`=0); -- 192651 (Clearwater Riverbeast) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188099 AND `DifficultyID`=0); -- 188099 (Plainswalker Calf) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=185882 AND `DifficultyID`=0); -- 185882 (Scout Tomul) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192028 AND `DifficultyID`=0); -- 192028 (Trunkalumpf) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191478 AND `DifficultyID`=0); -- 191478 (Meadowhoof Calf) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=190908 AND `DifficultyID`=0); -- 190908 (Plainswalker Calf) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=198670 AND `DifficultyID`=0); -- 198670 (Tracker Dragon Glyph) - Sessile, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=190905 AND `DifficultyID`=0); -- 190905 (Meadowhoof Musken) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=198458 AND `DifficultyID`=0); -- 198458 (Dragon Glyph) - Sessile, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=190906 AND `DifficultyID`=0); -- 190906 (Plainswalker Mammoth) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=192347 AND `DifficultyID`=0); -- 192347 (Prairie Buzzwing) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=189066 AND `DifficultyID`=0); -- 189066 (Thaelin Darkanvil) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191788 AND `DifficultyID`=0); -- 191788 (Wild Proto-Drake) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=190909 AND `DifficultyID`=0); -- 190909 (Swift Hornstrider) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `StaticFlags3`=0x40000, `VerifiedBuild`=67451 WHERE (`Entry`=191058 AND `DifficultyID`=0); -- 191058 (Shikaar Fighter) - Sessile, CanSwim - AllowInteractionWhileInCombat
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188089 AND `DifficultyID`=0); -- 188089 (Swift Hornstrider) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188088 AND `DifficultyID`=0); -- 188088 (Windblessed Ohuna) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=185878 AND `DifficultyID`=0); -- 185878 (Ambassador Taurasza) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191828 AND `DifficultyID`=0); -- 191828 (Thunder Lizard Calf) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=196663 AND `DifficultyID`=0); -- 196663 (Ruby Scaleguard) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags3`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=194633 AND `DifficultyID`=0); -- 194633 (Etched Ancient Monolith) - Sessile, Floating - CannotTurn - UntargetableByClient
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2078, `VerifiedBuild`=67451 WHERE (`Entry`=190503 AND `DifficultyID`=0); -- 190503 (Bronze Timekeeper) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=196673 AND `DifficultyID`=0); -- 196673 (Vacationing Tender) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=196668 AND `DifficultyID`=0); -- 196668 (Wistful Watcher) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000100 WHERE (`DifficultyID`=0 AND `Entry` IN (184812,187194));
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=194092 AND `DifficultyID`=0); -- 194092 (Wild Proto-Drake) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=194091 AND `DifficultyID`=0); -- 194091 (Wild Proto-Dragon) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=194089 AND `DifficultyID`=0); -- 194089 (Wild Proto-Dragon) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=194086 AND `DifficultyID`=0); -- 194086 (Wild Proto-Drake) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191396 AND `DifficultyID`=0); -- 191396 (Wild Proto-Drake) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=196379 AND `DifficultyID`=0); -- 196379 (Khakad the Ancient) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=193640 AND `DifficultyID`=0); -- 193640 (Snake) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191180 AND `DifficultyID`=0); -- 191180 (Wild Proto-Drake) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191395 AND `DifficultyID`=0); -- 191395 (Wild Proto-Drake) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=190812 AND `DifficultyID`=0); -- 190812 (Wild Proto-Drake) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000100, `VerifiedBuild`=67451 WHERE (`Entry`=189486 AND `DifficultyID`=0); -- 189486 (Captured Proto-Drake) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=189515 AND `DifficultyID`=0); -- 189515 (Infused Proto-Drake) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191413 AND `DifficultyID`=0); -- 191413 (Wild Proto-Drake) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2287, `VerifiedBuild`=67451 WHERE (`Entry`=189823 AND `DifficultyID`=0); -- 189823 (Ravenous Nestling) - 
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191014 AND `DifficultyID`=0); -- 191014 (Becca Black) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=199020 AND `DifficultyID`=0); -- 199020 (Atticus Belle) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=185589 AND `DifficultyID`=0); -- 185589 (Worldbreaker Brute) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=67451 WHERE (`Entry`=186110 AND `DifficultyID`=0); -- 186110 (Djaradin Crustshaper) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=181764 AND `DifficultyID`=0); -- 181764 (Lava Phoenix) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=193550 AND `DifficultyID`=0); -- 193550 (Dograc The Burning) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=198163 AND `DifficultyID`=0); -- 198163 (Mammoth-Tamer Tavok) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=198200 AND `DifficultyID`=0); -- 198200 (Stonescale Proto-Dragon) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=196556 AND `DifficultyID`=0); -- 196556 (Proud Dragonhunter) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=197788 AND `DifficultyID`=0); -- 197788 (Qalashi Drakehunter) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags3`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=181029 AND `DifficultyID`=0); -- 181029 (Generic - Empty Bunny) - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=189604 AND `DifficultyID`=0); -- 189604 (Provisioner Ojito) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=186509 AND `DifficultyID`=0); -- 186509 (Modak Flamespit) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=186609 AND `DifficultyID`=0); -- 186609 (Dragonbane Shieldcracker) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191476 AND `DifficultyID`=0); -- 191476 (Searing Flame Harchek) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=67451 WHERE (`Entry`=196605 AND `DifficultyID`=0); -- 196605 (Invis Bunny Template - Gigantic AOI) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191679 AND `DifficultyID`=0); -- 191679 (Magmammoth Bull) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=67451 WHERE (`Entry`=45354 AND `DifficultyID`=0); -- 45354 (Draenei Air Totem) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=67451 WHERE (`Entry`=45352 AND `DifficultyID`=0); -- 45352 (Draenei Water Totem) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=187609 AND `DifficultyID`=0); -- 187609 (Earthcaller Yevaa) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x100, `VerifiedBuild`=67451 WHERE (`Entry`=45348 AND `DifficultyID`=0); -- 45348 (Draenei Earth Totem) - Sessile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000100, `VerifiedBuild`=67451 WHERE (`Entry`=188226 AND `DifficultyID`=0); -- 188226 (Melter Igneous) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=186855 AND `DifficultyID`=0); -- 186855 (Immutable Oretz) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=186849 AND `DifficultyID`=0); -- 186849 (Earthen Protector) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=191708 AND `DifficultyID`=0); -- 191708 (Wild Proto-Drake) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=194144 AND `DifficultyID`=0); -- 194144 (Riverbeast Elder) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=194145 AND `DifficultyID`=0); -- 194145 (Mature Riverbeast) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191043 AND `DifficultyID`=0); -- 191043 (Itchy Sparkwobble) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=373, `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=201849 AND `DifficultyID`=0); -- 201849 (Adinakon) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2955, `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=189905 AND `DifficultyID`=0); -- 189905 (High Thaumaturge Fural) - CanSwim
UPDATE `creature_template_difficulty` SET `LevelScalingDeltaMin`=1, `LevelScalingDeltaMax`=1, `ContentTuningID`=2955, `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=189235 AND `DifficultyID`=0); -- 189235 (Overseer Lahar) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2955, `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192787 AND `DifficultyID`=0); -- 192787 (Qalashi Spinecrusher) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2955, `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192781 AND `DifficultyID`=0); -- 192781 (Ore Elemental) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000 WHERE (`Entry`=192284 AND `DifficultyID`=0);
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=193377 AND `DifficultyID`=0); -- 193377 (Alexstrasza the Life-Binder) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191875 AND `DifficultyID`=0); -- 191875 (Wrathion) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags5`=0x100, `VerifiedBuild`=67451 WHERE (`Entry`=191878 AND `DifficultyID`=0); -- 191878 (Sabellian) - CanSwim - InteractWhileHostile
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191826 AND `DifficultyID`=0); -- 191826 (Mature Thunder Lizard) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=194088 AND `DifficultyID`=0); -- 194088 (Wild Proto-Drake) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags3`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=187709 AND `DifficultyID`=0); -- 187709 (Handhold) - Sessile, Floating - CannotTurn - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags3`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=187710 AND `DifficultyID`=0); -- 187710 (Handhold) - Sessile, Floating - CannotTurn - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags3`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=187708 AND `DifficultyID`=0); -- 187708 (Handhold) - Sessile, Floating - CannotTurn - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags3`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=187711 AND `DifficultyID`=0); -- 187711 (Handhold) - Sessile, Floating - CannotTurn - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=196385 AND `DifficultyID`=0); -- 196385 (Grod the Ancient) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=193517 AND `DifficultyID`=0); -- 193517 (Twinkle) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000000, `VerifiedBuild`=67451 WHERE (`Entry`=191704 AND `DifficultyID`=0); -- 191704 (Apex Proto-Dragon) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000100, `VerifiedBuild`=67451 WHERE (`Entry`=188234 AND `DifficultyID`=0); -- 188234 (Earthen Ward) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags4`=0x2000000, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=194915 AND `DifficultyID`=0); -- 194915 (Invis Bunny) - Sessile, Floating - HideInCombatLog - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=189942 AND `DifficultyID`=0); -- 189942 (Wrathion) - CanSwim, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=186109 AND `DifficultyID`=0); -- 186109 (Qalashi Necksnapper) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x1000000, `VerifiedBuild`=67451 WHERE (`Entry`=186261 AND `DifficultyID`=0); -- 186261 (Restless Lava) - CanSwim - PreventSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=194874 AND `DifficultyID`=0); -- 194874 (Appetizing Ingot) - CanSwim - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=67451 WHERE (`Entry`=186511 AND `DifficultyID`=0); -- 186511 (Piercer Gigra) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=189581 AND `DifficultyID`=0); -- 189581 (Lava Fledgling) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=186336 AND `DifficultyID`=0); -- 186336 (Blazing Manifestation) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=198767 AND `DifficultyID`=0); -- 198767 (Explorative Fisher) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=189557 AND `DifficultyID`=0); -- 189557 (Qalashi Boltthrower) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=181875 AND `DifficultyID`=0); -- 181875 (Olphis the Molten) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000100, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=197016 AND `DifficultyID`=0); -- 197016 (Fishing Net) - Sessile, CanSwim, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=186483 AND `DifficultyID`=0); -- 186483 (Qalashi Steelcrafter) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000100, `VerifiedBuild`=67451 WHERE (`Entry`=197566 AND `DifficultyID`=0); -- 197566 (Lunker Spawner Loc, Rare) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=196053 AND `DifficultyID`=0); -- 196053 (Lunker Spawner Loc) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=197645 AND `DifficultyID`=0); -- 197645 (Daring Fisher) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=193967 AND `DifficultyID`=0); -- 193967 (Dragonbane Wingshredder) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000100, `VerifiedBuild`=67451 WHERE (`Entry`=191525 AND `DifficultyID`=0); -- 191525 (Qalashi Wallcrasher) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191876 AND `DifficultyID`=0); -- 191876 (Goruk Steelwall) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=186638 AND `DifficultyID`=0); -- 186638 (Qalashi Crustshaper) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191940 AND `DifficultyID`=0); -- 191940 (Qalashi Necksnapper) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=67451 WHERE (`Entry`=186239 AND `DifficultyID`=0); -- 186239 (Qalashi Skullhauler) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191479 AND `DifficultyID`=0); -- 191479 (Charred Hornspike) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000100, `VerifiedBuild`=67451 WHERE (`Entry`=189643 AND `DifficultyID`=0); -- 189643 (Qalashi Crustshaper) - Sessile, CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191524 AND `DifficultyID`=0); -- 191524 (Qalashi Ironskin) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=181763 AND `DifficultyID`=0); -- 181763 (Lava Phoenix) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=67451 WHERE (`Entry`=195128 AND `DifficultyID`=0); -- 195128 (Invisible Bunny) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=67451 WHERE (`Entry`=195001 AND `DifficultyID`=0); -- 195001 (Invisible Bunny) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=67451 WHERE (`Entry`=195009 AND `DifficultyID`=0); -- 195009 (Invisible Bunny) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=67451 WHERE (`Entry`=195127 AND `DifficultyID`=0); -- 195127 (Invisible Bunny) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=187868 AND `DifficultyID`=0); -- 187868 (Molten Uprising) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=188141 AND `DifficultyID`=0); -- 188141 (Malsiran) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags3`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=196783 AND `DifficultyID`=0); -- 196783 (Generic) - Sessile, Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=187867 AND `DifficultyID`=0); -- 187867 (Qalashi Magmacrafter) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000100, `VerifiedBuild`=67451 WHERE (`Entry`=187366 AND `DifficultyID`=0); -- 187366 (Worldcarver Wurmling) - Sessile, CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188693 AND `DifficultyID`=0); -- 188693 (Boneshaper Jardak) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=196858 AND `DifficultyID`=0); -- 196858 (Fyranian) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x30000000, `VerifiedBuild`=67451 WHERE (`Entry`=198737 AND `DifficultyID`=0); -- 198737 (Firewing) - CanSwim, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=196518 AND `DifficultyID`=0); -- 196518 (Voraxian) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x1000000, `VerifiedBuild`=67451 WHERE (`Entry`=198851 AND `DifficultyID`=0); -- 198851 (Slugha) - CanSwim - PreventSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags4`=0x1000000, `VerifiedBuild`=67451 WHERE (`Entry`=198850 AND `DifficultyID`=0); -- 198850 (Inchy) - CanSwim - PreventSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=189207 AND `DifficultyID`=0); -- 189207 (Griftah) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags3`=0x40000, `StaticFlags4`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=189199 AND `DifficultyID`=0); -- 189199 (Vadrya Valte) - CanSwim - AllowInteractionWhileInCombat - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags3`=0x40000, `StaticFlags4`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=192164 AND `DifficultyID`=0); -- 192164 (Gringot Coldsteel) - CanSwim - AllowInteractionWhileInCombat - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=195957 AND `DifficultyID`=0); -- 195957 (Blacktalon Strategist) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188380 AND `DifficultyID`=0); -- 188380 (Briona Macolm) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags3`=0x40000, `StaticFlags4`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=192121 AND `DifficultyID`=0); -- 192121 (Falron Greygold) - CanSwim - AllowInteractionWhileInCombat - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags3`=0x40000, `StaticFlags4`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=192155 AND `DifficultyID`=0); -- 192155 (Felona Oregrab) - CanSwim - AllowInteractionWhileInCombat - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=187533 AND `DifficultyID`=0); -- 187533 (Gulchak) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000000, `VerifiedBuild`=67451 WHERE (`Entry`=198847 AND `DifficultyID`=0); -- 198847 (Swart) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=67451 WHERE (`Entry`=188443 AND `DifficultyID`=0); -- 188443 (Zinja Ujomei) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188415 AND `DifficultyID`=0); -- 188415 (Jovax the Angry) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags3`=0x40000, `StaticFlags4`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=189202 AND `DifficultyID`=0); -- 189202 (Uirreg Stoutbrow) - CanSwim - AllowInteractionWhileInCombat - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188625 AND `DifficultyID`=0); -- 188625 (Lorena Belle) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags3`=0x40000, `StaticFlags4`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=186451 AND `DifficultyID`=0); -- 186451 (Blacktalon Avenger) - CanSwim - AllowInteractionWhileInCombat - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=186452 AND `DifficultyID`=0); -- 186452 (Blacktalon Assassin) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=198877 AND `DifficultyID`=0); -- 198877 (Blacktalon Sentry) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188440 AND `DifficultyID`=0); -- 188440 (Djaradin Prisoner) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188344 AND `DifficultyID`=0); -- 188344 (Chydrass) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `StaticFlags3`=0x40000, `StaticFlags4`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=188349 AND `DifficultyID`=0); -- 188349 (Outfitter Tipech) - CanSwim - AllowInteractionWhileInCombat - HideInCombatLog
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188623 AND `DifficultyID`=0); -- 188623 (Samia Inkling) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000000, `VerifiedBuild`=67451 WHERE (`Entry`=187524 AND `DifficultyID`=0); -- 187524 (Obsidian Bladewing) - Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=187272 AND `DifficultyID`=0); -- 187272 (Forgemaster Bazentus) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192356 AND `DifficultyID`=0); -- 192356 (Enraged Embers) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=198854 AND `DifficultyID`=0); -- 198854 (Obsidian Outfitter) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=192344 AND `DifficultyID`=0); -- 192344 (Ashbinder Tornecha) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188179 AND `DifficultyID`=0); -- 188179 (Stoker Volrax) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `StaticFlags3`=0x2000000, `VerifiedBuild`=67451 WHERE (`Entry`=187596 AND `DifficultyID`=0); -- 187596 (Weapon Rack Widget) - Sessile, Floating - CannotTurn
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=189875 AND `DifficultyID`=0); -- 189875 (Animated Cindershards) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=193456 AND `DifficultyID`=0); -- 193456 (Archivist Edress) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=187541 AND `DifficultyID`=0); -- 187541 (Obsidian Protector) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=188336 AND `DifficultyID`=0); -- 188336 (Foehn Breezeskimmer) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=187598 AND `DifficultyID`=0); -- 187598 (Rohzor Forgesmash) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=187602 AND `DifficultyID`=0); -- 187602 (Qalashi Scaleripper) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=67451 WHERE (`Entry`=191010 AND `DifficultyID`=0); -- 191010 (Scalesmith Mita) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2958, `StaticFlags1`=0x30000100, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=211151 AND `DifficultyID`=205); -- 211151 (Path Helper) - Sessile, CanSwim, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2958, `StaticFlags1`=0x30000100, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=211149 AND `DifficultyID`=205); -- 211149 (Path Helper) - Sessile, CanSwim, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2958, `StaticFlags1`=0x30000100, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=211152 AND `DifficultyID`=205); -- 211152 (Path Helper) - Sessile, CanSwim, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2958, `StaticFlags1`=0x30000100, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=211150 AND `DifficultyID`=205); -- 211150 (Path Helper) - Sessile, CanSwim, Floating - UntargetableByClient
UPDATE `creature_template_difficulty` SET `ContentTuningID`=2958, `StaticFlags1`=0x30000100, `StaticFlags5`=0x1, `VerifiedBuild`=67451 WHERE (`Entry`=211137 AND `DifficultyID`=205); -- 211137 (Path Helper) - Sessile, CanSwim, Floating - UntargetableByClient


UPDATE `creature_model_info` SET `VerifiedBuild`=67451 WHERE `DisplayID` IN (102071, 108415, 104115, 104154, 103856, 108413, 101603, 108412, 104155, 106024, 107842, 104741, 107772, 108233, 103352, 77477, 107655, 103553, 106617, 103527, 103350, 105533, 106616, 103520, 107657, 1072, 107545, 106583, 75347, 107653, 100347, 106586, 100358, 106584, 104750, 110475, 107654, 104095, 103522, 101442, 103826, 102781, 107614, 107610, 107612, 107377, 107609, 107430, 104520, 107431, 107371, 109672, 107373, 107375, 107320, 110504, 109658, 102501, 102457, 102598, 19071, 19075, 106212, 19073, 110289, 102565, 102643, 107369, 103547, 107327, 111513, 107847, 108664, 108366, 108029, 109407, 104559, 104052, 108808, 107646, 105597, 38418, 107644, 107592, 102453, 102596, 101513, 106380, 102480, 102459, 106404, 108351, 102496, 102386, 102428, 102539, 101838, 106199, 102495, 102466, 102472, 102458, 102491, 102492, 107304, 102288, 109218, 82235, 102406, 107710, 109712, 106447, 107709, 102388, 107946, 102196, 107305, 102426, 106459, 101819, 110370, 102429, 71112, 102383, 102285, 101824, 110369, 102206, 106468, 102207, 102487, 102287, 108313, 101840, 102484, 102289, 102460, 102488, 101836, 108715, 102481, 102490, 102519, 102470, 101835, 34194, 101839, 102011, 26776, 102630, 106264, 107873, 100197, 107104, 102494, 107197, 107214, 102452, 107251, 101837, 102471, 101834, 102473, 101841, 107920, 102469, 107919, 108324, 102465, 109460, 102085, 92726, 108693, 112785, 108996, 107189, 109595, 107029, 111187, 111188, 108666, 108248, 111190, 105131, 104023, 108349, 108348, 108347, 101960, 108346, 108665, 111189, 108345, 108344, 104022, 104021, 107985, 108663, 84893, 108350, 113756, 109031, 63016, 109230);
UPDATE `creature_model_info` SET `BoundingRadius`=0.227289453148841857, `VerifiedBuild`=67451 WHERE `DisplayID`=106618;
UPDATE `creature_model_info` SET `BoundingRadius`=0.303333312273025512, `CombatReach`=0.866666615009307861, `VerifiedBuild`=67451 WHERE `DisplayID`=104050;
UPDATE `creature_model_info` SET `BoundingRadius`=2.118564605712890625, `CombatReach`=1.399999976158142089, `VerifiedBuild`=67451 WHERE `DisplayID`=100949;
UPDATE `creature_model_info` SET `CombatReach`=3.300000190734863281, `VerifiedBuild`=67451 WHERE `DisplayID`=102230;
UPDATE `creature_model_info` SET `CombatReach`=7, `VerifiedBuild`=67451 WHERE `DisplayID`=102489;
UPDATE `creature_model_info` SET `BoundingRadius`=1.314786672592163085, `CombatReach`=1.875 WHERE `DisplayID` IN (108234, 104025);
UPDATE `creature_model_info` SET `BoundingRadius`=7.499999523162841796, `CombatReach`=6, `VerifiedBuild`=67451 WHERE `DisplayID`=55283;
UPDATE `creature_model_info` SET `BoundingRadius`=5.041165828704833984, `CombatReach`=9.600000381469726562 WHERE `DisplayID`=104578;

DELETE FROM `creature_template_gossip` WHERE (`CreatureID`=187955 AND `MenuID`=27892);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
(187955, 27892, 67451); -- Krendisc


DELETE FROM `npc_vendor` WHERE (`entry`=187955 AND `item`=2901 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=187955 AND `item`=5956 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=187955 AND `item`=10498 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=187955 AND `item`=6219 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=187955 AND `item`=201832 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=187955 AND `item`=190452 AND `ExtendedCost`=0 AND `type`=1);
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `ExtendedCost`, `type`, `PlayerConditionID`, `IgnoreFiltering`, `VerifiedBuild`) VALUES
(187955, 6, 2901, 0, 0, 1, 0, 0, 67451), -- Mining Pick
(187955, 5, 5956, 0, 0, 1, 0, 0, 67451), -- Blacksmith Hammer
(187955, 4, 10498, 0, 0, 1, 0, 0, 67451), -- Gyromatic Micro-Adjustor
(187955, 3, 6219, 0, 0, 1, 0, 0, 67451), -- Arclight Spanner
(187955, 2, 201832, 0, 0, 1, 0, 0, 67451), -- -Unknown-
(187955, 1, 190452, 0, 0, 1, 0, 0, 67451); -- -Unknown-


DELETE FROM `npc_text` WHERE `ID` BETWEEN @NPCTEXTID+0 AND @NPCTEXTID+0;
INSERT INTO `npc_text` (`ID`, `Probability0`, `Probability1`, `Probability2`, `Probability3`, `Probability4`, `Probability5`, `Probability6`, `Probability7`, `BroadcastTextId0`, `BroadcastTextId1`, `BroadcastTextId2`, `BroadcastTextId3`, `BroadcastTextId4`, `BroadcastTextId5`, `BroadcastTextId6`, `BroadcastTextId7`, `VerifiedBuild`) VALUES
(@NPCTEXTID+0, 1, 0, 0, 0, 0, 0, 0, 0, 217389, 0, 0, 0, 0, 0, 0, 0, 67451); -- 187955 (Krendisc)

DELETE FROM `gossip_menu` WHERE (`MenuID`=27892 AND `TextID`=@NPCTEXTID+0);
INSERT INTO `gossip_menu` (`MenuID`, `TextID`, `VerifiedBuild`) VALUES
(27892, @NPCTEXTID+0, 67451); -- 187955 (Krendisc)

DELETE FROM `gossip_menu_option` WHERE (`MenuID`=27892 AND `OptionID`=0);
INSERT INTO `gossip_menu_option` (`MenuID`, `GossipOptionID`, `OptionID`, `OptionNpc`, `OptionText`, `OptionBroadcastTextID`, `Language`, `Flags`, `ActionMenuID`, `ActionPoiID`, `GossipNpcOptionID`, `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextID`, `SpellID`, `OverrideIconID`, `VerifiedBuild`) VALUES
(27892, 55283, 0, 1, 'I want to browse your goods.', 3370, 0, 0, 0, 0, NULL, 0, 0, NULL, 0, NULL, NULL, 67451);

UPDATE `creature_template` SET `faction`=3279, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=190132; -- Shikaar Ohuna
UPDATE `creature_template` SET `faction`=3280, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=180393; -- Shikaar Ranger
UPDATE `creature_template` SET `faction`=3279, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=182725; -- Shikaar Preyseeker
UPDATE `creature_template` SET `faction`=3280, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=184656; -- Loyal Bakar
UPDATE `creature_template` SET `faction`=3280, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=184657; -- Shikaar Hunter
UPDATE `creature_template` SET `faction`=3280, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=182727; -- Shikaar Beastmaster
UPDATE `creature_template` SET `faction`=14, `speed_run`=1.571428537368774414, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=185883; -- Blazing Proto-Dragon
UPDATE `creature_template` SET `faction`=3279, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=192387; -- Shikaar Scout
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=187937; -- Plainswalker Bull
UPDATE `creature_template` SET `faction`=188, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=192856; -- Clearwater Ottuk Pup
UPDATE `creature_template` SET `faction`=188, `speed_walk`=0.400000005960464477, `speed_run`=0.285714298486709594, `BaseAttackTime`=2000, `unit_flags`=0x200, `unit_flags2`=0x800 WHERE `entry`=192255; -- Stoneshell
UPDATE `creature_template` SET `faction`=16, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry` IN (190530, 191788, 191180, 191413, 191708, 194088); -- Wild Proto-Drake
UPDATE `creature_template` SET `faction`=188, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=192382; -- Mudlopper
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=192652; -- Clearwater Bull
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=192654; -- Clearwater Calf
UPDATE `creature_template` SET `faction`=2136, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=192854; -- Clearwater Ottuk
UPDATE `creature_template` SET `faction`=188, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=187799; -- Reed Skimmer
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=192651; -- Clearwater Riverbeast
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry` IN (188099, 190908); -- Plainswalker Calf
UPDATE `creature_template` SET `faction`=3279, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x800 WHERE `entry`=185882; -- Scout Tomul
UPDATE `creature_template` SET `faction`=634, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=191478; -- Meadowhoof Calf
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry` IN (198670, 198630); -- Tracker Dragon Glyph
UPDATE `creature_template` SET `faction`=634, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=190905; -- Meadowhoof Musken
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry` IN (198458, 194984); -- Dragon Glyph
UPDATE `creature_template` SET `faction`=634, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=190906; -- Plainswalker Mammoth
UPDATE `creature_template` SET `faction`=188, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=192347; -- Prairie Buzzwing
UPDATE `creature_template` SET `faction`=31, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=187981; -- Grassland Hopper
UPDATE `creature_template` SET `faction`=3270, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=189066; -- Thaelin Darkanvil
UPDATE `creature_template` SET `faction`=188, `speed_run`=0.857142865657806396, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=192345; -- Grass Gopher
UPDATE `creature_template` SET `faction`=14, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=190909; -- Swift Hornstrider
UPDATE `creature_template` SET `faction`=1665, `npcflag`=16777216, `BaseAttackTime`=2000, `unit_flags`=0x200, `unit_flags2`=0x800, `unit_flags3`=0x1 WHERE `entry`=191058; -- Shikaar Fighter
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=188089; -- Swift Hornstrider
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=188088; -- Windblessed Ohuna
UPDATE `creature_template` SET `faction`=3276, `npcflag`=2, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=185878; -- Ambassador Taurasza
UPDATE `creature_template` SET `faction`=2938, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=191828; -- Thunder Lizard Calf
UPDATE `creature_template` SET `faction`=3272, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x800000 WHERE `entry`=196663; -- Ruby Scaleguard
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry`=194633; -- Etched Ancient Monolith
UPDATE `creature_template` SET `faction`=35, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=190503; -- Bronze Timekeeper
UPDATE `creature_template` SET `faction`=3277, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=196673; -- Vacationing Tender
UPDATE `creature_template` SET `faction`=3277, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=196668; -- Wistful Watcher
UPDATE `creature_template` SET `faction`=16, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry` IN (194092, 194086, 191396, 191395, 190812); -- Wild Proto-Drake
UPDATE `creature_template` SET `faction`=16, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry` IN (194091, 194089); -- Wild Proto-Dragon
UPDATE `creature_template` SET `faction`=14, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=196379; -- Khakad the Ancient
UPDATE `creature_template` SET `faction`=188, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=193640; -- Snake
UPDATE `creature_template` SET `faction`=2000, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800, `unit_flags3`=0x40000000 WHERE `entry`=189486; -- Captured Proto-Drake
UPDATE `creature_template` SET `faction`=16, `speed_run`=1.571428537368774414, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189515; -- Infused Proto-Drake
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x800 WHERE `entry`=189823; -- Ravenous Nestling
UPDATE `creature_template` SET `faction`=3285, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=191014; -- Becca Black
UPDATE `creature_template` SET `faction`=3285, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=199020; -- Atticus Belle
UPDATE `creature_template` SET `faction`=2348, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=185589; -- Worldbreaker Brute
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800, `unit_flags3`=0x1 WHERE `entry`=186110; -- Djaradin Crustshaper
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=181764; -- Lava Phoenix
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=193550; -- Dograc The Burning
UPDATE `creature_template` SET `faction`=2136, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=198163; -- Mammoth-Tamer Tavok
UPDATE `creature_template` SET `faction`=16, `speed_run`=1.571428537368774414, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=198200; -- Stonescale Proto-Dragon
UPDATE `creature_template` SET `faction`=2136, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=196556; -- Proud Dragonhunter
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=197788; -- Qalashi Drakehunter
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x800 WHERE `entry`=181029; -- Generic - Empty Bunny
UPDATE `creature_template` SET `faction`=3314, `npcflag`=128, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=189604; -- Provisioner Ojito
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=186509; -- Modak Flamespit
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=186609; -- Dragonbane Shieldcracker
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=191476; -- Searing Flame Harchek
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x800, `unit_flags3`=0x41000000 WHERE `entry`=196605; -- Invis Bunny Template - Gigantic AOI
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=191679; -- Magmammoth Bull
UPDATE `creature_template` SET `speed_run`=1, `unit_flags`=0x2000300 WHERE `entry`=45354; -- Draenei Air Totem
UPDATE `creature_template` SET `speed_run`=1, `unit_flags`=0x2000300 WHERE `entry`=45352; -- Draenei Water Totem
UPDATE `creature_template` SET `faction`=35, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=187609; -- Earthcaller Yevaa
UPDATE `creature_template` SET `speed_run`=1, `unit_flags`=0x2000300 WHERE `entry`=45348; -- Draenei Earth Totem
UPDATE `creature_template` SET `faction`=16, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x40000000 WHERE `entry`=188226; -- Melter Igneous
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=186855; -- Immutable Oretz
UPDATE `creature_template` SET `faction`=3309, `BaseAttackTime`=2000, `unit_flags`=0x140, `unit_flags2`=0x800 WHERE `entry`=186849; -- Earthen Protector
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=194144; -- Riverbeast Elder
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=194145; -- Mature Riverbeast
UPDATE `creature_template` SET `faction`=3285, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=191043; -- Itchy Sparkwobble
UPDATE `creature_template` SET `faction`=190, `npcflag`=1073741824, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=201849; -- Adinakon
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x2000000, `unit_flags2`=0x800, `unit_flags3`=0x1000000 WHERE `entry` IN (190630, 190023); -- Lava Tentacles
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x140, `unit_flags2`=0x800 WHERE `entry`=191498; -- Qalashi Mammoth Trainer
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x140, `unit_flags2`=0x800 WHERE `entry`=189905; -- High Thaumaturge Fural
UPDATE `creature_template` SET `faction`=16, `speed_walk`=1.20000004768371582, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189235; -- Overseer Lahar
UPDATE `creature_template` SET `faction`=16, `speed_walk`=1.60000002384185791, `speed_run`=1.428571462631225585, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=192787; -- Qalashi Spinecrusher
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=192781; -- Ore Elemental
UPDATE `creature_template` SET `faction`=35, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=193377; -- Alexstrasza the Life-Binder
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=191875; -- Wrathion
UPDATE `creature_template` SET `faction`=3288, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=191878; -- Sabellian
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=191826; -- Mature Thunder Lizard
UPDATE `creature_template` SET `faction`=35, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000, `VehicleId`=7559 WHERE `entry` IN (187709, 187710, 187708, 187711); -- Handhold
UPDATE `creature_template` SET `faction`=14, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=196385; -- Grod the Ancient
UPDATE `creature_template` SET `faction`=3270, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=193517; -- Twinkle
UPDATE `creature_template` SET `faction`=16, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=191704; -- Apex Proto-Dragon
UPDATE `creature_template` SET `faction`=14, `speed_walk`=1.20000004768371582, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=195160; -- Igneous Wanderer
UPDATE `creature_template` SET `faction`=35, `npcflag`=16777216, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800, `unit_flags3`=0x41000000 WHERE `entry`=188234; -- Earthen Ward
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000000, `unit_flags2`=0x4000800, `unit_flags3`=0x40000000 WHERE `entry`=194915; -- Invis Bunny
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x1008000 WHERE `entry`=189942; -- Wrathion
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=186109; -- Qalashi Necksnapper
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=186261; -- Restless Lava
UPDATE `creature_template` SET `faction`=94, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x1000000 WHERE `entry`=194874; -- Appetizing Ingot
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=186511; -- Piercer Gigra
UPDATE `creature_template` SET `faction`=14, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=189581; -- Lava Fledgling
UPDATE `creature_template` SET `faction`=3307, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=186336; -- Blazing Manifestation
UPDATE `creature_template` SET `faction`=3273, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=198767; -- Explorative Fisher
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=189557; -- Qalashi Boltthrower
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=181875; -- Olphis the Molten
UPDATE `creature_template` SET `faction`=94, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x41000001 WHERE `entry`=197016; -- Fishing Net
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=186483; -- Qalashi Steelcrafter
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x800, `unit_flags3`=0x40000000 WHERE `entry`=197566; -- Lunker Spawner Loc, Rare
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x800 WHERE `entry`=196053; -- Lunker Spawner Loc
UPDATE `creature_template` SET `faction`=3273, `npcflag`=83, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=197645; -- Daring Fisher
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=193967; -- Dragonbane Wingshredder
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800, `unit_flags3`=0x41000001 WHERE `entry`=191525; -- Qalashi Wallcrasher
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=191876; -- Goruk Steelwall
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=186638; -- Qalashi Crustshaper
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=191940; -- Qalashi Necksnapper
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800, `unit_flags3`=0x1 WHERE `entry`=186239; -- Qalashi Skullhauler
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=191479; -- Charred Hornspike
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800, `unit_flags3`=0x1000001 WHERE `entry`=189643; -- Qalashi Crustshaper
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=1739, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=191524; -- Qalashi Ironskin
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=181763; -- Lava Phoenix
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x800, `unit_flags3`=0x41000000 WHERE `entry` IN (195128, 195001, 195009, 195127); -- Invisible Bunny
UPDATE `creature_template` SET `faction`=3307, `speed_walk`=0.800000011920928955, `speed_run`=0.428571432828903198, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=187868; -- Molten Uprising
UPDATE `creature_template` SET `faction`=3315, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=188141; -- Malsiran
UPDATE `creature_template` SET `faction`=3276, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x800, `unit_flags3`=0x41008000 WHERE `entry`=196783; -- Generic
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=187867; -- Qalashi Magmacrafter
UPDATE `creature_template` SET `faction`=14, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x40000000 WHERE `entry`=187366; -- Worldcarver Wurmling
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=188693; -- Boneshaper Jardak
UPDATE `creature_template` SET `faction`=3315, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=196858; -- Fyranian
UPDATE `creature_template` SET `faction`=35, `speed_walk`=0.400000005960464477, `speed_run`=0.285714298486709594, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=198737; -- Firewing
UPDATE `creature_template` SET `faction`=3315, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=196518; -- Voraxian
UPDATE `creature_template` SET `faction`=190, `speed_walk`=0.200000002980232238, `speed_run`=0.142857149243354797, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=198851; -- Slugha
UPDATE `creature_template` SET `faction`=190, `speed_walk`=0.200000002980232238, `speed_run`=0.142857149243354797, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=198850; -- Inchy
UPDATE `creature_template` SET `npcflag`=1 WHERE `entry`=186765; -- Blacktalon Provisioner
UPDATE `creature_template` SET `faction`=35, `npcflag`=131, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=189207; -- Griftah
UPDATE `creature_template` SET `faction`=3285, `npcflag`=128, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189199; -- Vadrya Valte
UPDATE `creature_template` SET `faction`=3285, `npcflag`=82, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=192164; -- Gringot Coldsteel
UPDATE `creature_template` SET `faction`=3285, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=195957; -- Blacktalon Strategist
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=197772; -- Gigi I Wynn
UPDATE `creature_template` SET `faction`=3285, `npcflag`=4224, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x800 WHERE `entry`=188380; -- Briona Macolm
UPDATE `creature_template` SET `faction`=3285, `npcflag`=82, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=192121; -- Falron Greygold
UPDATE `creature_template` SET `faction`=3285, `npcflag`=128, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=192155; -- Felona Oregrab
UPDATE `creature_template` SET `faction`=3288, `speed_walk`=2, `speed_run`=1.428571462631225585, `BaseAttackTime`=1300, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=187533; -- Gulchak
UPDATE `creature_template` SET `faction`=3288, `BaseAttackTime`=2000, `unit_flags`=0x200, `unit_flags2`=0x800, `unit_flags3`=0x1 WHERE `entry`=198847; -- Swart
UPDATE `creature_template` SET `faction`=3288, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x200, `unit_flags2`=0x800, `unit_flags3`=0x1 WHERE `entry`=190910; -- Evorian
UPDATE `creature_template` SET `faction`=3285, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800, `unit_flags3`=0x40000000 WHERE `entry`=188443; -- Zinja Ujomei
UPDATE `creature_template` SET `faction`=3285, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=191711; -- Right
UPDATE `creature_template` SET `faction`=3285, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=191710; -- Left
UPDATE `creature_template` SET `faction`=3285, `npcflag`=65665, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=188351; -- Stabitha Sharptwist
UPDATE `creature_template` SET `faction`=3318, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=188415; -- Jovax the Angry
UPDATE `creature_template` SET `faction`=3285, `npcflag`=4194433, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189202; -- Uirreg Stoutbrow
UPDATE `creature_template` SET `faction`=3285, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=188625; -- Lorena Belle
UPDATE `creature_template` SET `faction`=3314, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=186451; -- Blacktalon Avenger
UPDATE `creature_template` SET `faction`=3314, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=186452; -- Blacktalon Assassin
UPDATE `creature_template` SET `faction`=3286, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x800000 WHERE `entry`=198877; -- Blacktalon Sentry
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=188440; -- Djaradin Prisoner
UPDATE `creature_template` SET `faction`=3285, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=188344; -- Chydrass
UPDATE `creature_template` SET `faction`=3285, `npcflag`=128, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=188349; -- Outfitter Tipech
UPDATE `creature_template` SET `faction`=3288, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=188623; -- Samia Inkling
UPDATE `creature_template` SET `faction`=3288, `BaseAttackTime`=2000, `unit_flags`=0x200, `unit_flags2`=0x800, `unit_flags3`=0x1 WHERE `entry`=187524; -- Obsidian Bladewing
UPDATE `creature_template` SET `faction`=3277, `npcflag`=2, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=187272; -- Forgemaster Bazentus
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=192356; -- Enraged Embers
UPDATE `creature_template` SET `faction`=3288, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=198854; -- Obsidian Outfitter
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=192344; -- Ashbinder Tornecha
UPDATE `creature_template` SET `faction`=3277, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=188179; -- Stoker Volrax
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800, `unit_flags3`=0x41000000 WHERE `entry`=187596; -- Weapon Rack Widget
UPDATE `creature_template` SET `faction`=14, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=189875; -- Animated Cindershards
UPDATE `creature_template` SET `faction`=3285, `npcflag`=3, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=193456; -- Archivist Edress
UPDATE `creature_template` SET `faction`=3277, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x800000 WHERE `entry`=187541; -- Obsidian Protector
UPDATE `creature_template` SET `faction`=3277, `npcflag`=8193, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=188336; -- Foehn Breezeskimmer
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=187598; -- Rohzor Forgesmash
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry` IN (187602, 187908); -- Qalashi Scaleripper
UPDATE `creature_template` SET `faction`=3276, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=191010; -- Scalesmith Mita
UPDATE `creature_template` SET `faction`=35, `npcflag`=1, `speed_walk`=1.20000004768371582, `speed_run`=1, `BaseAttackTime`=3600 WHERE `entry`=213636; -- Captain Garrick
UPDATE `creature_template` SET `faction`=3276, `npcflag`=1, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800, `VehicleId`=7779 WHERE `entry`=194916; -- Zakastia
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=187813; -- Qalashi Wallcrasher
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=199298; -- Qalashi Skullhauler
UPDATE `creature_template` SET `faction`=14, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x800, `unit_flags3`=0x4000000 WHERE `entry`=189825; -- Ravenous Nestling
UPDATE `creature_template` SET `faction`=14, `speed_walk`=2, `speed_run`=1.428571462631225585, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800 WHERE `entry`=190971; -- Shas'ith
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=196772; -- Battleseared Magmatusk
UPDATE `creature_template` SET `faction`=35, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x340, `unit_flags2`=0x800, `unit_flags3`=0x41000000 WHERE `entry`=187791; -- Earthen Ward
UPDATE `creature_template` SET `faction`=3288, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=188135; -- Zepharion
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x200, `unit_flags2`=0x800, `unit_flags3`=0x1 WHERE `entry`=192323; -- Trapped Whelpling
UPDATE `creature_template` SET `faction`=3288, `npcflag`=1, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=190314; -- Broodmother Dracasia
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=196336; -- Qalashi Flameslinger
UPDATE `creature_template` SET `faction`=3315, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=188155; -- Osoria
UPDATE `creature_template` SET `faction`=3315, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=188158; -- Baskilan
UPDATE `creature_template` SET `faction`=3306, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189811; -- Agitated Keystone
UPDATE `creature_template` SET `faction`=14, `speed_walk`=1.20000004768371582, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=187919; -- Caldera Stomper
UPDATE `creature_template` SET `faction`=3315, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=188140; -- Vazallia
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x4000800, `unit_flags3`=0x40000001 WHERE `entry`=196814; -- Neltharus
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=187599; -- Qalashi Bonebreaker
UPDATE `creature_template` SET `faction`=14, `speed_run`=1, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x40000001 WHERE `entry`=192325; -- Stone Shackles
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=182815; -- Summoned Lava Elemental
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=187600; -- Qalashi Stonemender
UPDATE `creature_template` SET `faction`=14, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x800, `unit_flags3`=0x40000001, `VehicleId`=7782 WHERE `entry`=189826; -- Hanging Spider Vehicle
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=192371; -- Qalashi Dusttwister
UPDATE `creature_template` SET `faction`=3288, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=192564; -- Myridian
UPDATE `creature_template` SET `faction`=3315, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=196791; -- Kixandria
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=196758; -- Magma Skitter
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry`=190118; -- Winds of the Isles
UPDATE `creature_template` SET `faction`=3295, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=190983; -- Qalashi Pillager
UPDATE `creature_template` SET `faction`=14, `speed_walk`=0.800000011920928955, `speed_run`=0.428571432828903198, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x80000 WHERE `entry`=192464; -- Raging Ember
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800, `unit_flags3`=0x80000 WHERE `entry`=194389; -- Lava Spawn
UPDATE `creature_template` SET `BaseAttackTime`=2000, `unit_flags`=0x2000000, `unit_flags2`=0x800 WHERE `entry`=120271; -- Invisible Bunny (IGC)
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x2000000, `unit_flags2`=0x800, `unit_flags3`=0x41088000 WHERE `entry`=190085; -- Grounding Spear
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags2`=0x800, `unit_flags3`=0x40080000 WHERE `entry`=189669; -- Binding Spear

UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=194816; -- Forgewrought Monstrosity
UPDATE `creature_template` SET `faction`=190, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry`=189786; -- Blazing Aegis
UPDATE `creature_template` SET `faction`=190, `npcflag`=1, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x4000800, `unit_flags3`=0x41000001 WHERE `entry`=189219; -- Qalashi Goulash
UPDATE `creature_template` SET `faction`=16, `speed_walk`=1.20000004768371582, `speed_run`=1.714285731315612792, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189478; -- Forgemaster Gorek
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=193291; -- Apex Blazewing
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=3000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189471; -- Qalashi Blacksmith
UPDATE `creature_template` SET `faction`=16, `speed_walk`=1.20000004768371582, `speed_run`=2, `BaseAttackTime`=2000, `unit_flags`=0x40, `VehicleId`=7839 WHERE `entry`=189901; -- Warlord Sargha
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=193293; -- Qalashi Warden
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=193944; -- Qalashi Lavamancer
UPDATE `creature_template` SET `faction`=16, `speed_walk`=1.20000004768371582, `speed_run`=1.714285731315612792, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags3`=0x800000 WHERE `entry`=189340; -- Chargath, Bane of Scales
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189466; -- Irontorch Commander
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry`=194883; -- Cosmetic Flavor[DNT]
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x2000200, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry`=189474; -- The Scorching Forge
UPDATE `creature_template` SET `faction`=16, `speed_walk`=2.40000009536743164, `speed_run`=2, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags3`=0x4000000, `VehicleId`=7784 WHERE `entry`=181861; -- Magmatusk
UPDATE `creature_template` SET `faction`=35, `npcflag`=16777216, `BaseAttackTime`=2000, `unit_flags2`=0x4000800, `unit_flags3`=0x41000000 WHERE `entry`=197709; -- Draconic Tincture
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x800, `unit_flags3`=0x41000000 WHERE `entry`=197337; -- Burning 3 Books
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189265; -- Qalashi Bonetender
UPDATE `creature_template` SET `faction`=190, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x800, `unit_flags3`=0x41000000 WHERE `entry`=197333; -- Burning 1 Book
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189464; -- Qalashi Irontorch
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=3000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189472; -- Qalashi Lavabearer
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x4000000, `unit_flags3`=0x41008001 WHERE `entry` IN (211204, 211227); -- Path Helper
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=1500, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189467; -- Qalashi Bonesplitter
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189470; -- Lava Flare
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x820 WHERE `entry`=189266; -- Qalashi Trainee
UPDATE `creature_template` SET `faction`=16, `speed_walk`=1.60000002384185791, `speed_run`=1.428571462631225585, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=192788; -- Qalashi Thaumaturge
UPDATE `creature_template` SET `faction`=16, `speed_walk`=1.60000002384185791, `speed_run`=1.428571462631225585, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=192786; -- Qalashi Plunderer
UPDATE `creature_template` SET `faction`=3417, `speed_run`=1.428571462631225585, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=209068; -- Ash
UPDATE `creature_template` SET `faction`=16, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189227; -- Qalashi Hunter
UPDATE `creature_template` SET `faction`=3417, `speed_walk`=1.20000004768371582, `speed_run`=1, `BaseAttackTime`=3000, `unit_flags3`=0x10020, `VehicleId`=8663 WHERE `entry`=209065; -- Austin Huxworth
UPDATE `creature_template` SET `faction`=3285, `npcflag`=2, `BaseAttackTime`=2000, `unit_flags`=0x300, `unit_flags2`=0x800 WHERE `entry`=205781; -- Archivist Edress
UPDATE `creature_template` SET `faction`=190, `npcflag`=16777216, `BaseAttackTime`=2000, `unit_flags`=0x100, `unit_flags2`=0x4000800, `unit_flags3`=0x41000001 WHERE `entry`=189342; -- Burning Chain
UPDATE `creature_template` SET `faction`=35, `BaseAttackTime`=2000, `unit_flags`=0x2000300, `unit_flags2`=0x4000000, `unit_flags3`=0x41008001 WHERE `entry`=234222; -- Helper
UPDATE `creature_template` SET `faction`=3417, `speed_run`=1.428571462631225585, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=209069; -- Nyx
UPDATE `creature_template` SET `faction`=16, `speed_walk`=0.800000011920928955, `speed_run`=0.857142865657806396, `BaseAttackTime`=2000, `unit_flags`=0x40, `unit_flags2`=0x800 WHERE `entry`=189247; -- Tamed Phoenix
UPDATE `creature_template` SET `faction`=188, `BaseAttackTime`=2000, `unit_flags2`=0x800 WHERE `entry`=189361; -- Lava Pincer

UPDATE `quest_objectives` SET `VerifiedBuild`=67451 WHERE `ID` IN (450539, 429325, 429339, 427370, 430136, 429546, 430138, 429550, 429551, 429553, 429357, 429559, 429560, 429567, 429579, 429580, 431747, 426823, 429583, 429584, 433737, 289535, 289536, 289167);
UPDATE `quest_objectives` SET `ConditionalAmount`=93858, `VerifiedBuild`=67451 WHERE `ID`=423756; -- 423756
UPDATE `quest_objectives` SET `ConditionalAmount`=-1828716544, `VerifiedBuild`=67451 WHERE `ID`=423757; -- 423757
UPDATE `quest_objectives` SET `ConditionalAmount`=80981, `VerifiedBuild`=67451 WHERE `ID`=430134; -- 430134
UPDATE `quest_objectives` SET `ConditionalAmount`=1, `VerifiedBuild`=67451 WHERE `ID`=430135; -- 430135
UPDATE `quest_objectives` SET `ConditionalAmount`=1920554797, `VerifiedBuild`=67451 WHERE `ID`=397237; -- 397237
UPDATE `quest_objectives` SET `ConditionalAmount`=127894, `VerifiedBuild`=67451 WHERE `ID`=430137; -- 430137
UPDATE `quest_objectives` SET `ConditionalAmount`=19412, `VerifiedBuild`=67451 WHERE `ID`=288101; -- 288101
UPDATE `quest_objectives` SET `ConditionalAmount`=19412, `VerifiedBuild`=67451 WHERE `ID`=287327; -- 287327
UPDATE `quest_objectives` SET `ConditionalAmount`=1920554797, `VerifiedBuild`=67451 WHERE `ID`=391353; -- 391353
UPDATE `quest_objectives` SET `ConditionalAmount`=-1151687951, `VerifiedBuild`=67451 WHERE `ID`=431741; -- 431741
UPDATE `quest_objectives` SET `ConditionalAmount`=-670797568, `VerifiedBuild`=67451 WHERE `ID`=431746; -- 431746
UPDATE `quest_objectives` SET `ConditionalAmount`=755, `VerifiedBuild`=67451 WHERE `ID`=431751; -- 431751
UPDATE `quest_objectives` SET `ConditionalAmount`=135416843, `VerifiedBuild`=67451 WHERE `ID`=427440; -- 427440
UPDATE `quest_objectives` SET `ConditionalAmount`=1936291941, `VerifiedBuild`=67451 WHERE `ID`=423347; -- 423347

DELETE FROM `creature_questitem` WHERE (`CreatureEntry`=190630 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=190630 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=190630 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=190023 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=190023 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=190023 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=191498 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=191498 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=191498 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=191498 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=191498 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189905 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=189905 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=189905 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189905 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189905 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189905 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189905 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=192787 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=192787 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=192787 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=192787 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=192787 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=192787 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=192787 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189235 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=189235 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=189235 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189235 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189235 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189235 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189235 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=192781 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=192781 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=192781 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=194816 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=194816 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=193291 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=193291 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189471 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189471 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189471 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189471 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189471 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=12) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=11) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=10) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=9) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=8) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=7) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189901 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=193944 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=193944 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=193944 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=193944 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=193944 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=193293 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=193293 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=193293 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=193293 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=193293 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=193293 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=193293 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189340 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=189340 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=189340 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189340 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189340 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189340 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189340 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189466 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189466 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189466 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189466 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189466 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=181861 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=181861 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=181861 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=181861 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189464 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189464 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189464 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189464 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189464 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189265 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189265 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189265 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189265 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189265 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189472 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189472 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189472 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189472 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189472 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189467 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189467 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189467 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189467 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189467 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189470 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189470 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=192788 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=192788 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=192788 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=192788 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=192788 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=192788 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=192788 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189266 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=189266 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=189266 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189266 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189266 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189266 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189266 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189247 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189247 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189247 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189247 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=192786 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=192786 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=192786 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=192786 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=192786 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=192786 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=192786 AND `DifficultyID`=205 AND `Idx`=0) OR (`CreatureEntry`=189227 AND `DifficultyID`=205 AND `Idx`=6) OR (`CreatureEntry`=189227 AND `DifficultyID`=205 AND `Idx`=5) OR (`CreatureEntry`=189227 AND `DifficultyID`=205 AND `Idx`=4) OR (`CreatureEntry`=189227 AND `DifficultyID`=205 AND `Idx`=3) OR (`CreatureEntry`=189227 AND `DifficultyID`=205 AND `Idx`=2) OR (`CreatureEntry`=189227 AND `DifficultyID`=205 AND `Idx`=1) OR (`CreatureEntry`=189227 AND `DifficultyID`=205 AND `Idx`=0);
INSERT INTO `creature_questitem` (`CreatureEntry`, `DifficultyID`, `Idx`, `ItemId`, `VerifiedBuild`) VALUES
(190630, 205, 2, 193864, 67451), -- Lava Tentacles
(190630, 205, 1, 220737, 67451), -- Lava Tentacles
(190630, 205, 0, 220737, 67451), -- Lava Tentacles
(190023, 205, 2, 193864, 67451), -- Lava Tentacles
(190023, 205, 1, 220737, 67451), -- Lava Tentacles
(190023, 205, 0, 220737, 67451), -- Lava Tentacles
(191498, 205, 4, 192125, 67451), -- Qalashi Mammoth Trainer
(191498, 205, 3, 193624, 67451), -- Qalashi Mammoth Trainer
(191498, 205, 2, 194435, 67451), -- Qalashi Mammoth Trainer
(191498, 205, 1, 220737, 67451), -- Qalashi Mammoth Trainer
(191498, 205, 0, 220737, 67451), -- Qalashi Mammoth Trainer
(189905, 205, 6, 201263, 67451), -- High Thaumaturge Fural
(189905, 205, 5, 201264, 67451), -- High Thaumaturge Fural
(189905, 205, 4, 192125, 67451), -- High Thaumaturge Fural
(189905, 205, 3, 193624, 67451), -- High Thaumaturge Fural
(189905, 205, 2, 194435, 67451), -- High Thaumaturge Fural
(189905, 205, 1, 220737, 67451), -- High Thaumaturge Fural
(189905, 205, 0, 220737, 67451), -- High Thaumaturge Fural
(192787, 205, 6, 201263, 67451), -- Qalashi Spinecrusher
(192787, 205, 5, 201264, 67451), -- Qalashi Spinecrusher
(192787, 205, 4, 192125, 67451), -- Qalashi Spinecrusher
(192787, 205, 3, 193624, 67451), -- Qalashi Spinecrusher
(192787, 205, 2, 194435, 67451), -- Qalashi Spinecrusher
(192787, 205, 1, 220737, 67451), -- Qalashi Spinecrusher
(192787, 205, 0, 220737, 67451), -- Qalashi Spinecrusher
(189235, 205, 6, 201263, 67451), -- Overseer Lahar
(189235, 205, 5, 201264, 67451), -- Overseer Lahar
(189235, 205, 4, 192125, 67451), -- Overseer Lahar
(189235, 205, 3, 193624, 67451), -- Overseer Lahar
(189235, 205, 2, 194435, 67451), -- Overseer Lahar
(189235, 205, 1, 220737, 67451), -- Overseer Lahar
(189235, 205, 0, 220737, 67451), -- Overseer Lahar
(192781, 205, 2, 201819, 67451), -- Ore Elemental
(192781, 205, 1, 220737, 67451), -- Ore Elemental
(192781, 205, 0, 220737, 67451), -- Ore Elemental
(194816, 205, 1, 220737, 67451), -- Forgewrought Monstrosity
(194816, 205, 0, 220737, 67451), -- Forgewrought Monstrosity
(193291, 205, 1, 220737, 67451), -- Apex Blazewing
(193291, 205, 0, 220737, 67451), -- Apex Blazewing
(189471, 205, 4, 192125, 67451), -- Qalashi Blacksmith
(189471, 205, 3, 193624, 67451), -- Qalashi Blacksmith
(189471, 205, 2, 194435, 67451), -- Qalashi Blacksmith
(189471, 205, 1, 220737, 67451), -- Qalashi Blacksmith
(189471, 205, 0, 220737, 67451), -- Qalashi Blacksmith
(189901, 205, 12, 201266, 67451), -- Warlord Sargha
(189901, 205, 11, 201263, 67451), -- Warlord Sargha
(189901, 205, 10, 201264, 67451), -- Warlord Sargha
(189901, 205, 9, 220737, 67451), -- Warlord Sargha
(189901, 205, 8, 220737, 67451), -- Warlord Sargha
(189901, 205, 7, 198515, 67451), -- Warlord Sargha
(189901, 205, 6, 193411, 67451), -- Warlord Sargha
(189901, 205, 5, 193396, 67451), -- Warlord Sargha
(189901, 205, 4, 220737, 67451), -- Warlord Sargha
(189901, 205, 3, 202002, 67451), -- Warlord Sargha
(189901, 205, 2, 192125, 67451), -- Warlord Sargha
(189901, 205, 1, 193624, 67451), -- Warlord Sargha
(189901, 205, 0, 194435, 67451), -- Warlord Sargha
(193944, 205, 4, 192125, 67451), -- Qalashi Lavamancer
(193944, 205, 3, 193624, 67451), -- Qalashi Lavamancer
(193944, 205, 2, 194435, 67451), -- Qalashi Lavamancer
(193944, 205, 1, 220737, 67451), -- Qalashi Lavamancer
(193944, 205, 0, 220737, 67451), -- Qalashi Lavamancer
(193293, 205, 6, 201263, 67451), -- Qalashi Warden
(193293, 205, 5, 201264, 67451), -- Qalashi Warden
(193293, 205, 4, 192125, 67451), -- Qalashi Warden
(193293, 205, 3, 193624, 67451), -- Qalashi Warden
(193293, 205, 2, 194435, 67451), -- Qalashi Warden
(193293, 205, 1, 220737, 67451), -- Qalashi Warden
(193293, 205, 0, 220737, 67451), -- Qalashi Warden
(189340, 205, 6, 201263, 67451), -- Chargath, Bane of Scales
(189340, 205, 5, 201264, 67451), -- Chargath, Bane of Scales
(189340, 205, 4, 192125, 67451), -- Chargath, Bane of Scales
(189340, 205, 3, 193624, 67451), -- Chargath, Bane of Scales
(189340, 205, 2, 194435, 67451), -- Chargath, Bane of Scales
(189340, 205, 1, 220737, 67451), -- Chargath, Bane of Scales
(189340, 205, 0, 220737, 67451), -- Chargath, Bane of Scales
(189466, 205, 4, 192125, 67451), -- Irontorch Commander
(189466, 205, 3, 193624, 67451), -- Irontorch Commander
(189466, 205, 2, 194435, 67451), -- Irontorch Commander
(189466, 205, 1, 220737, 67451), -- Irontorch Commander
(189466, 205, 0, 220737, 67451), -- Irontorch Commander
(181861, 205, 3, 193864, 67451), -- Magmatusk
(181861, 205, 2, 194424, 67451), -- Magmatusk
(181861, 205, 1, 220737, 67451), -- Magmatusk
(181861, 205, 0, 220737, 67451), -- Magmatusk
(189464, 205, 4, 192125, 67451), -- Qalashi Irontorch
(189464, 205, 3, 193624, 67451), -- Qalashi Irontorch
(189464, 205, 2, 194435, 67451), -- Qalashi Irontorch
(189464, 205, 1, 220737, 67451), -- Qalashi Irontorch
(189464, 205, 0, 220737, 67451), -- Qalashi Irontorch
(189265, 205, 4, 192125, 67451), -- Qalashi Bonetender
(189265, 205, 3, 193624, 67451), -- Qalashi Bonetender
(189265, 205, 2, 194435, 67451), -- Qalashi Bonetender
(189265, 205, 1, 220737, 67451), -- Qalashi Bonetender
(189265, 205, 0, 220737, 67451), -- Qalashi Bonetender
(189472, 205, 4, 192125, 67451), -- Qalashi Lavabearer
(189472, 205, 3, 193624, 67451), -- Qalashi Lavabearer
(189472, 205, 2, 194435, 67451), -- Qalashi Lavabearer
(189472, 205, 1, 220737, 67451), -- Qalashi Lavabearer
(189472, 205, 0, 220737, 67451), -- Qalashi Lavabearer
(189467, 205, 4, 192125, 67451), -- Qalashi Bonesplitter
(189467, 205, 3, 193624, 67451), -- Qalashi Bonesplitter
(189467, 205, 2, 194435, 67451), -- Qalashi Bonesplitter
(189467, 205, 1, 220737, 67451), -- Qalashi Bonesplitter
(189467, 205, 0, 220737, 67451), -- Qalashi Bonesplitter
(189470, 205, 1, 220737, 67451), -- Lava Flare
(189470, 205, 0, 220737, 67451), -- Lava Flare
(192788, 205, 6, 201263, 67451), -- Qalashi Thaumaturge
(192788, 205, 5, 201264, 67451), -- Qalashi Thaumaturge
(192788, 205, 4, 192125, 67451), -- Qalashi Thaumaturge
(192788, 205, 3, 193624, 67451), -- Qalashi Thaumaturge
(192788, 205, 2, 194435, 67451), -- Qalashi Thaumaturge
(192788, 205, 1, 220737, 67451), -- Qalashi Thaumaturge
(192788, 205, 0, 220737, 67451), -- Qalashi Thaumaturge
(189266, 205, 6, 201263, 67451), -- Qalashi Trainee
(189266, 205, 5, 201264, 67451), -- Qalashi Trainee
(189266, 205, 4, 192125, 67451), -- Qalashi Trainee
(189266, 205, 3, 193624, 67451), -- Qalashi Trainee
(189266, 205, 2, 194435, 67451), -- Qalashi Trainee
(189266, 205, 1, 220737, 67451), -- Qalashi Trainee
(189266, 205, 0, 220737, 67451), -- Qalashi Trainee
(189247, 205, 3, 193864, 67451), -- Tamed Phoenix
(189247, 205, 2, 193839, 67451), -- Tamed Phoenix
(189247, 205, 1, 220737, 67451), -- Tamed Phoenix
(189247, 205, 0, 220737, 67451), -- Tamed Phoenix
(192786, 205, 6, 201263, 67451), -- Qalashi Plunderer
(192786, 205, 5, 201264, 67451), -- Qalashi Plunderer
(192786, 205, 4, 192125, 67451), -- Qalashi Plunderer
(192786, 205, 3, 193624, 67451), -- Qalashi Plunderer
(192786, 205, 2, 194435, 67451), -- Qalashi Plunderer
(192786, 205, 1, 220737, 67451), -- Qalashi Plunderer
(192786, 205, 0, 220737, 67451), -- Qalashi Plunderer
(189227, 205, 6, 201263, 67451), -- Qalashi Hunter
(189227, 205, 5, 201264, 67451), -- Qalashi Hunter
(189227, 205, 4, 192125, 67451), -- Qalashi Hunter
(189227, 205, 3, 193624, 67451), -- Qalashi Hunter
(189227, 205, 2, 194435, 67451), -- Qalashi Hunter
(189227, 205, 1, 220737, 67451), -- Qalashi Hunter
(189227, 205, 0, 220737, 67451); -- Qalashi Hunter

UPDATE `gameobject_template` SET `VerifiedBuild`=67451 WHERE `entry` IN (380540, 324067, 376645, 376035, 386655, 376121, 375235, 381326, 381521, 381323, 379194, 381533, 381577, 381097, 381231, 456130, 377522, 377521, 377520, 377523, 381100, 384972, 376289, 376284, 376282, 377227, 381341, 381303, 380869, 380918, 381304, 381302, 381236, 381235, 381233, 381232, 381229, 381228, 379210, 379209, 379213, 379211, 379212, 376281, 376288, 376283, 376286, 380870, 384839, 384836, 376678, 268883, 381560, 381209, 384306, 384305, 381671, 381664, 385209, 381748, 381747, 381556, 381555, 381554, 378944, 381561, 381743, 376392, 381553, 381742, 378168, 378169, 378166, 378171, 378167, 376454, 376453, 376397, 376711, 378891, 376762, 377541, 384958, 381746, 381745, 381744, 381563, 381562, 378946, 378945, 380880, 375241, 403540, 382345, 376824, 377022, 376151, 379196, 379195, 378849, 378850, 378705, 381102, 379184, 369686, 379185, 376646, 380648, 379191, 377073, 379190, 380878, 379188, 377074, 380877, 380876, 186722, 380874, 364849, 379252, 379197, 379192, 379041, 379199, 252247, 381340, 380875, 380534, 376634, 376920, 379200, 379201, 379158, 381336, 379193, 376587, 379262, 379256, 379257, 379255, 379258, 379259, 379254, 379260, 379261, 379131, 377496, 379132, 377495, 377494, 377493, 379130, 379129, 379128, 377966, 252248);
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=382287; -- Blooming Fangtooth
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=375244; -- Frigid Bubble Poppy
UPDATE `gameobject_template` SET `RequiredLevel`=8, `VerifiedBuild`=67451 WHERE `entry`=376338; -- Wurmling Bone Pile
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=381104; -- Rich Serevite Deposit
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=381516; -- Molten Serevite Deposit
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=378857; -- Lost Obsidian Cache
UPDATE `gameobject_template` SET `Data25`=25183, `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=379263; -- Rich Draconium Deposit
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=381257; -- Rich Draconium Deposit
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=381617; -- Kolgar's Pack
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=382068; -- Barbed Tulip
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry` IN (377101, 377100, 377102); -- Unpollinated Flora
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry` IN (380835, 380850); -- Meeting Stone
UPDATE `gameobject_template` SET `Data20`=65552, `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=380643; -- Flashfrozen Scroll
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=381960; -- Lush Hochenblume
UPDATE `gameobject_template` SET `Data20`=65552, `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=380559; -- Frostforged Potion
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=381518; -- Primal Serevite Deposit
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=380901; -- Ancient Waygate
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=380840; -- Torn Riding Pack
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=381103; -- Serevite Deposit
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=381483; -- Marmoni's Prize
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=421736; -- Explorer Pepe
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=379248; -- Draconium Deposit
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=382031; -- Oily Dropcap
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=377485; -- Sour Apple
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=382030; -- Nectar Filled Panthis
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=381674; -- Mature White Bell
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=376104; -- Siege Arbalest
UPDATE `gameobject_template` SET `Data20`=65536, `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=380617; -- Qalashi Weapon Diagram
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry` IN (380826, 380825, 380606); -- Focusing Crystal
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=268888; -- Fishing Bobber
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=380557; -- Igneous Geode
UPDATE `gameobject_template` SET `RequiredLevel`=1, `VerifiedBuild`=67451 WHERE `entry`=381207; -- Saxifrage
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=382032; -- Flowering Inferno Pod
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=376580; -- Djaradin Cache
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=381420; -- Dragonsteel Axe
UPDATE `gameobject_template` SET `RequiredLevel`=8, `VerifiedBuild`=67451 WHERE `entry`=382241; -- Earth-Warder's Forge
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=382033; -- Djaradin Supply Jar
UPDATE `gameobject_template` SET `RequiredLevel`=8, `VerifiedBuild`=67451 WHERE `entry`=381071; -- Box of Rocks
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=378348; -- Sunlight Flower
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=381421; -- Obsidian Anvil
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=377161; -- Cindershard Coal
UPDATE `gameobject_template` SET `RequiredLevel`=10, `VerifiedBuild`=67451 WHERE `entry`=384317; -- Crumpled Schematic

UPDATE `gameobject_questitem` SET `VerifiedBuild`=67451 WHERE (`GameObjectEntry`=381617 AND `Idx`=0);