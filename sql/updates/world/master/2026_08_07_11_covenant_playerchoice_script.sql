-- Covenant selection: bind PlayerChoice 644 ("Which covenant do you want to join?") to its script.
--
-- Reported from testing: the covenant panel opens in Oribos (see 2026_08_07_10_covenant_map_ui_link.sql)
-- but picking a covenant does nothing. WorldSession::HandlePlayerChoiceResponse only validates the
-- response and forwards it to ScriptMgr::OnPlayerChoiceResponse, which dispatches through
-- `GET_SCRIPT(PlayerChoiceScript, choice->ScriptId, ...)`. PlayerChoice 644 has no ScriptName, so
-- ScriptId is 0 and the response is dropped - nothing grants the covenant.
--
-- src/server/scripts/Shadowlands/playerchoice_covenant.cpp adds
-- `playerchoice_covenant_selection`, which maps the four "Join" responses of choice 644 to their
-- covenant reward spells (299204/299205/299206/299207, each carrying SPELL_EFFECT_SET_COVENANT) and
-- leaves the four "Preview Covenant" responses alone.
--
-- Idempotent.

UPDATE `playerchoice`
SET `ScriptName` = 'playerchoice_covenant_selection'
WHERE `ChoiceId` = 644;
