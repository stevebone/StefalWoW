-- Order Advancement: REMOVE the GarrisonTalentNpc npcflag2 bit from the class-hall advisors (reverses 2026_08_03_10).
--
-- Setting UNIT_NPC_FLAG_2_GARRISON_TALENT_NPC (0x200 << 32 = 0x20000000000) made the client STOP sending gossip-hello
-- for the advisor entirely - it attempts a client-side talent interaction that is gated and silently no-ops, so
-- clicking the NPC did nothing (no server packet at all). The Order Advancement tree is instead opened via a synthesized
-- gossip option (Player::PrepareGossipMenu -> SMSG_GOSSIP_OPTION_NPC_INTERACTION), which needs the advisor to remain a
-- plain gossip NPC. Clear the talent bit; keep the gossip bit (0x1) so the NPC stays clickable.
UPDATE `creature_template` SET `npcflag` = `npcflag` & ~0x20000000000 WHERE `entry` IN (
    108050, 97989, 108331, 108018, 98939, 107994, 105998, 108527, 112199, 109901, 110725, 97485
);
