--
-- gossip_menu_option - restore English base text where a non-English string leaked into the base row
-- ==================================================================================================
--
-- FINDING
-- -------
-- integ_world.gossip_menu_option holds 17,439 rows. 179 of them (123 distinct MenuIDs) carry a
-- non-English string in the BASE OptionText column instead of in gossip_menu_option_locale:
--
--     ptBR  87    esES/esMX  81    deDE  8    frFR  2    ruRU  1
--
-- 101 of those rows are additionally double-encoded (UTF-8 bytes stored as latin-1 and re-encoded),
-- so they render as mojibake ("Tengo que regresar a BastiA3n."). 62 of the 179 have VerifiedBuild 0,
-- i.e. they were hand-inserted rather than sniffed; the rest carry sniff builds 37474-59679.
--
-- WHAT IS FIXABLE FROM REAL DATA
-- ------------------------------
-- 42 of the 179 have a clean, pure-ASCII English row for the same (MenuID, OptionID) in the local
-- upstream TDB world databases `tc_world` / `world` / `playerbot_world` (all three agree). Those 42
-- are restored below, copied verbatim - nothing is translated or invented.
--
-- The remaining 137 rows have NO English source anywhere on this machine: the client does not ship
-- gossip option text (it is server-supplied), all seven local world schemas share the same polluted
-- lineage, and the available sniffs (C:\sniff: 8.0.1 / 22996 era) predate them. They are left alone
-- and reported rather than machine-translated.
--
-- ONE EXCEPTION, sourced separately (see the bottom of this file): menu 26693 option 0, the Oribos
-- Ring of Transference gossip, whose English wording is recoverable from a documented source.
--

