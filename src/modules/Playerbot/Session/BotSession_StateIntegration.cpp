// This is the MODIFIED HandleBotPlayerLogin method with Issue #1 fix
// Lines 896-995 from the original BotSession.cpp

void BotSession::HandleBotPlayerLogin(Player* bot)
{
    try
    {
        ObjectGuid characterGuid = bot ? bot->GetGUID() : ObjectGuid::Empty;
        // Ensure we have a valid player
    if (!bot)
        {
            TC_LOG_ERROR("module.playerbot.session", "HandleBotPlayerLogin called with null bot");
            _loginState.store(LoginState::LOGIN_FAILED);
            m_playerLoading.Clear();
            return;
        }

        Player* pCurrChar = bot;

        // Create instance if not already there
    if (!pCurrChar)
        {
            pCurrChar = new Player(this);
        }

        // Now load the character from database
    if (!pCurrChar->LoadFromDB(characterGuid, nullptr, true))
        {
            TC_LOG_ERROR("module.playerbot.session", "Failed to load bot character {} from database", characterGuid.ToString());
            _loginState.store(LoginState::LOGIN_FAILED);
            m_playerLoading.Clear();
            return;
        }

        // Bot-specific initialization
        pCurrChar->SetVirtualPlayerRealm(GetVirtualRealmAddress());

        // Set the player for this session
        SetPlayer(pCurrChar);

        // Clear the loading state
        m_playerLoading.Clear();

        // CRITICAL FIX: Add bot to world (missing step that prevented bots from entering world)
        pCurrChar->SendInitialPacketsBeforeAddToMap();

        if (!pCurrChar->GetMap()->AddPlayerToMap(pCurrChar))
        {
            TC_LOG_ERROR("module.playerbot.session", "Failed to add bot player {} to map", characterGuid.ToString());
            // Try to teleport to homebind if map addition fails
            AreaTriggerStruct const* at = sObjectMgr->GetGoBackTrigger(pCurrChar->GetMapId());
            if (at)
                pCurrChar->TeleportTo(at->target_mapId, at->target_X, at->target_Y, at->target_Z, pCurrChar->GetOrientation());
            else
                pCurrChar->TeleportTo(pCurrChar->m_homebind);
        }

        ObjectAccessor::AddObject(pCurrChar);
        pCurrChar->SendInitialPacketsAfterAddToMap();

        TC_LOG_INFO("module.playerbot.session", "Bot player {} successfully added to world", pCurrChar->GetName());

        // Create and assign BotAI to take control of the character
    if (GetPlayer())
        {
            auto botAI = sBotAIFactory->CreateAI(GetPlayer());
            if (botAI)
            {
                SetAI(botAI.release()); // Transfer ownership to BotSession
                TC_LOG_INFO("module.playerbot.session", "Successfully created BotAI for character {}", characterGuid.ToString());
                // ========================================================================
                // CRITICAL FIX FOR ISSUE #1: Remove old group check
                // ========================================================================
                // OLD BROKEN CODE (lines 944-960) - REMOVED:
                // if (Player* player = GetPlayer())
                // {
                //     Group* group = player->GetGroup();
                //     TC_LOG_ERROR("module.playerbot.session", "Bot {} login group check: player={}, group={}",
                //                 player->GetName(), (void*)player, (void*)group);
                //
                //     if (group)
                //     {
                //         TC_LOG_INFO("module.playerbot.session", "Bot {} is already in group at login - activating strategies", player->GetName());
                //         if (BotAI* ai = GetAI())
                //         {
                //             TC_LOG_ERROR("module.playerbot.session", "About to call OnGroupJoined with group={}", (void*)group);
                //             ai->OnGroupJoined(group);  // TOO EARLY! Bot not IsInWorld() yet!
                //         }
                //     }
                // }

                // NEW CODE - State machine handles this properly:
                // The BotInitStateMachine will:
                // 1. Wait until bot is IsInWorld()
                // 2. Check for group membership in CHECKING_GROUP state
                // 3. Call OnGroupJoined() in ACTIVATING_STRATEGIES state
                // No explicit call needed here - state machine handles it automatically

                TC_LOG_INFO("module.playerbot.session",
                    "Bot {} login complete - state machine will handle initialization",
                    GetPlayer()->GetName());
            }
            else
            {
                TC_LOG_ERROR("module.playerbot.session", "Failed to create BotAI for character {}", characterGuid.ToString());
            }
        }

        // BOT-SPECIFIC LOGIN SPELL CLEANUP: Clear all pending spell events to prevent m_spellModTakingSpell crash
        // Issue: LOGINEFFECT (Spell 836) and other login spells are queued in EventProcessor during bot login
        // These spells modify spell behavior (m_spellModTakingSpell) but bots don't send client ACK packets
        // When the SpellEvent destructor fires, it tries to destroy a spell that's still referenced → ASSERTION FAILURE
        // Solution: Clear all events immediately after AI initialization to ensure no stale spell events execute
    if (Player* player = GetPlayer())
        {
            // Core Fix Applied: SpellEvent::~SpellEvent() now automatically clears m_spellModTakingSpell (Spell.cpp:8455)
            player->m_Events.KillAllEvents(false);  // false = don't force, let graceful shutdown happen
            TC_LOG_DEBUG("module.playerbot.session", " Bot {} cleared login spell events to prevent m_spellModTakingSpell crash", player->GetName());
        }

        // Mark login as complete
        _loginState.store(LoginState::LOGIN_COMPLETE);

        TC_LOG_INFO("module.playerbot.session", "ASYNC bot login successful for character {}", characterGuid.ToString());
    }
    catch (std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.session", "Exception in HandleBotPlayerLogin: {}", e.what());
        if (GetPlayer())
        {
            delete GetPlayer();
            SetPlayer(nullptr);
        }
        _loginState.store(LoginState::LOGIN_FAILED);
        m_playerLoading.Clear();
    }
    catch (...)
    {
        TC_LOG_ERROR("module.playerbot.session", "Unknown exception in HandleBotPlayerLogin");
        if (GetPlayer())
        {
            delete GetPlayer();
            SetPlayer(nullptr);
        }
        _loginState.store(LoginState::LOGIN_FAILED);
        m_playerLoading.Clear();
    }
}