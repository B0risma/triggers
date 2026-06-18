#include "command.hpp"
#include <exception>
#include <stdexcept>
#include <iostream>
#include <string>
#include <utility>

using namespace std;
std::optional<Rule> Rule::fromJson(const json& j) noexcept{
    try{
        Rule tmp;
        const auto type = RuleType::_from_string(j.at("type").get_ref<const string&>().c_str());
        switch(type._value){
            case RuleType::Analitic: {
                tmp = AnaliticCmd(j.at("detector"), j.at("state"));
                break;
            }
            case RuleType::Alarm: {
                auto s_type = AlarmCmdSubtype::_from_string(j.at("subtype").get_ref<const string&>().c_str());
                auto cmd_type = AlarmCmdType::_from_string(j.at("target").get_ref<const string&>().c_str());
                tmp = AlarmCmd(cmd_type, s_type, j.at("state"));
                break;
            }
            case RuleType::Video: {
                VideoCmd v_c{};
                v_c.data["preset_on"] = j.at("preset_on");
                v_c.data["preset_off"] = j.at("preset_off");
                break;
            }
            default: throw logic_error("no type");
        }
        if(j.contains("target")){
            tmp.target = j.at("target");
        }
        return tmp;
    }
    catch(const std::exception& ex){
        cout << __PRETTY_FUNCTION__ << " " << ex.what() << endl;
        return {};
    }
    return {};
}