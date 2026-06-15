#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include "json.hpp"
#include "enum.h"

using json = nlohmann::json;
using namespace std;


BETTER_ENUM(EventType, uint8_t, NoType=0, GPIO=1, VirtSwitch, Analityc, Shedule);


/// Core event data structure.
/// Matches API2.json format: {"type":"analitics", "source":"fire", "data":{...}}
/// Event is also a "command" — data for processing an action.
struct Event {
    //maybe int
    EventType type = EventType::NoType;      ///< Event category: "analitics", "gpio", "alarm", etc.
    // int
    string source;   ///< Event specific kind: "fire", "in1", "light", etc.
    json data;        ///< Additional payload parameters

    string toString() const {
        return string(type._to_string()) + "|" + source + "|" + data.dump();
    }

    json toJson() const {
        return json{
            {"type", type._to_string()},
            {"source", source},
            {"data", data}
        };
    }

    static Event fromJson(const json& j) {
        Event e;
        e.type._from_string_nothrow(string(j.at("type")).c_str());
        e.source = j.value("source", "");
        e.data = j.value("data", json::object());
        return e;
    }

    /// Composite key for routing: type+source
    string key() const {
        return string(type._to_string()) + ":" + source;
    }
};

BETTER_ENUM(GPIO_edge, uint8_t, Falling = 0,Rising = 1);

struct GPIOEvent : public Event{
    GPIOEvent(const string gpio_num, GPIO_edge edge = GPIO_edge::Falling)
    /*: edge_(edge)*/{
        type = EventType::GPIO;
        source = gpio_num;
        data["edge"] = edge._to_string();
    }
    // GPIO_edge edge_ = GPIO_edge::Falling;
};

struct VswitchEvent : public Event{
    VswitchEvent(const string gpio_num, GPIO_edge edge = GPIO_edge::Falling){
        type = EventType::VirtSwitch;
        source = gpio_num;
        data["edge"] = edge._to_string();
    }
};


BETTER_ENUM(DetectorType, uint8_t, Fire, Smoke, Weapon, Sabotage, Cross_line); //....
BETTER_ENUM(AlarmEdge, uint8_t, Start = 1, End = 0);
struct AnaliticEvent : public Event{
    AnaliticEvent(const DetectorType detector, AlarmEdge started = AlarmEdge::End){
        type = EventType::Analityc;
        source = detector._to_string();
        data["alarmEdge"] = started._to_string();
    }
};




