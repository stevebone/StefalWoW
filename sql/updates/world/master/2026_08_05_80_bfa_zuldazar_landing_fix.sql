-- BfA War Campaign: correct the Zuldazar sail landing (was dropping the player in the wrong zone)
--
-- My earlier landings came from eyeballing the old phantom-900037 placeholder coords, and ALL
-- THREE were in the wrong zone (verified: "Zuldazar" 2098,192 = zone 8500 Nazmir; "Nazmir"
-- 2738,4177 = zone 8501 Vol'dun; "Vol'dun" -2610,2270 = zone 8499 Zuldazar).
--
-- Authoritative Zuldazar landing: Brigadier Thom (136197) ENDS quest 51308 "Zuldazar Foothold"
-- at the Alliance beachhead in Zuldazar (zone 8499), where the "Recovering Shipwrecked Sailor"
-- NPCs spawn (~ -1752,-803,27). Land the player in that camp.
UPDATE `smart_scripts` SET `target_x`=-1720, `target_y`=-825, `target_z`=25, `target_o`=1.5
  WHERE `entryorguid`=135681 AND `source_type`=0 AND `id` IN (1,6);

-- Remove the Nazmir/Vol'dun sail teleports (ids 2,3): their placeholder-derived coords land in
-- the wrong zones. They will be re-added, anchored to each foothold's real quest hub and with
-- each foothold's own "Speak with Jes-Tereth" kill-credit, when that content is reached/verified.
DELETE FROM `smart_scripts` WHERE `entryorguid`=135681 AND `source_type`=0 AND `id` IN (2,3);
