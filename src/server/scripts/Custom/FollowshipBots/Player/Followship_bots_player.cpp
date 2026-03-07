#include "Group.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"

#include "Followship_bots_db.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_party_handler.h"

class followship_bots_player : public PlayerScript
{
public:
    followship_bots_player() : PlayerScript("followship_bots_player") { }

    void OnLogin(Player* /*player*/, bool /*firstLogin*/) override
    {

    }

    void OnLogout(Player* /*player*/) override
    {
    }

    void OnMapChanged(Player* player) override
    {
        FSBMgr::Get()->RemovePersistentExpiredPlayerBots(player);
        FSBMgr::Get()->SpawnPlayerBots(player);

        // Send fake party frame after bots are spawned (delayed slightly so bots are in world)
        // The actual initial send happens from the bot's hired maintenance loop once bots are ready
    }
};

// GroupScript to re-inject bot data when the real group changes
class followship_bots_group : public GroupScript
{
public:
    followship_bots_group() : GroupScript("followship_bots_group") { }

    void OnAddMember(Group* group, ObjectGuid guid) override
    {
        FSBParty::OnMemberAdd(group, guid);
    }

    void OnRemoveMember(Group* group, ObjectGuid guid, RemoveMethod /*method*/, ObjectGuid /*kicker*/, char const* /*reason*/) override
    {
        FSBParty::OnMemberRemove(group, guid);
    }

    void OnDisband(Group* /*group*/) override
    {
        // Group disbanded — each player's bots will be re-sent as standalone fake parties
        // This happens naturally through the periodic maintenance
    }
};

void AddSC_followship_bots_player()
{
    new followship_bots_player();
    new followship_bots_group();
}
