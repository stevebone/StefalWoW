-- Tal-Inara 159478, gossip menu 26284: stop offering every option to everybody.
--
-- All nine options of menu 26284 had no conditions at all, so a character standing in the Ring of Fates was
-- shown the whole Shadowlands campaign at once - quest answers for content they had not reached, plus the two
-- red skip-ahead options. Only the options whose owning quest can be established from the data are gated here;
-- see the bottom of this file for the ones deliberately left alone.
--
-- Mechanism: CONDITION_SOURCE_TYPE_GOSSIP_MENU_OPTION (15) with SourceGroup = MenuID and
-- SourceEntry = gossip_menu_option.OptionID. ConditionMgr::addToGossipMenuItems matches
-- GossipMenuItems::OrderIndex, which ObjectMgr::LoadGossipMenuItems reads from the OptionID column, so
-- SourceEntry is OptionID and NOT GossipOptionID. Player::PrepareGossipMenu filters on these before the menu is
-- sent, and WorldSession::HandleGossipSelectOptionOpcode only accepts options that are in the menu it sent, so
-- a hidden option cannot be selected either.
--
-- Rows in different ElseGroups are OR'd, rows in the same ElseGroup are AND'd.
--
-- CONDITION_QUEST_OBJECTIVE_PROGRESS (48) is used throughout rather than CONDITION_QUESTTAKEN (9): it is true
-- only while the quest is in the log AND the named objective is still at ConditionValue3 = 0, so each option
-- disappears the moment its credit lands instead of lingering until the quest is turned in.
--
-- Evidence per option (quest_objectives.ObjectID -> creature_template, all credit-only entries with zero
-- spawns, i.e. nothing but a gossip selection can ever award them):
--
--   Opt | Option text                                | Quest  | Objective | Credit NPC                              | Quest LogDescription
--   ----+--------------------------------------------+--------+-----------+-----------------------------------------+---------------------------------------------------------------
--    0  | Maldraxxus has attacked Bastion.           | 60056  |  409163   | 175991 Kill Credit: News Delivered       | "Inform Tal-Inara in Oribos of Maldraxxus's attack on Bastion."
--    3  | I'm prepared for you to examine my anima.  | 63660  |  419489   | 179107 Kill Credit (DNT)                 | "Allow Tal-Inara to examine your anima."
--    4  | I am ready to go.                          | 62159  |  407308   | 173614 Scene Kill Credit                 | "Choose where to go in the Shadowlands."
--       |                                            | 63208  |  409231   | 173614 Scene Kill Credit                 | "Choose where to go in the Shadowlands."
--       |                                            | 63209  |  409233   | 173614 Scene Kill Credit                 | "Choose where to go in the Shadowlands."
--       |                                            | 63210  |  409235   | 173614 Scene Kill Credit                 | "Choose where to go in the Shadowlands."
--    7  | Take me to the Crucible.                   | 65260  |  422749   | 184160 [DNT] Kill Credit: Speak to ...   | "...then speak to Tal-Inara to be brought to the Crucible."
--
-- Options 0, 3 and 7 are matched one-to-one: each is the only quest in the whole quest_objectives table whose
-- objective text names Tal-Inara together with that option's subject (Maldraxxus's attack / anima examined /
-- the Crucible). Option 4 has four owners because 62159, 63208, 63209 and 63210 all share the single objective
-- text "Speak with Tal-Inara to choose where to go" on the same credit 173614; only one of them can be in the
-- log at a time, and the four ElseGroups make the option appear for whichever it is.

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 15 AND `SourceGroup` = 26284 AND `SourceEntry` IN (0, 3, 4, 7);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
-- Option 0 - "Maldraxxus has attacked Bastion."
(15, 26284, 0, 0, 0, 48, 0, 409163, 0, 0, 0, 0, 0, '', 'Tal-Inara - "Maldraxxus has attacked Bastion." only while quest 60056 "Follow the Path" needs objective 409163'),
-- Option 3 - "I'm prepared for you to examine my anima."
(15, 26284, 3, 0, 0, 48, 0, 419489, 0, 0, 0, 0, 0, '', 'Tal-Inara - "examine my anima" only while quest 63660 "Opening the Maw" needs objective 419489'),
-- Option 4 - "I am ready to go." (npc_tal_inara awards the 173614 credit)
(15, 26284, 4, 0, 0, 48, 0, 407308, 0, 0, 0, 0, 0, '', 'Tal-Inara - "I am ready to go." while quest 62159 "Aiding the Shadowlands" needs objective 407308'),
(15, 26284, 4, 0, 1, 48, 0, 409231, 0, 0, 0, 0, 0, '', 'Tal-Inara - "I am ready to go." while quest 63208 "The Next Step" needs objective 409231'),
(15, 26284, 4, 0, 2, 48, 0, 409233, 0, 0, 0, 0, 0, '', 'Tal-Inara - "I am ready to go." while quest 63209 "Furthering the Purpose" needs objective 409233'),
(15, 26284, 4, 0, 3, 48, 0, 409235, 0, 0, 0, 0, 0, '', 'Tal-Inara - "I am ready to go." while quest 63210 "The Last Step" needs objective 409235'),
-- Option 7 - "Take me to the Crucible."
(15, 26284, 7, 0, 0, 48, 0, 422749, 0, 0, 0, 0, 0, '', 'Tal-Inara - "Take me to the Crucible." only while quest 65260 "A Long Walk" needs objective 422749');

-- Deliberately left unconditioned - the owning quest could not be established from the data, so gating them
-- would be a guess and could hide a working option:
--
--   Option 1  "Show me how I can help the Shadowlands."  - generic, and the only option on the menu with
--             GossipOptionID = 0; no quest objective text in the database matches it.
--   Option 2  "I am ready."                              - no subject at all; several Tal-Inara quests could
--             own it and nothing distinguishes them.
--   Option 6  "May I visit with the Arbiter?"            - two candidates and no way to choose between them:
--             quest 60149 "Audience with the Arbiter" (objective 397706 "Accompany Tal-Inara to visit the
--             Arbiter", credit 167486) and quest 64942 "Call of the Primus" (objective 422461 "Speak with
--             Tal-Inara to travel to the Arbiter's Chamber", on 159478 herself). It may also simply be a
--             standing travel option with no quest behind it. Resolving this needs a sniff of her gossip.
--   Option 8  "I have been to Zereth Mortis before..."   - skip-ahead; gated on account/campaign-wide
--   Option 10 "I've been here before..."                    progress, not on a single quest.
