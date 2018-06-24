/*
  Created on 23.06.18.
*/

#include <online/Online.hpp>
#include <core/cvwrapper.hpp>

#include <unordered_map>
#include <optional>
#include <timer.hpp>
#include <sstream>

#undef null

#include <co/OnlineService.hpp>
#include <fstream>
#include <init.hpp>
#include <thread>

namespace online
{

void saveApiKey();
void claimSteamId();

static co::OnlineService cathookOnlineService{};
static std::unordered_map<unsigned, std::optional<user_data>> data{};
static std::unordered_map<unsigned, bool> identify_queue{};
static Timer identify_timer{};
static bool identify_stale{ false };
static std::string api_key{};

static CatVar enable(CV_SWITCH, "online", "1", "Enable online features");
static CatCommand login("online_login", "Login", [](const CCommand& args) {
    if (args.ArgC() != 2)
    {
        logging::Info("\nUsage: online_login <API_KEY>\nKey will be saved in your data folder");
        return;
    }
    std::string key(args.Arg(1));
    try
    {
        cathookOnlineService.login(key, [key](co::ApiCallResult result, std::optional<co::logged_in_user> me) {
            if (result == co::ApiCallResult::OK)
            {
                logging::Info("[CO] Successfully logged in. Welcome, %s", me->username.c_str());
                api_key = key;
                saveApiKey();
                claimSteamId();
            }
            else
            {
                logging::Info("[CO] There was an error logging in: code %d", result);
            }
        });
    } catch (std::exception& ex) {
        logging::Info("[CO] Exception: %s", ex.what());
    }
});
static CatCommand flush("online_flush_cache", "Flush player cache", [](const CCommand& args) {
    data.clear();
    identify_queue.clear();
    identify_stale = true;
});
static CatVar host(CV_STRING, "online_host", "localhost:8000", "Online host");

// INTERNAL METHODS

void claimSteamId()
{
    auto id = g_ISteamUser->GetSteamID();
    logging::Info("[CO] Claiming SteamID %u", id.GetAccountID());
    cathookOnlineService.gameStartup(id.GetAccountID());
}

bool tryLoadApiKey()
{
    std::ifstream keyfile(DATA_PATH "/api_key", std::ios::in);
    if (keyfile)
    {
        keyfile >> api_key;
        if (!api_key.empty())
            return true;
    }
    return false;
}

void saveApiKey()
{
    std::ofstream keyfile(DATA_PATH "/api_key", std::ios::out);
    if (!keyfile)
    {
        logging::Info("[CO] Something went wrong while saving API key");
        return;
    }
    keyfile << api_key << '\n';
}

void queueUserForIdentification(unsigned steamId)
{

    identify_queue[steamId] = false;
    identify_timer.update();
    identify_stale = true;
}

void markSteamIdNonOnline(unsigned id)
{
    logging::Info("[CO] %u - not online", id);
    data[id] = std::nullopt;
}

void processOnlineIdentity(unsigned id, co::identified_user& user)
{
    logging::Info("[CO] %u - online", id);
    user_data udata{};
    udata.username = user.username;
    udata.is_anonymous = (user.username == "anonymous");
    udata.is_steamid_verified = user.steamid_verified;
    for (auto& i: user.groups)
    {
        if (i.display_name.has_value())
            udata.shown_roles.push_back(*i.display_name);
        if (i.name == "notarget")
            udata.no_target = true;
        if (i.name == "owner" || i.name == "contributor")
        {
            udata.is_developer = true;
#if ENABLE_VISUALS
            udata.rainbow = true;
#endif
        }
    }
#if ENABLE_VISUALS
    if (user.color.has_value())
    {
        udata.has_color = true;
        udata.color = colors::rgba_t(user.color->c_str());
    }
#endif
    if (user.uses_software.has_value())
    {
        udata.has_software = true;
        udata.is_using_friendly_software = user.uses_software->friendly;
        udata.software_name = user.uses_software->name;
    }
    data[id] = std::move(udata);
}

void processIdentifyResponse(std::vector<unsigned> input, co::identified_user_group& group)
{
    logging::Info("[CO] Processing identify response containing %u / %u entries", group.users.size(), input.size());
    for (auto i: input)
    {
        auto u = group.users.find(i);
        if (u == group.users.end())
            markSteamIdNonOnline(i);
        else
            processOnlineIdentity(i, (*u).second);

        identify_queue.erase(i);
        logging::Info("[CO] Removed %u from identify queue, left %u\n", i, identify_queue.size());
    }
}

void sendIdentifyRequest()
{
    std::vector<unsigned> steamIds{};
    auto it = identify_queue.begin();
    // Create a list of up to 32 steamId's
    for (int i = 0; i < 32 && it != identify_queue.end(); ++i, ++it)
    {
        if (!it->second)
        {
            it->second = true;
            steamIds.push_back(it->first);
        }
    }
    logging::Info("[CO] Sending identify request for %u players", steamIds.size());
    cathookOnlineService.userIdentify(steamIds, (std::function<void(co::ApiCallResult, std::optional<co::identified_user_group>)>)[steamIds](co::ApiCallResult result, std::optional<co::identified_user_group> group) {
        if (result == co::ApiCallResult::OK)
        {
            processIdentifyResponse(steamIds, *group);
        }
        else
        {
            logging::Info("[CO] Something went wrong while identifying %u players: code %d", steamIds.size(), result);
            for (auto i: steamIds)
            {
                identify_queue[i] = false;
            }
            identify_stale = true;
        }
    });
}

InitRoutine init([]() {
    cathookOnlineService.setErrorHandler((std::function<void(std::string)>)[](std::string error) {
        logging::Info("[CO] Error: %s", error.c_str());
    });
    host.InstallChangeCallback([](IConVar *var, const char *pszOldValue, float flOldValue) {
        logging::Info("[CO] Host = %s", host.GetString());
        cathookOnlineService.setHost(host.GetString());
    });
    if (tryLoadApiKey())
    {
        logging::Info("[CO] API key loaded successfully");
        cathookOnlineService.login(api_key, [](co::ApiCallResult result, std::optional<co::logged_in_user> me) {
            if (result == co::ApiCallResult::OK)
            {
                logging::Info("[CO] Successfully logged in. Welcome, %s", me->username.c_str());
                claimSteamId();
            }
            else
            {
                logging::Info("[CO] There was an error logging in: code %d", result);
            }
        });
    }
});

// EXTERNAL METHODS

void update()
{
    if (!enable)
        return;
    // Only send a request after 3 seconds passed since last unknown steamId was added to the queue
    if (!api_key.empty() && identify_stale && identify_timer.check(3000) && !identify_queue.empty())
    {
        sendIdentifyRequest();
        identify_stale = false;
    }
    try {
        cathookOnlineService.processPendingCalls();
    } catch (std::exception& ex) {
        logging::Info("[CO] Exception: %s", ex.what());
    }
}

user_data *getUserData(unsigned steamId)
{
    if (!enable)
        return nullptr;

    if (!steamId)
        return nullptr;

    auto it = data.find(steamId);
    // User not identified
    if (it == data.end())
    {
        // Queue user for identification
        if (identify_queue.find(steamId) == identify_queue.end())
            queueUserForIdentification(steamId);
        return nullptr;
    }
    // SteamID belongs to online user
    if (it->second.has_value())
        return &*it->second;
    // SteamID does not belong to online user
    return nullptr;
}

}