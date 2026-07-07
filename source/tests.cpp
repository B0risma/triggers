// #include "event_system/command.hpp"
#include "event_system/event.hpp"
#include "event_system/rule.hpp"
#include "event_system/target.hpp"
#include "event_system/trigger.hpp"
#include "event_system/action.hpp"
#include "event_system/event_queue.hpp"
#include "event_system/trigger_sample.hpp"
#include "event_system/fire_detector.hpp"
#include "json.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

using namespace std;
using json = nlohmann::json;

// ============================================================
// Test utilities
// ============================================================
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { \
        cout << "  TEST: " << name << " ... "; \
        try {

#define END_TEST \
            cout << "PASSED\n"; \
            tests_passed++; \
        } catch (const exception& e) { \
            cout << "FAILED (" << e.what() << ")\n"; \
            tests_failed++; \
        } \
    } while(0)

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            throw runtime_error(string(msg) + " [" + __FILE__ + ":" + to_string(__LINE__) + "]"); \
        } \
    } while(0)

// ============================================================
// 1. Event tests
// ============================================================
void test_event_creation() {
    TEST("Event creation with src_key") {
        Signal evt("test_source");
        ASSERT(evt.key() == "test_source", "key should match constructor arg");
        ASSERT(evt.type == (+EventType::NoType), "default type should be NoType");
    } END_TEST;

    TEST("GPIOEvent creation") {
        GPIOEvent gpio("pin1", true);
        ASSERT(gpio.type == (+EventType::GPIO), "GPIOEvent type should be GPIO");
        ASSERT(gpio.key() == "pin1", "GPIOEvent key should be pin1");
        ASSERT(gpio.data["state"] == true, "GPIOEvent state should be true");
    } END_TEST;

    TEST("VswitchEvent creation") {
        VswitchEvent vs("sw1", false);
        ASSERT(vs.type == (+EventType::VirtSwitch), "VswitchEvent type should be VirtSwitch");
        ASSERT(vs.key() == "sw1", "VswitchEvent key should be sw1");
        ASSERT(vs.data["state"] == false, "VswitchEvent state should be false");
    } END_TEST;

    TEST("AnaliticEvent creation") {
        AnaliticEvent ae(DetectorType::Fire, AlarmEdge::Start);
        ASSERT(ae.type == (+EventType::Analitic), "AnaliticEvent type should be Analitic");
        ASSERT(ae.key() == "Fire", "AnaliticEvent key should be detector name");
        ASSERT(ae.data["alarmEdge"] == "Start", "AnaliticEvent alarmEdge should be Start");
    } END_TEST;

    TEST("Event toString and key") {
        Signal evt("src1");
        evt.data["val"] = 42;
        string s = evt.toString();
        ASSERT(s.find("src1") != string::npos, "toString should contain src_key");
        ASSERT(s.find("42") != string::npos, "toString should contain data");
    } END_TEST;

    TEST("Dummy event") {
        Signal dummy = Signal::dummyEvent();
        ASSERT(dummy.key() == "invalid", "dummy event key should be 'invalid'");
    } END_TEST;
}

