#include "event_queue.hpp"
#include "event_system/command.hpp"
#include "event_system/event.hpp"
#include <algorithm>
#include <exception>
#include <string>

EventQueue::~EventQueue() {
    stop();
}

void EventQueue::subscribeTarget(shared_ptr<Target> target) {
    if (!target) return;
    // Also add to TargetList if not already there
    if (targets_) {
        targets_->add(target);
    }
    // Subscribe target to its supported event keys
    for (const auto& event_key : target->supported_cmds) {
        subscriptions_[event_key].push_back(target->name);
    }
    cout << "EventQueue: subscribed target '" << target->name
         << "' (" << target->type_name << ") for "
         << target->supported_cmds.size() << " event types\n";
}

void EventQueue::unsubscribeTarget(const string& target_name) {
    if (targets_) {
        auto tgt = targets_->find(target_name);
        if (tgt) {
            for (const auto& event_key : tgt->supported_cmds) {
                auto& subs = subscriptions_[event_key];
                std::erase(subs, target_name);
            }
            targets_->remove(target_name);
        }
    }
}

void EventQueue::sendEvent(Event &evn) {
    cout << "EventQueue::sendCmd: " << evn.toString() << endl;
    processTriggerEvent(std::move(evn));
}

void EventQueue::pushEvent(Event& evn) {
    {
        lock_guard<mutex> lock(queue_mutex_);
        pending_events_.emplace(std::move(evn));
    }
    queue_cv_.notify_one();
}

void EventQueue::processTriggerEvent(Event &&trigger_event) {
    cout << "EventQueue::processTriggerEvent: trigger='" /*<< trigger_name*/
         << "' event=" << trigger_event.toString() << endl;

    // First, send the trigger's event itself to any targets that subscribe to it
    // deliverToTargets(trigger_event);

    // Then, find all actions linked to this trigger and execute their cmds
    if (actions_) {
        auto linked_actions = actions_->getActionsForEvn(trigger_event);
        for (const auto* action : linked_actions) {
            cout << "EventQueue: executing action '" << action->name
                 << "' with " << action->cmds.size() << " cmds\n";
            for (auto rule : action->cmds) {
                deliverToTargets({rule, std::move(trigger_event)});
            }
        }
    }
}

void EventQueue::deliverToTargets(Command evn) {
    const string event_key = evn.first.key();

    // Find targets subscribed to this exact event key
    auto it = subscriptions_.find(event_key);
    if (it != subscriptions_.end()) {
        for (const auto& target_name : it->second) {
            if (targets_) {
                auto tgt = targets_->find(target_name);
                if (tgt) {
                    cout << "EventQueue: delivering to target '" << target_name << "'\n";
                    tgt->procEvent(std::move(evn));
                }
            }
        }
    }

    // Also find targets subscribed to the broad type (e.g. "analitics:" for all analitics events)
    // string broad_key = cmd.type + ":";
    // auto broad_it = subscriptions_.find(broad_key);
    // if (broad_it != subscriptions_.end()) {
    //     for (const auto& target_name : broad_it->second) {
    //         if (targets_) {
    //             auto tgt = targets_->find(target_name);
    //             if (tgt) {
    //                 // Avoid double delivery if already delivered via exact key
    //                 if (it == subscriptions_.end() ||
    //                     std::find(it->second.begin(), it->second.end(), target_name) == it->second.end()) {
    //                     cout << "EventQueue: delivering (broad) to target '" << target_name << "'\n";
    //                     tgt->procEvent(evn);
    //                 }
    //             }
    //         }
    //     }
    // }
}

void EventQueue::start() {
    if (running_.load()) return;
    running_ = true;
    worker_thread_ = thread([this]() { processLoop(); });
    cout << "EventQueue: worker thread started\n";
}

void EventQueue::stop() {
    running_ = false;
    queue_cv_.notify_all();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    cout << "EventQueue: worker thread stopped\n";
}

void EventQueue::processLoop() {
    while (running_.load()) {
        Event evn;
        {
            unique_lock<mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this]() {
                return !pending_events_.empty() || !running_.load();
            });
            if (!running_.load() && pending_events_.empty()) break;
            if (pending_events_.empty()) continue;
            std::swap(evn,pending_events_.front());
            pending_events_.pop();
        }
        processTriggerEvent(std::move(evn));
    }
}
