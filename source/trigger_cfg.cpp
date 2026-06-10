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



