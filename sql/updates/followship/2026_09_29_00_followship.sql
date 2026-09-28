-- bot_templates: repair rows where gender and chatter_type were swapped at insert time.
-- Field 5 held the intended gender (0=male, 1=female), field 6 the intended personality.
-- Males ended up with chatter_type=0 (no personality -> missing DB chatter keys) and
-- females with chatter_type=1 (Neutral) plus an invalid gender.

-- males (gender = 0)
UPDATE `bot_templates` SET `chatter_type` = 15, `gender` = 0 WHERE `entry` = 2119;   -- undead / warrior / Morbid
UPDATE `bot_templates` SET `chatter_type` = 8,  `gender` = 0 WHERE `entry` = 2122;   -- undead / rogue / Serious
UPDATE `bot_templates` SET `chatter_type` = 9,  `gender` = 0 WHERE `entry` = 2123;   -- undead / priest / Dry
UPDATE `bot_templates` SET `chatter_type` = 4,  `gender` = 0 WHERE `entry` = 2126;   -- undead / warlock / Cynical
UPDATE `bot_templates` SET `chatter_type` = 20, `gender` = 0 WHERE `entry` = 38911;  -- undead / hunter / Autistic
UPDATE `bot_templates` SET `chatter_type` = 7,  `gender` = 0 WHERE `entry` = 48615;  -- undead / rogue / Guarded
UPDATE `bot_templates` SET `chatter_type` = 19, `gender` = 0 WHERE `entry` = 49715;  -- undead / priest / Autotelic
UPDATE `bot_templates` SET `chatter_type` = 17, `gender` = 0 WHERE `entry` = 49716;  -- undead / mage / Sadist
UPDATE `bot_templates` SET `chatter_type` = 6,  `gender` = 0 WHERE `entry` = 49720;  -- undead / warrior / Cold
UPDATE `bot_templates` SET `chatter_type` = 8,  `gender` = 0 WHERE `entry` = 49781;  -- dwarf / warrior / Serious
UPDATE `bot_templates` SET `chatter_type` = 12, `gender` = 0 WHERE `entry` = 49786;  -- gnome / mage / Enthusiastic
UPDATE `bot_templates` SET `chatter_type` = 10, `gender` = 0 WHERE `entry` = 49791;  -- gnome / mage / Curious
UPDATE `bot_templates` SET `chatter_type` = 5,  `gender` = 0 WHERE `entry` = 49793;  -- dwarf / paladin / Bitter
UPDATE `bot_templates` SET `chatter_type` = 2,  `gender` = 0 WHERE `entry` = 49958;  -- undead / hunter / Positive

-- females (gender = 1)
UPDATE `bot_templates` SET `chatter_type` = 16, `gender` = 1 WHERE `entry` = 2121;   -- undead / mage / Narcissist
UPDATE `bot_templates` SET `chatter_type` = 5,  `gender` = 1 WHERE `entry` = 48612;  -- undead / warlock / Bitter
UPDATE `bot_templates` SET `chatter_type` = 18, `gender` = 1 WHERE `entry` = 48613;  -- undead / mage / Hyperthymic
UPDATE `bot_templates` SET `chatter_type` = 11, `gender` = 1 WHERE `entry` = 48614;  -- undead / priest / Warm
UPDATE `bot_templates` SET `chatter_type` = 3,  `gender` = 1 WHERE `entry` = 48616;  -- undead / warrior / Negative
UPDATE `bot_templates` SET `chatter_type` = 14, `gender` = 1 WHERE `entry` = 48618;  -- undead / hunter / Devoted
UPDATE `bot_templates` SET `chatter_type` = 13, `gender` = 1 WHERE `entry` = 49870;  -- undead / rogue / Cheerful
UPDATE `bot_templates` SET `chatter_type` = 15, `gender` = 1 WHERE `entry` = 49806;  -- dwarf / hunter / Morbid
UPDATE `bot_templates` SET `chatter_type` = 3,  `gender` = 1 WHERE `entry` = 49808;  -- dwarf / shaman / Negative
UPDATE `bot_templates` SET `chatter_type` = 20, `gender` = 1 WHERE `entry` = 49782;  -- dwarf / rogue / Autistic
UPDATE `bot_templates` SET `chatter_type` = 7,  `gender` = 1 WHERE `entry` = 49785;  -- dwarf / priest / Guarded
UPDATE `bot_templates` SET `chatter_type` = 12, `gender` = 1 WHERE `entry` = 63272;  -- pandaren / monk / Enthusiastic
