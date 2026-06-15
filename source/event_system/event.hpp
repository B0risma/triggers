#pragma once
#include <string>
#include <utility>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

/// Core event data structure.
/// Matches API2.json format: {"type":"analitics", "subtype":"fire", "data":{...}}
/// Event is also a "command" — data for processing an action.
struct Event {
    string type;      ///< Event category: "analitics", "gpio", "alarm", etc.
    string subtype;   ///< Event specific kind: "fire", "in1", "light", etc.
    json data;        ///< Additional payload parameters

    string toString() const {
        return type + "|" + subtype + "|" + data.dump();
    }

    json toJson() const {
        return json{
            {"type", type},
            {"subtype", subtype},
            {"data", data}
        };
    }

    static Event fromJson(const json& j) {
        Event e;
        e.type = j.at("type");
        e.subtype = j.value("subtype", "");
        e.data = j.value("data", json::object());
        return e;
    }

    /// Composite key for routing: type+subtype
    string key() const {
        return type + ":" + subtype;
    }
};
