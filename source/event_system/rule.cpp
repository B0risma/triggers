#include "rule.hpp"
#include "event_system/event.hpp"
#include <exception>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <string>
#include <utility>

using namespace std;
std::optional<Rule> Rule::fromJson(const json& j) noexcept{
    try{
        optional<Rule> tmp;
        
        const auto type = RuleType::_from_string(j.at("type").get_ref<const string&>().c_str());
        const auto target = TargetType::_from_string(j.at("target").get_ref<const string&>().c_str());
        switch(target._value){
            case TargetType::Invalid:{
                cout << __PRETTY_FUNCTION__ << "WARN: test only\n";
                tmp.emplace(Rule());
                break;
            }
            
            case TargetType::Alarm: {
                // auto s_type = AlarmCmdSubtype::_from_string(j.at("subtype").get_ref<const string&>().c_str());
                tmp.emplace(AlarmCmd(target, type));
                break;
            }
            case TargetType::Video: {
                tmp.emplace(VideoCmd());
                tmp->data["preset_on"] = j.at("preset_on");
                tmp->data["preset_off"] = j.at("preset_off");
                break;
            }
            default: 
            {
                if(target >= (+TargetType::Analitic) && target <= (+TargetType::AnaliticFire)) {
                    tmp.emplace(AnaliticCmd(target, j.at("detector")));
                }
                else 
                    throw logic_error("no type");
                break;
            }
        }

        return *tmp;
    }
    catch(const std::exception& ex){
        cout << __PRETTY_FUNCTION__ << " " << ex.what() << endl;
        return {};
    }
    return {};
}