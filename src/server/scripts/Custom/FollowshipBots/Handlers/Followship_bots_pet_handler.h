
enum FSB_PET_SPELLS
{
    //Combat
    SPELL_HUNTER_PET_FURIOUS_BITE = 263840, //10s //Wolf
    SPELL_HUNTER_PET_PRIMAL_RAGE = 264667, //6m  //Wolf
    SPELL_HUNTER_PET_PREDATOR_THIRST = 264663, //x  //Wolf
    SPELL_HUNTER_PET_BITE = 17253, //3s //Wolf

    SPELL_HUNTER_PET_SCRATCH = 256337, //target
    SPELL_HUNTER_AGILE_REFLEXES = 160011, //self
    SPELL_HUNTER_MASTER_CALL = 53271, //self
};

enum class FSB_PetFamily
{
    Unknown = 0,
    Wolf,
    Cat,
    Bear,
    Boar,
    Fox,
    // add more as needed
};

static const std::unordered_map<uint32, FSB_PetFamily> CreatureFamilyMap =
{
    { 43292,    FSB_PetFamily::Wolf },
    //{ 705,  FSB_PetFamily::Wolf },
    //{ 1132, FSB_PetFamily::Wolf },
    //{ 1922, FSB_PetFamily::Wolf },
    //{ 42717,FSB_PetFamily::Wolf },

    //{ 6368, FSB_PetFamily::Cat },
    //{ 682,  FSB_PetFamily::Cat },
    //{ 736,  FSB_PetFamily::Cat },

    { 46982,  FSB_PetFamily::Fox },


};

static const std::unordered_map<FSB_PetFamily, std::vector<uint32>> CreatureFamilySpells =
{
    { FSB_PetFamily::Wolf, { SPELL_HUNTER_PET_FURIOUS_BITE, SPELL_HUNTER_PET_BITE, SPELL_HUNTER_PET_PRIMAL_RAGE, SPELL_HUNTER_PET_PREDATOR_THIRST } },   
    { FSB_PetFamily::Fox,  { SPELL_HUNTER_PET_SCRATCH, SPELL_HUNTER_PET_BITE, SPELL_HUNTER_AGILE_REFLEXES, SPELL_HUNTER_MASTER_CALL } },
    //{ FSB_PetFamily::Cat,  { 16827, 16828, 16829 } },   // example cat spells
    //{ FSB_PetFamily::Bear, { 16810, 16811 } },
    //{ FSB_PetFamily::Boar, { 17260, 17261 } }
};

namespace FSBPet
{
    Unit* GetBotPet(Creature* owner);
    bool BotHasPet(Creature* owner);

    FSB_PetFamily GetPetFamily(uint32 entry);
    const std::vector<uint32>& GetFamilySpells(FSB_PetFamily family);

    bool DoAttackSpell(Creature* owner);

    bool SetBasePetInformation(Creature* owner, Creature* pet);
}
