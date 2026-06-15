#include "httplib.h"
#include "json.hpp"
#include <memory>
#include <iostream>
#include <chrono>
#include <thread>

#include "event_system/event_queue.hpp"
#include "event_system/target.hpp"
#include "event_system/trigger.hpp"
#include "event_system/action.hpp"
#include "event_system/api_handler.hpp"
#include "event_system/gpio_trigger.hpp"
#include "event_system/fire_detector.hpp"

using namespace std;
using namespace std::string_literals;
using json = nlohmann::json;

int main() {
    cout << "=== Event System V2 ===\n\n";

    // --- 1. Create registries ---
    auto targets   = make_shared<TargetList>();
    auto triggers  = make_shared<TriggerList>();
    auto actions   = make_shared<ActionList>();
    auto queue     = make_shared<EventQueue>();

    // --- 2. Wire up the EventQueue with registries ---
    queue->setRegistries(targets, triggers, actions);

    // --- 3. Create and register Trigger examples ---
    // GPIO triggers: pins "in1" and "in2"
    auto gpio_in1 = make_shared<GPIOTrigger>("gpio", "in1");
    auto gpio_in2 = make_shared<GPIOTrigger>("gpio", "in2");
    gpio_in1->setEventQueue(queue);
    gpio_in2->setEventQueue(queue);
    triggers->add(gpio_in1);
    triggers->add(gpio_in2);

    // --- 4. Create and register Target examples ---
    // Fire detector targets
    auto fire_det1 = make_shared<FireDetector>("fire_detector_1");
    auto fire_det2 = make_shared<FireDetector>("fire_detector_2");
    queue->subscribeTarget(fire_det1);
    queue->subscribeTarget(fire_det2);

    // --- 5. Start the EventQueue worker thread ---
    queue->start();

    // --- 6. Set up API handler ---
    APIHandler api;
    api.setRegistries(targets, triggers, actions, queue);

    httplib::Server srv;
    api.registerRoutes(srv);

    constexpr int port = 8080;
    constexpr auto addr = "localhost";

    cout << "\nServer listen on http://" << addr << ":" << port << "\n";
    cout << "API endpoints:\n";
    cout << "  GET  /trigger  → trigger list by kinds\n";
    cout << "  GET  /target   → target list\n";
    cout << "  GET  /action   → action list\n";
    cout << "  POST /action   → create action\n";
    cout << "  DELETE /action → delete action\n";
    cout << "  GET  /link     → trigger-action links\n";
    cout << "  POST /link     → link trigger to action\n";
    cout << "  DELETE /link   → unlink trigger from action\n\n";

    // --- 7. Demo: manual trigger event ---
    cout << "--- Demo: Manual GPIO state change ---\n";
    gpio_in1->setState(true);   // This emits event → targets

    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n--- Demo: Creating action and linking to trigger ---\n";
    // Create an action with a fire detector command and an alarm command
    Action act_set2;
    act_set2.name = "act_set2";
    act_set2.cmds = {
        Event{"analitics", "fire", json{{"detector", "fire"}, {"enabled", true}}},
        Event{"alarm", "light", json{{"alarm_in", "light"}}}
    };
    actions->addAction(act_set2);

    // Link trigger "in1" to action "act_set2"
    TriggerActionLink link;
    link.trigger = "in1";
    link.action = "act_set2";
    actions->addLink(link);

    cout << "\n--- Demo: Trigger event through action pipeline ---\n";
    // Now when gpio_in1 changes state, it should:
    // 1. Send the GPIO event directly to subscribed targets
    // 2. Execute linked action "act_set2" which sends its cmds
    gpio_in1->setState(false);

    this_thread::sleep_for(chrono::milliseconds(500));

    // --- 8. Start HTTP server (blocks) ---
    // Optional: start GPIO simulation in background
    // gpio_in2->startSimulation(3000);  // toggles every 3 seconds

    cout << "\nStarting HTTP server...\n";
    srv.listen(addr, port);

    // Cleanup on exit
    queue->stop();
    cout << "Event system shutdown complete.\n";

    return 0;
}
