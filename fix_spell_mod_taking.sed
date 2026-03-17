# Fix location 1 (line 564)
/^        m_bot->RemoveAllAuras();$/ {
    i\        // CRITICAL FIX: Clear m_spellModTakingSpell and kill all spell events FIRST\
        // Delayed spell events (like Fire Extinguisher 80209) still have m_spellModTakingSpell set\
        // When SpellEvent destructor fires during Player::Update(), Spell::~Spell() checks this pointer\
        // Must clear before KillAllEvents() to prevent Spell::~Spell assertion failure\
        m_bot->SetSpellModTakingSpell(nullptr, false);\
        m_bot->m_Events.KillAllEvents(false);\

}
