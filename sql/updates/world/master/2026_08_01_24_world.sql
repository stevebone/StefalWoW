--
-- Titanstrike: make the Titan Chest (249718) clickable. It was imported as a type-5 GENERIC display prop, which the
-- client does not treat as interactable, so go_titanstrike's OnGossipHello never fired ("nothing to click"). Convert
-- it to a GOOBER (type 10, no lock) - the same clickable type as the working Garrison Cache - so clicking it fires the
-- script (claim Titanstrike -> objective 2 + Prustaga betrayal + Mimiron transfer).
--
UPDATE `gameobject_template` SET `type`=10, `Data0`=0 WHERE `entry`=249718;
