#include "system_manager.hpp"

SystemManager& SystemManager::instance() {
    static SystemManager inst;
    return inst;
}

void SystemManager::init() {
    event_queue_  = make_shared<EventQueue>();
    trigger_list_ = make_shared<TriggerList>();
    action_list_  = make_shared<ActionList>();
    target_list_  = make_shared<TargetList>();

    // Wire cross-references
    event_queue_->setRegistries(trigger_list_, action_list_, target_list_);
    trigger_list_->setEventQueue(event_queue_);
}