
enum FSB_TeleportReasons
{
    BOT_DEATH = 0,
    BOT_TOO_FAR = 1
};

namespace FSBTeleport
{
    bool BotTeleport(Creature* bot, FSB_TeleportReasons reason);
}