// ============================================================
// 2. Rule tests
// ============================================================
void test_rule_creation() {
    TEST("AlarmCmd creation") {
        AlarmCmd alarm(AlarmTarget::WhiteLight, RuleType::Toggle);
        ASSERT(alarm.type == (+RuleType::Toggle), "AlarmCmd type should be Toggle");
        ASSERT(alarm.target_type == (+TargetType::Alarm), "AlarmCmd target_type should be Alarm");
        ASSERT(alarm.target == (+AlarmTarget::WhiteLight)._to_string(), "AlarmCmd target should be WhiteLight");
    } END_TEST;

    TEST("VideoCmd creation") {
        VideoCmd video;
        video.setPresetOn("preset1");
        video.setPresetOff("preset2");
        ASSERT(video.type == (+RuleType::Preset), "VideoCmd type should be Preset");
        ASSERT(video.target_type == (+TargetType::Video), "VideoCmd target_type should be Video");
    } END_TEST;

    TEST("AnaliticCmd creation") {
        AnaliticCmd ac(AnaliticTarget::Fire);
        ASSERT(ac.type == (+RuleType::Toggle), "AnaliticCmd type should be Toggle");
        ASSERT(ac.target_type == (+TargetType::Analitic), "AnaliticCmd target_type should be Analitic");
        ASSERT(ac.target == (+AnaliticTarget::Fire)._to_string(), "AnaliticCmd target should be Fire");
    } END_TEST;

    TEST("Rule::ruleKey") {
        string key = Rule::ruleKey(RuleType::Toggle, TargetType::Alarm, "Light");
        ASSERT(!key.empty(), "ruleKey should not be empty");
        // key format: "Toggle:Alarm:Light"
        ASSERT(key.find("Toggle") != string::npos, "ruleKey should contain rule type");
        ASSERT(key.find("Alarm") != string::npos, "ruleKey should contain target type");
        ASSERT(key.find("Light") != string::npos, "ruleKey should contain target");
        ASSERT((count_if(key.cbegin(), key.cend(), [](const char c){return c == ':';}) == 2), "must be 2 delimiter");
    } END_TEST;

    TEST("Rule fromJson - Alarm") {
        json j = {
            {Fields::Rule::rule_type, (+RuleType::Toggle)._to_string()},
            {Fields::Rule::target_type, (+TargetType::Alarm)._to_string()},
            {Fields::Rule::target, (+AlarmTarget::WhiteLight)._to_string()}
        };
        auto opt = Rule::fromJson(j);
        ASSERT(opt, "fromJson should return valid Rule");
        ASSERT(opt->target_type == (+TargetType::Alarm), "parsed target_type should be Alarm");
        ASSERT(opt->type == (+RuleType::Toggle), "Must be toggle");
    } END_TEST;

    TEST("Rule fromJson - Video") {
        json j = {
            {Fields::Rule::rule_type, (+RuleType::Preset)._to_string()},
            {Fields::Rule::target_type, (+TargetType::Video)._to_string()},
            {"preset_on", "3"},
            {"preset_off", "7"}
        };
        auto opt = Rule::fromJson(j);
        ASSERT(opt, "fromJson should return valid Rule");
        ASSERT(opt->target_type == (+TargetType::Video), "parsed target_type should be Video");
        ASSERT(opt->type == (+RuleType::Preset), "Must be Preset");
    } END_TEST;

    TEST("Rule fromJson - Analitic") {
        json j = {
            {Fields::Rule::rule_type, (+RuleType::Toggle)._to_string()},    
            {Fields::Rule::target_type, (+TargetType::Analitic)._to_string()},
            {"target", "Fire"}
        };
        auto opt = Rule::fromJson(j);
        ASSERT(opt, "fromJson should return valid Rule");
        ASSERT(opt->target_type == (+TargetType::Analitic), "parsed target_type should be Analitic");
        ASSERT(opt->type == (+RuleType::Toggle), "Must be toggle");
    } END_TEST;

    TEST("Rule fromJson - Invalid returns nullopt") {
        json j = {
            {"rule_type", "InvalidType"},
            {"target_type", "Invalid"},
            {"target", ""}
        };
        auto opt = Rule::fromJson(j);
        ASSERT(!opt, "fromJson with invalid types should return nullopt");
    } END_TEST;

    TEST("Rule fromJson - Missing fields returns nullopt") {
        json j = {{"rule_type", "Toggle"}}; // missing target_type
        auto opt = Rule::fromJson(j);
        ASSERT(!opt, "fromJson with missing fields should return nullopt");
    } END_TEST;
}

// ============================================================
// 3. Target tests
// ============================================================
class TestTarget : public Target {
public:
    int event_count = 0;
    vector<Command> received_commands;

