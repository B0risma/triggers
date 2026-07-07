#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include "json.hpp"
#include "event.hpp"
#include "rule.hpp"
#include "json_fields.h"
// #include "v2.h"

using json = nlohmann::json;
using namespace std;

/// An Action is a named set of events (commands).
/// When a trigger fires, linked actions are executed — all their events
/// are sent to the EventQueue for processing by targets.
/// Actions ARE editable via webAPI (POST create, DELETE remove).
struct Action {
    string name;            ///< Action identifier (e.g. "act_set2")
    vector<Rule> rules;     ///< Set of events to send when action is triggered

    json toJson() const {
        json j;
        j[Fields::Action::name] = name;
        json cmds_j = json::array();
        for (const auto& cmd : rules) {
            cmds_j.push_back(cmd.toJson());
        }
        j[Fields::Action::rules] = cmds_j;
        return j;
    }

    static Action fromJson(const json& j) noexcept(false){
        Action a;
        a.name = j.at("name");
        const auto& rules = j.at(Fields::Action::rules);
        if(!rules.is_array() || rules.empty()) throw logic_error("empty rules");
        for (const auto& cmd_j : rules) {
            a.rules.push_back(Rule::fromJson(cmd_j).value());
        }
        return a;
    }
};

 


/// Trigger-Action link: associates a trigger with an action,
/// optionally with conditions for emitting.
// bind trigger events to actions
struct EventActionLink {
    string evn_key;     ///< Trigger name
    string action;      ///< Action name
    // for what?
    // string condition;   ///< Optional precondition for emitting moved into target as Event arguments

    json toJson() const {
        json j;
        j[Fields::Link::evn_key] = evn_key;
        j[Fields::Link::action] = action;
        // if (!condition.empty()) {
            // j["condition"] = condition;
        // }
        return j;
    }

    static EventActionLink fromJson(const json& j) {
        EventActionLink link;
        link.evn_key = j.at(Fields::Link::evn_key);
        link.action = j.at(Fields::Link::action);
        // link.condition = j.value("condition", "");
        return link;
    }
    static string linkKey(const string& evn_key, const string& action){
        return evn_key + "->"s + action;
    }
    string key() const{
        return linkKey(evn_key, action);
    }
};

/// ActionList — registry of actions and trigger-action links.
/// Editable via webAPI (POST create action, POST link trigger to action).
class ActionList {
public:
    using link_key_t = string;
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
        for (const auto& pair : links_) {
            const auto & key = pair.first;
            const auto & link = pair.second;
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
    void addLink(const EventActionLink& link) {
        // string key = ;
        links_[link.key()] = link;
    }

    /// Remove a trigger-action link
    void removeLink(const string& evn_key, const string& action) {
        string key = EventActionLink::linkKey(evn_key,action);
        links_.erase(key);
    }

    vector<const Action*> getActionsForEvn(const Signal& evn){
        vector<const Action *> res;
        const auto evn_key = evn.key();
        for (const auto& pair : links_) {
            const auto & key = pair.first;
            const auto & link = pair.second;
            if (link.evn_key == evn_key) {
                auto it = actions_.find(link.action);
                if (it != actions_.end()) {
                    res.push_back(&it->second);
                }
            }
        }
        if(res.empty()) cout << __PRETTY_FUNCTION__ << " DBG: " << "no actions for " << evn.toString() << endl;
        return res;
    }


    /// Get all actions
    const unordered_map<string, Action>& allActions() const {
        return actions_;
    }

    /// Get all links
    const unordered_map<string, EventActionLink>& allLinks() const {
        return links_;
    }

    /// Serialize actions to JSON
    json actionsToJson() const {
        json j;
        for (const auto& pair : actions_) {
            const auto & name = pair.first;
            const auto & action = pair.second;
            j[name] = action.toJson();
        }
        return j;
    }

    /// Serialize links to JSON
    json linksToJson() const {
        json j = json::array();
        for (const auto& link : links_) {
            j.push_back(link.second.toJson());
        }
        return j;
    }

private:
    unordered_map<string, Action> actions_; //all actions
    unordered_map<link_key_t, EventActionLink> links_; // store actions for targets // by event keys
};
