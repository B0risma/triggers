#include "rule.hpp"
#include "event_system/event.hpp"
#include <exception>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <string>
#include <utility>


const char* Rule::key_delimiter = ":";

using namespace std;
optional<Rule> Rule::fromJson(const json& j) noexcept{
    try{
        optional<Rule> tmp;
        
        const auto r_type = RuleType::_from_string(j.at(Fields::Rule::rule_type).get_ref<const string&>().c_str());
        const auto t_type = TargetType::_from_string(j.at(Fields::Rule::target_type).get_ref<const string&>().c_str());
        const string target = j.value(Fields::Rule::target, "");
        switch(t_type._value){
            case TargetType::Invalid:{
                cout << __PRETTY_FUNCTION__ << "WARN: test only\n";
                tmp.emplace(Rule());
                break;
            }
            
            case TargetType::Alarm: {
                auto a_target = AlarmTarget::_from_string(target.c_str());
                tmp.emplace(AlarmCmd(a_target, r_type));
                break;
            }
            case TargetType::Video: {
                VideoCmd v{};
                v.setPresetOn(j.at("preset_on"));
                v.setPresetOff(j.at("preset_off"));
                tmp.emplace(std::move(v));
                break;
            }
            case TargetType::Analitic: {
                AnaliticTarget a_targ = AnaliticTarget::_from_string(target.c_str());
                tmp.emplace(AnaliticCmd(a_targ));
                break;
            }
            default: 
            {
                throw logic_error("no type");
                break;
            }
        }

        return tmp;
    }
    catch(const std::exception& ex){
        cout << __PRETTY_FUNCTION__ << " " << ex.what() << endl;
        return {};
    }
    return {};
}