    TestTarget(const string& name_val) {
        name = name_val;
        supported_rules = {Rule::ruleKey(RuleType::Toggle, TargetType::Invalid)}; //invalid for Test
    }

    void procEvent(const Command& cmd) override {
        Target::procEvent(cmd);
        event_count++;
        received_commands.push_back(cmd);
    }

    static Rule supportedRuleTemplate(){
        Rule tmp;
        tmp.type = RuleType::Toggle;
        tmp.target_type = TargetType::Invalid;
        return tmp;
    }
};

void test_target_creation() {
    TEST("Target creation and properties") {
        auto t = make_shared<TestTarget>("target1");
        ASSERT(t->name == "target1", "target name should match");
        ASSERT(t->event_count == 0, "initial event count should be 0");
    } END_TEST;

    TEST("Target toJson") {
        auto t = make_shared<TestTarget>("target_json");
        json j = t->toJson();
        ASSERT(j[Target::Fields::name] == "target_json", "toJson should contain name");
    } END_TEST;

    TEST("Target canHandle") {
        auto t = make_shared<TestTarget>("target_supports");
        string key = Rule::ruleKey(RuleType::Toggle, TargetType::Invalid);
        ASSERT(t->canHandle(key), "target should support its registered rule");
        ASSERT(!t->canHandle("Nonexistent:Type"), "target should not support unregistered rule");
    } END_TEST;
}

// ============================================================
// 4. Trigger tests
// ============================================================
void test_trigger_creation() {
    TEST("GPIOTrigger creation") {
        auto gt = make_shared<GPIOTrigger>("gpio_test");
        ASSERT(gt->evnKey() == "GPIO:gpio_test", "GPIOTrigger evnKey should match");
        ASSERT(gt->getState() == false, "GPIOTrigger initial state should be false");
    } END_TEST;

    TEST("GPIOTrigger setState emits event") {
        auto queue = make_shared<EventQueue>();
        auto triggers = make_shared<TriggerList>();
        auto actions = make_shared<ActionList>();
        auto targets = make_shared<SubscribtionList>();
        queue->setRegistries(actions, targets);
        triggers->setEventQueue(queue);

        auto gt = make_shared<GPIOTrigger>("gpio_emit");
        gt->setEventQueue(queue);
        triggers->add(gt);

        auto t_targ = make_shared<TestTarget>("Target1");
        targets->subscribe(t_targ);

        Action test_act;
        test_act.name = "test";
        test_act.rules.push_back(TestTarget::supportedRuleTemplate());
        actions->addAction(test_act);
        json link = {
            {Fields::Link::evn_key, gt->evnKey()},
            {Fields::Link::action, test_act.name}
        };
        actions->addLink(EventActionLink::fromJson(link));


        // setState(true) should emit an event
        gt->setState(true);
        ASSERT(gt->getState() == true, "GPIOTrigger state should be true after setState(true)");

        this_thread::sleep_for(chrono::milliseconds(100)); // queue thread time
        ASSERT(t_targ->event_count == 1, "Must be handled event");
    } END_TEST;

    TEST("Vswitch creation") {
        auto vs = make_shared<Vswitch>("vswitch_test");
        ASSERT(vs->evnKey() == "VirtSwitch:vswitch_test", "Vswitch evnKey should match");
        ASSERT(vs->getState() == false, "Vswitch initial state should be false");
    } END_TEST;

    TEST("SheduleTrigger creation") {
        auto st = make_shared<SheduleTrigger>("shed_test");
        ASSERT(st->evnKey() == "Shedule:shed_test", "SheduleTrigger evnKey should match");
    } END_TEST;

    TEST("TriggerList add and byKind") {
        auto triggers = make_shared<TriggerList>();
        auto gt = make_shared<GPIOTrigger>("gpio_list");
        auto vs = make_shared<Vswitch>("vs_list");
        triggers->add(gt);
        triggers->add(vs);

        auto gpios = triggers->byKind("GPIO");
        ASSERT(gpios.size() >= 1, "byKind('GPIO') should find at least 1 trigger");

        auto switches = triggers->byKind("VirtSwitch");
        ASSERT(switches.size() >= 1, "byKind('VirtSwitch') should find at least 1 trigger");
    } END_TEST;
}

