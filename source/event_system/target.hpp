#pragma once
#include <string>
#include <set>
#include <unordered_map>
#include <memory>
#include <functional>
#include <iostream>
#include "rule.hpp"
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

/// Base class for all targets.
/// A Target is any object that can handle events (commands), process data.
/// Targets are registered in TargetList and subscribe to event types they support.
class Target {
public:
    using Ptr = shared_ptr<Target>;
    struct Fields{
        static constexpr auto name = "name";
        static constexpr auto suppported_rule = "supported_rules";
    };

    string name;                        ///< Target identifier (e.g. "fire_detector_1")
    // string type_name;                   ///< Target type (e.g. "fire_detector")
    set<string> supported_rules;       ///< Set of event keys ("analitics:fire", etc.)

    virtual ~Target() = default;

    /// Process an incoming event. Override in derived classes.
    virtual void procEvent(const Command &evn) {
        cout << name << ": handling event " << evn.first.toString() << endl;
    }

    /// Check if this target can handle a given event key
    bool canHandle(const string& rule_key) const {
        return supported_rules.count(rule_key) > 0;
    }

    /// Serialize target info to JSON (for listing)
    virtual json toJson() const {
        json j;
        j[Fields::name] = name;
        // j["type"] = type_name;
        j[Fields::suppported_rule] = supported_rules;
        return j;
    }
};

/// TargetList — registry of all targets in the system.
/// CANNOT be edited from webAPI — only system-editable.
/// Targets are added/removed programmatically by the system.
class TargetList {
public:
    /// Add a target to the registry
    void add(Target::Ptr target) {
        if (!target) return;
        targets_[target->name] = target;
    }

    /// Remove a target by name
    void remove(const string& name) {
        targets_.erase(name);
    }

    /// Find a target by name
    Target::Ptr find(const string& name) const {
        auto it = targets_.find(name);
        return it != targets_.end() ? it->second : nullptr;
    }

    /// Get all targets that can handle a specific event key
    vector<Target::Ptr> findByRule(const string& rule_key) const {
        vector<Target::Ptr> result;
        for (const auto& tgt : targets_) {
            if (tgt.second->canHandle(rule_key)) {
                result.push_back(tgt.second);
            }
        }
        return result;
    }

    /// Get all targets
    const unordered_map<string, Target::Ptr>& all() const {
        return targets_;
    }

    /// Serialize to JSON (for GET response)
    json toJson() const {
        json j = json::array();
        for (const auto& tgt : targets_) {
            j.push_back(tgt.second->toJson());
        }
        return j;
    }

protected:
    unordered_map<string, Target::Ptr> targets_;
};
