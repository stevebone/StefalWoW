#!/bin/bash
# DefensiveBehaviorManager.cpp Manual Migration Script
# PHASE 0 WEEK 3 PHASE 3.4 (2025-10-30)
# Migrates 5 CastSpell calls to packet-based spell casting

FILE="C:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/CombatBehaviors/DefensiveBehaviorManager.cpp"

echo "===== MIGRATING DefensiveBehaviorManager.cpp ====="
echo "Step 1: Add SpellPacketBuilder.h include after SpatialGridQueryHelpers.h (line 25)"

# Add include after line 25 (SpatialGridQueryHelpers.h)
sed -i '25 a #include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting' "$FILE"

echo "Step 2: Migrate line 452 - Self defensive cooldown"

# Replace line 452: _bot->CastSpell(_bot, defensive, false);
# Context: Pre-emptive defensive when major threat detected
sed -i '452,453s|            _bot->CastSpell(_bot, defensive, false);\
            MarkDefensiveUsed(defensive);|            // MIGRATION COMPLETE (2025-10-30): Packet-based defensive cooldown\
            SpellPacketBuilder::BuildOptions options;\
            options.skipGcdCheck = false;\
            options.skipResourceCheck = false;\
            options.skipTargetCheck = false;\
            options.skipStateCheck = false;\
            options.skipRangeCheck = false;\
            options.logFailures = true;\
\
            auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, defensive, _bot, options);\
            if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)\
            {\
                TC_LOG_DEBUG("playerbot.defensive.cooldown",\
                             "Bot {} queued defensive cooldown {} (self-cast, major threat detected)",\
                             _bot->GetName(), defensive);\
                MarkDefensiveUsed(defensive);\
            }|' "$FILE"

echo "Step 3: Migrate line 643 - Hand of Protection (Paladin save)"

# Replace line 643: _bot->CastSpell(target, HAND_OF_PROTECTION, false);
sed -i '643,644s|                        _bot->CastSpell(target, HAND_OF_PROTECTION, false);\
                        provided = true;|                        // MIGRATION COMPLETE (2025-10-30): Packet-based emergency save\
                        SpellPacketBuilder::BuildOptions options;\
                        options.skipGcdCheck = false;\
                        options.skipResourceCheck = false;\
                        options.skipTargetCheck = false;\
                        options.skipStateCheck = false;\
                        options.skipRangeCheck = false;\
                        options.logFailures = true;\
\
                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, HAND_OF_PROTECTION, target, options);\
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)\
                        {\
                            TC_LOG_DEBUG("playerbot.defensive.save",\
                                         "Bot {} queued HAND_OF_PROTECTION for {} (emergency save)",\
                                         _bot->GetName(), target->GetName());\
                            provided = true;\
                        }|' "$FILE"

echo "Step 4: Migrate line 648 - Hand of Sacrifice (Paladin save)"

# Replace line 648: _bot->CastSpell(target, HAND_OF_SACRIFICE, false);
sed -i '648,649s|                        _bot->CastSpell(target, HAND_OF_SACRIFICE, false);\
                        provided = true;|                        // MIGRATION COMPLETE (2025-10-30): Packet-based emergency save\
                        SpellPacketBuilder::BuildOptions options;\
                        options.skipGcdCheck = false;\
                        options.skipResourceCheck = false;\
                        options.skipTargetCheck = false;\
                        options.skipStateCheck = false;\
                        options.skipRangeCheck = false;\
                        options.logFailures = true;\
\
                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, HAND_OF_SACRIFICE, target, options);\
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)\
                        {\
                            TC_LOG_DEBUG("playerbot.defensive.save",\
                                         "Bot {} queued HAND_OF_SACRIFICE for {} (emergency save)",\
                                         _bot->GetName(), target->GetName());\
                            provided = true;\
                        }|' "$FILE"

echo "Step 5: Migrate line 656 - Pain Suppression (Priest save)"

# Replace line 656: _bot->CastSpell(target, PAIN_SUPPRESSION, false);
sed -i '656,657s|                        _bot->CastSpell(target, PAIN_SUPPRESSION, false);\
                        provided = true;|                        // MIGRATION COMPLETE (2025-10-30): Packet-based emergency save\
                        SpellPacketBuilder::BuildOptions options;\
                        options.skipGcdCheck = false;\
                        options.skipResourceCheck = false;\
                        options.skipTargetCheck = false;\
                        options.skipStateCheck = false;\
                        options.skipRangeCheck = false;\
                        options.logFailures = true;\
\
                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, PAIN_SUPPRESSION, target, options);\
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)\
                        {\
                            TC_LOG_DEBUG("playerbot.defensive.save",\
                                         "Bot {} queued PAIN_SUPPRESSION for {} (emergency save)",\
                                         _bot->GetName(), target->GetName());\
                            provided = true;\
                        }|' "$FILE"

echo "Step 6: Migrate line 661 - Guardian Spirit (Priest save)"

# Replace line 661: _bot->CastSpell(target, GUARDIAN_SPIRIT, false);
sed -i '661,662s|                        _bot->CastSpell(target, GUARDIAN_SPIRIT, false);\
                        provided = true;|                        // MIGRATION COMPLETE (2025-10-30): Packet-based emergency save\
                        SpellPacketBuilder::BuildOptions options;\
                        options.skipGcdCheck = false;\
                        options.skipResourceCheck = false;\
                        options.skipTargetCheck = false;\
                        options.skipStateCheck = false;\
                        options.skipRangeCheck = false;\
                        options.logFailures = true;\
\
                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, GUARDIAN_SPIRIT, target, options);\
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)\
                        {\
                            TC_LOG_DEBUG("playerbot.defensive.save",\
                                         "Bot {} queued GUARDIAN_SPIRIT for {} (emergency save)",\
                                         _bot->GetName(), target->GetName());\
                            provided = true;\
                        }|' "$FILE"

echo ""
echo "===== MIGRATION COMPLETE ====="
echo "File: $FILE"
echo "Changes: 1 include + 5 CastSpell calls replaced"
echo "Next: Compile with 'cmake --build . --config RelWithDebInfo --target playerbot --parallel 8'"
