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
class SystemManager {
public:
    static SystemManager& instance();

    /// Accessors
    shared_ptr<EventQueue>   eventQueue()   const { return event_queue_; }
    shared_ptr<TriggerList>  triggerList()  const { return trigger_list_; }
    shared_ptr<ActionList>   actionList()   const { return action_list_; }
    shared_ptr<TargetList>   targetList()   const { return target_list_; }

    /// One-call initialization — creates empty instances and wires cross-references
    void init();

    // Non-copyable
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;

private:
    SystemManager() = default;

    shared_ptr<EventQueue>   event_queue_;
    shared_ptr<TriggerList>  trigger_list_;
    shared_ptr<ActionList>   action_list_;
    shared_ptr<TargetList>   target_list_;
};
