#pragma once
#include <string>
#include <utility>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

//Command to process
/// Core Command structure
struct Command {
    //maybe int
    string type;      ///< Event category: "analitics", "gpio", "alarm", etc.
    // int
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

    static Command fromJson(const json& j) {
        Command e;
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
