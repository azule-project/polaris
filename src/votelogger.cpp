/*
 * votelogger.cpp
 *
 *  Created on: Dec 31, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <boost/algorithm/string.hpp>
#include <settings/Bool.hpp>
#include "CatBot.hpp"
#include "votelogger.hpp"

//static settings::Boolean random_vote_reason{ "votelogger.random.reason", "false" };
//static settings::String vote_reasons{ "Scamming", "Cheating", "Idling", "No reason given"};
static settings::Boolean vote_kicky{ "votelogger.autovote.yes", "false" };
static settings::Boolean vote_kickn{ "votelogger.autovote.no", "false" };
static settings::Boolean vote_rage_vote{ "votelogger.autovote.no.rage", "false" };
static settings::Boolean chat{ "votelogger.chat", "true" };
static settings::Boolean chat_partysay{ "votelogger.chat.partysay", "false" };
static settings::Boolean chat_casts{ "votelogger.chat.casts", "false" };
static settings::Boolean chat_casts_f1_only{ "votelogger.chat.casts.f1-only", "true" };
// Leave party and crash, useful for personal party bots
static settings::Boolean abandon_and_crash_on_kick{ "votelogger.restart-on-kick", "false" };

namespace votelogger
{
static bool was_local_player{ false };
static Timer local_kick_timer{};
static int F1_count = 0;
static int F2_count = 0;

static void vote_rage_back()
{
    static Timer attempt_vote_time;
    char cmd[40];
    player_info_s info{};
    std::vector<int> targets;

    if (!g_IEngine->IsInGame() || !attempt_vote_time.test_and_set(1000))
        return;

    for (const auto &ent : entity_cache::player_cache)
    {
        // TO DO: m_bEnemy check only when you can't vote off players from the opposite team
        if (ent == LOCAL_E || ent->m_bEnemy())
            continue;

        if (!GetPlayerInfo(ent->m_IDX, &info))
            continue;

        auto &pl = playerlist::AccessData(info.friendsID);
        if (pl.state == playerlist::k_EState::RAGE)
            targets.emplace_back(info.userID);
    }
    if (targets.empty())
        return;
    
    /*std::string reason_to_con;
    if (*random_vote_reason)
    //0= cheating, 1= scamming, 2= idling, 3=other
    int random = rand() % 3;
    if (random != 0)
    reason_to_con = "cheating";
    std::snprintf(cmd, sizeof(cmd), "callvote kick \"%d %s\"", targets[UniformRandomInt(0, targets.size() - 1)], reason_to_con);
    g_IEngine->ClientCmd_Unrestricted(cmd);
    else if (random != 1)
    reason_to_con = "scamming";
    std::snprintf(cmd, sizeof(cmd), "callvote kick \"%d %s\"", targets[UniformRandomInt(0, targets.size() - 1)], reason_to_con);
    g_IEngine->ClientCmd_Unrestricted(cmd);
    else if (random != 2)
    reason_to_con = "idle";
    std::snprintf(cmd, sizeof(cmd), "callvote kick \"%d %s\"", targets[UniformRandomInt(0, targets.size() - 1)], reason_to_con);
    g_IEngine->ClientCmd_Unrestricted(cmd);
    else if (random != 3)
    reason_to_con = "other";
    std::snprintf(cmd, sizeof(cmd), "callvote kick \"%d %s\"", targets[UniformRandomInt(0, targets.size() - 1)], reason_to_con);
    g_IEngine->ClientCmd_Unrestricted(cmd);
    }*/

    std::snprintf(cmd, sizeof(cmd), "callvote kick \"%d cheating\"", targets[UniformRandomInt(0, targets.size() - 1)]);
    g_IEngine->ClientCmd_Unrestricted(cmd);
}

// Call Vote RNG
struct CVRNG
{
    std::string content;
    unsigned int time{};
    Timer timer{};
};

