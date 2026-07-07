#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include "json.hpp"
#include "enum.h"

using json = nlohmann::json;
using namespace std;


BETTER_ENUM(EventType, uint8_t, NoType=0, GPIO=1, VirtSwitch, Analitic, Shedule);


/// Core event data structure.
/// Event is also a "command" — data for processing an action.
struct Signal {
    //maybe int
    EventType type = EventType::NoType;      ///< Event category: "analitics", "gpio", "alarm", etc.
    // int
    
    json data;        ///< Additional payload parameters
    explicit Signal(const string &src_key_):src_key(src_key_){}
    static Signal dummyEvent(){return Signal("invalid");}
    string toString() const {
        return src_key + "|" + data.dump();
    }

    // json toJson() const {
    //     return json{
    //         {"type", type._to_string()},
    //         {"source", src_key},
    //         {"data", data}
    //     };
    // }

    string key() const {
        return src_key;
    }
    private:
    string src_key;   ///< Event specific kind: "fire", "in1", "light", etc.
};

// event occurs only when state changed
struct GPIOEvent : public Signal{
    GPIOEvent(const string &src_key_,  bool new_state) : Signal(src_key_)
    /*: edge_(edge)*/{
        type = EventType::GPIO;
        data["state"] = new_state;
    }
    // GPIO_edge edge_ = GPIO_edge::Falling;
};

struct VswitchEvent : public Signal{
    VswitchEvent(const string &src_key_,  bool new_state): Signal(src_key_){
        type = EventType::VirtSwitch;
        data["state"] = new_state;
    }
};


BETTER_ENUM(DetectorType, uint8_t, Fire, Smoke, Weapon, Sabotage, Cross_line); //....
BETTER_ENUM(AlarmEdge, uint8_t, Start = 1, End = 0);
struct AnaliticEvent : public Signal{
    AnaliticEvent(const DetectorType detector, AlarmEdge started = AlarmEdge::End) : Signal(detector._to_string()){
        type = EventType::Analitic;
        data["alarmEdge"] = started._to_string();
    }
};




