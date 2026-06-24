#pragma once
#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include "json.hpp"
#include "trigger.hpp"
#include "event.hpp"

using json = nlohmann::json;
using namespace std;

/// GPIO Trigger example.
/// Emits a signal (event) on every GPIO state change.
/// Simulates GPIO input pin — when state changes, it emits an event
/// into the EventQueue for processing by linked targets/actions.
class GPIOTrigger : public Trigger {
public:
    /// Pin identifier (e.g. "in1", "in2")
    string pin_id;

    /// Current GPIO state
    atomic<bool> state{false};

    GPIOTrigger(const string& pin)
        : pin_id(pin) {
        name = pin_id;
        evn_type = EventType::GPIO;
    }
    virtual Event createEvent(bool new_state)const {
        return GPIOEvent(evnKey(), new_state);
    }
    /// Simulate a state change on the GPIO pin.
    /// When state changes, emit an event into the queue.
    void setState(bool new_state) {
        bool old = state.exchange(new_state);
        if (old != new_state) {
            cout << "GPIOTrigger: pin '" << pin_id << "' changed to "
                 << (new_state ? "HIGH" : "LOW") << "\n";
            emitEvent(createEvent(new_state));
        }
    }

    /// Get current state
    bool getState() const {
        return state.load();
    }

    ~GPIOTrigger() override {
    }
};


struct Vswitch : public GPIOTrigger{

    Vswitch(const string& pin_name) : 
    GPIOTrigger(pin_name){
        pin_id = pin_name;
        name = pin_name;
        evn_type = EventType::VirtSwitch;
    }
    virtual Event createEvent(bool new_state)const override{
        return VswitchEvent(evnKey(), new_state);
    }

};

struct VswitchList{
    map<string, shared_ptr<Vswitch>> list;
    shared_ptr<TriggerList> trgList;
    shared_ptr<EventQueue> que;

    void add(json data){
        auto sw = make_shared<Vswitch>(data.at("name"));
        sw->state = data.at("state");
        list[sw->name] = sw;
        if(trgList){
            trgList->add(sw);
        }
        if(que) {
            sw->setEventQueue(que);
        }
    }

    void del(string name){
        auto count = list.erase(name);
        trgList->remove(name);
    }

    void set(json data){
        string name = data.at("name");
        bool new_state = data.at("state");
        auto it = list.find(name);
        if(it != list.end()){
            auto sw = it->second;
            sw->setState(new_state);
        }
    }

};

struct SheduleTrigger : public Trigger{
    SheduleTrigger(const string &sh_name){
        evn_type = EventType::Shedule;
        name = sh_name;
    }
    using tp_t = std::chrono::system_clock::time_point;
    std::chrono::system_clock::time_point start = {};;
    std::chrono::system_clock::time_point stop = {};
   

    void checkTime(const tp_t tp){
        if(tp > start && tp < stop){
            setState(true);    
        }
        else{
            setState(false);
        }
    }

private:
    void setState(bool state){
        if(state == last_state){
            return;
        }

        last_state = state;

        Event evn(evnKey());
        evn.type = EventType::Shedule;
        evn.data["active_p"] = state; //in_period

        emitEvent(std::move(evn));
    }    
    bool last_state = false;
};