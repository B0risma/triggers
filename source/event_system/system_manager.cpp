#include "system_manager.hpp"
#include "event_system/event_queue.hpp"

EventsCore& EventsCore::instance() {
    static EventsCore inst;
    return inst;
}

void EventsCore::init() {
    event_queue_  = make_shared<EventQueue>();
    trigger_list_ = make_shared<TriggerList>();
    action_list_  = make_shared<ActionList>();
    target_list_  = make_shared<SubscribtionList>();

    // Wire cross-references
    event_queue_->setRegistries(action_list_, target_list_);
    trigger_list_->setEventQueue(event_queue_);
}