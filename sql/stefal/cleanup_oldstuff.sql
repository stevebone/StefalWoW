# Cleanu OLD crap
# Quest IDs are old Potentially IDs can also be reused later
DELETE FROM quest_template_addon WHERE id IN (3, 16, 3904, 3905, 7961, 7962);

# Incorrect Loot
UPDATE creature_template_difficulty SET LootID=0 where entry=721; -- rabbit with no loot.
UPDATE creature_template_difficulty SET LootID=0 where entry=1412; -- squirrel with no loot.