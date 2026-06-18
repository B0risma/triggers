#pragma once
#include <string>
#include <set>
#include <unordered_map>
#include <memory>
#include <functional>
#include <iostream>
#include "event_system/command.hpp"
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

/// Base class for all targets.
/// A Target is any object that can handle events (commands), process data.
/// Targets are registered in TargetList and subscribe to event types they support.
class Target {
public:
    string name;                        ///< Target identifier (e.g. "fire_detector_1")
    string type_name;                   ///< Target type (e.g. "fire_detector")
    set<string> supported_cmds;       ///< Set of event keys ("analitics:fire", etc.)

    virtual ~Target() = default;

    /// Process an incoming event. Override in derived classes.
    virtual void procEvent(Command evn) {
        cout << name << ": unhandled event " << evn.first.toString() << endl;
    }

    /// Check if this target can handle a given event key
    bool canHandle(const string& cmd_key) const {
        return supported_cmds.count(cmd_key) > 0;
    }

    /// Serialize target info to JSON (for listing)
    virtual json toJson() const {
        json j;
        j["name"] = name;
        j["type"] = type_name;
        j["supported_events"] = supported_cmds;
        return j;
    }
};

/// TargetList — registry of all targets in the system.
/// CANNOT be edited from webAPI — only system-editable.
/// Targets are added/removed programmatically by the system.
class TargetList {
public:
    using Ptr = shared_ptr<Target>;

    /// Add a target to the registry
    void add(Ptr target) {
        if (!target) return;
        targets_[target->name] = target;
    }

    /// Remove a target by name
    void remove(const string& name) {
        targets_.erase(name);
    }

    /// Find a target by name
    Ptr find(const string& name) const {
        auto it = targets_.find(name);
        return it != targets_.end() ? it->second : nullptr;
    }

    /// Get all targets that can handle a specific event key
    vector<Ptr> findByEvent(const string& event_key) const {
        vector<Ptr> result;
        for (const auto& [_, tgt] : targets_) {
            if (tgt->canHandle(event_key)) {
                result.push_back(tgt);
            }
        }
        return result;
    }

    /// Get all targets
    const unordered_map<string, Ptr>& all() const {
        return targets_;
    }

    /// Serialize to JSON (for GET response)
    json toJson() const {
        json j = json::array();
        for (const auto& [name, tgt] : targets_) {
            j.push_back(tgt->toJson());
        }
        return j;
    }

private:
    unordered_map<string, Ptr> targets_;
};
