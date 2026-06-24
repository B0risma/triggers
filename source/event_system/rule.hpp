#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include "json.hpp"
#include "enum.h"
#include "event.hpp"

using json = nlohmann::json;
using namespace std;

BETTER_ENUM(TargetType, uint8_t, Invalid = 0, Video, Analitic, AnaliticFire, AnaliticWeapon, Alarm, WhiteLight, RedBlue, Sound); //.....
BETTER_ENUM(RuleType, uint8_t, Toggle, OneShot, Number, Preset);//...
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
#pragma message("these are cmd TEMPLATES!!! not ready to invoke cmd - no data!!!")
struct Rule {
    RuleType type = RuleType::Toggle;      ///< Cmd type for target routing
    TargetType target = TargetType::Invalid;   ///< addition field for routing - not used now
    json data;        ///< type specific data arguments: detector name for Analitics, preset for Video ...

    string toString() const {
        return  target._to_string() + "|"s + string(type._to_string()) + "|" + data.dump();
    }

    json toJson() const {
        json tmp{
            {"type", type._to_string()},
            {"target", target._to_string()},
        };
        tmp.update(data, true);
        return tmp;
    }

    static std::optional<Rule> fromJson(const json& j) noexcept;

    /// Composite key for routing: type+target
    string key() const {
        return ruleKey(type, target);
    }
    static string ruleKey(const RuleType& type, const TargetType& target){
        return target._to_string() + ""s + type._to_string();
    }
};

/*
standart "data" fields:
subtype - cmd type(mode/what to do): toggle, single shot, preset....
state - for toggle command (boolean)
timeout - for single-shots (not implemented now)

*/

// BETTER_ENUM(AnaliticCmdType, uint8_t, toggle);
struct AnaliticCmd : public Rule{
    AnaliticCmd(TargetType targ, string detector = {}){
        type = RuleType::Toggle;
        // data["subtype"] = AnaliticCmdType(AnaliticCmdType::toggle)._to_string();
        if(!detector.empty())
            data["detector"] = detector;
        target = targ;
        // data["state"] = state;
    }

    string detector() const{
        return data.value("detector", "");
    }
    // AnaliticCmdType subtype() const{
    //     return AnaliticCmdType::_from_string(data.at("subtype").get_ref<const string&>().c_str());
    // }

    // bool fillEventData(const Event &evn){
    //     // auto s_type = AnaliticCmdType::_from_string(data.at("subtype").get_ref<const string&>().c_str())._value;
    //     // if(s_type == AnaliticCmdType::toggle){
    //     //     data["state"] = evn.data.at("state");
    //     // }
    //     // else if(false){
    //     //     some types
    //     // }
    //     // else return false;

    //     return true;
    // }
};

struct VideoCmd final : public Rule{
    VideoCmd(){
        type = RuleType::Preset;
        target = TargetType::Video;
        // data["subtype"] = "Preset"; //?!
        data["preset_on"] = "preset1";
        data["preset_off"] = "preset2";
    }
    // bool fillEventData(const Event &evn){
    //     if(data.at("subtype") == "Preset"){
    //         data["preset"] = evn.data.at("state") ? data.at("preset_on") : data.at("preset_off");
    //     }
    //     // else if(false){
    //     //     some types
    //     // }
    //     else return false;

    //     return true;
    // }
};

// BETTER_ENUM(AlarmCmdType, uint8_t, WhiteLight, RedBlue, Sound);//.....
// BETTER_ENUM(AlarmCmdSubtype, uint8_t, Toggle, SingleShot);
//single shot only
struct AlarmCmd final : public Rule{
    AlarmCmd(const TargetType alarm_type, RuleType type_){
        type = type_;
        // target = alarm_type._to_string(); // rebblue, sound, ....
        // data["subtype"] = subtype._to_string();
        if(type._value == RuleType::Toggle){
            if(type._value == TargetType::Sound) throw logic_error("Sound cmd is single-shot only");
            // data["state"] = state;
        }
    }

    // bool fillEventData(const Event &evn){
    //     auto s_type = AnaliticCmdType::_from_string(data.at("subtype").get_ref<const string&>().c_str())._value;
    //     // if(s_type == AnaliticCmdType::toggle){
    //     //     data["state"] = evn.data.at("state");
    //     // }
    //     // // else if(false){
    //     // //     some types
    //     // // }
    //     // else return false;

    //     return true;
    // }
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
        "target" : "string",
        NO "subtype" : "string",
        NO "state" : "boolean",
        "preset_on" : "string"
        "preset_off" : "string",
        "timeout" : "int",
    }
}
*/
// get range for command and command types
struct RuleRange{
    // cmd type
    static json getTypes(){
        json ret;
        {
            auto types = json::array();
            for(const auto t : RuleType::_values()){
                types.push_back(t._to_string());
            }
            ret["type"] = types;

            types.clear();
            for(const auto t : TargetType::_values()){
                types.push_back(t._to_string());
            }
            ret["target"] = types;
        }

        // enumerate type args
        ret[(+TargetType::Analitic)._to_string()] = {
            {"type",{(+RuleType::Toggle)._to_string()}},
            {"fields", {"detector"}
            },
        };

        ret[(+TargetType::Video)._to_string()] = {
            {{"type", (+RuleType::Preset)._to_string()}},
            {"fields",{"preset_on", "preset_off"}}
        };
        
        ret[(+TargetType::WhiteLight)._to_string()] = {
            {{"type", {(+RuleType::OneShot)._to_string(), (+RuleType::Toggle)._to_string(),}}},
            {"fields",{"preset_on", "preset_off"}}
        };

        // {//alarm cmd
        //     json tmp;
            

        //     {
        //         json tmp_array = json::array();
        //         for(const auto st : AlarmCmdSubtype::_values()){
        //             tmp_array.push_back(st._to_string());
        //         }
        //         tmp["subtype"] = tmp_array;
        //     }
        //     tmp["fields"] = {
        //         {(+AlarmCmdSubtype::Toggle)._to_string(), {}}, 
        //         {(+AlarmCmdSubtype::SingleShot)._to_string(), {"timeout"}}
        //     };

        //     ret[(+RuleType::Alarm)._to_string()] = tmp;
        // }
        return ret;
    }
};

using Command = std::pair<Rule, Event>;
