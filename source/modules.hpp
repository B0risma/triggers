#pragma once
#include "trigger_cfg.h"
#include <ostream>
#include <thread>
#include <sstream>

struct CoutBuf : public ostringstream{
    CoutBuf() : ostringstream() {}
    CoutBuf(CoutBuf&& other) noexcept 
        : ostringstream(std::move(other)) {}
    ~CoutBuf(){
        cout << this->str();
    }

    CoutBuf(const CoutBuf&) = delete;
    CoutBuf& operator=(const CoutBuf&) = delete;
};

class Logger{
public:
    Logger() = default;
    Logger(Logger&&) = default;
    Logger& operator=(Logger&&) = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    string name = "Logger";
    CoutBuf log() const{
        CoutBuf ss;
        ss << name+": "s;
        return ss;
    }
};

class Module : public Logger{
    weak_ptr<RuleList> rule_ref_;
public:
    void setupPolicy(weak_ptr<RuleList> rule_ref){
        rule_ref_ = rule_ref;
    }
    
    bool check() const {
        auto rules = rule_ref_.lock();
        if(rules){
            return rules->module_active(name);
        }
        log() << "no rules - no limits!\n";
        return true;
    }

    void work() const{
        auto rules = rule_ref_.lock();
        if(!rules) log() << "Cant work - no rules\n";
        
        if(rules->module_active(name)) {
            log() << "V - OK\n";
            log() << "Do....\n";
        }
        else{
            log() << "X - sleep\n";
        }
        this_thread::sleep_for(1s);
    };
};