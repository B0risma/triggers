#pragma once
// #include "vswitch_cfg.h"
#include <algorithm>
#include <csignal>
#include <cstdint>
#include <exception>
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

//global state mask for all modules?
/* 
| name | state now | why? |
| fire | off | sheduler |
| smoke | on | default | 
| MD | off | Switcher |
| cross | on | singleshot for 10s |

and module want where it needs to work

notification events and singleshot callbacks need other hadling

MODULE --check_state--> RULE_LIST
API --edit--> RULE_LIST
SignalEvent(API\Onvif\GPIO) --enable_rule--> RULE_LIST
Signal (==? Trigger) --activate_rule--> RULE_LIST //for switch|shot rules
*/


struct Policy{
    using Ptr = shared_ptr<Policy>;
    virtual ~Policy() = default;
    RuleI::Ptr rule;
    set<string> targets;
    virtual string type() const{return "policy";}
    virtual json toJson() const{
        json j;
        j["rule"] = rule ? rule->toJson() : json(); 
        auto targs = json::array();
        for(const auto &m : targets){
            targs.emplace_back(m);
        }
        j["targets"] = std::move(targs);
        j["type"] = type();
        return j;
    }
    // static Policy fromJson(ConstRef<json> j){
    //     Policy p;
    //     p.rule = ruleFactory(j.at("rule"));
    //     p.targets = j.at("targets");
    //     return p;
    // }
};

Policy::Ptr policyFromJson(ConstRef<json> j);

//need Rule list for easy rules management
class RuleList{
    unordered_map<string, RuleI::WeakPtr> by_module; //list of rules - for many rules for one module
    unordered_map<string, Policy::Ptr> rules; // for managing (set, edit ...)
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
        return *(rules.at(rule_n).get());
    }

    //
    void addPolicy(ConstRef<string> name, Policy::Ptr p){
        if(rules.count(name)) throw range_error("rule exist");
        if(name.empty()) throw logic_error("bad name");
        if(p->targets.empty()) throw range_error("no targets");
        for(ConstRef<string> t : p->targets){
            if(t.empty()) continue;
            by_module.emplace(t, p->rule);
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
            j[name] = p->toJson();
        }
        return j;
    }

    void fromJson(ConstRef<json> j) noexcept(false){
        clear();
        for(auto it = j.begin(); it != j.end(); std::advance(it,1)){
            if(it->is_object()){
                string name = it.key();
                auto p = policyFromJson(it.value());
                addPolicy(name, std::move(p));
            }
            else{
                throw logic_error("invalid object");
            }
        }
    }
};


/*++++++++++++SWITCHER CTRL+++++++*/

/*TODO:
- switch "button" API - sending signal!
*/

class Switcher : public RuleI{
private:
    atomic<bool> active = {false};
public:
    json toJson()const noexcept override{
        return json{
            {"enabled", active.load()},
            // {"signal", source},
            {"type", RuleType::Switcher}
        };
    }
    void fromJson(ConstRef<json> j) noexcept(false) override{
        active = j.at("enabled");
        // source = j.at("signal");
        // if(source.empty()) throw logic_error("invalid source");
    }
    bool isActive()const override{ return active;}
    void on(){active = true;}
    void off(){active = false;}
    void set(const bool state){active = state;}
    // string source; //signal name for tests - move out!
    //source - signal name or switcher - external manager?
};

struct Signal{
    std::string name;
    bool new_state = false; //on = true, off = false
};

struct SwitchMgr{
    unordered_map<string, shared_ptr<Switcher>> switchers; //may live on weak_ptr
    static SwitchMgr& instance();
    //! call when rule added
    bool registerSwitch(string source, shared_ptr<Switcher> sw);
    //! call when 
    void unregister(ConstRef<string> n);
    void notify(ConstRef<Signal> signal);
    void printSws(){
        cout << "switchers: \n";
        for(const auto &x : switchers){
            cout << x.first << endl;
        }
    }

    private:
    SwitchMgr() = default;
};


struct SwitchPolicy : public Policy{
    string signal;
    virtual ~SwitchPolicy(){
        if(!signal.empty() && registered){
            // cout << __func__ << endl;
            SwitchMgr::instance().unregister(signal);
        }
    }
    virtual string type() const override {return "switch_policy";}
    json toJson() const override{
        auto j = Policy::toJson();
        j["signal"] = signal;
        return j;
    }

    void registerToMgr(){
        registered = SwitchMgr::instance().registerSwitch(signal, dynamic_pointer_cast<Switcher>(rule));
    }
    SwitchPolicy() = default;//{cout << __func__ << endl;}
    SwitchPolicy(SwitchPolicy&& r){
        this->swap(r);
    };
    SwitchPolicy& operator =(SwitchPolicy && p){
        if(this != &p){
            this->swap(p);
        }
        return *this;
    }

    SwitchPolicy(const SwitchPolicy&) = delete;
    SwitchPolicy& operator =(ConstRef<SwitchPolicy>) = delete;
    
private:
    void swap(SwitchPolicy& r){
        signal.swap(r.signal);
        std::swap(registered, r.registered);
        // from parent policy
        rule.swap(r.rule);
        targets.swap(r.targets);
    }
    bool registered = false;
};

//! for VirtSwitch|API|GPIO inheritance - meybe agregation is better
struct SwitchI{
    string name() const;
    bool state() const;
    void notify(){
        SwitchMgr::instance().notify(Signal{.name = name(), .new_state = state()});
    }    
};

