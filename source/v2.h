#pragma once 
#include <algorithm>
#include <cstdint>
// event BASED VERSION
/*

"target" - any object that can handle event
"event" - object with information about event
"trigger" - event source (API, ONVIF, other event, sheduler ....)


- trigger -> event -> target
- target handles event (off\on\reboot ....)

trigger{
emitEvent() ///
}
Toggle : trigger{
    set(){emitEvent}
}
....

// shedule rules object
Shedule_entry{
...
}
Sheduler{
    shedule_list{} /// shedules for targets
    timer_thread() /// proc shedules and emitEvents
}

Target{
    procEvent()
    supportedEvents() // list of supported events
}

EventQueue{
    registerTarget(Target)
}

*/

#include "json.hpp"
#include <forward_list>
#include <functional>
#include <ios>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <list>
#include <iostream>
#include <thread>
#include <array>
using namespace std;
using  json = nlohmann::json;

template <typename T>
using ConstRef = const T&;

template <typename T>
using noown_ptr = T*;

struct Event{
    enum Type : uint8_t{
        CV,
        Video,
        MD,
        Image
    };

    Type type;
    json data;
    string toString(){
        return to_string(type) + "|"s + data.dump();
    }
};

struct Target;


struct EventQueue : enable_shared_from_this<EventQueue>{
    unordered_map<Event::Type, set<noown_ptr<Target>>> event_map;
    void addTarget(noown_ptr<Target> t);
    void delTarget(noown_ptr<Target> t);
    list<Event> events;
    void pushEvent(Event evn);
    void sendEvent(Event evn);
    void run(std::function<void()> f);

    array<atomic<bool>, 5> free = {true,true,true,true,true};
    array<thread,5> ths;
};

struct Target {
    virtual void procEvent(ConstRef<Event> evn){
    }
    void setEventQueue(shared_ptr<EventQueue> q){
        if(q) {
            e_q = q;
            q->addTarget(this);
        }

    }
    ~Target(){
        //if setuped -> need free, otherwise not
        auto q_strong = e_q.lock();
        if(q_strong) q_strong->delTarget(this);
    }

    set<Event::Type> eventTypes;
private:
    weak_ptr<EventQueue> e_q;
    
};

struct Trigger{
    virtual ~Trigger() = default;
    void setEventQueue(shared_ptr<EventQueue> q){
        if(q) e_queue = q;
    }
    virtual void emitEvent() const{};

    weak_ptr<EventQueue> e_queue; // weak_ptr for safety
};


// detached async like
/*
thread pool
and run every new task at free thread
*/
//-------------Concrete ---------

struct cv_event : public Event{
    cv_event(){
        type = Type::CV;
        data = {{"module", "fire"}, {"enable", true}};
    }
};

struct cv_ctrl : public Target{
    cv_ctrl(){
        eventTypes = {Event::CV};
    }
    void procEvent(ConstRef<Event> evn) override{
        cout << __func__ << endl;
        if(evn.type == Event::CV){
            cout << __func__ << ": " << evn.data.dump() << endl;
            state = evn.data.at("enable");
            enable_module(evn.data.at("module"), evn.data.at("enable"));
        }
    }
    void enable_module(ConstRef<string> n, bool state){
        cout << __PRETTY_FUNCTION__ << " " << n << ": " << std::boolalpha << state << endl;
    }
    void work(){
        cout << __PRETTY_FUNCTION__ << " " << (state ? "" : "CANT") << "work\n";
    }
    bool state = true;
};

struct Toggle : public Trigger{
    void emitEvent()const override{
        cout << __func__ << endl;
        auto strong_q = e_queue.lock();
        if(strong_q) {
            // strong_q->pushEvent(cv_event{});
            strong_q->sendEvent(cv_event{});
        }
    };

    // Event on_evn;
    // Event off_evn;
};
