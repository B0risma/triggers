#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>
#include <iostream>
#include "event_system/command.hpp"
#include "json.hpp"
#include "event.hpp"
#include "target.hpp"
#include "trigger.hpp"
#include "action.hpp"

using json = nlohmann::json;
using namespace std;

/// EventQueue — central event connector and transmitter.
/// Routes events from triggers through actions to targets.
/// Maintains target subscriptions and processes event delivery.
class EventQueue : public enable_shared_from_this<EventQueue> {
public:
    EventQueue() = default;
    ~EventQueue();

    /// Initialize with system registries
    void setRegistries(
        shared_ptr<TargetList> targets,
        shared_ptr<TriggerList> triggers,
        shared_ptr<ActionList> actions
    ) {
        targets_ = targets;
        triggers_ = triggers;
        actions_ = actions;
    }

    /// Register a target's event subscriptions
    void subscribeTarget(shared_ptr<Target> target);

    /// Unsubscribe a target
    void unsubscribeTarget(const string& target_name);

    /// Send an event immediately to all subscribed targets
    /// Used by triggers to emit events
    void sendEvent(Event &evn);

    /// Push an event into the queue for async processing
    void pushEvent(Event &evn);

    /// Process a trigger event: find linked actions and send their cmds
    /// This implements the trigger -> action -> events -> targets pipeline
    void processTriggerEvent(Event&& trigger_event);

    /// Start the background event processing thread
    void start();

    /// Stop the background event processing thread
    void stop();

    /// Process all pending events (called by worker thread)
    void processLoop();

private:
    /// Deliver a single event to matching targets
    void deliverToTargets(Command evn);

    shared_ptr<TargetList> targets_;
    shared_ptr<TriggerList> triggers_;
    shared_ptr<ActionList> actions_;

    // Target subscriptions: command_key -> list of target names
    unordered_map<string, vector<string>> subscriptions_; 

    // Async event queue
    queue<Event> pending_events_;
    mutex queue_mutex_;
    condition_variable queue_cv_;
    atomic<bool> running_{false};
    thread worker_thread_;
};