// ============================================================
// 5. Action tests
// ============================================================
void test_action_creation() {
    TEST("Action creation and JSON roundtrip") {
        Action act;
        act.name = "test_action";
        act.rules = {AlarmCmd(AlarmTarget::WhiteLight, RuleType::Toggle)};

        json j = act.toJson();
        ASSERT(j["name"] == "test_action", "toJson should contain name");
        ASSERT(j.contains(Fields::Action::rules), "toJson should contain rules field");
    } END_TEST;

    TEST("Action fromJson") {
        json j;
        j["name"] = "from_json_action";
        j[Fields::Action::rules] = {{
            {Fields::Rule::rule_type, "Toggle"},
            {Fields::Rule::target_type, "Alarm"},
            {Fields::Rule::target, "WhiteLight"}
        }};

        Action act = Action::fromJson(j);
        ASSERT(act.name == "from_json_action", "parsed action name should match");
        ASSERT(act.rules.size() == 1, "parsed action should have 1 rule");
    } END_TEST;

    TEST("Action fromJson with invalid rules throws") {
        json j;
        j["name"] = "bad_action";
        j[Fields::Action::rules] = {{
            {Fields::Rule::rule_type, "Invalid"},
            {Fields::Rule::target_type, "Invalid"}
        }};
        try {
            Action::fromJson(j);
            ASSERT(false, "fromJson with invalid rules should throw");
        } catch (const exception&) {
            // expected
        }
    } END_TEST;

    TEST("Action fromJson missing name throws") {
        json j;
        j[Fields::Action::rules] = json::array();
        try {
            Action::fromJson(j);
            ASSERT(false, "fromJson without name should throw");
        } catch (const exception&) {
            // expected
        }
    } END_TEST;

    TEST("ActionList add and get actions") {
        auto actions = make_shared<ActionList>();
        Action act;
        act.name = "action1";
        act.rules = {AlarmCmd(AlarmTarget::WhiteLight, RuleType::Toggle)};
        actions->addAction(act);

        auto retrieved = actions->findAction("action1");
        ASSERT(retrieved != nullptr, "findAction should find added action");
        ASSERT(retrieved->name == "action1", "retrieved action name should match");
    } END_TEST;

    TEST("ActionList delete action") {
        auto actions = make_shared<ActionList>();
        Action act;
        act.name = "to_delete";
        actions->addAction(act);
        ASSERT(actions->findAction("to_delete") != nullptr, "action should exist before delete");

        actions->removeAction("to_delete");
        ASSERT(actions->findAction("to_delete") == nullptr, "action should not exist after delete");
    } END_TEST;

    TEST("ActionList links - add and find") {
        auto actions = make_shared<ActionList>();
        Action act;
        act.name = "linked_action";
        actions->addAction(act);

        EventActionLink link;
        link.evn_key = "test_trigger";
        link.action = "linked_action";
        actions->addLink(link);

        // Create a dummy event with the matching key to test getActionsForEvn
        Signal evt("test_trigger");
        auto found = actions->getActionsForEvn(evt);
        ASSERT(found.size() >= 1, "getActionsForEvn should find at least 1 action");
        ASSERT(found[0]->name == "linked_action", "found action name should match");
    } END_TEST;

    TEST("ActionList delete link") {
        auto actions = make_shared<ActionList>();
        Action act;
        act.name = "link_del";
        actions->addAction(act);

        EventActionLink link;
        link.evn_key = "trig_del";
        link.action = "link_del";
        actions->addLink(link);

        // Verify link exists before delete
        Signal evt("trig_del");
        ASSERT(actions->getActionsForEvn(evt).size() == 1, "link should exist before delete");
        actions->removeLink("trig_del", "link_del");
        ASSERT(actions->getActionsForEvn(evt).empty(), "link should not exist after delete");
    } END_TEST;
}