static CVRNG vote_command;
void dispatchUserMessage(bf_read &buffer, int type)
{
    switch (type)
    {
    case 45:
    {
        // Vote setup Failed
        int reason   = buffer.ReadByte();
        int cooldown = buffer.ReadShort();
        int delay    = 4;

        if (reason == 2) // VOTE_FAILED_RATE_EXCEEDED
            delay = cooldown;

        hacks::catbot::timer_votekicks.last -= std::chrono::seconds(delay);
        break;
    }
    case 46:
    {
        was_local_player = false;
        int team         = buffer.ReadByte();
        int vote_id      = buffer.ReadLong();
        int caller       = buffer.ReadByte();
        char reason[64];
        char name[64];
        buffer.ReadString(reason, 64, false, nullptr);
        buffer.ReadString(name, 64, false, nullptr);
        auto target = static_cast<unsigned char>(buffer.ReadByte());
        buffer.Seek(0);
        target >>= 1;

        // info is the person getting kicked,
        // info2 is the person calling the kick.
        player_info_s info{}, info2{};
        if (!GetPlayerInfo(target, &info) || !GetPlayerInfo(caller, &info2))
            break;

        auto team_name = teamname(team);
        logging::Info("[%s] Vote called to kick %s [U:1:%u] for %s by %s [U:1:%u]", team_name, info.name, info.friendsID, reason, info2.name, info2.friendsID);
        if (info.friendsID == g_ISteamUser->GetSteamID().GetAccountID())
        {
            was_local_player = true;
            local_kick_timer.update();
        }

        if (*vote_kickn || *vote_kicky)
        {
            using namespace playerlist;

            auto &pl             = AccessData(info.friendsID);
            auto &pl_caller      = AccessData(info2.friendsID);
            bool friendly_kicked = pl.state != k_EState::RAGE && pl.state != k_EState::DEFAULT;
            bool friendly_caller = pl_caller.state != k_EState::RAGE && pl_caller.state != k_EState::DEFAULT;

            if (*vote_kickn && friendly_kicked)
            {
                vote_command = { strfmt("vote %d option2", vote_id).get(), 1000u + (rand() % 5000) };
                vote_command.timer.update();
                logging::Info("Voting No (F2) because %s [U:1:%u] is %s playerlist state", info.name, info.friendsID, pl.state);
                if (*vote_rage_vote && !friendly_caller)
                    pl_caller.state = k_EState::RAGE;
                    logging::Info("Voting No (F2) because %s [U:1:%u] is %s playerlist state. A Counter-kick will be called on %s [U:1:%u] when we can vote.", info.name, info.friendsID, pl.state, info2.name, info2.friendsID);
            }
            else if (*vote_kicky && !friendly_kicked)
            {
                vote_command = { strfmt("vote %d option1", vote_id).get(), 1000u + (rand() % 5000) };
                vote_command.timer.update();
                logging::Info("Voting Yes (F1) because %s [U:1:%u] is %s playerlist state.", info.name, info.friendsID);
            }
        }
        if (*chat_partysay)
        {
            char formated_string[256];
            std::snprintf(formated_string, sizeof(formated_string), "Called votekick: %s => %s (%s)", team_name, info2.name, info.name, reason);
            re::CTFPartyClient::GTFPartyClient()->SendPartyChat(formated_string);
        }
#if ENABLE_VISUALS
        if (*chat)
            PrintChat("Player '\x07%06X%s\x01' has called a vote on '\x07%06X%s\x01' (\"%s\")", 0xe1ad01, info2.name, 0xe1ad01, info.name, reason);
#endif
        break;
    }
    case 47:
    {
        logging::Info("Vote passed on %s [U:1:%u]", info.name, info.friendsID);
#if ENABLE_VISUALS
        PrintChat("Vote passed on \x07%06X%s\x01 [U:1:%u]", 0xe1ad01, info.name, info.friendsID);
        PrintChat("Vote cast: Yes: %i, No: %i", F1_count + 1, F2_count + 1);
#endif
        break;
    }
    case 48:
#if ENABLE_VISUALS
        PrintChat("Vote failed on \x07%06X%s\x01 [U:1:%u]", 0xe1ad01, info.name, info.friendsID);
        PrintChat("Vote cast: Yes: %i, No: %i", F1_count + 1, F2_count + 1);
#endif
        logging::Info("Vote failed on %s [U:1:%u]", info.name, info.friendsID);
        break;
    case 49:
        logging::Info("VoteSetup?");
        [[fallthrough]];
    default:
        break;
    }
}