UPDATE `gossip_menu_option` SET `OptionText` = 'I require training, Lumak.' WHERE `MenuID` = 4741 AND `OptionID` = 0 AND `OptionText` <> 'I require training, Lumak.';
UPDATE `gossip_menu_option` SET `OptionText` = 'Take me back to Darkspear Hold if you would, Vanira.' WHERE `MenuID` = 11107 AND `OptionID` = 0 AND `OptionText` <> 'Take me back to Darkspear Hold if you would, Vanira.';
UPDATE `gossip_menu_option` SET `OptionText` = 'We\'re ready to open the gates.' WHERE `MenuID` = 12797 AND `OptionID` = 0 AND `OptionText` <> 'We\'re ready to open the gates.';
UPDATE `gossip_menu_option` SET `OptionText` = 'Nalorakk is dead, you\'re free to go.' WHERE `MenuID` = 12800 AND `OptionID` = 0 AND `OptionText` <> 'Nalorakk is dead, you\'re free to go.';
UPDATE `gossip_menu_option` SET `OptionText` = 'The coast is clear. You\'re free!' WHERE `MenuID` = 12802 AND `OptionID` = 0 AND `OptionText` <> 'The coast is clear. You\'re free!';
UPDATE `gossip_menu_option` SET `OptionText` = 'We\'ve killed your captors. You\'re free to go.' WHERE `MenuID` = 12805 AND `OptionID` = 0 AND `OptionText` <> 'We\'ve killed your captors. You\'re free to go.';
UPDATE `gossip_menu_option` SET `OptionText` = 'They must\'ve drugged you. It\'s safe now.' WHERE `MenuID` = 12807 AND `OptionID` = 0 AND `OptionText` <> 'They must\'ve drugged you. It\'s safe now.';
UPDATE `gossip_menu_option` SET `OptionText` = 'I challenge you, Tor.' WHERE `MenuID` = 18848 AND `OptionID` = 0 AND `OptionText` <> 'I challenge you, Tor.';
UPDATE `gossip_menu_option` SET `OptionText` = 'I challenge you, Haldor.' WHERE `MenuID` = 18850 AND `OptionID` = 0 AND `OptionText` <> 'I challenge you, Haldor.';
UPDATE `gossip_menu_option` SET `OptionText` = 'I challenge you, Bjorn.' WHERE `MenuID` = 18851 AND `OptionID` = 0 AND `OptionText` <> 'I challenge you, Bjorn.';
UPDATE `gossip_menu_option` SET `OptionText` = 'We are ready to challenge you Odyn!' WHERE `MenuID` = 19198 AND `OptionID` = 0 AND `OptionText` <> 'We are ready to challenge you Odyn!';
UPDATE `gossip_menu_option` SET `OptionText` = 'I want to browse your goods.' WHERE `MenuID` = 20440 AND `OptionID` = 0 AND `OptionText` <> 'I want to browse your goods.';
UPDATE `gossip_menu_option` SET `OptionText` = 'Shall we continue?' WHERE `MenuID` = 20985 AND `OptionID` = 0 AND `OptionText` <> 'Shall we continue?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Where is Elder Primestone?' WHERE `MenuID` = 21004 AND `OptionID` = 1 AND `OptionText` <> 'Where is Elder Primestone?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Where is Elder Thunderhorn?' WHERE `MenuID` = 21004 AND `OptionID` = 2 AND `OptionText` <> 'Where is Elder Thunderhorn?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Where is Elder Bellowrage?' WHERE `MenuID` = 21058 AND `OptionID` = 1 AND `OptionText` <> 'Where is Elder Bellowrage?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Where is Elder Starglade?' WHERE `MenuID` = 21058 AND `OptionID` = 2 AND `OptionText` <> 'Where is Elder Starglade?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Where is Elder Skychaser?' WHERE `MenuID` = 21058 AND `OptionID` = 3 AND `OptionText` <> 'Where is Elder Skychaser?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Where is Elder Stormbrow?' WHERE `MenuID` = 21058 AND `OptionID` = 4 AND `OptionText` <> 'Where is Elder Stormbrow?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Where is Elder Winterhoof?' WHERE `MenuID` = 21058 AND `OptionID` = 5 AND `OptionText` <> 'Where is Elder Winterhoof?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Why is an ethereal traveling on a ship full of draenei?' WHERE `MenuID` = 21059 AND `OptionID` = 1 AND `OptionText` <> 'Why is an ethereal traveling on a ship full of draenei?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Do you know anyone by the name "Locus-Walker"?' WHERE `MenuID` = 21059 AND `OptionID` = 2 AND `OptionText` <> 'Do you know anyone by the name "Locus-Walker"?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Where is Elder Riversong?' WHERE `MenuID` = 21059 AND `OptionID` = 3 AND `OptionText` <> 'Where is Elder Riversong?';
UPDATE `gossip_menu_option` SET `OptionText` = 'Fire!' WHERE `MenuID` = 21208 AND `OptionID` = 2 AND `OptionText` <> 'Fire!';
UPDATE `gossip_menu_option` SET `OptionText` = 'Tell me about Grand Vizier Jarasum.' WHERE `MenuID` = 21238 AND `OptionID` = 0 AND `OptionText` <> 'Tell me about Grand Vizier Jarasum.';
UPDATE `gossip_menu_option` SET `OptionText` = 'Tell me about Arc-Consul Velara.' WHERE `MenuID` = 21238 AND `OptionID` = 1 AND `OptionText` <> 'Tell me about Arc-Consul Velara.';
UPDATE `gossip_menu_option` SET `OptionText` = 'Tell me about High Wakener Aargon.' WHERE `MenuID` = 21238 AND `OptionID` = 2 AND `OptionText` <> 'Tell me about High Wakener Aargon.';
UPDATE `gossip_menu_option` SET `OptionText` = 'Let\'s go!' WHERE `MenuID` = 21326 AND `OptionID` = 1 AND `OptionText` <> 'Let\'s go!';
UPDATE `gossip_menu_option` SET `OptionText` = 'What is that behind you?' WHERE `MenuID` = 21451 AND `OptionID` = 0 AND `OptionText` <> 'What is that behind you?';
UPDATE `gossip_menu_option` SET `OptionText` = 'What is that portal doing here?' WHERE `MenuID` = 21454 AND `OptionID` = 0 AND `OptionText` <> 'What is that portal doing here?';
UPDATE `gossip_menu_option` SET `OptionText` = 'What happened to you in the Seat of the Triumvirate?' WHERE `MenuID` = 21478 AND `OptionID` = 0 AND `OptionText` <> 'What happened to you in the Seat of the Triumvirate?';
UPDATE `gossip_menu_option` SET `OptionText` = 'What information have you been able to gather, Alleria?' WHERE `MenuID` = 21505 AND `OptionID` = 0 AND `OptionText` <> 'What information have you been able to gather, Alleria?';
UPDATE `gossip_menu_option` SET `OptionText` = 'What happened here, Prophet?' WHERE `MenuID` = 21709 AND `OptionID` = 0 AND `OptionText` <> 'What happened here, Prophet?';
UPDATE `gossip_menu_option` SET `OptionText` = 'I have something else to ask.' WHERE `MenuID` = 24644 AND `OptionID` = 0 AND `OptionText` <> 'I have something else to ask.';
UPDATE `gossip_menu_option` SET `OptionText` = 'Let me browse your goods.' WHERE `MenuID` = 24885 AND `OptionID` = 0 AND `OptionText` <> 'Let me browse your goods.';
UPDATE `gossip_menu_option` SET `OptionText` = 'Tell me about my specialization options.' WHERE `MenuID` = 25345 AND `OptionID` = 11 AND `OptionText` <> 'Tell me about my specialization options.';
UPDATE `gossip_menu_option` SET `OptionText` = 'Who is the Archon?' WHERE `MenuID` = 25475 AND `OptionID` = 0 AND `OptionText` <> 'Who is the Archon?';
UPDATE `gossip_menu_option` SET `OptionText` = 'I have something else to ask.' WHERE `MenuID` = 25475 AND `OptionID` = 1 AND `OptionText` <> 'I have something else to ask.';
UPDATE `gossip_menu_option` SET `OptionText` = 'I have something else to ask.' WHERE `MenuID` = 25476 AND `OptionID` = 0 AND `OptionText` <> 'I have something else to ask.';
UPDATE `gossip_menu_option` SET `OptionText` = 'I have something else to ask.' WHERE `MenuID` = 26605 AND `OptionID` = 0 AND `OptionText` <> 'I have something else to ask.';
UPDATE `gossip_menu_option` SET `OptionText` = 'I have something else to ask.' WHERE `MenuID` = 26606 AND `OptionID` = 0 AND `OptionText` <> 'I have something else to ask.';
UPDATE `gossip_menu_option` SET `OptionText` = 'I seem to have misplaced my Keystone.' WHERE `MenuID` = 26941 AND `OptionID` = 0 AND `OptionText` <> 'I seem to have misplaced my Keystone.';

