/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Containers.h"
#include "DB2Stores.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include <MovementPackets.h>
#include <G3D/g3dmath.h>
#include "MotionMaster.h"
#include "ScriptedCreature.h"

enum ToySpells
{
    SPELL_NECRONOM_I_NOM_JUMP               = 340184,
    SPELL_NECRONOM_I_NOM_MORPH              = 340118,
    SPELL_NECRONOM_I_NOM_SCALE              = 340161,
    SPELL_NECRONOM_I_NOM_FILTER             = 341640,
    SPELL_TWELVE_STRING_GUITAR              = 232247,
    SPELL_TWELVE_STRING_GUITAR_2            = 232596,
    SPELL_TWELVE_STRING_GUITAR_3            = 232597,
    SPELL_TWELVE_STRING_GUITAR_4            = 232598,
    SPELL_TWELVE_STRING_GUITAR_5            = 232599



};

// itemid - 182732
// spellid - 340110
class spell_necronom_i_nom : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_NECRONOM_I_NOM_JUMP,
                SPELL_NECRONOM_I_NOM_MORPH,
                SPELL_NECRONOM_I_NOM_SCALE,
                SPELL_NECRONOM_I_NOM_FILTER
            });
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        uint32 RandomEffects = urand(0, 3);

        switch (RandomEffects)
        {
        case 0:
            //jump
            caster->CastSpell(caster, SPELL_NECRONOM_I_NOM_JUMP, true);
            break;
        case 1:
            //morph
            caster->CastSpell(caster, SPELL_NECRONOM_I_NOM_MORPH, true);
            break;
        case 2:
            //scale
            caster->CastSpell(caster, SPELL_NECRONOM_I_NOM_SCALE, true);
            break;
        case 3:
            //filter
            caster->CastSpell(caster, SPELL_NECRONOM_I_NOM_FILTER, true);
            break;
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_necronom_i_nom::HandleOnCast);
    }
};

enum ItemIds
{
    YELLOW  = 163521,
    PURPLE  = 163518,
    GREEN   = 163172,
    RED     = 163517,
    ORANGE  = 163520,
    BLUE    = 163516
};

// itemid - 166704
// spellid - 291106
class spell_bowl_of_glowing_pufferfish : public SpellScript
{

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        assert(caster);

        if (Player* player = caster->ToPlayer())
        {
            uint32 RandomItem = urand(0, 5);

            switch (RandomItem)
            {
            case 0:
                player->AddItem(YELLOW, 1);
                break;
            case 1:
                player->AddItem(PURPLE, 1);
                break;
            case 2:
                player->AddItem(GREEN, 1);
                break;
            case 3:
                player->AddItem(RED, 1);
                break;
            case 4:
                player->AddItem(ORANGE, 1);
                break;
            case 5:
                player->AddItem(BLUE, 1);
                break;
            }
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_bowl_of_glowing_pufferfish::HandleOnCast);
    }
};

// itemid - 143543
// spellid - 232592
class spell_twelve_string_guitar : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_TWELVE_STRING_GUITAR,
                SPELL_TWELVE_STRING_GUITAR_2,
                SPELL_TWELVE_STRING_GUITAR_3,
                SPELL_TWELVE_STRING_GUITAR_4,
                SPELL_TWELVE_STRING_GUITAR_5
            });
    }

    void HandleOnCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        uint32 RandomEffects = urand(0, 4);

        switch (RandomEffects)
        {
        case 0:
            caster->CastSpell(caster, SPELL_TWELVE_STRING_GUITAR, true);
            break;
        case 1:
            caster->CastSpell(caster, SPELL_TWELVE_STRING_GUITAR_2, true);
            break;
        case 2:
            caster->CastSpell(caster, SPELL_TWELVE_STRING_GUITAR_3, true);
            break;
        case 3:
            caster->CastSpell(caster, SPELL_TWELVE_STRING_GUITAR_4, true);
            break;
        case 4:
            caster->CastSpell(caster, SPELL_TWELVE_STRING_GUITAR_5, true);
            break;
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_twelve_string_guitar::HandleOnCast);
    }
};

//176199	Mystery Keg
//176202	Mystery Keg
//176203	Mystery Keg
//176205	Mystery Keg
//176206	Mystery Keg
//176209	Mystery Keg
//176212	Mystery Keg

void AddSC_toy_spell_scripts()
{
    RegisterSpellScript(spell_necronom_i_nom);
    RegisterSpellScript(spell_bowl_of_glowing_pufferfish);
    RegisterSpellScript(spell_twelve_string_guitar);
}
