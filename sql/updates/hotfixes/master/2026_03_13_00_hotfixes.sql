--
-- Rename misnamed columns in battle_pet_ability_effect to match WoWDBDefs 12.0
-- Old names were swapped: Aura was actually BattlePetEffectPropertiesID,
-- BattlePetEffectPropertiesID was actually AuraBattlePetAbilityID,
-- VisualID was actually BattlePetVisualID
--

-- Two-step rename to avoid name collision (Aura->BattlePetEffectPropertiesID conflicts with existing column)
-- Step 1: Rename the conflicting column out of the way first
ALTER TABLE `battle_pet_ability_effect`
  CHANGE COLUMN `BattlePetEffectPropertiesID` `AuraBattlePetAbilityID` smallint unsigned NOT NULL DEFAULT '0',
  CHANGE COLUMN `Aura` `BattlePetEffectPropertiesID` smallint unsigned NOT NULL DEFAULT '0',
  CHANGE COLUMN `VisualID` `BattlePetVisualID` smallint unsigned NOT NULL DEFAULT '0';
