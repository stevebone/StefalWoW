#include "Followship_bots_db.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots.h"
#include "Followship_bots_priest.h"

class followship_bots_player : public PlayerScript
{
public:
    followship_bots_player() : PlayerScript("followship_bots_player") { }

    void OnLogin(Player* player, bool /*firstLogin*/ ) override
    {
        FSBMgr::Get()->RemovePersistentExpiredPlayerBots(player);
    }

    void OnLogout(Player* /*player*/) override
    {
    }

    void OnMapChanged(Player* player) override
    {
        FSBMgr::Get()->SpawnPlayerBots(player);
    }
};

void AddSC_followship_bots_player()
{
    new followship_bots_player();
}
