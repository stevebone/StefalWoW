// Runtime monitoring code to validate memory safety fixes
// Add this to BotWorldSessionMgr.h as private members:

class BotWorldSessionMgr {
private:
    // Memory corruption detection
    std::atomic<uint32> _sessionAccessCount{0};
    std::atomic<uint32> _invalidCastDetected{0};
    std::atomic<uint32> _nullPointerPrevented{0};

public:
    void ValidateSessionIntegrity() const {
        std::lock_guard<std::mutex> lock(_sessionsMutex);

        for (auto const& [guid, session] : _botSessions) {
            if (!session) {
                TC_LOG_FATAL("module.playerbot.session",
                    "CRITICAL: Null session in _botSessions map for {}",
                    guid.ToString());
                std::abort();
            }

            // Validate session is actually a BotSession
            BotSession* botSession = dynamic_cast<BotSession*>(session);
            if (!botSession) {
                TC_LOG_FATAL("module.playerbot.session",
                    "CRITICAL: Invalid session type in _botSessions for {}",
                    guid.ToString());
                std::abort();
            }

            // Validate session memory isn't corrupted
            if (session->GetAccountId() == 0) {
                TC_LOG_FATAL("module.playerbot.session",
                    "CRITICAL: Session with zero account ID for {}",
                    guid.ToString());
                std::abort();
            }
        }
    }

    void LogMemoryStats() const {
        TC_LOG_INFO("module.playerbot.session",
            "Memory Safety Stats - Access Count: {}, Invalid Casts: {}, Null Pointers Prevented: {}",
            _sessionAccessCount.load(), _invalidCastDetected.load(), _nullPointerPrevented.load());
    }
};

// Call ValidateSessionIntegrity() periodically in Update()
// Call LogMemoryStats() every 10 minutes to monitor safety