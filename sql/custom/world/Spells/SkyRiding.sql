-- ============================================================
-- Skyriding / Dragonriding SQL for TrinityCore Master (Retail)
-- Apply to 'world' database unless noted otherwise
-- ============================================================

-- Register spell scripts
DELETE FROM spell_script_names WHERE ScriptName IN (
    'spell_switch_flight',
    'spell_af_skyward_ascent',
    'spell_af_surge_forward',
    'spell_af_whirling_surge',
	'spell_dragonriding_launch_boost',
	'spell_dragonrider_energy',
	'spell_af_energy',
	'spell_dragonriding'
);

INSERT INTO spell_script_names (spell_id, ScriptName) VALUES
    (436854, 'spell_switch_flight'),
    (374763, 'spell_af_skyward_ascent'),
    (372610, 'spell_af_skyward_ascent'),
    (386451, 'spell_af_skyward_ascent'),
    (372608, 'spell_af_surge_forward'),
    (361584, 'spell_af_whirling_surge'),
	(372771, 'spell_dragonrider_energy'),
	(372773, 'spell_af_energy'),
	(406095, 'spell_dragonriding'),
	(392752, 'spell_dragonriding_launch_boost');