-- --------------------------------------------------------------------------------------------------
-- Oribos - Pathscribe Roh-Avonavi (creature 162666), gossip_menu 26693, OptionID 0.
--
-- The base row currently reads "Tengo que regresar a Bastion." (esES, double-encoded). Its three
-- siblings in the same menu are English and follow one fixed pattern:
--     OptionID 1  "I need to get back to Maldraxxus."      (GossipOptionID 52882, VerifiedBuild 64743)
--     OptionID 4  "Show me all my travel options."         (GossipOptionID 52885, VerifiedBuild 64743)
-- The Spanish row is the odd one out: GossipOptionID 0, VerifiedBuild 0 (hand-inserted, never sniffed).
--
-- Source for the English wording: Warcraft Wiki, "Pathscribe Roh-Avonavi"
--   https://warcraft.wiki.gg/wiki/Pathscribe_Roh-Avonavi
-- which documents this NPC's gossip as "I need to get back to Bastion." / "...Maldraxxus." /
-- "...Ardenweald." / "...Revendreth." / "Show me all my travel options."
-- [WEB-SOURCED - labelled as such because it is not client data or a sniff.]
--
-- The wiki also documents Ardenweald and Revendreth entries for this menu (OptionID 2 and 3), which
-- are absent from every world DB on this machine. They are NOT added here - their GossipOptionID and
-- broadcast text are unknown and would have to be invented.
-- --------------------------------------------------------------------------------------------------
UPDATE `gossip_menu_option` SET `OptionText` = 'I need to get back to Bastion.'
 WHERE `MenuID` = 26693 AND `OptionID` = 0 AND `OptionText` <> 'I need to get back to Bastion.';
