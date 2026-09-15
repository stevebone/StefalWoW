-- BfA War Campaign: create the war-campaign garrison (GarrType 9 = WAR_CAMPAIGN) on completing the
-- "The War Campaign" intro quest, mirroring the WoD/Legion create-garrison-on-quest pattern.
--   Alliance: quest 52654 "The War Campaign" (giver Genn Greymane) -> RewardSpell 273382
--             (SPELL_EFFECT_CREATE_GARRISON, EffectMiscValue 168 -> GarrSiteLevel 599, map 1643).
--   Horde:    quest 52749 "The War Campaign" (giver Nathanos Blightcaller) -> RewardSpell 273381
--             (EffectMiscValue 169 -> Horde war-campaign site).
-- Chain: quest turn-in -> RewardSpell -> Spell::EffectCreateGarrison -> Player::CreateGarrison(siteId)
--        -> Garrison::Create -> GetGarrSiteLevelEntry(siteId,1). GetGarrisonTypeFromSiteId maps 168/169
--        -> GARRISON_TYPE_WAR_CAMPAIGN. Safe alongside the order hall now that G4 multi-garrison
--        persistence (character_garrison PK (guid,type)) is in place.
UPDATE `quest_template` SET `RewardSpell` = 273382 WHERE `ID` = 52654;
UPDATE `quest_template` SET `RewardSpell` = 273381 WHERE `ID` = 52749;
