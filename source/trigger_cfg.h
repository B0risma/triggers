#pragma once
// #include "vswitch_cfg.h"
#include <algorithm>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <ios>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <list>
#include <utility>
#include <atomic>
#include <cassert>
#include <iostream>
#include <set>


#include "json.hpp"
#include "virt_switch.hpp"

using namespace std::string_literals;
using json = nlohmann::json;

template <typename T>
using ConstRef = const T&;

using namespace std;
//no need


class RuleI{
    public:
    virtual ~RuleI() = default;
    using Ptr = shared_ptr<RuleI>;
    using WeakPtr = weak_ptr<RuleI>;
    virtual bool isActive()const {return false;};
    virtual json toJson() const noexcept{return {};};
    virtual void fromJson(ConstRef<json> j){};
};

enum class RuleType : uint8_t{
    Switcher = 0,
    Sheduler = 1
};
NLOHMANN_JSON_SERIALIZE_ENUM(RuleType, 
    {
        {RuleType::Switcher, "sw"},
        {RuleType::Sheduler, "shed"}
    })

//use it for buttons too


class Sheduler : public RuleI{
    public:
    bool isActive() const override{return false;};
};

RuleI::Ptr ruleFactory(const RuleType t);
RuleI::Ptr ruleFactory(ConstRef<json>j);

// class SingleShot : public RuleI{
//     public:
//     void run();
// };
// VSwitch is a Switcher child

class Switcher : public RuleI{
private:
    // atomic<bool> active = {false};
    string name = ""s; // == SwitchI name\identifier
public:
    json toJson()const noexcept override{
        return json{
            // {"enabled", active.load()},
            {"name", name}, //source
            {"type", RuleType::Switcher}
        };
    }
    void fromJson(ConstRef<json> j) noexcept(false) override{
        // active = j.at("enabled");
        name = j.at("name");
        if(name.empty()) throw logic_error("invalid source");
        else{
            if(SwitchI::find(name) == nullptr){
                cout << "invalid source - no switch!\n";
                throw logic_error("invalid source");//?
            }
        }
    }
    //need tristate - true/false/error
    bool isActive()const override{
        auto sw_ptr = SwitchI::find(name);
        if(sw_ptr){
            return sw_ptr->state();
        }
        return true; // for temporary - no switch no rules
    }
};

/* 
state switcher for modules for limiting module activity and status management

"module" is program object abstraction, object that has two states: on\off.
"rule" - limitation mode: sheduler, button like switch, ... something else
"policy" - applies one rule for multiple objects
RuleList - set of different policies

Architecture:
module --check_active_rule-->|
                             | RuleList
        API --add_rules-->   |

Examples:
| name | state now | why? |
| ---- | --------- | ---- |
| fire | off | sheduler |
| smoke | on | default | 
| MD | off | Switcher |
| cross | on | singleshot for 10s |
*/

/*
TODO:
- multiple policies for one module + status(limit) order
- garbage collector for weak_ptr`s
- switch rule invertion flag
*/ 


struct Policy{
    virtual ~Policy() = default;
    RuleI::Ptr rule;
    set<string> targets;
    virtual json toJson() const{
        json j;
        j["rule"] = rule ? rule->toJson() : json(); 
        auto targs = json::array();
        for(const auto &m : targets){
            targs.emplace_back(m);
        }
        j["targets"] = std::move(targs);
        return j;
    }
    static Policy fromJson(ConstRef<json> j){
        Policy p;
        p.rule = ruleFactory(j.at("rule"));
        p.targets = j.at("targets");
        return p;
    }
};

//need Rule list for easy rules management
class RuleList{
    unordered_map<string, RuleI::WeakPtr> by_module; //list of rules - for many rules for one module
    unordered_map<string, Policy> rules; // for managing (set, edit ...)
public:
    bool module_active(ConstRef<string> module) noexcept{
        try{
            auto rule_strong = by_module.at(module).lock();
            if(!rule_strong) {
                cout << __func__ << ": ERROR: no rule\n";
                //we cant arrive here when all is ok
                assert(false);
                throw range_error("no rule");
            }
            return rule_strong->isActive();
        }
        catch(...){
            return true; // by default
        }
    }

    ConstRef<Policy> byRule(ConstRef<string> rule_n) noexcept(false){
        return (rules.at(rule_n));
    }

    //
    void addPolicy(ConstRef<string> name, Policy p){
        if(rules.count(name)) throw range_error("rule exist");
        if(name.empty()) throw logic_error("bad name");
        if(p.targets.empty()) throw range_error("no targets");
        for(ConstRef<string> t : p.targets){
            if(t.empty()) continue;
            by_module.emplace(t, p.rule);
        }
        rules.emplace(name, std::move(p));
    }

    void delPolicy(ConstRef<string> name){
        // auto p_it = rules.find(name);
        if(rules.erase(name)) throw range_error("no rule");
        //no need to delete from by_module - weak_ptr
    }

    void clear()noexcept{
        rules.clear();
        by_module.clear();
    }

    json toJson()const noexcept{
        json j;
        for(const auto &[name, p] : rules){
            j[name] = p.toJson();
        }
        return j;
    }

    void fromJson(ConstRef<json> j) noexcept(false){
        clear();
        for(auto it = j.begin(); it != j.end(); std::advance(it,1)){
            if(it->is_object()){
                string name = it.key();
                auto p = Policy::fromJson(it.value());
                addPolicy(name, std::move(p));
            }
            else{
                throw logic_error("invalid object");
            }
        }
    }
};





