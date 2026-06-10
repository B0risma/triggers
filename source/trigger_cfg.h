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


//! for VirtSwitch|API|GPIO inheritance - meybe agregation is better
struct SwitchI{
    explicit SwitchI(string name){
        n_ = name;
        regist();
    }
    virtual ~SwitchI(){
        if(registered){
            switches.erase(n_);
        }
    }
    SwitchI(SwitchI && p){
        swap(p);
    }
    SwitchI& operator=(SwitchI&& p){
        if(this != std::addressof(p)){
            swap(p);
        }
        return *this;
    }

    SwitchI(ConstRef<SwitchI>) = delete;
    SwitchI& operator=(ConstRef<SwitchI>) = delete;

    ConstRef<string> name() const{return n_;};
    virtual bool state() const{return false;}

    static SwitchI const*  find(ConstRef<string> name){
        auto sw_it = switches.find(name);
        if(sw_it == switches.cend()){
            return nullptr;
        }
        else{
            return sw_it->second;
        }
    }

protected:
    SwitchI() = default; //for child usage
    void regist(){
        if(n_.empty()){
            cout << __func__ << " empty\n";
            return;
        } 
        if(switches.count(n_)) {
            cout << __func__ << " " << n_ << " already exists\n";
            return;
        } 
        switches.emplace(n_, this);
        registered = true;
    }
    void swap(SwitchI & p){
        n_.swap(p.n_);
        std::swap(registered, p.registered);
        if(registered){
            switches[n_] = this;
        }
    }
    static unordered_map<string, SwitchI*> switches; 
    bool registered = false;
    string n_ = ""s;

};
 

struct Switch : public SwitchI{
    explicit Switch(string name, bool state = false) : SwitchI(name), state_(state){
    }
    //try default implementations V
    Switch(Switch && p){
        swap(p);
    }
    Switch& operator=(Switch&& p){
        if(this != std::addressof(p)){
            swap(p);
        }
        return *this;
    }

    // Switch(ConstRef<Switch>) = delete;
    // Switch& operator=(ConstRef<Switch>) = delete;

    virtual bool state() const override{return state_;}
    void set(bool state){
        cout << __func__ << " " << name() << ": " << 
            std::boolalpha << state_ << "->" << state << endl;
        state_ = state;
    }
private:
    void swap(Switch &p){
        SwitchI::swap(p);
        std::swap(state_,p.state_);
    }
    bool state_ = false;
};


struct SwitchILess{
    using is_transparent = std::true_type;
    bool operator()(ConstRef<Switch> l, ConstRef<Switch> r) const{
        return l.name() <  r.name();
    }
    bool operator()(const Switch& a, const std::string& b) const {
        return a.name()< b;
    }
    
    bool operator()(const std::string& a, const Switch& b) const {
        return a < b.name();
    }
};

struct SwitchList {
    SwitchList() = default;
    SwitchList(SwitchList &&) = delete;
    SwitchList(ConstRef<SwitchList>) = delete;
    SwitchList & operator=(SwitchList &&) = delete;
    SwitchList & operator=(ConstRef<SwitchList>) = delete;


    set<Switch, SwitchILess> list;
    static SwitchList& ins(){
        static SwitchList _;
        return _;
    }

    void add(ConstRef<json> j){
        string name = j.at("name");
        bool ena = j.at("enabled");
        if(name.empty()) throw logic_error("empty name");
        list.emplace(name, ena);
    }
    void del(ConstRef<string> name){
        auto it = list.find(name);
        if(it != list.cend()) list.erase(it);
    }
    void change(ConstRef<json> j){
        string name = j.at("name");
        bool ena = j.at("enabled");
        if(name.empty()) throw logic_error("empty name");

        auto it = list.find(name);
        if(it != list.cend()){
            auto node = list.extract(it);
            node.value().set(ena);
            list.insert(std::move(node));
        }
    }
    json getAll() const{
        json j;
        for(ConstRef<Switch> sw : list){
            j[sw.name()] = sw.state();
        }
        return j;

    }
};

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

// struct Signal{
//     std::string name;
//     bool new_state = false; //on = true, off = false
// };

// struct SwitchMgr{
//     unordered_map<string, shared_ptr<Switcher>> switchers; //may live on weak_ptr
//     static SwitchMgr& instance();
//     //! call when rule added
//     bool registerSwitch(string source, shared_ptr<Switcher> sw);
//     //! call when 
//     void unregister(ConstRef<string> n);
//     void notify(ConstRef<Signal> signal);
//     void printSws(){
//         cout << "switchers: \n";
//         for(const auto &x : switchers){
//             cout << x.first << endl;
//         }
//     }

//     private:
//     SwitchMgr() = default;
// };


// struct SwitchPolicy : public Policy{
//     string signal;
//     virtual ~SwitchPolicy(){
//         if(!signal.empty() && registered){
//             // cout << __func__ << endl;
//             SwitchMgr::instance().unregister(signal);
//         }
//     }
//     virtual string type() const override {return "switch_policy";}
//     json toJson() const override{
//         auto j = Policy::toJson();
//         j["signal"] = signal;
//         return j;
//     }

//     void registerToMgr(){
//         registered = SwitchMgr::instance().registerSwitch(signal, dynamic_pointer_cast<Switcher>(rule));
//     }
//     SwitchPolicy() = default;//{cout << __func__ << endl;}
//     SwitchPolicy(SwitchPolicy&& r){
//         this->swap(r);
//     };
//     SwitchPolicy& operator =(SwitchPolicy && p){
//         if(this != &p){
//             this->swap(p);
//         }
//         return *this;
//     }

//     SwitchPolicy(const SwitchPolicy&) = delete;
//     SwitchPolicy& operator =(ConstRef<SwitchPolicy>) = delete;
    
// private:
//     void swap(SwitchPolicy& r){
//         signal.swap(r.signal);
//         std::swap(registered, r.registered);
//         // from parent policy
//         rule.swap(r.rule);
//         targets.swap(r.targets);
//     }
//     bool registered = false;
// };



