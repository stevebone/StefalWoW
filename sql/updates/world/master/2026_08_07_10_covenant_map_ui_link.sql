-- Oribos "Shadowlands Covenant Map" (GO 357095) opens the covenant-choice UI.
--
-- Reported from testing: clicking the map table in Oribos opened a plain gossip window instead of the
-- covenant selection UI. Two gameobjects sit on the exact same spot (-1937.35, 1385.97, Oribos / map 2222):
--   355835 "Shadowlands Map"          type 2  (QUESTGIVER)  -> opens gossip, which is what was being hit
--   357095 "Shadowlands Covenant Map" type 5  (GENERIC)     -> did nothing at all
--
-- GENERIC has no use-handler, so the covenant map was inert and the questgiver won every click.
-- The client opens the covenant-choice panel on SMSG_NPC_INTERACTION_OPEN_RESULT carrying
-- PlayerInteractionType::CovenantPreview (46). TrinityCore already emits exactly that for
-- GAMEOBJECT_TYPE_UI_LINK (48) whenever its PlayerInteractionType field is non-zero
-- (GameObject.cpp, `case GAMEOBJECT_TYPE_UI_LINK`), so this needs no code - only the right template.
--
-- UI_LINK field layout (GameObjectData.h): Data0 UILinkType, Data1 allowMounted, Data2 GiganticAOI,
-- Data3 spellFocusType, Data4 radius, Data5 InteractRadiusOverride, Data6 ItemInteractionID,
-- Data7 PlayerInteractionType, Data8 spell.
--
-- Data3 was 1 under the GENERIC layout (a different field there) and is cleared so it is not read as
-- spellFocusType. Idempotent.

-- Data8 (spell) is the part that actually opens the panel: 343884 carries
-- SPELL_EFFECT_LAUNCH_QUEST_CHOICE (205) with MiscValue 644 = "Which covenant do you want to join?".
-- The UI_LINK handler casts Data8 on use, which is what reaches Player::SendPlayerChoice. Setting only
-- Data7 makes the object send an interaction-open the client ignores, i.e. a clickable table that does nothing.
UPDATE `gameobject_template`
SET `type` = 48,
    `Data3` = 0,
    `Data7` = 46,
    `Data8` = 343884
WHERE `entry` = 357095;
