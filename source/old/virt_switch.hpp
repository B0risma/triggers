#pragma once 
#include <string>
#include <iostream>
#include <set>
#include <unordered_map>

#include "json.hpp"

using json = nlohmann::json;
using namespace std;
using namespace string_literals;


template <typename T>
using ConstRef = const T&;

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