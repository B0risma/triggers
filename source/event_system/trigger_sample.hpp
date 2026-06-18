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
    static constexpr auto gpio_kind = "GPIO";
public:
    /// Pin identifier (e.g. "in1", "in2")
    string pin_id;

    /// Current GPIO state
    atomic<bool> state{false};

    GPIOTrigger(const string& pin)
        : pin_id(pin) {
        name = pin_id;
        kind = gpio_kind;
    }
    virtual Event createEvent(bool new_state)const {
        return GPIOEvent(pin_id, new_state);
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

    /// Descriptor for trigger listing
    // json getDescriptor() const override {
    //     return json{
    //         {"name", name},
    //         {"kind", kind},
    //         {"pin", pin_id},
    //         {"state", state.load()}
    //     };
    // }

    // /// Start periodic state simulation (for demo/testing).
    // /// Alternates state every `interval_ms` milliseconds.
    // void startSimulation(int interval_ms = 2000) {
    //     sim_running_ = true;
    //     sim_thread_ = thread([this, interval_ms]() {
    //         bool cur = state.load();
    //         while (sim_running_.load()) {
    //             cur = !cur;
    //             setState(cur);
    //             this_thread::sleep_for(chrono::milliseconds(interval_ms));
    //         }
    //     });
    //     cout << "GPIOTrigger: simulation started for pin '" << pin_id << "'\n";
    // }

    /// Stop the simulation
    // void stopSimulation() {
    //     sim_running_ = false;
    //     if (sim_thread_.joinable()) {
    //         sim_thread_.join();
    //     }
    //     cout << "GPIOTrigger: simulation stopped for pin '" << pin_id << "'\n";
    // }

    ~GPIOTrigger() override {
        // stopSimulation();
    }

    string evnKey(){
        return kind +":"+pin_id;
    }

private:
    // atomic<bool> sim_running_{false};
    // thread sim_thread_;
};


struct Vswitch : public GPIOTrigger{

    Vswitch(const string& pin_name) : 
    GPIOTrigger(pin_name){
        pin_id = pin_name;
        name = pin_name;
    }
    virtual Event createEvent(bool new_state)const override{
        return VswitchEvent(pin_id, new_state);
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