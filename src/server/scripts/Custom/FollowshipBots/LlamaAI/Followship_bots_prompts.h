/*
 * This file is part of the Stefal WoW Project.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Creature.h"
#include "Followship_bots_defines.h"

class FSB_BaseAI;

namespace FSBLlamaPrompts
{
    void DispatchBotInfoRequest(Creature* bot);
    void DispatchBotRoleAcknowledge(Creature* bot);
}
