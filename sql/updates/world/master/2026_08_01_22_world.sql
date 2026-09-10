--
-- Titanstrike scenario (map 1495) - correction: Yrgrim the Truthseeker (105695) is the trial-JUDGE, not an enemy.
-- His subname is "Champion of Tyr" - a titan guardian who runs the tomb's Trial of the Truthseeker (you prove
-- yourself against the tomb's dead guardians, then he judges you). P1 wrongly set him hostile (faction 16) along
-- with the guardians; revert him to friendly (35). The dead defenders (spectral, rattling dead, disturbed) and the
-- vrykul tomb-GUARDS stay hostile - they are the enemies you fight, not your allies (Prustaga is the vrykul ally).
--
UPDATE `creature_template` SET `faction`=35 WHERE `entry`=105695;
