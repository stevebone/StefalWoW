-- Account bank tab definitions are shipped in BankTab.db2 (rows 9-13, BankType=2)
-- with correct costs (1k/25k/100k/500k/2.5M gold) and prompt SharedString IDs.
-- The earlier hotfix entries (IDs 100-104) inserted here were redundant and had
-- 1000x-too-low costs plus empty prompt strings; this DELETE cleans them up on
-- previously-applied deployments.
DELETE FROM `bank_tab` WHERE `ID` IN (100, 101, 102, 103, 104);
