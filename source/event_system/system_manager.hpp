#pragma once
#include <memory>
#include "event_queue.hpp"
#include "trigger.hpp"
#include "action.hpp"
#include "target.hpp"

using namespace std;

/// SystemManager — global singleton that owns and provides access to
/// all major components of the event system.
///
/// Usage:
///   SystemManager::instance().init();
///   auto queue = SystemManager::instance().eventQueue();
class EventsCore {
public:
    static EventsCore& instance();

    /// Accessors
    std::weak_ptr<EventQueue> eventQueue() const { return event_queue_; }
    std::weak_ptr<TriggerList> triggerList()  const { return trigger_list_; }
    std::weak_ptr<ActionList> actionList()   const { return action_list_; }
    std::weak_ptr<SubscribtionList> targetList()   const { return target_list_; }

    /// One-call initialization — creates empty instances and wires cross-references
    void init();

    // Non-copyable
    EventsCore(const EventsCore&) = delete;
    EventsCore& operator=(const EventsCore&) = delete;

private:
    EventsCore() = default;

    shared_ptr<EventQueue>   event_queue_;
    shared_ptr<TriggerList>  trigger_list_;
    shared_ptr<ActionList>   action_list_;
    shared_ptr<SubscribtionList>   target_list_;
};
