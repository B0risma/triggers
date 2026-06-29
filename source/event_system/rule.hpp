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

BETTER_ENUM(TargetType, uint8_t, Invalid = 0, Video, Analitic, Alarm); //.....
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
//! As like as Command temmplate - rule for executing something
struct Rule {
    static constexpr const string key_delimiter = ":";

    struct Fields {
        static constexpr auto rule_type = "rule_type";
        static constexpr auto target_type = "target_type";
        static constexpr auto target = "target";
    };


    RuleType type = RuleType::Toggle;      ///< Cmd type for target routing
    TargetType target_type = TargetType::Invalid;   ///< addition field for routing - not used now
    string target = {}; //empty by default
    json data;        ///< type specific data arguments: detector name for Analitics, preset for Video ...

    string toString() const {
        return  target_type._to_string() + key_delimiter + 
                target + key_delimiter + 
                string(type._to_string()) + 
                key_delimiter + data.dump();
    }
    /*
    standart "data" fields:
    type - cmd type(mode/what to do): toggle, single shot, preset....
    target_type - for what handler
    Target - detailed handler

    */
    json toJson() const {
        json tmp{
            {Fields::rule_type, type._to_string()},
            {Fields::target, target},
            {Fields::target_type, target_type._to_string()}
        };
        tmp.update(data, true);
        return tmp;
    }

    static std::optional<Rule> fromJson(const json& j) noexcept;

    /// Composite key for routing: type+target
    string key() const {
        return ruleKey(type, target_type, target);
    }
    static string ruleKey(const RuleType& type, const TargetType& target_type, const string & target = {}){
        return target_type._to_string() + key_delimiter + 
                target + key_delimiter +
                type._to_string();
    }
};



BETTER_ENUM(AnaliticTarget, uint8_t, All = 0, Fire, Weapon);///....
struct AnaliticCmd final : public Rule{
    AnaliticCmd(AnaliticTarget detector){
        type = RuleType::Toggle;
        target_type = TargetType::Analitic;
        target = detector._to_string();
    }

    string detector() const{
        return target;
    }
};

struct VideoCmd final : public Rule{
    VideoCmd(){
        type = RuleType::Preset;
        target = TargetType::Video;
        // data["subtype"] = "Preset"; //?!
        data["preset_on"] = "preset1";
        data["preset_off"] = "preset2";
    }
    inline void setPresetOn(const string &preset_name){
        data["preset_on"] = preset_name;
    }
    inline void setPresetOff(const string &preset_name){
        data["preset_off"] = preset_name;
    }
};


BETTER_ENUM(AlarmTarget, uint8_t, Invalid = 0, Sound, WhiteLight, RedBlue);///....
//single shot only
struct AlarmCmd final : public Rule{
    AlarmCmd(const AlarmTarget alarm_type, RuleType type_){
        type = type_;
        if(type._value == RuleType::Toggle){
            if(alarm_type == (+AlarmTarget::Sound)) throw logic_error("Sound cmd is single-shot only");
        }
        target = alarm_type._to_string();
    }
};



/*
standart range struct for Rules
{
  "TargetType" : {
    "RuleType1" : {
      "field1" : "type",
      "field2" : "type"
    }
  },
  "TargetType2" : {},
  "target" : [],
  "type" : []
}
...
}
*/
    // get range for command and command types
    struct RuleRange{
        // cmd type
        static json getTypes(){
            json ret;
            

            // enumerate type args: TargetType -> RuleType -> fields
            // Analitic: only Toggle, field = detector
            {
                json toggle_fields;
                toggle_fields["detector"] = json::array({"Fire", "Weapon"});
                ret[(+TargetType::Analitic)._to_string()][(+RuleType::Toggle)._to_string()] = toggle_fields;
            }

            // Video: only Preset, fields = preset_on, preset_off (string)
            {
                json preset_fields;
                preset_fields["preset_on"] = "string";
                preset_fields["preset_off"] = "string";
                ret[(+TargetType::Video)._to_string()][(+RuleType::Preset)._to_string()] = preset_fields;
            }

            // Alarm: Toggle (target: WhiteLight, RedBlue) and OneShot (target: WhiteLight, RedBlue, Sound)
            {
                json toggle_fields;
                toggle_fields["target"] = json::array({"WhiteLight", "RedBlue"});
                ret[(+TargetType::Alarm)._to_string()][(+RuleType::Toggle)._to_string()] = toggle_fields;

                json oneshot_fields;
                oneshot_fields["target"] = json::array({"WhiteLight", "RedBlue", "Sound"});
                ret[(+TargetType::Alarm)._to_string()][(+RuleType::OneShot)._to_string()] = oneshot_fields;
            }


            // target enum array
            {
                auto arr = json::array();
                for(const auto t : TargetType::_values()){
                    arr.push_back(t._to_string());
                }
                ret["target"] = arr;
            }

            // type enum array
            {
                auto arr = json::array();
                for(const auto t : RuleType::_values()){
                    arr.push_back(t._to_string());
                }
                ret["type"] = arr;
            }

            return ret;
        }
    };

using Command = std::pair<Rule, Event>;
