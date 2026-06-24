#include "event_system/event.hpp"
#include "event_system/rule.hpp"
#include "httplib.h"
#include "json.hpp"
#include <cstdlib>
#include <exception>
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

using namespace std;
using namespace std::string_literals;
using json = nlohmann::json;


std::tuple<shared_ptr<EventQueue>, shared_ptr<ActionList>, shared_ptr<TriggerList>> core = {};
void initCore(){
    static atomic_bool inited = {false};
    if(inited) return;
    core = {make_shared<EventQueue>(), make_shared<ActionList>(), make_shared<TriggerList>()};

    auto queue = std::get<0>(core);
    auto actions = std::get<1>(core);
    auto triggers = std::get<2>(core);
    queue->setRegistries(triggers, actions);
    inited = true;
}
auto trigList(){
    return std::get<2>(core);
}
auto testTriggers(){
    auto trg_list = trigList();
    auto tup = tuple(make_shared<GPIOTrigger> ("pin1"), make_shared<Vswitch>("switch1"), make_shared<SheduleTrigger>("shed1"));
    const auto& [in1, sw1, sh1] = tup;
    trg_list->add(in1);
    trg_list->add(sw1);
    trg_list->add(sh1);
    return tup;
}

void triggerTest(){
    cout << __PRETTY_FUNCTION__ << endl;
    try{
    auto trg_list = std::get<2>(core);
    auto test_trigs = testTriggers();
    {
        for(auto i : {EventType::GPIO, EventType::VirtSwitch, EventType::Shedule}){
            auto v = trg_list->byKind((+i)._to_string());
            if(v.empty()) 
            {
                cout << "ERROR: " << __PRETTY_FUNCTION__ << endl;
                std::abort();
            }
        }
    }
    trg_list->clean();
    }
    catch(const std::exception& ex){
        cout << __PRETTY_FUNCTION__ << " " << ex.what() << endl;
    }
}

void testEvents(){
    cout << __PRETTY_FUNCTION__ << endl;
    auto [in1, sw1, sh1] = testTriggers();
    
    {
        sh1->start = std::chrono::system_clock::now();
        sh1->stop = sh1->start + chrono::seconds(10);
    }
    {
        // no event
        cout << "NO EVENTS\n";
        in1->setState(false);
        sw1->setState(false);
        sh1->checkTime(sh1->start - chrono::seconds(5));
        sh1->checkTime(sh1->start + chrono::seconds(15));
        cout << "END\n";
    }
    class EventCntr : public Target{
    public:
        EventCntr(){
            name = "EventCntr";
            supported_rules = {(+RuleType::Invalid)._to_string()};
        }
        int evn_cnt = 0;
        void procEvent(const Command &evn) override{\
            Target::procEvent(evn);
            evn_cnt++;
        }
    };

   {
        auto cntr = make_shared<EventCntr>();
        auto que = get<0>(core);
        que->subscribeTarget(cntr);

        auto act_list = get<1>(core);
        {
            json act_j;
            act_j["name"] = "test";
            act_j[Action::rules_f] = {{{"type",(+RuleType::Invalid)._to_string()}}};
            cout << act_j.dump(1) << endl;
            act_list->addAction(Action::fromJson(act_j));
        }
        {
            EventActionLink ln;
            ln.action = "test";
            ln.evn_key = in1->evnKey();
            act_list->addLink(ln);

            ln.evn_key = sw1->evnKey();
            act_list->addLink(ln);

            ln.evn_key = sh1->evnKey();
            act_list->addLink(ln);
        }
        
        // switch event
        cout << "EVENTS\n";
        in1->setState(true);
        sw1->setState(true);
        sh1->checkTime(sh1->start + chrono::seconds(5));
        cout << "END\n";
        if(cntr->evn_cnt != 3){
            cout <<  __func__ << " bad events\n";
            abort();
        }
    }
    trigList()->clean();
}

int main() {
    initCore();
    auto queue = std::get<0>(core);
    auto actions = std::get<1>(core);
    auto triggers = std::get<2>(core);


    triggerTest();
    testEvents();
    return 0;

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
    auto fire_det1 = make_shared<FireDetector>("fire_detector_1");
    auto fire_det2 = make_shared<FireDetector>("fire_detector_2");
    queue->subscribeTarget(fire_det1);
    queue->subscribeTarget(fire_det2);

    // --- 5. Start the EventQueue worker thread ---
    queue->start();

    // --- 6. Set up API handler ---
    APIHandler api;
    api.setRegistries({},triggers, actions, queue, switches);

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
    act_set2.rules = {AnaliticCmd("Fire")};
    // act_set2.cmds = {
    //     Command{"analitics", "fire", json{{"detector", "fire"}, {"enabled", true}}},
    //     Command{"alarm", "light", json{{"alarm_in", "light"}}}
    // };
    actions->addAction(act_set2);

    // Link trigger "in1" to action "act_set2"
    EventActionLink link;
    link.evn_key = gpio_in1->evnKey();
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
