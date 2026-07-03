#include "event_queue.hpp"
#include "event_system/target.hpp"
#include "rule.hpp"
#include "event_system/event.hpp"
#include <algorithm>
#include <exception>
#include <string>

EventQueue::~EventQueue() {
    stop();
}


// void EventQueue::subscribeTarget(shared_ptr<Target> target) {
//     if (!target) return;
//     targets_->add(target);
//     // Subscribe target to its supported event keys
//     for (const auto& rule_key : target->supported_rules) {
//         subscriptions_[rule_key].push_back(target->name);
//     }
//     cout << "EventQueue: subscribed target '" << target->name
//          << "' (" /*<< target->type_name*/ << ") for "
//          << target->supported_rules.size() << " event types\n";
// }

// void EventQueue::unsubscribeTarget(const string& target_name) {
//     auto tgt = targets_->find(target_name);
//     if (tgt) {
//         for (const auto& rule_key : tgt->supported_rules) {
//             auto& subs = subscriptions_[rule_key];
//             std::erase(subs, target_name);
//         }
//         targets_->remove(target_name);
//     }
// }

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

void EventQueue::processTriggerEvent(Event trigger_event) {
    cout << "EventQueue::processTriggerEvent: trigger='" /*<< trigger_name*/
         << "' event=" << trigger_event.toString() << endl;

    // First, send the trigger's event itself to any targets that subscribe to it
    // deliverToTargets(trigger_event);

    // Then, find all actions linked to this trigger and execute their cmds
    auto actions = actions_.lock();
    if (actions) {
        auto linked_actions = actions->getActionsForEvn(trigger_event);
        for (const auto* action : linked_actions) {
            cout << "EventQueue: executing action '" << action->name
                 << "' with " << action->rules.size() << " cmds\n";
            for (auto rule : action->rules) {
                deliverToTargets({rule, trigger_event});
            }
        }
    }
}

void EventQueue::deliverToTargets(Command evn) {
    const string rule_key = evn.first.key();
    auto subscribtions = subscribtions_.lock();
    if(!subscribtions) return;
    subscribtions->forEachByRule(rule_key, [cmd = std::move(evn)](Target::Ptr tg_ptr){
        if(tg_ptr){
            cout << "EventQueue: delivering to target '" << tg_ptr->name << "'\n";
            tg_ptr->procEvent(std::move(cmd));
        }
    });
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
        auto evn = Event::dummyEvent();
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
