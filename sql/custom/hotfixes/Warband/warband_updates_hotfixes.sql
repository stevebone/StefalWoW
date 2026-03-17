-- Account bank tab pricing (BankType 2 = Account)
-- IDs 100-104 chosen to avoid collision with character bank tab entries
DELETE FROM `bank_tab` WHERE `ID` IN (100, 101, 102, 103, 104);
INSERT INTO `bank_tab` (`ID`, `Cost`, `BankType`, `OrderIndex`, `PlayerConditionID`, `PurchasePromptTitle`, `PurchasePromptBody`, `PurchasePromptConfirmation`, `TabCleanupConfirmation`, `TabNameEditBoxHeader`, `VerifiedBuild`) VALUES
(100,     10000, 2, 0, 0, 0, 0, 0, 0, 0, -1),  -- Tab 1:   1,000g
(101,    250000, 2, 1, 0, 0, 0, 0, 0, 0, -1),  -- Tab 2:  25,000g
(102,   1000000, 2, 2, 0, 0, 0, 0, 0, 0, -1),  -- Tab 3: 100,000g
(103,   5000000, 2, 3, 0, 0, 0, 0, 0, 0, -1),  -- Tab 4: 500,000g
(104,  25000000, 2, 4, 0, 0, 0, 0, 0, 0, -1);  -- Tab 5: 2,500,000g
