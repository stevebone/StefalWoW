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

#ifndef TRINITY_VALIDATIONRESULT_H
#define TRINITY_VALIDATIONRESULT_H

#include "BotMovementDefines.h"
#include <string>

struct ValidationResult
{
    bool isValid = false;
    ValidationFailureReason failureReason = ValidationFailureReason::None;
    std::string errorMessage;

    ValidationResult() = default;

    ValidationResult(bool valid, ValidationFailureReason reason = ValidationFailureReason::None, std::string message = "")
        : isValid(valid), failureReason(reason), errorMessage(std::move(message))
    {
    }

    static ValidationResult Success()
    {
        return ValidationResult(true, ValidationFailureReason::None, "");
    }

    static ValidationResult Failure(ValidationFailureReason reason, std::string message = "")
    {
        return ValidationResult(false, reason, std::move(message));
    }
};

#endif
