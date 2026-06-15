#include "trigger.hpp"
#include "event_queue.hpp"

void Trigger::setEventQueue(shared_ptr<EventQueue> q) {
    if (q) e_queue = q;
}

void Trigger::emitEvent(const Event& evn) const {
    auto strong_q = e_queue.lock();
    if (strong_q) {
        strong_q->processTriggerEvent( evn);
    } else {
        cout << "Trigger::emitEvent: no event queue for trigger '" << name << "'\n";
    }
}
