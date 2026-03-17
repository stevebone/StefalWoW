/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include <string>
#include <any>

namespace Playerbot
{

// Forward declarations
class BotAI;

class TC_GAME_API Value
{
public:
    explicit Value(::std::string const& name);
    virtual ~Value() = default;

    // Core value interface
    virtual ::std::any Get(BotAI* ai) const = 0;
    virtual void Set(BotAI* ai, ::std::any const& value) = 0;

    // Value metadata
    ::std::string const& GetName() const { return _name; }

    // Type-safe getters
    template<typename T>
    T GetValue(BotAI* ai) const
    {
        ::std::any val = Get(ai);
        try {
            return ::std::any_cast<T>(val);
        } catch (::std::bad_any_cast const&)
        {
            return T{};
        }
    }

    template<typename T>
    void SetValue(BotAI* ai, T const& value)
    {
        Set(ai, ::std::make_any<T>(value));
    }

protected:
    ::std::string _name;
};

// Specialized value types
template<typename T>
class TC_GAME_API TypedValue : public Value
{
public:
    explicit TypedValue(::std::string const& name) : Value(name) {}

    ::std::any Get(BotAI* ai) const override
    {
        return ::std::make_any<T>(GetTypedValue(ai));
    }

    void Set(BotAI* ai, ::std::any const& value) override
    {
        try {
            T typedValue = ::std::any_cast<T>(value);
            SetTypedValue(ai, typedValue);
        } catch (::std::bad_any_cast const&)
        {
            // Ignore invalid type casts
        }
    }

protected:
    virtual T GetTypedValue(BotAI* ai) const = 0;
    virtual void SetTypedValue(BotAI* ai, T const& value) = 0;
};

} // namespace Playerbot