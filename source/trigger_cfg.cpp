#include "trigger_cfg.h"
#include "modules.hpp"


#include "httplib.h"

#include <istream>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

using namespace std;
using namespace chrono_literals;
using namespace std::string_literals;
using json = nlohmann::json;


RuleI::Ptr ruleFactory(const RuleType t){
    if(t == RuleType::Switcher){
        return make_shared<Switcher>();
    }
    else if(t == RuleType::Sheduler){
        return make_shared<Sheduler>();
    }
    else{
        cout << __func__ << ": invalid type\n";
        assert(false);
        return {};
    }
}

RuleI::Ptr ruleFactory(ConstRef<json>j){
    RuleType t = j.at("type");
    auto rule = ruleFactory(t);
    if(!rule) return rule;
    try{
        rule->fromJson(j);
    }
    catch(ConstRef<exception> ex){
        cout << __func__ << " exc: " << ex.what() << endl;
        rule.reset();
    }
    return rule;
}

Policy::Ptr policyFromJson(ConstRef<json> j){
    Policy p;
    p.rule = ruleFactory(j.at("rule"));
    p.targets = j.at("targets");

    string t = j.at("type");

    if( t == "switch_policy"){
        SwitchPolicy sw_p;
        #pragma message("dirty!")
        auto &tmp_p = static_cast<Policy&>(sw_p);
        tmp_p = p;
        sw_p.signal = j.at("signal");
        SwitchMgr::instance().registerSwitch(sw_p.signal, dynamic_pointer_cast<Switcher>(sw_p.rule));
        return make_shared<SwitchPolicy>(std::move(sw_p));
    }
    else return make_shared<Policy>(std::move(p));
}


/*++++++++++SWITCHERS++++++++*/

SwitchMgr SwitchMgr::instance(){
    static SwitchMgr _;
    return _;
}
void SwitchMgr::registerSwitch(string source, shared_ptr<Switcher> sw){
    if(switchers.count(source)) cout << __func__ << " element exists\n";
    switchers.emplace(std::move(source), sw);
}

void SwitchMgr::unregister(ConstRef<string> n){
    switchers.erase(n);
}

void SwitchMgr::notify(ConstRef<Signal> signal){
    auto sw_it = switchers.find(signal.name);
    if(sw_it != switchers.cend()) sw_it->second->set(signal.new_state);
}
