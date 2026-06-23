#pragma once
#include <string>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include "json.hpp"
#include "event.hpp"

using json = nlohmann::json;
using namespace std;

/// Forward declaration
class EventQueue;

/// Base class for all triggers.
/// A Trigger is an event source — input that emits a signal on external events.
/// Triggers are NOT editable by webAPI.
class Trigger {
public:
    string name;        ///< Trigger identifier (e.g. "gpio_in1")
    EventType evn_type = EventType::NoType;        ///< Trigger kind/category (e.g. "gpio", "analitics", "shedule")

    virtual ~Trigger() = default;

    /// Set the event queue this trigger will send events to
    void setEventQueue(shared_ptr<EventQueue> q);

    /// Emit an event into the event queue. Override to define trigger behavior.
    void emitEvent(Event&& evn) const;
    inline void emitEvent(const Event& evn) const{
        Event cp = evn;
        emitEvent(std::move(cp));
    }

    /// Get the trigger's static info (kind + available subtypes)
    /// Used for the GET trigger list API response
    // virtual json getDescriptor() const {
    //     return json{
    //         {"name", name},
    //         {"kind", kind}
    //     };
    // }
    string evnKey() const{
        return evn_type._to_string() +":"s+name;
    }
protected:
    weak_ptr<EventQueue> e_queue;
};

/// TriggerList — registry of all triggers in the system.
/// CANNOT be edited by webAPI — only system-editable.
/// Provides trigger kind listing (matches API2.json GET trigger list).
class TriggerList {
public:
    using Ptr = shared_ptr<Trigger>;

    /// Add a trigger to the registry
    void add(Ptr trigger) {
        if (!trigger) return;
        triggers_[trigger->name] = trigger;
        by_kind_[trigger->evn_type._to_string()].push_back(trigger);
    }

    /// Remove a trigger by name
    void remove(const string& name) {
        auto it = triggers_.find(name);
        if (it != triggers_.end()) {
            auto& list = by_kind_[it->second->evn_type._to_string()];
            std::erase(list, it->second);
            triggers_.erase(it);
        }
    }

    /// Find a trigger by name
    Ptr find(const string& name) const {
        auto it = triggers_.find(name);
        return it != triggers_.end() ? it->second : nullptr;
    }

    /// Get triggers by kind
    const vector<Ptr>& byKind(const string& kind) const {
        static const vector<Ptr> empty;
        auto it = by_kind_.find(kind);
        return it != by_kind_.end() ? it->second : empty;
    }

    /// Get all triggers
    const unordered_map<string, Ptr>& all() const {
        return triggers_;
    }

    /// Build the trigger list by kinds — matches API2.json GET format:
    /// {"analitics":["fire","smoke","cross"], "gpio":["in1","in2"], ...}
    json toKindListJson() const {
        json j;
        for (const auto& [kind, trigs] : by_kind_) {
            json subtypes = json::array();
            for (const auto& t : trigs) {
                subtypes.push_back(json{{"name",t->name}, {"event", t->evnKey()}});
            }
            j[kind] = subtypes;
        }
        return j;
    }

private:
    unordered_map<string, Ptr> triggers_;
    unordered_map<string, vector<Ptr>> by_kind_;
};
