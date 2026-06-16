#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include "json.hpp"
#include "enum.h"

using json = nlohmann::json;
using namespace std;

BETTER_ENUM(CommandType, uint8_t, Invalid = 0, Analitic, Video, Alarm, Light); //.....

//Command to process
/// Core Command structure
struct Command {
    //maybe int
    CommandType type = CommandType::Invalid;      ///< Event category: "analitics", "gpio", "alarm", etc.
    // int
    string subtype;   ///< Event specific kind: "fire", "in1", "light", etc.
    json data;        ///< Additional payload parameters

    string toString() const {
        return string(type._to_string()) + "|" + subtype + "|" + data.dump();
    }

    json toJson() const {
        return json{
            {"type", type._to_string()},
            {"subtype", subtype},
            {"data", data}
        };
    }

    static Command fromJson(const json& j) {
        Command e;
        e.type._from_string_nothrow(j.at("type").get_ref<const string&>().c_str());
        e.subtype = j.value("subtype", "");
        e.data = j.value("data", json::object());
        return e;
    }

    /// Composite key for routing: type+subtype
    string key() const {
        return string(type._to_string()) + ":" + subtype;
    }
};

BETTER_ENUM(AnaliticCmdType, uint8_t, toggle);

struct AnaliticCmd : public Command{
    AnaliticCmd(string detector,  bool state = true){
        type = CommandType::Analitic;
        subtype = AnaliticCmdType(AnaliticCmdType::toggle)._to_string();
        data["detector"] = detector;
        data["state"] = state;
    }

    string detector() const{
        return data.value("detector", "");
    }
    std::optional<bool> state() const{
        if(!data.count("state")) return {};
        return data.value("state", false);
    }
};

struct VideoCmd : public Command{
    VideoCmd(){
        type = CommandType::Video;
        subtype = "Preset"; //?!
        data["preset_on"] = "preset1";
        data["preset_off"] = "preset2";
    }
};

BETTER_ENUM(AlarmCmdType, uint8_t, WhiteLight, RedBlue, Sound);//.....
//single shoit only
struct AlarmCmd : public Command{
    AlarmCmd(const AlarmCmdType &alarm_type/*, bool state = false*/){
        type = CommandType::Alarm;
        subtype = alarm_type._to_string(); // rebblue, sound, ....
        // data["state"] = state;
    }
};

BETTER_ENUM(LightType, uint8_t, WhiteLight, Redblue);
struct LightCmd : public Command{    
    LightCmd(LightType light_type, bool state = false ){
        type = CommandType::Light;
        subtype = light_type._to_string();
        data["state"] = state;
    }
};