static bool found_message = false;
void onShutdown(const std::string &message)
{
    if (message.find("Generic_Kicked") == std::string::npos)
    {
        found_message = false;
        return;
    }
    if (local_kick_timer.check(60000) || !was_local_player)
    {
        found_message = false;
        return;
    }
    if (*abandon_and_crash_on_kick)
    {
        found_message = true;
        g_IEngine->ClientCmd_Unrestricted("tf_party_leave");
        local_kick_timer.update();
    }
    else
        found_message = false;
}

static void setup_paint_abandon()
{
    EC::Register(
        EC::Paint,
        []()
        {
            if (!vote_command.content.empty() && vote_command.timer.test_and_set(vote_command.time))
            {
                g_IEngine->ClientCmd_Unrestricted(vote_command.content.c_str());
                vote_command.content = "";
            }
            if (!found_message)
                return;
            if (local_kick_timer.check(60000) || !local_kick_timer.test_and_set(10000) || !was_local_player)
                return;
            if (*abandon_and_crash_on_kick)
                *(int *) nullptr = 0;
        },
        "vote_abandon_restart");
}

static void setup_vote_rage()
{
    EC::Register(EC::CreateMove, vote_rage_back, "vote_rage_back");
}

static void reset_vote_rage()
{
    EC::Unregister(EC::CreateMove, "vote_rage_back");
}

class VoteEventListener : public IGameEventListener
{
public:
    void FireGameEvent(KeyValues *event) override
    {
        if (!*chat_casts || (!*chat_partysay && !*chat))
            return;
        const char *name = event->GetName();
        if (!strcmp(name, "vote_cast"))
        {
            bool vote_option = event->GetInt("vote_option");
            if (*chat_casts_f1_only && vote_option)
                return;
            int eid = event->GetInt("entityid");

            player_info_s info{};
            if (!GetPlayerInfo(eid, &info))
                return;
            if (*chat_partysay)
            {
                char formated_string[256];
                std::snprintf(formated_string, sizeof(formated_string), "Player '%s' has voted %s", info.name, vote_option ? "No (F2)" : "Yes (F1)");

                re::CTFPartyClient::GTFPartyClient()->SendPartyChat(formated_string);
            }
#if ENABLE_VISUALS
            if (*chat)
            {
                switch (vote_option)
                {
                case true:
                {
                    PrintChat("\x07%06X%s\x01 has voted \x07%06XNo\x01", 0xe1ad01, info.name, 0x00ff00);
                    break;
                }
                case false:
                {
                    PrintChat("\x07%06X%s\x01 has voted \x07%06XYes\x01", 0xe1ad01, info.name, 0xff0000);
                    break;
                }
                }
            }
#endif
        }
    }
};

static VoteEventListener listener{};
static InitRoutine init(
    []()
    {
        if (*vote_rage_vote)
            setup_vote_rage();
        setup_paint_abandon();

        vote_rage_vote.installChangeCallback(
            [](settings::VariableBase<bool> &var, bool new_val)
            {
                if (new_val)
                    setup_vote_rage();
                else
                    reset_vote_rage();
            });
        g_IGameEventManager->AddListener(&listener, false);
        EC::Register(
            EC::Shutdown, []() { g_IGameEventManager->RemoveListener(&listener); }, "event_shutdown_vote");
    });
} // namespace votelogger
