SET @CGUID := 40000000;
SET @OGUID := 40000000;

--------------------------------------
--             creature             --
--------------------------------------
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+144;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `MovementType`, `npcflag`, `unit_flags`, `unit_flags2`, `unit_flags3`, `VerifiedBuild`) VALUES
(@CGUID+0, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2359.614501953125, -1431.0191650390625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+1, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2379.80908203125, -1431.1961669921875, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+2, 54917, 870, 5785, 5855, '0', 0, 0, 0, 0, 2333.71533203125, -1453.29345703125, 362.342742919921875, 0.331612557172775268, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Instructor Xann (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+3, 55183, 870, 5785, 5855, '0', 0, 0, 0, 0, 2395.882080078125, -1462.4322509765625, 353.89581298828125, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 103079 - Absorb)
(@CGUID+4, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2374.092041015625, -1426.234375, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+5, 55183, 870, 5785, 5855, '0', 0, 0, 0, 0, 2392.857666015625, -1410.920166015625, 353.9228515625, 3.857177734375, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 103079 - Absorb)
(@CGUID+6, 55183, 870, 5785, 5855, '0', 0, 0, 0, 0, 2400.083251953125, -1458.3333740234375, 353.976806640625, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 103079 - Absorb)
(@CGUID+7, 55184, 870, 5785, 5855, '0', 0, 0, 0, 0, 2393.864501953125, -1464.454833984375, 354.012939453125, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: )
(@CGUID+8, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2393.460205078125, -1462.0086669921875, 354.1890869140625, 4.876186370849609375, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+9, 55184, 870, 5785, 5855, '0', 0, 0, 0, 0, 2394.97216796875, -1412.9166259765625, 353.93048095703125, 3.857177734375, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: )
(@CGUID+10, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2369.241455078125, -1440.734375, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+11, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2374.232666015625, -1431.1510009765625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+12, 55184, 870, 5785, 5855, '0', 0, 0, 0, 0, 2398.04345703125, -1460.3629150390625, 353.95947265625, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: )
(@CGUID+13, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2359.177001953125, -1444.939208984375, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+14, 55199, 870, 5785, 5855, '0', 0, 0, 0, 0, 2352.7587890625, -1435.9461669921875, 354.1890869140625, 0.03490658476948738, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Instructor (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+15, 55183, 870, 5785, 5855, '0', 0, 0, 0, 0, 2357.048583984375, -1413.0625, 353.958648681640625, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 103079 - Absorb)
(@CGUID+16, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2374.2431640625, -1440.9149169921875, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+17, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2369.114501953125, -1445.23095703125, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+18, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2355.085205078125, -1417.4288330078125, 354.1890869140625, 1.638750910758972167, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+19, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2379.819580078125, -1440.9600830078125, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+20, 55184, 870, 5785, 5855, '0', 0, 0, 0, 0, 2359.017333984375, -1410.93408203125, 354.03033447265625, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: )
(@CGUID+21, 55183, 870, 5785, 5855, '0', 0, 0, 0, 0, 2397.232666015625, -1415.02783203125, 353.93048095703125, 3.857177734375, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 103079 - Absorb)
(@CGUID+22, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2374.473876953125, -1445.5052490234375, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+23, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2364.3125, -1440.623291015625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+24, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2364.161376953125, -1425.9444580078125, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+25, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2379.66845703125, -1426.279541015625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+26, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2379.73779296875, -1436.0660400390625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+27, 55184, 870, 5785, 5855, '0', 0, 0, 0, 0, 2399.251708984375, -1417.0625, 353.93341064453125, 3.857177734375, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: )
(@CGUID+28, 70265, 870, 5785, 5855, '0', 0, 0, 0, 0, 2386.442626953125, -1364.5677490234375, 342.839508056640625, 1.195741772651672363, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Dan Mei (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+29, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2369.553955078125, -1431.310791015625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+30, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2364.232666015625, -1435.73095703125, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+31, 70264, 870, 5785, 5855, '0', 0, 0, 0, 0, 2388.796875, -1359.2117919921875, 344.5009765625, 3.803035497665405273, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Jimmy Truong (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+32, 70267, 870, 5785, 5855, '0', 0, 0, 0, 0, 2383.779541015625, -1360.095458984375, 340.384246826171875, 0.197355687618255615, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Emily Mei (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+33, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2364.541748046875, -1445.2135009765625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+34, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2359.345458984375, -1413.2447509765625, 354.1890869140625, 1.711857438087463378, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+35, 70266, 870, 5785, 5855, '0', 0, 0, 0, 0, 2383.848876953125, -1361.5850830078125, 340.728271484375, 0.357501685619354248, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Kevin Horng (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+36, 55183, 870, 5785, 5855, '0', 0, 0, 0, 0, 2401.3837890625, -1419.1754150390625, 353.93682861328125, 3.857177734375, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 103079 - Absorb)
(@CGUID+37, 55183, 870, 5785, 5855, '0', 0, 0, 0, 0, 2404.223876953125, -1454.1129150390625, 354.040924072265625, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 103079 - Absorb)
(@CGUID+38, 55184, 870, 5785, 5855, '0', 0, 0, 0, 0, 2403.411376953125, -1421.1492919921875, 353.9228515625, 3.857177734375, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: )
(@CGUID+39, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2364.303955078125, -1430.859375, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+40, 55184, 870, 5785, 5855, '0', 0, 0, 0, 0, 2402.229248046875, -1456.1632080078125, 353.849609375, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: )
(@CGUID+41, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2369.248291015625, -1436.0572509765625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+42, 55183, 870, 5785, 5855, '0', 0, 0, 0, 0, 2348.723876953125, -1421.32470703125, 353.916961669921875, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 103079 - Absorb)
(@CGUID+43, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2380.048583984375, -1445.5504150390625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+44, 55183, 870, 5785, 5855, '0', 0, 0, 0, 0, 2352.89404296875, -1417.2396240234375, 353.940093994140625, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 103079 - Absorb)
(@CGUID+45, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2401.723876953125, -1453.4913330078125, 354.18914794921875, 4.899325370788574218, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+46, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2359.6337890625, -1425.76220703125, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+47, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2369.572998046875, -1426.0538330078125, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+48, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2374.161376953125, -1436.0208740234375, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+49, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2359.30908203125, -1435.765625, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+50, 55184, 870, 5785, 5855, '0', 0, 0, 0, 0, 2350.710205078125, -1419.326416015625, 353.869293212890625, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: )
(@CGUID+51, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2392.677001953125, -1413.3160400390625, 354.18939208984375, 0.172298550605773925, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+52, 55184, 870, 5785, 5855, '0', 0, 0, 0, 0, 2354.93408203125, -1415.2083740234375, 353.990386962890625, 5.497786998748779296, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Training Bag (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: )
(@CGUID+53, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2359.302001953125, -1440.4427490234375, 354.1890869140625, 3.089232683181762695, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+54, 64775, 870, 5785, 5855, '0', 0, 0, 0, 0, 2478.59130859375, -1346.926513671875, 374.677215576171875, 3.849583625793457031, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Masked Tanuki (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+55, 59666, 870, 5785, 5855, '0', 0, 0, 0, 0, 2488.526123046875, -1453.4114990234375, 388.790008544921875, 1.4486008882522583, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Emerald Turtle (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+56, 54982, 870, 5785, 5855, '0', 0, 0, 0, 0, 2506.9072265625, -1465.2117919921875, 388.24090576171875, 3.071779489517211914, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Brewmaster Lei Kanglei (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+57, 59666, 870, 5785, 5855, '0', 0, 0, 0, 0, 2513.60107421875, -1430.0250244140625, 388.33160400390625, 4.174480438232421875, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Emerald Turtle (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+58, 54981, 870, 5785, 5855, '0', 0, 0, 0, 0, 2528.147705078125, -1438.6944580078125, 388.24090576171875, 3.054326057434082031, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- June Whiteblossom (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+59, 54980, 870, 5785, 5855, '0', 0, 0, 0, 0, 2513.626708984375, -1465.170166015625, 388.24090576171875, 0, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Yao the Collector (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+60, 59666, 870, 5785, 5855, '0', 0, 0, 0, 0, 2535.443359375, -1365.305908203125, 388.02239990234375, 5.978714466094970703, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Emerald Turtle (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+61, 59666, 870, 5785, 5855, '0', 0, 0, 0, 0, 2475.445556640625, -1268.439453125, 388.341094970703125, 1.828567385673522949, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Emerald Turtle (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+62, 59666, 870, 5785, 5855, '0', 0, 0, 0, 0, 2459.537841796875, -1269.4912109375, 384.14031982421875, 4.964586257934570312, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Emerald Turtle (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+63, 59772, 870, 5785, 5855, '0', 0, 0, 0, 0, 2404.973388671875, -1216.3089599609375, 387.076385498046875, 6.275372982025146484, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Smoky Porcupine (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+64, 59666, 870, 5785, 5855, '0', 0, 0, 0, 0, 2482.6875, -1258.685791015625, 388.320068359375, 1.4486008882522583, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Emerald Turtle (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+65, 64776, 870, 5785, 5855, '0', 0, 0, 0, 0, 2408.42578125, -1250.0335693359375, 385.42431640625, 0.851326048374176025, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Masked Tanuki Pup (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+66, 64775, 870, 5785, 5855, '0', 0, 0, 0, 0, 2411.280029296875, -1247.2071533203125, 385.27374267578125, 0.981840729713439941, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Masked Tanuki (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+67, 64776, 870, 5785, 5855, '0', 0, 0, 0, 0, 2410.1689453125, -1248.8702392578125, 385.35870361328125, 0.083588004112243652, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Masked Tanuki Pup (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+68, 54918, 870, 5785, 5855, '0', 0, 0, 0, 0, 2532.963623046875, -1269.0728759765625, 388.17138671875, 0.03490658476948738, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Instructor Myang (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+69, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2553.5390625, -1257.78125, 377.8021240234375, 2.216568231582641601, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+70, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2561.540771484375, -1249.04345703125, 377.800201416015625, 2.426007747650146484, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+71, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2557.65283203125, -1253.6024169921875, 377.800201416015625, 2.426007747650146484, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+72, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2582.282958984375, -1290.3160400390625, 377.800201416015625, 5.602506637573242187, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+73, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2590.19091796875, -1289.9757080078125, 377.800201416015625, 5.602506637573242187, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+74, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2590.364501953125, -1281.8819580078125, 377.800201416015625, 5.602506637573242187, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+75, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2597.725830078125, -1281.529541015625, 377.800201416015625, 5.602506637573242187, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+76, 55198, 870, 5785, 5855, '0', 0, 0, 0, 0, 2598.059814453125, -1273.8819580078125, 377.800201416015625, 5.602506637573242187, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Student (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+77, 55094, 870, 5785, 5855, '0', 0, 0, 0, 0, 2544.61376953125, -1202.5260009765625, 380.774627685546875, 0.837758064270019531, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Guard Shan Long (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+78, 64775, 870, 5785, 5868, '0', 0, 0, 0, 0, 2608.744873046875, -1196.5660400390625, 374.1494140625, 0, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Masked Tanuki (Area: The Ring of Inner Focus - Difficulty: 0) CreateObject1
(@CGUID+79, 59772, 870, 5785, 5874, '0', 0, 0, 0, 0, 2395.650390625, -1155.1927490234375, 397.6087646484375, 4.418126106262207031, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Smoky Porcupine (Area: Waxwood - Difficulty: 0) CreateObject1
(@CGUID+80, 64775, 870, 5785, 6398, '0', 0, 0, 0, 0, 2407.494384765625, -1177.9176025390625, 395.36016845703125, 1.48960268497467041, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Masked Tanuki (Area: Shrine of Twin Serpents - Difficulty: 0) CreateObject1
(@CGUID+81, 54915, 870, 5785, 5855, '0', 0, 0, 0, 0, 2549.1171875, -1454.125, 406.209930419921875, 3.071779489517211914, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Groundskeeper Wu (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+82, 62867, 870, 5785, 5855, '0', 0, 0, 0, 0, 2574.36279296875, -1461.2882080078125, 407.216552734375, 2.593628644943237304, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Bolo the Elder (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+83, 54926, 870, 5785, 5855, '0', 0, 0, 0, 0, 2552.33935546875, -1559.6822509765625, 406.922088623046875, 5.532693862915039062, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Xiao (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+84, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2555.3369140625, -1540.51220703125, 406.062408447265625, 5.375614166259765625, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+85, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2558.0087890625, -1542.5225830078125, 406.063720703125, 2.251474618911743164, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+86, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2569.244873046875, -1547.8367919921875, 406.062255859375, 2.251474618911743164, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+87, 55029, 870, 5785, 5855, '0', 0, 0, 0, 0, 2564.9462890625, -1546.8941650390625, 407.0040283203125, 5.375614166259765625, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+88, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2545, -1555.3367919921875, 406.146026611328125, 2.251474618911743164, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+89, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2542.647705078125, -1553.15625, 406.146026611328125, 5.375614166259765625, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+90, 55030, 870, 5785, 5855, '0', 0, 0, 0, 0, 2546.568603515625, -1553.7430419921875, 406.227783203125, 2.321287870407104492, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+91, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2563.647705078125, -1548.625, 406.062408447265625, 5.375614166259765625, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+92, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2566.247314453125, -1550.9097900390625, 406.062255859375, 2.251474618911743164, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+93, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2497.288330078125, -1559.2274169921875, 400.884613037109375, 5.078907966613769531, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+94, 55207, 870, 5785, 5855, '0', 0, 0, 0, 0, 2583.90625, -1539.951416015625, 406.37774658203125, 2.914699792861938476, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Beer-Fetcher (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+95, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2498.76904296875, -1562.1910400390625, 400.884613037109375, 2.076941728591918945, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+96, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2552.993896484375, -1563.6302490234375, 406.146026611328125, 2.251474618911743164, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+97, 54922, 870, 5785, 5855, '0', 0, 0, 0, 0, 2499.927001953125, -1567.24658203125, 400.951629638671875, 4.939281940460205078, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Master Stone Fist (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+98, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2554.7578125, -1561.9774169921875, 406.146026611328125, 2.321287870407104492, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+99, 55028, 870, 5785, 5855, '0', 0, 0, 0, 0, 2556.29248046875, -1560.5086669921875, 406.146026611328125, 2.251474618911743164, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Trainee (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+100, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2481.704833984375, -1564.5728759765625, 400.951568603515625, 3.874630928039550781, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+101, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2487.41845703125, -1576.1146240234375, 400.951568603515625, 3.281219005584716796, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+102, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2479.1181640625, -1566.654541015625, 400.951568603515625, 0.593411922454833984, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+103, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2484.11279296875, -1576.3489990234375, 400.951568603515625, 0, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+104, 59736, 870, 5785, 5855, '0', 0, 0, 0, 0, 2510.256103515625, -1590.8697509765625, 400.95159912109375, 2.87212228775024414, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Studious Chu (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+105, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2471.614501953125, -1579.40625, 400.951568603515625, 1.93731546401977539, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+106, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2470.6025390625, -1576.251708984375, 400.951568603515625, 4.939281940460205078, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+107, 54944, 870, 5785, 5863, '0', 0, 0, 0, 0, 2523.895751953125, -1603.9635009765625, 410.4600830078125, 4.15388345718383789, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Banquet Grounds - Difficulty: 0) CreateObject1
(@CGUID+108, 54944, 870, 5785, 5863, '0', 0, 0, 0, 0, 2504.612060546875, -1600.4132080078125, 400.951568603515625, 4.939281940460205078, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Banquet Grounds - Difficulty: 0) CreateObject1
(@CGUID+109, 54944, 870, 5785, 5863, '0', 0, 0, 0, 0, 2522.42529296875, -1606.9288330078125, 410.880218505859375, 1.151917338371276855, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Banquet Grounds - Difficulty: 0) CreateObject1
(@CGUID+110, 54944, 870, 5785, 5863, '0', 0, 0, 0, 0, 2505.624267578125, -1603.5677490234375, 400.95166015625, 1.93731546401977539, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Banquet Grounds - Difficulty: 0) CreateObject1
(@CGUID+111, 55207, 870, 5785, 5863, '0', 0, 0, 0, 0, 2580.62939453125, -1595.2083740234375, 417.529541015625, 2.30383467674255371, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Beer-Fetcher (Area: Banquet Grounds - Difficulty: 0) CreateObject1 (Auras: 102402 - Carry Keg (TEMP))
(@CGUID+112, 54924, 870, 5785, 5855, '0', 0, 0, 0, 1, 2442.217041015625, -1554.717041015625, 411.548614501953125, 0, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Zhi-Zhi (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+113, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2489.7744140625, -1599.2847900390625, 400.951568603515625, 4.939281940460205078, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+114, 54919, 870, 5785, 5855, '0', 0, 0, 0, 0, 2453.185791015625, -1594.09033203125, 400.951568603515625, 6.199227333068847656, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Spirit-Sage Gaoquan (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 118697 - Meditate)
(@CGUID+115, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2474.166748046875, -1604.2864990234375, 400.951568603515625, 5.759586334228515625, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+116, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2490.786376953125, -1602.439208984375, 400.951568603515625, 1.93731546401977539, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+117, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2477.151123046875, -1605.734375, 400.951568603515625, 2.757620096206665039, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+118, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2490.90625, -1614.6910400390625, 400.951568603515625, 0, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+119, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2494.213623046875, -1614.4566650390625, 400.951568603515625, 3.281219005584716796, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+120, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2496.654541015625, -1628.96533203125, 400.951568603515625, 1.029744267463684082, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+121, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2445.779541015625, -1625.07470703125, 410.58905029296875, 5.427973747253417968, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+122, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2498.505126953125, -1626.2117919921875, 400.951568603515625, 4.031710624694824218, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+123, 59782, 870, 5785, 5855, '0', 0, 0, 0, 0, 2608.96337890625, -1654.793701171875, 515.43408203125, 1.842472672462463378, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Cypress Condor (Area: Tian Monastery - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+124, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2475.598876953125, -1627.2396240234375, 400.951568603515625, 5.393355369567871093, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+125, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2478.515625, -1629.4791259765625, 400.951568603515625, 2.513561725616455078, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+126, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2448.1875, -1627.3472900390625, 410.60723876953125, 2.426007747650146484, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+127, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2458.657958984375, -1642.4791259765625, 400.973724365234375, 4.939281940460205078, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+128, 54944, 870, 5785, 5855, '0', 0, 0, 0, 0, 2459.670166015625, -1645.6336669921875, 400.914093017578125, 1.93731546401977539, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Tian Pupil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@CGUID+129, 54925, 870, 5785, 6397, '0', 0, 0, 0, 1, 2532.64501953125, -1659.8507080078125, 401.276611328125, 5.707226753234863281, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Husshun (Area: The Clarion Bell - Difficulty: 0) CreateObject1
(@CGUID+130, 59782, 870, 5785, 6397, '0', 0, 0, 0, 0, 2590.20263671875, -1677.144775390625, 557.63134765625, 0.796553671360015869, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Cypress Condor (Area: The Clarion Bell - Difficulty: 0) CreateObject1 (possible waypoints or random movement)
(@CGUID+131, 54913, 870, 5785, 5863, '0', 0, 0, 0, 0, 2490.7119140625, -1689.84375, 430.049285888671875, 3.979350566864013671, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- Lin Tenderpaw (Area: Banquet Grounds - Difficulty: 0) CreateObject1
(@CGUID+132, 54914, 870, 5785, 5855, '0', 0, 0, 0, 0, 2485.407958984375, -1705.782958984375, 448.911651611328125, 1.466076612472534179, 120, 0, 0, 0, NULL, NULL, NULL, NULL, 60037), -- High Elder Cloudfall (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: 82343 - Generic Quest Invisibility 4)
(@CGUID+133, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2396.06509765625, -1752.3752197265625, 373.899611328125, 2.598137936172485351, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+134, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2388.3557421875, -1734.0590087890625, 373.6164345703125, 4.244442375106811523, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+135, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2373.50830078125, -1738.9556884765625, 374.78741455078125, 2.099133014678955078, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+136, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2373.51048828125, -1738.96056640625, 374.787834716796875, 2.099135708618164062, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+137, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2381.154296875, -1760.38720703125, 374.88665771484375, 2.862542152404785156, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+138, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2367.918828125, -1756.9880126953125, 375.188102294921875, 1.302621106262207031, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+139, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2343.958369140625, -1707.5255517578125, 328.30817431640625, 3.227650882110595703, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+140, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2338.672607421875, -1695.72216796875, 330.435760498046875, 0.73208630084991455, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+141, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2343.48734375, -1718.4568359375, 327.269291259765625, 2.465810091762542724, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+142, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2324.21826171875, -1723.8531494140625, 323.782501220703125, 5.396851062774658203, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+143, 54930, 870, 5785, 5855, '0', 0, 0, 0, 0, 2327.8818359375, -1701.607177734375, 324.74395751953125, 4.063465118408203125, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037), -- Greenwood Thief (Area: Tian Monastery - Difficulty: 0) CreateObject1 (Auras: ) (possible waypoints or random movement)
(@CGUID+144, 64761, 870, 5785, 0, '0', 0, 0, 0, 0, 2329.201416015625, -1631.954833984375, 376.7938232421875, 0.157079637050628662, 120, 10, 0, 1, NULL, NULL, NULL, NULL, 60037); -- Jumping Spider (Area: 0 - Difficulty: 0) CreateObject1 (possible waypoints or random movement)

--------------------------------------
--     creature_template_addon      --
--------------------------------------
DELETE FROM `creature_template_addon` WHERE `entry` IN (59782 /*59782 (Cypress Condor)*/, 55207 /*55207 (Tian Beer-Fetcher)*/, 55029 /*55029 (Tian Trainee)*/, 54926 /*54926 (Xiao)*/, 54980 /*54980 (Yao the Collector)*/, 70266 /*70266 (Kevin Horng)*/, 70267 /*70267 (Emily Mei)*/, 70265 /*70265 (Dan Mei)*/);
INSERT INTO `creature_template_addon` (`entry`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(59782, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 59782 (Cypress Condor)
(55207, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 55207 (Tian Beer-Fetcher)
(55029, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 55029 (Tian Trainee)
(54926, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 54926 (Xiao)
(54980, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 54980 (Yao the Collector)
(70266, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 70266 (Kevin Horng)
(70267, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''), -- 70267 (Emily Mei)
(70265, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''); -- 70265 (Dan Mei)

--------------------------------------
--          creature_addon          --
--------------------------------------
DELETE FROM `creature_addon` WHERE `guid` BETWEEN @CGUID+0 AND @CGUID+132;
INSERT INTO `creature_addon` (`guid`, `PathId`, `mount`, `StandState`, `AnimTier`, `VisFlags`, `SheathState`, `PvpFlags`, `emote`, `aiAnimKit`, `movementAnimKit`, `meleeAnimKit`, `visibilityDistanceType`, `auras`) VALUES
(@CGUID+0, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+1, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+4, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+8, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+10, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+11, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+13, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+14, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Instructor
(@CGUID+16, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+17, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+18, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+19, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+22, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+23, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+24, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+25, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+26, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+29, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+30, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+31, 0, 0, 0, 0, 0, 1, 0, 512, 0, 0, 0, 0, ''), -- Jimmy Truong
(@CGUID+33, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+34, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+39, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+41, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+43, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+45, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+46, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+47, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+48, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+49, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+51, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+53, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+69, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+70, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+71, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+72, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+73, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+74, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+75, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+76, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Student
(@CGUID+84, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+85, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+86, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+88, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+89, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+90, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+91, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+92, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+93, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+95, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+96, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+98, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+99, 0, 0, 0, 0, 0, 1, 0, 426, 0, 0, 0, 0, ''), -- Tian Trainee
(@CGUID+100, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+101, 0, 0, 0, 0, 0, 1, 0, 510, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+102, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+103, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+105, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+106, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+107, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+108, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+109, 0, 0, 0, 0, 0, 1, 0, 510, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+110, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+113, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+114, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, '118697'), -- Spirit-Sage Gaoquan - 118697 - Meditate
(@CGUID+115, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+116, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+117, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+118, 0, 0, 0, 0, 0, 1, 0, 510, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+119, 0, 0, 0, 0, 0, 1, 0, 510, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+120, 0, 0, 0, 0, 0, 1, 0, 510, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+121, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+122, 0, 0, 0, 0, 0, 1, 0, 510, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+124, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+125, 0, 0, 0, 0, 0, 1, 0, 510, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+126, 0, 0, 0, 0, 0, 1, 0, 27, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+127, 0, 0, 0, 0, 0, 1, 0, 510, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+128, 0, 0, 0, 0, 0, 1, 0, 510, 0, 0, 0, 0, ''), -- Tian Pupil
(@CGUID+132, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, ''); -- High Elder Cloudfall - 82343

--------------------------------------
--            gameobject            --
--------------------------------------
DELETE FROM `gameobject` WHERE `guid` BETWEEN @OGUID+0 AND @OGUID+136;
INSERT INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `PhaseId`, `PhaseGroup`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `VerifiedBuild`) VALUES
(@OGUID+0, 211495, 870, 5785, 5855, '0', 0, 0, 2502.229248046875, -1444.4322509765625, 387.940826416015625, 2.783795118331909179, 0, 0, 0.984040260314941406, 0.177946031093597412, 120, 255, 1, 60037), -- Anvil (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+1, 211496, 870, 5785, 5855, '0', 0, 0, 2505.2587890625, -1440.4617919921875, 388.15234375, 0.183258801698684692, 0, 0, 0.091501235961914062, 0.995804965496063232, 120, 255, 1, 60037), -- Forge (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+2, 212613, 870, 5785, 5855, '0', 0, 0, 2296.2080078125, -1425.3232421875, 374.1951904296875, 2.975770950317382812, 0, 0, 0.996564865112304687, 0.082815870642662048, 120, 255, 1, 60037), -- Fireplace (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+3, 340097, 870, 5785, 5855, '0', 0, 0, 2298.11328125, -1428.6380615234375, 374.96771240234375, 3.312786102294921875, 0, 0, -0.9963388442993164, 0.085492104291915893, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+4, 340096, 870, 5785, 5855, '0', 0, 0, 2290.75, -1440.220458984375, 374.983001708984375, 3.574577569961547851, 0, 0, -0.97665691375732421, 0.214805185794830322, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+5, 209464, 870, 5785, 5855, '0', 0, 0, 2512.397705078125, -1462.6875, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+6, 212171, 870, 5785, 5855, '0', 0, 0, 2518.09033203125, -1396.6492919921875, 388.148406982421875, 3.141592741012573242, 0, 0, -1, 0, 120, 255, 1, 60037), -- Jade Lungfish School (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+7, 209461, 870, 5785, 5855, '0', 0, 0, 2472.423583984375, -1444.109375, 390.370147705078125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+8, 209311, 870, 5785, 5855, '0', 0, 0, 2416.489501953125, -1289.9757080078125, 365.958892822265625, 0.212233960628509521, 0, 0, 0.105917930603027343, 0.994374871253967285, 120, 255, 1, 60037), -- Ghost Iron Deposit (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+9, 209461, 870, 5785, 5855, '0', 0, 0, 2494.833251953125, -1471.71875, 388.975250244140625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+10, 209464, 870, 5785, 5855, '0', 0, 0, 2512.4609375, -1463.29345703125, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+11, 209464, 870, 5785, 5855, '0', 0, 0, 2512.7197265625, -1462.876708984375, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+12, 209463, 870, 5785, 5855, '0', 0, 0, 2497.935791015625, -1380.4010009765625, 388.49053955078125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+13, 209462, 870, 5785, 5855, '0', 0, 0, 2502.116455078125, -1508.3629150390625, 393.8125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+14, 209464, 870, 5785, 5855, '0', 0, 0, 2512.55029296875, -1462.984375, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+15, 209461, 870, 5785, 5855, '0', 0, 0, 2427.432373046875, -1359.7535400390625, 384.299224853515625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+16, 209461, 870, 5785, 5855, '0', 0, 0, 2432.545166015625, -1522.673583984375, 395.143707275390625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+17, 209464, 870, 5785, 5855, '0', 0, 0, 2512.3447265625, -1463.21875, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+18, 209460, 870, 5785, 5855, '0', 0, 0, 2472.454833984375, -1443.8958740234375, 392.19329833984375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+19, 209463, 870, 5785, 5855, '0', 0, 0, 2405.982666015625, -1376.5521240234375, 357.654510498046875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+20, 209462, 870, 5785, 5855, '0', 0, 0, 2434.416748046875, -1520.1510009765625, 395.444366455078125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+21, 209462, 870, 5785, 5855, '0', 0, 0, 2493.100830078125, -1380.0242919921875, 389.051727294921875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+22, 209464, 870, 5785, 5855, '0', 0, 0, 2512.560791015625, -1462.7430419921875, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+23, 209463, 870, 5785, 5855, '0', 0, 0, 2483.192626953125, -1321.1632080078125, 374.395416259765625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+24, 209463, 870, 5785, 5855, '0', 0, 0, 2491.854248046875, -1322.829833984375, 375.348846435546875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+25, 209461, 870, 5785, 5855, '0', 0, 0, 2434.1875, -1520.32470703125, 395.392364501953125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+26, 209460, 870, 5785, 5855, '0', 0, 0, 2404.1494140625, -1376.798583984375, 356.743072509765625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+27, 209462, 870, 5785, 5855, '0', 0, 0, 2432.31591796875, -1522.8472900390625, 395.109375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+28, 209462, 870, 5785, 5855, '0', 0, 0, 2492.729248046875, -1323.142333984375, 375.251617431640625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+29, 209464, 870, 5785, 5855, '0', 0, 0, 2512.420166015625, -1463.0555419921875, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+30, 209461, 870, 5785, 5855, '0', 0, 0, 2495.0625, -1471.545166015625, 388.957427978515625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+31, 209463, 870, 5785, 5855, '0', 0, 0, 2462.927001953125, -1523.8785400390625, 395.186798095703125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+32, 209462, 870, 5785, 5855, '0', 0, 0, 2472.65283203125, -1443.935791015625, 392.32720947265625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+33, 209463, 870, 5785, 5855, '0', 0, 0, 2407.803955078125, -1354.1336669921875, 365.830963134765625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+34, 209460, 870, 5785, 5855, '0', 0, 0, 2406.257080078125, -1376.359375, 357.698944091796875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+35, 209462, 870, 5785, 5855, '0', 0, 0, 2497.34716796875, -1335.298583984375, 373.871795654296875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+36, 209462, 870, 5785, 5855, '0', 0, 0, 2495.579833984375, -1380.310791015625, 387.637725830078125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+37, 209461, 870, 5785, 5855, '0', 0, 0, 2502.345458984375, -1508.189208984375, 393.911468505859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+38, 209460, 870, 5785, 5855, '0', 0, 0, 2494.864501953125, -1471.5052490234375, 388.956817626953125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+39, 209462, 870, 5785, 5855, '0', 0, 0, 2424.920166015625, -1358.6353759765625, 384.2841796875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+40, 209464, 870, 5785, 5855, '0', 0, 0, 2512.3125, -1462.85595703125, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+41, 209464, 870, 5785, 5855, '0', 0, 0, 2512.587646484375, -1463.170166015625, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+42, 209463, 870, 5785, 5855, '0', 0, 0, 2462.748291015625, -1524.203125, 395.232635498046875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+43, 209462, 870, 5785, 5855, '0', 0, 0, 2408.416748046875, -1354.6180419921875, 365.84844970703125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+44, 209462, 870, 5785, 5855, '0', 0, 0, 2426.40966796875, -1359.859375, 384.35699462890625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+45, 209462, 870, 5785, 5855, '0', 0, 0, 2502.147705078125, -1508.1492919921875, 393.776031494140625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+46, 209464, 870, 5785, 5855, '0', 0, 0, 2512.267333984375, -1463.07470703125, 388.157562255859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+47, 209353, 870, 5785, 5855, '0', 0, 0, 2467.444580078125, -1407.7882080078125, 388.827545166015625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rain Poppy (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+48, 209462, 870, 5785, 5871, '0', 0, 0, 2514.47314453125, -1492.8629150390625, 390.34375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+49, 209461, 870, 5785, 5871, '0', 0, 0, 2514.702392578125, -1492.689208984375, 390.154510498046875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+50, 212623, 870, 5785, 5871, '0', 0, 0, 2432.99072265625, -1611.21826171875, 411.79656982421875, 0.314158439636230468, 0, 0, 0.156434059143066406, 0.987688362598419189, 120, 255, 1, 60037), -- Stool (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+51, 212622, 870, 5785, 5871, '0', 0, 0, 2434.5966796875, -1612.35302734375, 411.796905517578125, 1.535889506340026855, 0, 0, 0.694658279418945312, 0.719339847564697265, 120, 255, 1, 60037), -- Stool (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+52, 212625, 870, 5785, 5871, '0', 0, 0, 2431.067138671875, -1605.880126953125, 411.78497314453125, 1.579522013664245605, 0, 0, 0.710185050964355468, 0.704015016555786132, 120, 255, 1, 60037), -- Bed (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+53, 212624, 870, 5785, 5871, '0', 0, 0, 2434.489013671875, -1609.14013671875, 411.796905517578125, 4.729844093322753906, 0, 0, -0.70090866088867187, 0.713251054286956787, 120, 255, 1, 60037), -- Stool (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+54, 212621, 870, 5785, 5871, '0', 0, 0, 2426.902099609375, -1624.67138671875, 411.79681396484375, 5.393068790435791015, 0, 0, -0.43051052093505859, 0.902585566043853759, 120, 255, 1, 60037), -- Stool (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+55, 214569, 870, 5785, 5871, '0', 0, 0, 2537.3896484375, -1514.654296875, 398.169952392578125, 5.209807395935058593, 0, 0, -0.51129341125488281, 0.859406232833862304, 120, 255, 1, 60037), -- Stool (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+56, 209462, 870, 5785, 5871, '0', 0, 0, 2534.107666015625, -1467.814208984375, 388.44061279296875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+57, 209461, 870, 5785, 5871, '0', 0, 0, 2534.318603515625, -1468.1805419921875, 388.539947509765625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+58, 214570, 870, 5785, 5871, '0', 0, 0, 2536.1044921875, -1517.3179931640625, 398.26153564453125, 6.175110340118408203, 0, 0, -0.05401134490966796, 0.998540341854095458, 120, 255, 1, 60037), -- Stool (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+59, 209460, 870, 5785, 5871, '0', 0, 0, 2533.5087890625, -1430.9427490234375, 388.746856689453125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+60, 209461, 870, 5785, 5871, '0', 0, 0, 2533.310791015625, -1430.90283203125, 388.649078369140625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+61, 209461, 870, 5785, 5871, '0', 0, 0, 2545.1728515625, -1402.2117919921875, 397.29632568359375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+62, 209460, 870, 5785, 5871, '0', 0, 0, 2548.2109375, -1402.2222900390625, 397.182281494140625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+63, 209461, 870, 5785, 5871, '0', 0, 0, 2533.279541015625, -1431.1163330078125, 388.59918212890625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+64, 216302, 870, 5785, 5871, '0', 0, 0, 2542.240478515625, -1513.91455078125, 398.233917236328125, 3.31314253807067871, 0, 0, -0.9963235855102539, 0.085669703781604766, 120, 255, 1, 60037), -- Stove (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+65, 209463, 870, 5785, 5871, '0', 0, 0, 2530.0478515625, -1411.4132080078125, 390.55572509765625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+66, 209462, 870, 5785, 5871, '0', 0, 0, 2496.866455078125, -1314.0382080078125, 378.554595947265625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: The Ring of Balance - Difficulty: 0) CreateObject1
(@OGUID+67, 209461, 870, 5785, 5855, '0', 0, 0, 2538.525146484375, -1306.3802490234375, 387.81951904296875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+68, 209463, 870, 5785, 5855, '0', 0, 0, 2542.908935546875, -1312.15283203125, 388.293487548828125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+69, 209349, 870, 5785, 5855, '0', 0, 0, 2389.157958984375, -1194.7274169921875, 388.04986572265625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Green Tea Leaf (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+70, 211494, 870, 5785, 5855, '0', 0, 0, 2562.442626953125, -1448.0572509765625, 406.114715576171875, 4.677483558654785156, 0, 0, -0.71933937072753906, 0.694658815860748291, 120, 255, 1, 60037), -- Mailbox (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+71, 209311, 870, 5785, 5855, '0', 0, 0, 2347.473876953125, -1183.4114990234375, 379.49896240234375, 3.888791084289550781, 0, 0, -0.93101978302001953, 0.364968687295913696, 120, 255, 1, 60037), -- Ghost Iron Deposit (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+72, 209460, 870, 5785, 5855, '0', 0, 0, 2578.77001953125, -1418.9254150390625, 398.27532958984375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+73, 209460, 870, 5785, 5855, '0', 0, 0, 2578.64501953125, -1418.546875, 398.24102783203125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+74, 209462, 870, 5785, 5855, '0', 0, 0, 2574.5634765625, -1435.0035400390625, 405.655059814453125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+75, 209460, 870, 5785, 5855, '0', 0, 0, 2574.84716796875, -1435, 405.7335205078125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+76, 212623, 870, 5785, 5855, '0', 0, 0, 2570.31494140625, -1461.288330078125, 407.124481201171875, 3.43830275535583496, 0, 0, -0.98901557922363281, 0.147811368107795715, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+77, 212622, 870, 5785, 5855, '0', 0, 0, 2568.729248046875, -1460.1256103515625, 407.12481689453125, 4.660029888153076171, 0, 0, -0.72537422180175781, 0.688354730606079101, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+78, 212624, 870, 5785, 5855, '0', 0, 0, 2568.78076171875, -1463.3399658203125, 407.12481689453125, 1.570795774459838867, 0, 0, 0.707106590270996093, 0.707106947898864746, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+79, 214567, 870, 5785, 5855, '0', 0, 0, 2566.234375, -1499.3248291015625, 398.39453125, 3.478586673736572265, 0, 0, -0.98583793640136718, 0.167700797319412231, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+80, 214566, 870, 5785, 5855, '0', 0, 0, 2564.5322265625, -1502.0634765625, 398.39453125, 2.440099716186523437, 0, 0, 0.939116477966308593, 0.343598932027816772, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+81, 209462, 870, 5785, 5855, '0', 0, 0, 2553.25341796875, -1516.576416015625, 398.273681640625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+82, 209462, 870, 5785, 5855, '0', 0, 0, 2553.0244140625, -1516.75, 398.260223388671875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rattan Switch (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+83, 212621, 870, 5785, 5855, '0', 0, 0, 2576.637451171875, -1447.9434814453125, 407.124725341796875, 2.234017848968505859, 0, 0, 0.898793220520019531, 0.438372820615768432, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+84, 212625, 870, 5785, 5855, '0', 0, 0, 2572.145263671875, -1466.6591796875, 407.112884521484375, 4.703663349151611328, 0, 0, -0.71018505096435546, 0.704015016555786132, 120, 255, 1, 60037), -- Bed (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+85, 212178, 870, 5785, 5855, '0', 0, 0, 2557.268310546875, -1541.9583740234375, 407.615875244140625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+86, 212178, 870, 5785, 5855, '0', 0, 0, 2556.2275390625, -1541.1944580078125, 407.615081787109375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+87, 212178, 870, 5785, 5855, '0', 0, 0, 2553.470458984375, -1560.873291015625, 407.615966796875, 6.20690774917602539, 0.12858438491821289, -0.11063098907470703, -0.02335548400878906, 0.985231637954711914, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+88, 212178, 870, 5785, 5855, '0', 0, 0, 2566.015625, -1547.982666015625, 407.6143798828125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+89, 212178, 870, 5785, 5855, '0', 0, 0, 2565.303955078125, -1550.232666015625, 407.6158447265625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+90, 212178, 870, 5785, 5855, '0', 0, 0, 2553.73876953125, -1560.8333740234375, 407.615966796875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+91, 212178, 870, 5785, 5855, '0', 0, 0, 2554.142333984375, -1561.4271240234375, 407.616607666015625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+92, 212178, 870, 5785, 5855, '0', 0, 0, 2544.473876953125, -1554.763916015625, 407.61956787109375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+93, 212178, 870, 5785, 5855, '0', 0, 0, 2555.5947265625, -1560.0660400390625, 407.616607666015625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+94, 212178, 870, 5785, 5855, '0', 0, 0, 2553.585205078125, -1560.642333984375, 407.75018310546875, 0.160758182406425476, -0.05081129074096679, -0.12443828582763671, 0.072244644165039062, 0.988288402557373046, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+95, 212178, 870, 5785, 5855, '0', 0, 0, 2553.80810546875, -1560.5035400390625, 407.61578369140625, 6.116181373596191406, -0.18993949890136718, -0.08211231231689453, -0.09973716735839843, 0.97325897216796875, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+96, 212178, 870, 5785, 5855, '0', 0, 0, 2543.510498046875, -1553.8646240234375, 407.618743896484375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+97, 212178, 870, 5785, 5855, '0', 0, 0, 2552.5634765625, -1562.970458984375, 407.617340087890625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+98, 212178, 870, 5785, 5855, '0', 0, 0, 2564.4765625, -1549.38720703125, 407.61505126953125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+99, 212178, 870, 5785, 5855, '0', 0, 0, 2553.362060546875, -1560.5347900390625, 407.61572265625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+100, 212178, 870, 5785, 5855, '0', 0, 0, 2546.029541015625, -1553.173583984375, 407.6181640625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+101, 212178, 870, 5785, 5855, '0', 0, 0, 2568.32373046875, -1547.263916015625, 407.615142822265625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Rice Bowl (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+102, 212615, 870, 5785, 5855, '0', 0, 0, 2580.83447265625, -1597.7110595703125, 417.41339111328125, 5.977754592895507812, 0, 0, -0.15212249755859375, 0.988361656665802001, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+103, 212614, 870, 5785, 5855, '0', 0, 0, 2585.633544921875, -1599.680419921875, 417.521240234375, 5.532694816589355468, 0, 0, -0.3665008544921875, 0.93041771650314331, 120, 255, 1, 60037), -- Kitchen Table (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+104, 212617, 870, 5785, 5855, '0', 0, 0, 2583.81591796875, -1594.8919677734375, 417.435516357421875, 4.97418975830078125, 0, 0, -0.60876083374023437, 0.793353796005249023, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+105, 212616, 870, 5785, 5855, '0', 0, 0, 2581.769287109375, -1596.085205078125, 417.435516357421875, 5.56760263442993164, 0, 0, -0.35020637512207031, 0.936672568321228027, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+106, 212619, 870, 5785, 5855, '0', 0, 0, 2575.78369140625, -1600.3853759765625, 417.41339111328125, 4.398232460021972656, 0, 0, -0.80901622772216796, 0.587786316871643066, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+107, 212618, 870, 5785, 5855, '0', 0, 0, 2578.41064453125, -1602.787353515625, 417.41339111328125, 3.359769821166992187, 0, 0, -0.99405574798583984, 0.108872212469577789, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+108, 212620, 870, 5785, 5855, '0', 0, 0, 2574.060546875, -1604.5460205078125, 416.667999267578125, 5.523968696594238281, 0, 0, -0.37055683135986328, 0.928809821605682373, 120, 255, 1, 60037), -- Fireplace (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+109, 212627, 870, 5785, 5855, '0', 0, 0, 2535.033447265625, -1610.296142578125, 412.195404052734375, 5.759587764739990234, 0, 0, -0.25881862640380859, 0.965925931930541992, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+110, 212626, 870, 5785, 5855, '0', 0, 0, 2541.602783203125, -1621.166015625, 412.19580078125, 0.986109614372253417, 0, 0, 0.473319053649902343, 0.880891084671020507, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+111, 212629, 870, 5785, 5855, '0', 0, 0, 2539.028076171875, -1609.43505859375, 412.196044921875, 4.764749526977539062, 0, 0, -0.6883544921875, 0.725374460220336914, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+112, 212628, 870, 5785, 5855, '0', 0, 0, 2537.60791015625, -1612.780517578125, 412.195404052734375, 1.413715124130249023, 0, 0, 0.649447441101074218, 0.760406434535980224, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+113, 212631, 870, 5785, 5855, '0', 0, 0, 2539.17236328125, -1626.8931884765625, 412.19622802734375, 3.141592741012573242, 0, 0, -1, 0, 120, 255, 1, 60037), -- Bench (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+114, 212630, 870, 5785, 5855, '0', 0, 0, 2530.29736328125, -1611.0479736328125, 412.196441650390625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Bench (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+115, 212632, 870, 5785, 5855, '0', 0, 0, 2541.45703125, -1617.3326416015625, 412.19580078125, 5.235990047454833984, 0, 0, -0.49999904632568359, 0.866025924682617187, 120, 255, 1, 60037), -- Stool (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+116, 212633, 870, 5785, 5855, '0', 0, 0, 2534.644775390625, -1631.524169921875, 412.186126708984375, 4.721118450164794921, 0, 0, -0.70401382446289062, 0.710186243057250976, 120, 255, 1, 60037), -- Bed (Area: Tian Monastery - Difficulty: 0) CreateObject1
(@OGUID+117, 209436, 870, 5785, 5863, '0', 0, 0, 2338.661376953125, -1717.4375, 326.197296142578125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: Banquet Grounds - Difficulty: 0) CreateObject1
(@OGUID+118, 209436, 870, 5785, 0, '0', 0, 0, 2334.138916015625, -1702.890625, 326.356658935546875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+119, 209436, 870, 5785, 0, '0', 0, 0, 2319.3369140625, -1717.876708984375, 322.2296142578125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+120, 209436, 870, 5785, 0, '0', 0, 0, 2326.09033203125, -1726.142333984375, 324.287567138671875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+121, 209436, 870, 5785, 0, '0', 0, 0, 2392.326416015625, -1758.157958984375, 374.435760498046875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+122, 209436, 870, 5785, 0, '0', 0, 0, 2403.94091796875, -1745.30908203125, 373.579864501953125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+123, 209436, 870, 5785, 0, '0', 0, 0, 2360.2431640625, -1751.5086669921875, 375.229156494140625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+124, 209436, 870, 5785, 0, '0', 0, 0, 2374.454833984375, -1746.5103759765625, 375.208343505859375, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+125, 209436, 870, 5785, 0, '0', 0, 0, 2390.46875, -1724.923583984375, 374.111114501953125, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+126, 209436, 870, 5785, 0, '0', 0, 0, 2349.882080078125, -1717.68408203125, 328.984771728515625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+127, 209436, 870, 5785, 0, '0', 0, 0, 2393.572998046875, -1735.3524169921875, 373.555572509765625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+128, 209436, 870, 5785, 0, '0', 0, 0, 2372, -1736.3663330078125, 374.64410400390625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+129, 209436, 870, 5785, 0, '0', 0, 0, 2382.579833984375, -1739.265625, 374.888885498046875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+130, 209436, 870, 5785, 0, '0', 0, 0, 2340.725830078125, -1693.8680419921875, 331.596160888671875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+131, 209436, 870, 5785, 0, '0', 0, 0, 2378.27783203125, -1759.5572509765625, 375.046875, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+132, 209436, 870, 5785, 0, '0', 0, 0, 2388.77783203125, -1765.5833740234375, 373.586822509765625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject1
(@OGUID+133, 209436, 870, 5785, 0, '0', 0, 0, 2392.267333984375, -1749.7882080078125, 374.383697509765625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+134, 209436, 870, 5785, 0, '0', 0, 0, 2317.7119140625, -1699.9583740234375, 322.621978759765625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject2
(@OGUID+135, 209436, 870, 5785, 0, '0', 0, 0, 2368.538330078125, -1758.0191650390625, 375.151031494140625, 0, 0, 0, 0, 1, 120, 255, 1, 60037), -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject1
(@OGUID+136, 209436, 870, 5785, 0, '0', 0, 0, 2381.892333984375, -1753.6475830078125, 374.98089599609375, 0, 0, 0, 0, 1, 120, 255, 1, 60037); -- Ripe Orange (Area: 0 - Difficulty: 0) CreateObject1

--------------------------------------
--         gameobject_addon         --
--------------------------------------
DELETE FROM `gameobject_addon` WHERE `guid` BETWEEN @OGUID+0 AND @OGUID+116;
INSERT INTO `gameobject_addon` (`guid`, `parent_rotation0`, `parent_rotation1`, `parent_rotation2`, `parent_rotation3`, `WorldEffectID`, `AIAnimKitID`) VALUES
(@OGUID+2, 0, 0, 0.996565520763397216, 0.082807965576648712, 0, 0), -- Fireplace
(@OGUID+3, 0, 0, 0.996565520763397216, 0.082807965576648712, 0, 0), -- Stool
(@OGUID+4, 0, 0, 0.996565520763397216, 0.082807965576648712, 0, 0), -- Stool
(@OGUID+50, 0, 0, 1, -0.00000004371138828, 0, 0), -- Stool
(@OGUID+51, 0, 0, 1, -0.00000004371138828, 0, 0), -- Stool
(@OGUID+52, 0, 0, 1, -0.00000004371138828, 0, 0), -- Bed
(@OGUID+53, 0, 0, 1, -0.00000004371138828, 0, 0), -- Stool
(@OGUID+54, 0, 0, 1, -0.00000004371138828, 0, 0), -- Stool
(@OGUID+55, 0, 0, -0.63607823848724365, 0.771624565124511718, 0, 0), -- Stool
(@OGUID+58, 0, 0, -0.63607823848724365, 0.771624565124511718, 0, 0), -- Stool
(@OGUID+64, 0, 0, -0.63607823848724365, 0.771624565124511718, 0, 0), -- Stove
(@OGUID+76, 0, 0, -0.00872663687914609, 0.999961912631988525, 0, 0), -- Stool
(@OGUID+77, 0, 0, -0.00872663687914609, 0.999961912631988525, 0, 0), -- Stool
(@OGUID+78, 0, 0, -0.00872663687914609, 0.999961912631988525, 0, 0), -- Stool
(@OGUID+79, 0, 0, -0.21643948554992675, 0.976296067237854003, 0, 0), -- Stool
(@OGUID+80, 0, 0, -0.21643948554992675, 0.976296067237854003, 0, 0), -- Stool
(@OGUID+83, 0, 0, -0.00872663687914609, 0.999961912631988525, 0, 0), -- Stool
(@OGUID+84, 0, 0, -0.00872663687914609, 0.999961912631988525, 0, 0), -- Bed
(@OGUID+102, 0, 0, -0.37055733799934387, 0.928809583187103271, 0, 0), -- Stool
(@OGUID+103, 0, 0, -0.37055733799934387, 0.928809583187103271, 0, 0), -- Kitchen Table
(@OGUID+104, 0, 0, -0.37055733799934387, 0.928809583187103271, 0, 0), -- Stool
(@OGUID+105, 0, 0, -0.37055733799934387, 0.928809583187103271, 0, 0), -- Stool
(@OGUID+106, 0, 0, -0.37055733799934387, 0.928809583187103271, 0, 0), -- Stool
(@OGUID+107, 0, 0, -0.37055733799934387, 0.928809583187103271, 0, 0), -- Stool
(@OGUID+108, 0, 0, -0.37055733799934387, 0.928809583187103271, 0, 0), -- Fireplace
(@OGUID+109, 0, 0, 0.000000087422776573, 1, 0, 0), -- Stool
(@OGUID+110, 0, 0, 0.000000087422776573, 1, 0, 0), -- Stool
(@OGUID+111, 0, 0, 0.000000087422776573, 1, 0, 0), -- Stool
(@OGUID+112, 0, 0, 0.000000087422776573, 1, 0, 0), -- Stool
(@OGUID+113, 0, 0, 0.000000087422776573, 1, 0, 0), -- Bench
(@OGUID+114, 0, 0, 0.000000087422776573, 1, 0, 0), -- Bench
(@OGUID+115, 0, 0, 0.000000087422776573, 1, 0, 0), -- Stool
(@OGUID+116, 0, 0, 0.000000087422776573, 1, 0, 0); -- Bed

--------------------------------------
--   creature_template_difficulty   --
--------------------------------------
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=50350 AND `DifficultyID`=0); -- 50350 (Morgrinn Crackfang) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54914 AND `DifficultyID`=0); -- 54914 (High Elder Cloudfall) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54913 AND `DifficultyID`=0); -- 54913 (Lin Tenderpaw) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54925 AND `DifficultyID`=0); -- 54925 (Husshun) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000000, `VerifiedBuild`=60037 WHERE (`Entry`=59782 AND `DifficultyID`=0); -- 59782 (Cypress Condor) - Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54919 AND `DifficultyID`=0); -- 54919 (Spirit-Sage Gaoquan) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54924 AND `DifficultyID`=0); -- 54924 (Zhi-Zhi) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x20000100, `VerifiedBuild`=60037 WHERE (`Entry`=65743 AND `DifficultyID`=0); -- 65743 (General Purpose Bunny (Gigantic AOI) (DLA)) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `ContentTuningID`=881, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=59736 AND `DifficultyID`=0); -- 59736 (Studious Chu) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54922 AND `DifficultyID`=0); -- 54922 (Master Stone Fist) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=55207 AND `DifficultyID`=0); -- 55207 (Tian Beer-Fetcher) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54944 AND `DifficultyID`=0); -- 54944 (Tian Pupil) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=55030 AND `DifficultyID`=0); -- 55030 (Tian Trainee) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=55029 AND `DifficultyID`=0); -- 55029 (Tian Trainee) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=55028 AND `DifficultyID`=0); -- 55028 (Tian Trainee) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x30000100, `StaticFlags3`=0x40000, `VerifiedBuild`=60037 WHERE (`Entry`=54926 AND `DifficultyID`=0); -- 54926 (Xiao) - Sessile, CanSwim, Floating - AllowInteractionWhileInCombat
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=62867 AND `DifficultyID`=0); -- 62867 (Bolo the Elder) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54915 AND `DifficultyID`=0); -- 54915 (Groundskeeper Wu) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=64761 AND `DifficultyID`=0); -- 64761 (Jumping Spider) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54987 AND `DifficultyID`=0); -- 54987 (Greenwood Trickster) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=55094 AND `DifficultyID`=0); -- 55094 (Guard Shan Long) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54918 AND `DifficultyID`=0); -- 54918 (Instructor Myang) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54980 AND `DifficultyID`=0); -- 54980 (Yao the Collector) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54981 AND `DifficultyID`=0); -- 54981 (June Whiteblossom) - CanSwim
UPDATE `creature_template_difficulty` SET `ContentTuningID`=57, `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=54982 AND `DifficultyID`=0); -- 54982 (Brewmaster Lei Kanglei) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x10000000, `VerifiedBuild`=60037 WHERE (`Entry`=59666 AND `DifficultyID`=0); -- 59666 (Emerald Turtle) - CanSwim
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=60037 WHERE (`Entry`=55183 AND `DifficultyID`=0); -- 55183 (Training Bag) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000100, `VerifiedBuild`=60037 WHERE (`Entry`=55184 AND `DifficultyID`=0); -- 55184 (Training Bag) - Sessile, Floating
UPDATE `creature_template_difficulty` SET `StaticFlags1`=0x20000000, `VerifiedBuild`=60037 WHERE (`Entry`=59782 AND `DifficultyID`=0); -- 59782 (Cypress Condor) - Floating

--------------------------------------
--     creature_template_gossip     --
--------------------------------------
DELETE FROM `creature_template_gossip` WHERE (`CreatureID`=54913 AND `MenuID`=13105) OR (`CreatureID`=54924 AND `MenuID`=13054) OR (`CreatureID`=54925 AND `MenuID`=13057) OR (`CreatureID`=54926 AND `MenuID`=13058) OR (`CreatureID`=62867 AND `MenuID`=9868) OR (`CreatureID`=54915 AND `MenuID`=13103) OR (`CreatureID`=54917 AND `MenuID`=13101);
INSERT INTO `creature_template_gossip` (`CreatureID`, `MenuID`, `VerifiedBuild`) VALUES
(54913, 13105, 60037), -- Lin Tenderpaw
(54924, 13054, 60037), -- Zhi-Zhi
(54925, 13057, 60037), -- Husshun
(54926, 13058, 60037), -- Xiao
(62867, 9868, 60037), -- Bolo the Elder
(54915, 13103, 60037), -- Groundskeeper Wu
(54917, 13101, 60037); -- Instructor Xann

--------------------------------------
--            npc_vendor            --
--------------------------------------
DELETE FROM `npc_vendor` WHERE (`entry`=62867 AND `item`=82451 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81413 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81412 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81411 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81410 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81409 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81408 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81405 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81404 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81403 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81402 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81401 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81400 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=74636 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81414 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=62867 AND `item`=81406 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=54982 AND `item`=40042 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=54982 AND `item`=40036 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=54982 AND `item`=40035 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=54981 AND `item`=3371 AND `ExtendedCost`=0 AND `type`=1) OR (`entry`=54981 AND `item`=22250 AND `ExtendedCost`=0 AND `type`=1);
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `ExtendedCost`, `type`, `PlayerConditionID`, `IgnoreFiltering`, `VerifiedBuild`) VALUES
(62867, 16, 82451, 0, 0, 1, 0, 0, 60037), -- Frybread
(62867, 15, 81413, 0, 0, 1, 0, 0, 60037), -- Skewered Peanut Chicken
(62867, 14, 81412, 0, 0, 1, 0, 0, 60037), -- Blanched Needle Mushrooms
(62867, 13, 81411, 0, 0, 1, 0, 0, 60037), -- Peach Pie
(62867, 12, 81410, 0, 0, 1, 0, 0, 60037), -- Green Curry Fish
(62867, 11, 81409, 0, 0, 1, 0, 0, 60037), -- Tangy Yogurt
(62867, 10, 81408, 0, 0, 1, 0, 0, 60037), -- Red Bean Bun
(62867, 9, 81405, 0, 0, 1, 0, 0, 60037), -- Boiled Silkworm Pupa
(62867, 8, 81404, 0, 0, 1, 0, 0, 60037), -- Dried Needle Mushrooms
(62867, 7, 81403, 0, 0, 1, 0, 0, 60037), -- Dried Peaches
(62867, 6, 81402, 0, 0, 1, 0, 0, 60037), -- Toasted Fish Jerky
(62867, 5, 81401, 0, 0, 1, 0, 0, 60037), -- Yak Cheese Curds
(62867, 4, 81400, 0, 0, 1, 0, 0, 60037), -- Pounded Rice Cake
(62867, 3, 74636, 0, 0, 1, 0, 0, 60037), -- Golden Carp Consomme
(62867, 2, 81414, 0, 0, 1, 0, 0, 60037), -- Pearl Milk Tea
(62867, 1, 81406, 0, 0, 1, 0, 0, 60037), -- Roasted Barley Tea
(54982, 4, 40042, 0, 0, 1, 0, 0, 60037), -- Caraway Burnwine
(54982, 3, 40036, 0, 0, 1, 0, 0, 60037), -- Snowplum Brandy
(54982, 2, 40035, 0, 0, 1, 0, 0, 60037), -- Northrend Honey Mead
(54981, 3, 3371, 0, 0, 1, 0, 0, 60037), -- Crystal Vial
(54981, 2, 22250, 0, 0, 1, 0, 0, 60037); -- Herb Pouch

--------------------------------------
--        gossip_menu_option        --
--------------------------------------
DELETE FROM `gossip_menu_option` WHERE GossipOptionID=-2526210;

--------------------------------------
--               misc               --
--------------------------------------
UPDATE `creature_equip_template` SET `VerifiedBuild`=60037 WHERE (`ID`=1 AND `CreatureID` IN (65092,50350,69768,54925,54924));
UPDATE `creature_model_info` SET `VerifiedBuild`=60037 WHERE `DisplayID` IN (42110, 34067, 43110, 43796, 36636, 41003, 39801, 44346, 39706, 42475, 47679, 41325, 23985, 39707, 45701, 40982, 41779, 39711, 39708, 39722, 41780, 42142, 45702, 39715, 42758, 39714, 43199, 39803, 39718, 42621, 43194, 39716, 43850, 39717, 27883, 40093, 45412, 2957, 2955, 45077, 45749, 39710, 11686, 39713, 39720, 41783, 45315, 38459, 21342, 39709, 41782, 39712);
UPDATE `gossip_menu` SET `VerifiedBuild`=60037 WHERE (`MenuID`=13106 AND `TextID`=18412) OR (`MenuID`=13105 AND `TextID`=18411) OR (`MenuID`=13054 AND `TextID`=18333) OR (`MenuID`=13057 AND `TextID`=18336) OR (`MenuID`=13058 AND `TextID`=18337) OR (`MenuID`=9868 AND `TextID`=9850) OR (`MenuID`=13103 AND `TextID`=18405) OR (`MenuID`=13101 AND `TextID`=18398);
UPDATE `creature_template_model` SET `VerifiedBuild`=60037 WHERE (`Idx`=0 AND `CreatureID` IN (59102,56464,56401,65092,56404,56563,54930,62867,54915,54987,55094,54918,54981,54980,54982)) OR (`Idx`=3 AND `CreatureID`=56464) OR (`Idx`=2 AND `CreatureID`=56464) OR (`Idx`=1 AND `CreatureID` IN (56464,56563));
UPDATE `creature_quest_currency` SET `VerifiedBuild`=60037 WHERE (`CurrencyId`=738 AND `CreatureId` IN (59102,56401,56404,54930,54987)) OR (`CurrencyId`=2778 AND `CreatureId` IN (59102,56401,56404,54930,54987));
UPDATE `gameobject_template` SET `VerifiedBuild`=60037 WHERE `entry`=213090;
UPDATE `creature_template_difficulty` SET `VerifiedBuild`=60037 WHERE (`DifficultyID`=0 AND `Entry` IN (56464,56401,56404,64776,64775,59772,59744,59745,41200,59743,32520));
UPDATE `creature_template` SET `VerifiedBuild`=60037 WHERE `entry` IN (54982, 55094, 56464, 62867, 54987, 54915, 65092, 54918, 59102, 56401, 56404, 54930, 56563, 54980, 54981);