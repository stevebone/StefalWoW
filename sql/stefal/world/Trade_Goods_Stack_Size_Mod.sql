/* Data from Wowhead http://www.wowhead.com/items?filter=82:194;4:3;30305:200 */

UPDATE `item_template`
SET `stackable` = 200
WHERE `entry`
IN (
    723, /* Goretusk Liver */
    765, /* Silverleaf */
    769, /* Chunk of Boar Meat */
    774, /* Malachite */
    783, /* Light Hide */
    785, /* Mageroyal */
    814, /* Flask of Oil */
    818, /* Tigerseye */
    1015, /* Lean Wolf Flank */
    1080, /* Tough Condor Meat */
    1206, /* Moss Agate */
    1210, /* Shadowgem */
    1288, /* Large Venom Sac */
    1468, /* Murloc Fin */
    1475, /* Small Venom Sac */
    1529, /* Jade */
    1705, /* Lesser Moonstone */
    2251, /* Gooey Spider Leg */
    2318, /* Light Leather */
    2319, /* Medium Leather */
    2320, /* Coarse Thread */
    2321, /* Fine Thread */
    2324, /* Bleach */
    2325, /* Black Dye */
    2447, /* Peacebloom */
    2449, /* Earthroot */
    2450, /* Briarthorn */
    2452, /* Swiftthistle */
    2453, /* Bruiseweed */
    2589, /* Linen Cloth */
    2592, /* Wool Cloth */
    2604, /* Red Dye */
    2605, /* Green Dye */
    2672, /* Stringy Wolf Meat */
    2673, /* Coyote Meat */
    2674, /* Crawler Meat */
    2675, /* Crawler Claw */
    2677, /* Boar Ribs */
    2678, /* Mild Spices */
    2770, /* Copper Ore */
    2771, /* Tin Ore */
    2772, /* Iron Ore */
    2775, /* Silver Ore */
    2776, /* Gold Ore */
    2835, /* Rough Stone */
    2836, /* Coarse Stone */
    2838, /* Heavy Stone */
    2840, /* Copper Bar */
    2841, /* Bronze Bar */
    2842, /* Silver Bar */
    2880, /* Weak Flux */
    2886, /* Crag Boar Rib */
    2924, /* Crocolisk Meat */
    2934, /* Ruined Leather Scraps */
    2946, /* Broken Balanced Throwing Dagger */
    2947, /* Broken Small Throwing Knife */
    2996, /* Bolt of Linen Cloth */
    2997, /* Bolt of Woolen Cloth */
    3107, /* Broken Keen Throwing Knife */
    3108, /* Broken Heavy Throwing Dagger */
    3111, /* Broken Crude Throwing Axe */
    3131, /* Broken Weighted Throwing Axe */
    3135, /* Broken Sharp Throwing Axe */
    3137, /* Broken Deadly Throwing Axe */
    3164, /* Discolored Worg Heart */
    3372, /* Leaded Vial */
    3173, /* Bear Meat */
    3182, /* Spider's Silk */
    3355, /* Wild Steelbloom */
    3356, /* Kingsblood */
    3357, /* Liferoot */
    3358, /* Khadgar's Whisker */
    3369, /* Grave Moss */
    3371, /* Crystal Vial */
    3404, /* Buzzard Wing */
    3463, /* Broken Silver Star */
    3466, /* Strong Flux */
    3470, /* Rough Grinding Stone */
    3478, /* Coarse Grinding Stone */
    3486, /* Heavy Grinding Stone */
    3575, /* Iron Bar */
    3576, /* Tin Bar */
    3577, /* Gold Bar */
    3667, /* Tender Crocolisk Meat */
    3685, /* Raptor Egg */
    3712, /* Turtle Meat */
    3730, /* Big Bear Meat */
    3731, /* Lion Meat */
    3818, /* Fadeleaf */
    3819, /* Dragon's Teeth */
    3820, /* Stranglekelp */
    3821, /* Goldthorn */
    3857, /* Coal */
    3858, /* Mithril Ore */
    3859, /* Steel Bar */
    3860, /* Mithril Bar */
    3864, /* Citrine */
    4231, /* Cured Light Hide */
    4232, /* Medium Hide */
    4233, /* Cured Medium Hide */
    4234, /* Heavy Leather */
    4235, /* Heavy Hide */
    4236, /* Cured Heavy Hide */
    4289, /* Salt */
    4291, /* Silken Thread */
    4304, /* Thick Leather */
    4305, /* Bolt of Silk Cloth */
    4306, /* Silk Cloth */
    4337, /* Thick Spider's Silk */
    4338, /* Mageweave Cloth */
    4339, /* Bolt of Mageweave */
    4340, /* Gray Dye */
    4341, /* Yellow Dye */
    4342, /* Purple Dye */
    4357, /* Rough Blasting Powder */
    4359, /* Handful of Copper Bolts */
    4364, /* Coarse Blasting Powder */
    4371, /* Bronze Tube */
    4375, /* Whirring Bronze Gizmo */
    4377, /* Heavy Blasting Powder */
    4382, /* Bronze Framework */
    4387, /* Iron Strut */
    4389, /* Gyrochronatom */
    4399, /* Wooden Stock */
    4400, /* Heavy Stock */
    4402, /* Small Flame Sac */
    4404, /* Silver Contact */
    4461, /* Raptor Hide */
    4470, /* Simple Wood */
    4603, /* Raw Spotted Yellowtail */
    4611, /* Blue Pearl */
    4625, /* Firebloom */
    4655, /* Giant Clam Meat */
    5051, /* Dig Rat */
    5082, /* Thin Kodo Leather */
    5373, /* Lucky Charm */
    5379, /* Broken Boot Knife */
    5465, /* Small Spider Leg */
    5466, /* Scorpid Stinger */
    5467, /* Kodo Meat */
    5468, /* Soft Frenzy Flesh */
    5469, /* Strider Meat */
    5470, /* Thunder Lizard Tail */
    5471, /* Stag Meat */
    5498, /* Small Lustrous Pearl */
    5500, /* Iridescent Pearl */
    5503, /* Clam Meat */
    5504, /* Tangy Clam Meat */
    5635, /* Sharp Claw */
    5637, /* Large Fang */
    5784, /* Slimy Murloc Scale */
    5785, /* Thick Murloc Scale */
    6037, /* Truesilver Bar */
    6260, /* Blue Dye */
    6261, /* Orange Dye */
    6289, /* Raw Longjaw Mud Snapper */
    6291, /* Raw Brilliant Smallfish */
    6303, /* Raw Slitherskin Mackerel */
    6308, /* Raw Bristle Whisker Catfish */
    6317, /* Raw Loch Frenzy */
    6358, /* Oily Blackmouth */
    6359, /* Firefin Snapper */
    6361, /* Raw Rainbow Fin Albacore */
    6362, /* Raw Rockscale Cod */
    6370, /* Blackmouth Oil */
    6371, /* Fire Oil */
    6470, /* Deviate Scale */
    6471, /* Perfect Deviate Scale */
    6889, /* Small Egg */
    7067, /* Elemental Earth */
    7068, /* Elemental Fire */
    7069, /* Elemental Air */
    7070, /* Elemental Water */
    7071, /* Iron Buckle */
    7072, /* Naga Scale */
    7075, /* Core of Earth */
    7076, /* Essence of Earth */
    7077, /* Heart of Fire */
    7078, /* Essence of Fire */
    7079, /* Globe of Water */
    7080, /* Essence of Water */
    7081, /* Breath of Wind */
    7082, /* Essence of Air */
    7191, /* Fused Wiring */
    7286, /* Black Whelp Scale */
    7392, /* Green Whelp Scale */
    7909, /* Aquamarine */
    7910, /* Star Ruby */
    7911, /* Truesilver Ore */
    7912, /* Solid Stone */
    7966, /* Solid Grinding Stone */
    7971, /* Black Pearl */
    7972, /* Ichor of Undeath */
    7973, /* Big-mouth Clam */
    7974, /* Zesty Clam Meat */
    8150, /* Deeprock Salt */
    8153, /* Wildvine */
    8154, /* Scorpid Scale */
    8165, /* Worn Dragonscale */
    8167, /* Turtle Scale */
    8169, /* Thick Hide */
    8170, /* Rugged Leather */
    8171, /* Rugged Hide */
    8172, /* Cured Thick Hide */
    8343, /* Heavy Silken Thread */
    8365, /* Raw Mithril Head Trout */
    8831, /* Purple Lotus */
    8836, /* Arthas' Tears */
    8838, /* Sungrass */
    8839, /* Blindweed */
    8845, /* Ghost Mushroom */
    8846, /* Gromsblood */
    9060, /* Inlaid Mithril Cylinder */
    9061, /* Goblin Rocket Fuel */
    9210, /* Ghost Dye */
    9262, /* Black Vitriol */
    10285, /* Shadow Silk */
    10286, /* Heart of the Wild */
    10290, /* Pink Dye */
    10505, /* Solid Blasting Powder */
    10558, /* Gold Power Core */
    10559, /* Mithril Tube */
    10560, /* Unstable Trigger */
    10561, /* Mithril Casing */
    10620, /* Thorium Ore */
    10647, /* Engineer's Ink */
    10938, /* Lesser Magic Essence */
    10939, /* Greater Magic Essence */
    10940, /* Strange Dust */
    10978, /* Small Glimmering Shard */
    10998, /* Lesser Astral Essence */
    11082, /* Greater Astral Essence */
    11083, /* Soul Dust */
    11084, /* Large Glimmering Shard */
    11134, /* Lesser Mystic Essence */
    11135, /* Greater Mystic Essence */
    11137, /* Vision Dust */
    11138, /* Small Glowing Shard */
    11139, /* Large Glowing Shard */
    11174, /* Lesser Nether Essence */
    11175, /* Greater Nether Essence */
    11176, /* Dream Dust */
    11177, /* Small Radiant Shard */
    11178, /* Large Radiant Shard */
    11291, /* Star Wood */
    11370, /* Dark Iron Ore */
    11371, /* Dark Iron Bar */
    11382, /* Blood of the Mountain */
    11754, /* Black Diamond */
    12037, /* Mystery Meat */
    12184, /* Raptor Flesh */
    12202, /* Tiger Meat */
    12203, /* Red Wolf Meat */
    12204, /* Heavy Kodo Meat */
    12205, /* White Spider Meat */
    12206, /* Tender Crab Meat */
    12207, /* Giant Egg */
    12208, /* Tender Wolf Meat */
    12223, /* Meaty Bat Wing */
    12359, /* Thorium Bar */
    12360, /* Arcanite Bar */
    12361, /* Blue Sapphire */
    12363, /* Arcane Crystal */
    12364, /* Huge Emerald */
    12365, /* Dense Stone */
    12607, /* Brilliant Chromatic Scale */
    12644, /* Dense Grinding Stone */
    12655, /* Enchanted Thorium Bar */
    12662, /* Demonic Rune */
    12799, /* Large Opal */
    12800, /* Azerothian Diamond */
    12803, /* Living Essence */
    12804, /* Powerful Mojo */
    12808, /* Essence of Undeath */
    12809, /* Guardian Stone */
    12810, /* Enchanted Leather */
    12811, /* Righteous Orb */
    12938, /* Blood of Heroes */
    13173, /* Broken Flightblade Throwing Axe */
    13354, /* Ectoplasmic Resonator */
    13356, /* Somatic Intensifier */
    13357, /* Osseous Agitator */
    13422, /* Stonescale Eel */
    13423, /* Stonescale Oil */
    13463, /* Dreamfoil */
    13464, /* Golden Sansam */
    13465, /* Mountain Silversage */
    13466, /* Sorrowmoss */
    13467, /* Icecap */
    13468, /* Black Lotus */
    13754, /* Raw Glossy Mightfish */
    13756, /* Raw Summer Bass */
    13757, /* Lightning Eel */
    13758, /* Raw Redgill */
    13759, /* Raw Nightfin Snapper */
    13760, /* Raw Sunscale Salmon */
    13888, /* Darkclaw Lobster */
    13889, /* Raw Whitescale Salmon */
    13926, /* Golden Pearl */
    14047, /* Runecloth */
    14048, /* Bolt of Runecloth */
    14227, /* Ironweb Spider Silk */
    14256, /* Felcloth */
    14341, /* Rune Thread */
    14342, /* Mooncloth */
    14343, /* Small Brilliant Shard */
    14344, /* Large Brilliant Shard */
    15326, /* Broken Gleaming Throwing Axe */
    15327, /* Broken Wicked Throwing Dagger */
    15407, /* Cured Rugged Hide */
    15408, /* Heavy Scorpid Scale */
    15410, /* Scale of Onyxia */
    15412, /* Green Dragonscale */
    15414, /* Red Dragonscale */
    15415, /* Blue Dragonscale */
    15416, /* Black Dragonscale */
    15417, /* Devilsaur Leather */
    15419, /* Warbear Leather */
    15992, /* Dense Blasting Powder */
    15994, /* Thorium Widget */
    16000, /* Thorium Tube */
    16006, /* Delicate Arcanite Converter */
    16202, /* Lesser Eternal Essence */
    16203, /* Greater Eternal Essence */
    16204, /* Light Illusion Dust */
    17010, /* Fiery Core */
    17011, /* Lava Core */
    17012, /* Core Leather */
    17056, /* Light Feather */
    17194, /* Holiday Spices */
    17203, /* Sulfuron Ingot */
    17771, /* Elementium Bar */
    18240, /* Ogre Tannin */
    18335, /* Pristine Black Diamond */
    18562, /* Elementium Ingot */
    18567, /* Elemental Flux */
    18631, /* Truesilver Transformer */
    19182, /* Tattered Darkmoon Faire Prize Ticket */
    19183, /* Hourglass Sand */
    19441, /* Huge Venom Sac */
    19767, /* Primal Bat Leather */
    19768, /* Primal Tiger Leather */
    19943, /* Massive Mojo */
    20086, /* Broken Dusksteel Throwing Knife */
    20381, /* Dreamscale */
    20384, /* Silithid Carapace Fragment */
    20424, /* Sandworm Meat */
    20520, /* Dark Rune */
    20725, /* Nexus Crystal */
    20816, /* Delicate Copper Wire */
    20817, /* Bronze Setting */
    20963, /* Mithril Filigree */
    21024, /* Chimaerok Tenderloin */
    21071, /* Raw Sagefish */
    21153, /* Raw Greater Sagefish */
    21752, /* Thorium Setting */
    21816, /* Heart Candy */
    21817, /* Heart Candy */
    21818, /* Heart Candy */
    21819, /* Heart Candy */
    21820, /* Heart Candy */
    21821, /* Heart Candy */
    21822, /* Heart Candy */
    21823, /* Heart Candy */
    21840, /* Bolt of Netherweave */
    21842, /* Bolt of Imbued Netherweave */
    21844, /* Bolt of Soulcloth */
    21845, /* Primal Mooncloth */
    21877, /* Netherweave Cloth */
    21881, /* Netherweb Spider Silk */
    21882, /* Soul Essence */
    21884, /* Primal Fire */
    21885, /* Primal Water */
    21886, /* Primal Life */
    21887, /* Knothide Leather */
    21929, /* Flame Spessarite */
    22202, /* Small Obsidian Shard */
    22203, /* Large Obsidian Shard */
    22373, /* Wartorn Leather Scrap */
    22374, /* Wartorn Chain Scrap */
    22375, /* Wartorn Plate Scrap */
    22376, /* Wartorn Cloth Scrap */
    22445, /* Arcane Dust */
    22446, /* Greater Planar Essence */
    22447, /* Lesser Planar Essence */
    22448, /* Small Prismatic Shard */
    22449, /* Large Prismatic Shard */
    22450, /* Void Crystal */
    22451, /* Primal Air */
    22452, /* Primal Earth */
    22456, /* Primal Shadow */
    22457, /* Primal Mana */
    22572, /* Mote of Air */
    22573, /* Mote of Earth */
    22574, /* Mote of Fire */
    22575, /* Mote of Life */
    22576, /* Mote of Mana */
    22577, /* Mote of Shadow */
    22578, /* Mote of Water */
    22644, /* Crunchy Spider Leg */
    22682, /* Frozen Rune */
    22785, /* Felweed */
    22786, /* Dreaming Glory */
    22787, /* Ragveil */
    22789, /* Terocone */
    22790, /* Ancient Lichen */
    22791, /* Netherbloom */
    22792, /* Nightmare Vine */
    22793, /* Mana Thistle */
    22794, /* Fel Lotus */
    23077, /* Blood Garnet */
    23079, /* Deep Peridot */
    23107, /* Shadow Draenite */
    23112, /* Golden Draenite */
    23117, /* Azure Moonstone */
    23424, /* Fel Iron Ore */
    23425, /* Adamantite Ore */
    23426, /* Khorium Ore */
    23427, /* Eternium Ore */
    23436, /* Living Ruby */
    23437, /* Talasite */
    23438, /* Star of Elune */
    23439, /* Noble Topaz */
    23440, /* Dawnstone */
    23441, /* Nightseye */
    23445, /* Fel Iron Bar */
    23446, /* Adamantite Bar */
    23447, /* Eternium Bar */
    23448, /* Felsteel Bar */
    23449, /* Khorium Bar */
    23571, /* Primal Might */
    23572, /* Primal Nether */
    23573, /* Hardened Adamantite Bar */
    23676, /* Moongraze Stag Tenderloin */
    23781, /* Elemental Blasting Powder */
    23782, /* Fel Iron Casing */
    23783, /* Handful of Fel Iron Bolts */
    23784, /* Adamantite Frame */
    23785, /* Hardened Adamantite Tube */
    23786, /* Khorium Power Core */
    23787, /* Felsteel Stabilizer */
    23793, /* Heavy Knothide Leather */
    24243, /* Adamantite Powder */
    24246, /* Sanguine Hibiscus */
    24271, /* Spellcloth */
    24272, /* Shadowcloth */
    24280, /* Naga Claws */
    24290, /* Mature Spore Sac */
    24291, /* Bog Lord Tendril */
    24368, /* Coilfang Armaments */
    24401, /* Unidentified Plant Parts */
    24449, /* Fertile Spores */
    24477, /* Jaggal Clam Meat */
    24478, /* Jaggal Pearl */
    24479, /* Shadow Pearl */
    25416, /* Oshu'gun Crystal Fragment */
    25433, /* Obsidian Warbeads */
    25463, /* Pair of Ivory Tusks */
    25649, /* Knothide Leather Scraps */
    25699, /* Crystal Infused Leather */
    25700, /* Fel Scales */
    25707, /* Fel Hide */
    25708, /* Thick Clefthoof Leather */
    25744, /* Dampscale Basilisk Eye */
    25802, /* Dreadfang Venom Sac */
    25867, /* Earthstorm Diamond */
    25868, /* Skyfire Diamond */
    27422, /* Barbed Gill Trout */
    27425, /* Spotted Feltail */
    27429, /* Zangarian Sporefish */
    27435, /* Figluster's Mudfish */
    27437, /* Icefin Bluefish */
    27438, /* Golden Darter */
    27439, /* Furious Crawdad */
    27515, /* Huge Spotted Feltail */
    27516, /* Enormous Barbed Gill Trout */
    27668, /* Lynx Meat */
    27669, /* Bat Flesh */
    27671, /* Buzzard Meat */
    27674, /* Ravager Flesh */
    27677, /* Chunk o' Basilisk */
    27678, /* Clefthoof Meat */
    27681, /* Warped Flesh */
    27682, /* Talbuk Venison */
    28558, /* Spirit Shard */
    29209, /* Zaxxis Insignia */
    29539, /* Cobra Scales */
    29547, /* Wind Scales */
    29548, /* Nether Dragonscales */
    30183, /* Nether Vortex */
    30817, /* Simple Flour */
    31079, /* Mercurial Adamantite */
    31670, /* Raptor Ribs */
    31671, /* Serpent Flesh */
    32227, /* Crimson Spinel */
    32228, /* Empyrean Sapphire */
    32229, /* Lionseye */
    32230, /* Shadowsong Amethyst */
    32231, /* Pyrestone */
    32249, /* Seaspray Emerald */
    32423, /* Icy Blasting Primers */
    32428, /* Heart of Darkness */
    33455, /* Brewfest Prize Ticket */
    33470, /* Frostweave Cloth */
    33567, /* Borean Leather Scraps */
    33568, /* Borean Leather */
    33823, /* Bloodfin Catfish */
    33824, /* Crescent-Tail Skullfish */
    34052, /* Dream Shard */
    34053, /* Small Dream Shard */
    34054, /* Infinite Dust */
    34055, /* Greater Cosmic Essence */
    34056, /* Lesser Cosmic Essence */
    34057, /* Abyss Crystal */
    34664, /* Sunmote */
    34736, /* Chunk o' Mammoth */
    35128, /* Hardened Khorium */
    35230, /* Darnarian's Scroll of Teleportation */
    35562, /* Bear Flank */
    35622, /* Eternal Water */
    35623, /* Eternal Air */
    35624, /* Eternal Earth */
    35625, /* Eternal Life */
    35627, /* Eternal Shadow */
    36782, /* Succulent Clam Meat */
    36783, /* Northsea Pearl */
    36784, /* Siren's Tear */
    36860, /* Eternal Fire */
    36901, /* Goldclover */
    36903, /* Adder's Tongue */
    36904, /* Tiger Lily */
    36905, /* Lichbloom */
    36906, /* Icethorn */
    36907, /* Talandra's Rose */
    36908, /* Frost Lotus */
    36909, /* Cobalt Ore */
    36910, /* Titanium Ore */
    36912, /* Saronite Ore */
    36913, /* Saronite Bar */
    36916, /* Cobalt Bar */
    36917, /* Bloodstone */
    36918, /* Scarlet Ruby */
    36919, /* Cardinal Ruby */
    36920, /* Sun Crystal */
    36921, /* Autumn's Glow */
    36922, /* King's Amber */
    36923, /* Chalcedony */
    36924, /* Sky Sapphire */
    36925, /* Majestic Zircon */
    36926, /* Shadow Crystal */
    36927, /* Twilight Opal */
    36928, /* Dreadstone */
    36929, /* Huge Citrine */
    36930, /* Monarch Topaz */
    36931, /* Ametrine */
    36932, /* Dark Jade */
    36933, /* Forest Emerald */
    36934, /* Eye of Zul */
    37101, /* Ivory Ink */
    37602, /* Armor Vellum II */
    37663, /* Titansteel Bar */
    37700, /* Crystallized Air */
    37701, /* Crystallized Earth */
    37702, /* Crystallized Fire */
    37703, /* Crystallized Shadow */
    37704, /* Crystallized Life */
    37705, /* Crystallized Water */
    37921, /* Deadnettle */
    38425, /* Heavy Borean Leather */
    38426, /* Eternium Thread */
    38557, /* Icy Dragonscale */
    38558, /* Nerubian Chitin */
    38561, /* Jormungar Scale */
    38682, /* Enchanting Vellum */
    39151, /* Alabaster Pigment */
    39328, /* Crusader Skull */
    39334, /* Dusky Pigment */
    39338, /* Golden Pigment */
    39339, /* Emerald Pigment */
    39340, /* Violet Pigment */
    39341, /* Silvery Pigment */
    39342, /* Nether Pigment */
    39343, /* Azure Pigment */
    39349, /* Weapon Vellum */
    39350, /* Weapon Vellum II */
    39354, /* Light Parchment */
    39469, /* Moonglow Ink */
    39681, /* Handful of Cobalt Bolts */
    39682, /* Overcharged Capacitor */
    39683, /* Froststeel Tube */
    39684, /* Hair Trigger */
    39690, /* Volatile Blasting Trigger */
    39774, /* Midnight Ink */
    39970, /* Fire Leaf */
    40195, /* Pygmy Oil */
    40199, /* Pygmy Suckerfish */
    40533, /* Walnut Stock */
    41163, /* Titanium Bar */
    41266, /* Skyflare Diamond */
    41334, /* Earthsiege Diamond */
    41510, /* Bolt of Frostweave */
    41511, /* Bolt of Imbued Frostweave */
    41593, /* Ebonweave */
    41594, /* Moonshroud */
    41595, /* Spellweave */
    41800, /* Deep Sea Monsterbelly */
    41801, /* Moonglow Cuttlefish */
    41802, /* Imperial Manta Ray */
    41803, /* Rockfin Grouper */
    41805, /* Borean Man O' War */
    41806, /* Musselback Sculpin */
    41807, /* Dragonfin Angelfish */
    41808, /* Bonescale Snapper */
    41809, /* Glacial Salmon */
    41810, /* Fangtooth Herring */
    41812, /* Barrelhead Goby */
    41813, /* Nettlefish */
    41814, /* Glassfin Minnow */
    42225, /* Dragon's Eye */
    42253, /* Iceweb Spider Silk */
    42780, /* Relic of Ulduar */
    43007, /* Northern Spices */
    43009, /* Shoveltusk Flank */
    43010, /* Worm Meat */
    43011, /* Worg Haunch */
    43012, /* Rhino Meat */
    43013, /* Chilled Meat */
    43102, /* Frozen Orb */
    43103, /* Verdant Pigment */
    43104, /* Burnt Pigment */
    43105, /* Indigo Pigment */
    43106, /* Ruby Pigment */
    43107, /* Sapphire Pigment */
    43108, /* Ebon Pigment */
    43109, /* Icy Pigment */
    43115, /* Hunter's Ink */
    43116, /* Lion's Ink */
    43117, /* Dawnstar Ink */
    43118, /* Jadefire Ink */
    43119, /* Royal Ink */
    43120, /* Celestial Ink */
    43121, /* Fiery Ink */
    43122, /* Shimmering Ink */
    43123, /* Ink of the Sky */
    43124, /* Ethereal Ink */
    43125, /* Darkflame Ink */
    43126, /* Ink of the Sea */
    43127, /* Snowfall Ink */
    43145, /* Armor Vellum III */
    43146, /* Weapon Vellum III */
    43501, /* Northern Egg */
    44128, /* Arctic Fur */
    44501, /* Goblin-Machined Piston */
    44834, /* Wild Turkey */
    44835, /* Autumnal Herbs */
    44853, /* Honey */
    44958, /* Ethereal Oil */
    44987, /* Valiant's Seal */
    45087, /* Runed Orb */
    47556, /* Crusader Orb */
    49908 /* Primordial Saronite */
);
