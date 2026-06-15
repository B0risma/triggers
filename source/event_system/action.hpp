#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include "json.hpp"
#include "event.hpp"

using json = nlohmann::json;
using namespace std;

/// An Action is a named set of events (commands).
/// When a trigger fires, linked actions are executed — all their events
/// are sent to the EventQueue for processing by targets.
/// Actions ARE editable via webAPI (POST create, DELETE remove).
struct Action {
    string name;            ///< Action identifier (e.g. "act_set2")
    vector<Event> cmds;     ///< Set of events to send when action is triggered

    json toJson() const {
        json j;
        j["name"] = name;
        json cmds_j = json::array();
        for (const auto& cmd : cmds) {
            cmds_j.push_back(cmd.toJson());
        }
        j["cmds"] = cmds_j;
        return j;
    }

    static Action fromJson(const json& j) {
        Action a;
        a.name = j.at("name");
        if (j.contains("cmds") && j["cmds"].is_array()) {
            for (const auto& cmd_j : j["cmds"]) {
                a.cmds.push_back(Event::fromJson(cmd_j));
            }
        }
        return a;
    }
};

/// Trigger-Action link: associates a trigger with an action,
/// optionally with conditions for emitting.
struct TriggerActionLink {
    string trigger;     ///< Trigger name
    string action;      ///< Action name
    string condition;   ///< Optional precondition for emitting

    json toJson() const {
        json j;
        j["trigger"] = trigger;
        j["action"] = action;
        if (!condition.empty()) {
            j["condition"] = condition;
        }
        return j;
    }

    static TriggerActionLink fromJson(const json& j) {
        TriggerActionLink link;
        link.trigger = j.at("trigger");
        link.action = j.at("action");
        link.condition = j.value("condition", "");
        return link;
    }
};

/// ActionList — registry of actions and trigger-action links.
/// Editable via webAPI (POST create action, POST link trigger to action).
class ActionList {
public:
    /// Create a new action. Throws if name already exists.
    void addAction(const Action& action) {
        if (actions_.count(action.name)) {
            throw runtime_error("Action already exists: " + action.name);
        }
        actions_[action.name] = action;
    }

    /// Remove an action by name
    void removeAction(const string& name) {
        actions_.erase(name);
        // Also remove any links referencing this action
        vector<string> links_to_remove;
        for (const auto& [key, link] : links_) {
            if (link.action == name) {
                links_to_remove.push_back(key);
            }
        }
        for (const auto& key : links_to_remove) {
            links_.erase(key);
        }
    }

    /// Find an action by name
    const Action* findAction(const string& name) const {
        auto it = actions_.find(name);
        return it != actions_.end() ? &it->second : nullptr;
    }

    /// Link a trigger to an action
    void addLink(const TriggerActionLink& link) {
        string key = link.trigger + "->" + link.action;
        links_[key] = link;
    }

    /// Remove a trigger-action link
    void removeLink(const string& trigger, const string& action) {
        string key = trigger + "->" + action;
        links_.erase(key);
    }

    /// Get all actions linked to a given trigger name
    vector<const Action*> getActionsForTrigger(const string& trigger_name) const {
        vector<const Action*> result;
        for (const auto& [key, link] : links_) {
            if (link.trigger == trigger_name) {
                auto it = actions_.find(link.action);
                if (it != actions_.end()) {
                    result.push_back(&it->second);
                }
            }
        }
        return result;
    }

    /// Get all actions
    const unordered_map<string, Action>& allActions() const {
        return actions_;
    }

    /// Get all links
    const unordered_map<string, TriggerActionLink>& allLinks() const {
        return links_;
    }

    /// Serialize actions to JSON
    json actionsToJson() const {
        json j;
        for (const auto& [name, action] : actions_) {
            j[name] = action.toJson();
        }
        return j;
    }

    /// Serialize links to JSON
    json linksToJson() const {
        json j = json::array();
        for (const auto& [key, link] : links_) {
            j.push_back(link.toJson());
        }
        return j;
    }

private:
    unordered_map<string, Action> actions_;
    unordered_map<string, TriggerActionLink> links_;
};
