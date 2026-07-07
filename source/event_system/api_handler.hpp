#pragma once
#include <string>
#include <memory>
#include "httplib.h"
#include "json.hpp"
#include "event.hpp"
#include "trigger.hpp"
#include "action.hpp"
#include "event_queue.hpp"
#include "trigger_sample.hpp"

using json = nlohmann::json;
using namespace std;

/// APIHandler — handles webAPI requests for the event system.
/// Based on API2.json:
///   GET  /trigger     → trigger list by kinds (read-only, not editable via API)
///   GET  /action      → list all actions
///   POST /action      → create action with cmds
///   DELETE /action    → delete action by name
///   GET  /link        → list trigger-action links
///   POST /link        → link trigger with action (with optional condition)
///   DELETE /link      → unlink trigger from action
class APIHandler {
public:
    APIHandler() = default;

    /// Set the system registries that this handler will query/modify
    void setRegistries(
        shared_ptr<TriggerList> triggers,
        shared_ptr<ActionList> actions,
        shared_ptr<EventQueue> queue,
        shared_ptr<VswitchList> switches
    ) {
        triggers_ = triggers;
        actions_ = actions;
        queue_ = queue;
        switches_ = switches;
    }

    /// Register all API routes on the httplib server
    void registerRoutes(httplib::Server& srv);

    // --- Route handlers ---
    void handleTriggerList(const httplib::Request& req, httplib::Response& res);
    void handleSwitchList(const  httplib::Request& req, httplib::Response& res);
    void handleActionList(const httplib::Request& req, httplib::Response& res);
    void handleActionCreate(const httplib::Request& req, httplib::Response& res);
    void handleActionDelete(const httplib::Request& req, httplib::Response& res);
    void handleLinkList(const httplib::Request& req, httplib::Response& res);
    void handleLinkCreate(const httplib::Request& req, httplib::Response& res);
    void handleLinkDelete(const httplib::Request& req, httplib::Response& res);

    // URI constants
    static constexpr auto trigger_URI = "/trigger";
    static constexpr auto action_URI = "/action";
    static constexpr auto link_URI = "/link";
    static constexpr auto switch_URI = "/switch";

private:
    shared_ptr<TriggerList> triggers_;
    shared_ptr<ActionList> actions_;
    shared_ptr<EventQueue> queue_;
    shared_ptr<VswitchList> switches_;
    static constexpr auto json_content = "text/json";
};
