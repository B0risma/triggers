#pragma once
#include <algorithm>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <list>
#include <functional>
#include <functional>
#include <iostream>
#include "rule.hpp"
#include "json.hpp"
#include "event.hpp"
#include "target.hpp"
#include "trigger.hpp"
#include "action.hpp"

using json = nlohmann::json;
using namespace std;

class SubscribtionList;

/// EventQueue — central event connector and transmitter.
/// Routes events from triggers through actions to targets.
/// Maintains target subscriptions and processes event delivery.
class EventQueue : public enable_shared_from_this<EventQueue> {
public:
    EventQueue() = default;
    ~EventQueue();

    /// Initialize with system registries
    void setRegistries(
        shared_ptr<ActionList> actions,
        shared_ptr<SubscribtionList> targets
    ) {
        actions_ = actions;
        subscribtions_ = targets;
    }

    /// Send an event immediately to all subscribed targets
    /// Used by triggers to emit events
    void sendEvent(Signal &evn);

    /// Process a trigger event: find linked actions and send their cmds
    /// This implements the trigger -> action -> events -> targets pipeline
    void processTriggerEvent(Signal trigger_event);


    /// own async eventing
    /// Push an event into the queue for async processing
    void pushEvent(Signal &evn);
    /// Start the background event processing thread
    void start();
    /// Stop the background event processing thread
    void stop();
    /// Process all pending events (called by worker thread)
    void processLoop();

private:
    /// Deliver a single event to matching targets
    void deliverToTargets(Command evn);

    weak_ptr<ActionList> actions_;
    weak_ptr<SubscribtionList> subscribtions_;

    // Async event queue
    queue<Signal> pending_events_;
    mutex queue_mutex_;
    condition_variable queue_cv_;
    atomic<bool> running_{false};
    thread worker_thread_;
};


class SubscribtionList final : public TargetList{
public:

    void forEach(std::function<void(Target::Ptr)> f){
        for(const auto& pair : targets_){
            f(pair.second);
        }
    }

    void forEachByRule(const std::string& rule_key, std::function<void(Target::Ptr)> f){
        const auto tgs = findByRule(rule_key);
        std::for_each(tgs.cbegin(), tgs.cend(), f);
    }

    std::list<Target::Ptr> findByRule(const string &rule_key)const{
        auto it_ = subscriptions_.find(rule_key);
        if(it_ == subscriptions_.end()) return {};
        const auto& tg_list = it_->second;
        std::list<Target::Ptr> ret;
        std::for_each(tg_list.cbegin(), tg_list.cend(),[&](const std::string& tg_name){
            auto tg_ptr = find(tg_name);
            if(tg_ptr) ret.push_back(tg_ptr);
        });
        return ret;
    }

    void subscribe(Target::Ptr target) {
        if(!target) return;
        if(targets_.count(target->name)) {
            cout << __PRETTY_FUNCTION__ << ": existed " << target->name << endl;
            return;
        }
        TargetList::add(target);
        for (const auto& rule_key : target->supported_rules) {
            subscriptions_[rule_key].push_back(target->name);
        }
        cout << "EventQueue: subscribed target '" << target->name
                << "' (" /*<< target->type_name*/ << ") for "
                << target->supported_rules.size() << " event types\n";
    }

    /// Remove a target by name
    void remove(const string& name) {
        auto tgt = find(name);


        if(tgt){
            for (const auto& rule_key : tgt->supported_rules) {
                auto& subs = subscriptions_[rule_key];
                auto it = std::remove(subs.begin(), subs.end(), name);
                subs.erase(it, subs.end());
            }
            TargetList::remove(name);
        }
    }
private:
    using TargetList::add;
    // rule_key -> target names
    unordered_map<string, vector<string>> subscriptions_; 
};