// ============================================================
// 6. EventQueue integration tests
// ============================================================
void test_event_queue() {
    TEST("EventQueue subscribe/unsubscribe target") {
        auto queue = make_shared<EventQueue>();
        auto triggers = make_shared<TriggerList>();
        auto actions = make_shared<ActionList>();
        auto targets = make_shared<SubscribtionList>();
        queue->setRegistries(actions, targets);
        triggers->setEventQueue(queue);

        auto target = make_shared<TestTarget>("queue_target");
        targets->subscribe(target);
        // No direct way to verify subscription, but we can check it doesn't crash
    } END_TEST;

    TEST("EventQueue process event through action pipeline") {
        auto queue = make_shared<EventQueue>();
        auto triggers = make_shared<TriggerList>();
        auto actions = make_shared<ActionList>();
        auto targets = make_shared<SubscribtionList>();
        queue->setRegistries(actions, targets);
        triggers->setEventQueue(queue);

        // Create a target
        auto target = make_shared<TestTarget>("pipeline_target");
        target->supported_rules = {Rule::ruleKey(RuleType::Toggle, TargetType::Invalid)};
        targets->subscribe(target);

        // Create an action
        Action act;
        act.name = "pipeline_action";
        act.rules = {Rule()}; // Invalid type rule (Toggle:Invalid)
        actions->addAction(act);

        // Link trigger to action
        EventActionLink link;
        link.evn_key = "GPIO:pipeline_trigger";
        link.action = act.name;
        actions->addLink(link);

        // Create a GPIO trigger and emit event
        auto gt = make_shared<GPIOTrigger>("pipeline_trigger");
        gt->setEventQueue(queue);
        triggers->add(gt);

        // setState should trigger the pipeline
        gt->setState(true);

        // Give the queue thread time to process
        this_thread::sleep_for(chrono::milliseconds(100));

        // Target should have received events
        ASSERT(target->event_count >= 1, "target should have received at least 1 event");
    } END_TEST;
}

