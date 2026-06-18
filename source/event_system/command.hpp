#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include "json.hpp"
#include "enum.h"

using json = nlohmann::json;
using namespace std;

BETTER_ENUM(CommandType, uint8_t, Invalid = 0, Analitic, Video, Alarm); //.....
//Command to process
/// Core Command structure

/* cmd json
{
"type" : X,
"target" : X, //if needed
...// fields from data
...
}
*/
struct Command {
    CommandType type = CommandType::Invalid;      ///< Cmd type for target routing
    string target;   ///< addition field for routing - not used now
    json data;        ///< type specific data arguments: detector name for Analitics, preset for Video ...

    string toString() const {
        return string(type._to_string()) + "|" + target + "|" + data.dump();
    }

    json toJson() const {
        json tmp{
            {"type", type._to_string()},
            {"target", target},
        };
        tmp.update(data, true);
        return tmp;
    }

    static std::optional<Command> fromJson(const json& j) noexcept;

    /// Composite key for routing: type+target
    string key() const {
        return type._to_string() + string(target.empty() ? "" : (":" + target));
    }
};

/*
standart "data" fields:
subtype - cmd type(mode/what to do): toggle, single shot, preset....
state - for toggle command (boolean)
timeout - for single-shots (not implemented now)

*/

BETTER_ENUM(AnaliticCmdType, uint8_t, toggle);
struct AnaliticCmd : public Command{
    AnaliticCmd(string detector,  bool state = true){
        type = CommandType::Analitic;
        data["subtype"] = AnaliticCmdType(AnaliticCmdType::toggle)._to_string();
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

struct VideoCmd final : public Command{
    VideoCmd(){
        type = CommandType::Video;
        data["subtype"] = "Preset"; //?!
        data["preset_on"] = "preset1";
        data["preset_off"] = "preset2";
    }
};

BETTER_ENUM(AlarmCmdType, uint8_t, WhiteLight, RedBlue, Sound);//.....
BETTER_ENUM(AlarmCmdSubtype, uint8_t, Toggle, SingleShot);
//single shot only
struct AlarmCmd final : public Command{
    AlarmCmd(const AlarmCmdType alarm_type, const AlarmCmdSubtype subtype, bool state = false){
        type = CommandType::Alarm;
        target = alarm_type._to_string(); // rebblue, sound, ....
        data["subtype"] = subtype._to_string();
        if(subtype._value == AlarmCmdSubtype::Toggle){
            if(type._value == AlarmCmdType::Sound) throw logic_error("Sound cmd is single-shot only");
            data["state"] = state;
        }
    }
};



/*
standart range struct for CmdType
{
 "fields_type" : {}, //see below
 "types" : [], // command types list
"CommandType1" : { //command type range 
    "subtype" : [toggle,singleshot, preset...],
    "target" : [x, A, B], // only if needed
    "fields" : ["detector", "state"]
}
...
}

standart field types
{//json root 
    "field_types" : {
        "type" : "string",
        "subtype" : "string",
        "state" : "boolean",
        "preset_on" : "string"
        "preset_off" : "string",
        "timeout" : "int",
        "target" : "string"
    }
}
*/
// get range for command and command types
struct CommandRange{
    // cmd type
    static json getTypes(){
        json ret;
        {
            auto types = json::array();
            for(const auto t : CommandType::_values()){
                types.push_back(t._to_string());
            }
            ret["type"] = types;
        }

        // enumerate type args
        ret[(+CommandType::Analitic)._to_string()] = {
            {"subtype",{(+AnaliticCmdType::toggle)._to_string()}},
            {"fields", {"detector", "state"}
            },
        };

        ret[(+CommandType::Video)._to_string()] = {
            {"fields",{"preset_on", "preset_off"}}
        };
        
        {//alarm cmd
            json tmp;
            {
                json tmp_array = json::array();
                for(const auto st : AlarmCmdType::_values()){
                    tmp_array.push_back(st._to_string());
                }
                tmp["target"] = tmp_array;
            }

            {
                json tmp_array = json::array();
                for(const auto st : AlarmCmdSubtype::_values()){
                    tmp_array.push_back(st._to_string());
                }
                tmp["subtype"] = tmp_array;
            }
            tmp["fields"] = {
                {(+AlarmCmdSubtype::Toggle)._to_string(), {"state"}}, 
                {(+AlarmCmdSubtype::SingleShot)._to_string(), {"timeout"}}
            };

            ret[(+CommandType::Alarm)._to_string()] = tmp;
        }
        return ret;
    }
};

