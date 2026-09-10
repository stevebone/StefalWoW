-- Gate the four Shadowlands covenant campaign entry quests to their own covenant.
--
-- Tal-Inara (159478) is creature_queststarter for EIGHT quests -- four covenant campaigns, each present as a
-- pre-sanctum / post-sanctum pair -- and none of them carried a CONDITION_SOURCE_TYPE_QUEST_AVAILABLE row. A
-- freshly-pledged player therefore saw all eight quest marks at once, including the three covenants they cannot
-- join. Retail offers only your own covenant's.
--
-- The covenant of each quest is derived from quest_template.QuestSortID (its covenant zone), corroborated by
-- LogDescription; nothing here is inferred from lore:
--   sort 10534 Bastion     -> covenant 1 Kyrian     : 63034 "Elysian Hold"          / 62707 "Hero's Rest"
--   sort 10413 Revendreth  -> covenant 2 Venthyr    : 63037 "Sinfall"               / 62740 "crypt beneath Darkhaven"
--   sort 11510 Ardenweald  -> covenant 3 Night Fae  : 63036 "Heart of the Forest"   / 62739 "Heart of the Forest"
--   sort 11462 Maldraxxus  -> covenant 4 Necrolord  : 63035 "Seat of the Primus"    / 62738 "Bleak Redoubt"
--
-- Uses CONDITION_COVENANT (62), added on this branch. Both members of each pair are gated: which of the two retail
-- offers at which point is NOT derivable from our data, so neither is hidden -- this only removes the three
-- foreign covenants (8 marks -> 2). Narrowing the pair needs a sniff.
--
-- Idempotent.

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 19
  AND `SourceEntry` IN (62707, 63034, 62738, 63035, 62739, 63036, 62740, 63037);

INSERT INTO `conditions`
 (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`ConditionStringValue1`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
 (19,0,63034,0,0,62,0,1,0,0,'',0,0,0,'','The Elysian Fields (Elysian Hold) - Kyrian only'),
 (19,0,62707,0,0,62,0,1,0,0,'',0,0,0,'','The Elysian Fields (Hero''s Rest) - Kyrian only'),
 (19,0,63037,0,0,62,0,2,0,0,'',0,0,0,'','Dark Aspirations (Sinfall) - Venthyr only'),
 (19,0,62740,0,0,62,0,2,0,0,'',0,0,0,'','Dark Aspirations (Darkhaven crypt) - Venthyr only'),
 (19,0,63036,0,0,62,0,3,0,0,'',0,0,0,'','Restoring Balance (Heart of the Forest) - Night Fae only'),
 (19,0,62739,0,0,62,0,3,0,0,'',0,0,0,'','Restoring Balance (Heart of the Forest) - Night Fae only'),
 (19,0,63035,0,0,62,0,4,0,0,'',0,0,0,'','A Fresh Blade (Seat of the Primus) - Necrolord only'),
 (19,0,62738,0,0,62,0,4,0,0,'',0,0,0,'','A Fresh Blade (Bleak Redoubt) - Necrolord only');
