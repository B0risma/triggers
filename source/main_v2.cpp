#include "event_system/event.hpp"
#include "event_system/rule.hpp"
#include "httplib.h"
#include "json.hpp"
#include <cstdlib>
#include <exception>
#include <fstream>
#include <memory>
#include <iostream>
#include <chrono>
#include <system_error>
#include <thread>
#include <tuple>

#include "event_system/event_queue.hpp"
#include "event_system/target.hpp"
#include "event_system/trigger.hpp"
#include "event_system/action.hpp"
#include "event_system/api_handler.hpp"
#include "event_system/trigger_sample.hpp"
#include "event_system/fire_detector.hpp"
#include "event_system/system_manager.hpp"

using namespace std;
using namespace std::string_literals;
using json = nlohmann::json;

int main() {
    EventsCore::instance().init();
    auto queue    = EventsCore::instance().eventQueue().lock();
    auto actions  = EventsCore::instance().actionList().lock();
    auto triggers = EventsCore::instance().triggerList().lock();
    auto targets  = EventsCore::instance().targetList().lock();

    auto switches = make_shared<VswitchList>();
    switches->que = queue;
    switches->trgList = triggers;

    // --- 3. Create and register Trigger examples ---
    // GPIO triggers: pins "in1" and "in2"
    auto gpio_in1 = make_shared<GPIOTrigger>("in1");
    auto gpio_in2 = make_shared<GPIOTrigger>("in2");
    gpio_in1->setEventQueue(queue);
    gpio_in2->setEventQueue(queue);
    triggers->add(gpio_in1);
    triggers->add(gpio_in2);

    // --- 4. Create and register Target examples ---
    // Fire detector targets
    auto fire_det1 = make_shared<FireDetector>("fire_detector");
    auto weapon_det = make_shared<WeaponDetector>("weapon_detector");
    targets->subscribe(fire_det1);
    targets->subscribe(weapon_det);

    // --- 5. Start the EventQueue worker thread ---
    queue->start();

    // --- 6. Set up API handler ---
    APIHandler api;
    api.setRegistries(triggers, actions, queue, switches);

    httplib::Server srv;
    api.registerRoutes(srv);

    constexpr int port = 8080;
    constexpr auto addr = "127.0.0.1";

    cout << "\nServer listen on http://" << addr << ":" << port << "\n";
    cout << "API endpoints:\n";
    cout << "  GET  /trigger  → trigger list by kinds\n";
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
    act_set2.rules = {AnaliticCmd(AnaliticTarget::Fire)};
    actions->addAction(act_set2);

    // Link trigger "in1" to action "act_set2"
    EventActionLink link;
    link.evn_key = gpio_in1->evnKey();
    link.action = "act_set2";
    actions->addLink(link);

    cout << "\n--- Demo: Trigger event through action pipeline ---\n";
    gpio_in1->setState(false);

    this_thread::sleep_for(chrono::milliseconds(500));

    // --- 8. Start HTTP server (blocks) ---
    cout << "\nStarting HTTP server...\n";

    srv.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        auto readFile = [](const string fName)->string{
            ifstream f(fName);
            if(!f.is_open()) return {}; 
            stringstream cont;
            cont << f.rdbuf();
            return cont.str();
        };
        std::string html = readFile("index.html");
        if (html.empty()) {
            res.status = 404;
            res.set_content("File not found", "text/plain");
            return;
        }
        res.set_content(html, "text/html");
    });

    srv.listen(addr, port);

    // Cleanup on exit
    queue->stop();
    cout << "Event system shutdown complete.\n";

    return 0;
}