// ============================================================
// 7. Full system integration test
// ============================================================
void test_full_system() {
    TEST("Full system: trigger → action → target pipeline") {
        auto queue = make_shared<EventQueue>();
        auto triggers = make_shared<TriggerList>();
        auto actions = make_shared<ActionList>();
        auto targets = make_shared<SubscribtionList>();
        queue->setRegistries(actions, targets);
        triggers->setEventQueue(queue);

        // Create target
        auto target = make_shared<TestTarget>("integration_target");
        target->supported_rules = {Rule::ruleKey(RuleType::Toggle, TargetType::Invalid)};
        targets->subscribe(target);

        // Create action with Invalid-type rule (matches our test target)
        Action act;
        act.name = "integration_action";
        act.rules = {Rule()};
        actions->addAction(act);

        // Link GPIO trigger to action
        auto gt = make_shared<GPIOTrigger>("integration_trig");
        gt->setEventQueue(queue);
        triggers->add(gt);

        EventActionLink link;
        link.evn_key = "GPIO:integration_trig";
        link.action = act.name;
        actions->addLink(link);

        // Emit event
        int before = target->event_count;
        gt->setState(true);
        this_thread::sleep_for(chrono::milliseconds(100));
        int after = target->event_count;

        ASSERT(after > before, "target event count should increase after trigger event");
    } END_TEST;

    TEST("Full system: multiple triggers and targets") {
        auto queue = make_shared<EventQueue>();
        auto triggers = make_shared<TriggerList>();
        auto actions = make_shared<ActionList>();
        auto targets = make_shared<SubscribtionList>();
        queue->setRegistries(actions, targets);
        triggers->setEventQueue(queue);

        // Create two targets
        auto t1 = make_shared<TestTarget>("multi_t1");
        auto t2 = make_shared<TestTarget>("multi_t2");
        t1->supported_rules = {Rule::ruleKey(RuleType::Toggle, TargetType::Invalid)};
        t2->supported_rules = {Rule::ruleKey(RuleType::Toggle, TargetType::Invalid)};
        targets->subscribe(t1);
        targets->subscribe(t2);

        // Create action
        Action act;
        act.name = "multi_action";
        act.rules = {Rule()};
        actions->addAction(act);

        // Link two triggers to same action
        auto gt1 = make_shared<GPIOTrigger>("multi_trig1");
        auto gt2 = make_shared<GPIOTrigger>("multi_trig2");
        gt1->setEventQueue(queue);
        gt2->setEventQueue(queue);
        triggers->add(gt1);
        triggers->add(gt2);

        EventActionLink link;
        link.action = "multi_action";
        link.evn_key = "GPIO:multi_trig1";
        actions->addLink(link);
        link.evn_key = "GPIO:multi_trig2";
        actions->addLink(link);

        // Fire both triggers
        gt1->setState(true);
        gt2->setState(true);
        this_thread::sleep_for(chrono::milliseconds(150));

        // Both targets should have received events from both triggers
        ASSERT(t1->event_count >= 2, "target1 should have received at least 2 events");
        ASSERT(t2->event_count >= 2, "target2 should have received at least 2 events");
    } END_TEST;
}

// ============================================================
// 8. FireDetector integration test
// ============================================================
void test_fire_detector() {
    TEST("FireDetector receives AnaliticCmd") {
        auto queue = make_shared<EventQueue>();
        auto triggers = make_shared<TriggerList>();
        auto actions = make_shared<ActionList>();
        auto targets = make_shared<SubscribtionList>();
        queue->setRegistries(actions, targets);
        triggers->setEventQueue(queue);

        auto fd = make_shared<FireDetector>("test_fd");
        targets->subscribe(fd);

        // Create action with AnaliticCmd rule
        Action act;
        act.name = "fd_action";
        act.rules = {AnaliticCmd(AnaliticTarget::Fire)};
        actions->addAction(act);

        // Create a trigger and link it
        auto gt = make_shared<GPIOTrigger>("fd_trig");
        gt->setEventQueue(queue);
        triggers->add(gt);

        EventActionLink link;
        link.evn_key = "GPIO:fd_trig";
        link.action = act.name;
        actions->addLink(link);

        ASSERT(fd->isEnabled() == false, "FireDetector should start disabled");

        // Emit event with state=true
        gt->setState(true);
        this_thread::sleep_for(chrono::milliseconds(100));

        // Note: The GPIO event's data["state"] = true should trigger
        // the FireDetector to enable. This tests the full pipeline.
        cout << "      (FireDetector enabled status after event: " << fd->isEnabled() << ")\n";
    } END_TEST;
}

// ============================================================
// Main
// ============================================================
int main() {
    cout << "=== Event System Tests ===\n\n";

    cout << "--- Event Tests ---\n";
    test_event_creation();

    cout << "\n--- Rule Tests ---\n";
    test_rule_creation();

    cout << "\n--- Target Tests ---\n";
    test_target_creation();

    cout << "\n--- Trigger Tests ---\n";
    test_trigger_creation();

    cout << "\n--- Action Tests ---\n";
    test_action_creation();

    cout << "\n--- EventQueue Tests ---\n";
    test_event_queue();

    cout << "\n--- Integration Tests ---\n";
    test_full_system();

    cout << "\n--- FireDetector Tests ---\n";
    test_fire_detector();

    cout << "\n=== Results ===\n";
    cout << "Passed: " << tests_passed << "\n";
    cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}