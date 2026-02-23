ALTER TABLE `character_garrison_followers`
  ADD COLUMN `customName` varchar(128) NOT NULL DEFAULT '' AFTER `status`;

ALTER TABLE `character_garrison`
  ADD COLUMN `type` int unsigned NOT NULL DEFAULT '2' AFTER `followerActivationsRemainingToday`;
