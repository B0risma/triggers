#include "v2.h"
#include <exception>


void EventQueue::addTarget(noown_ptr<Target> t){
    if(!t) return;
    for(const auto &evn : t->eventTypes){
        event_map[evn].emplace(t);
    }
}
void EventQueue::delTarget(noown_ptr<Target> t){
    if(!t) return;
    for(const auto &evn : t->eventTypes){
        event_map.at(evn).erase(t);
    }
}
void EventQueue::pushEvent(Event evn){
    cout << __func__ << endl;
    events.emplace_back(std::move(evn)); // or use toolkit async and simultanuosly handle event!
    //run()
    //notify thread
}
void EventQueue::sendEvent(Event evn){
    cout << __PRETTY_FUNCTION__ <<  " " << evn.toString() << endl;
    try{
        auto tgs = event_map.at(evn.type);
        for(const auto & t : tgs){
            run([t, this, e = std::move(evn)](){
                t->procEvent(e);
            });
        }
    }
    catch(ConstRef<exception> ex){
        cout << __PRETTY_FUNCTION__<< ": " << ex.what() << endl;
    }
}
void EventQueue::run(std::function<void()> f){
    using namespace chrono_literals;
    f();
    // int free_num = -1;
    // do {
    //     auto it = find(begin(free), end(free), true);
    //     if(it) free_num = std::distance(begin(free), it);
    //     this_thread::sleep_for(1s);
    // }while(free_num < 0 );

    // if(free.at(free_num)){
    //     auto &free_thread = ths[free_num];
    //     free_thread.join();
    //     free_thread = std::move(thread([num = free_num, func = f, this](){
    //         free.at(num) = false;
    //         func();
    //         free.at(num) = true;
    // }));
    // }
}


