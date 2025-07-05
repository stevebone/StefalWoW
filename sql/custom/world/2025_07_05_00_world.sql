-- Update Hyjal spell to no longer be autocasted and allowing characters to be in Hyjal
UPDATE `335_world`.`spell_area` SET `autocast` = '0' WHERE (`spell` = '42202') and (`area` = '616') and (`quest_start` = '0') and (`aura_spell` = '0') and (`racemask` = '0') and (`gender` = '2');
