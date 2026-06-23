#pragma once
#include <cstdint>
#include <exception>
#include <string>
#include <iostream>
#include <functional>
#include "rule.hpp"
#include "json.hpp"
#include "target.hpp"
#include "rule.hpp"
#include "event.hpp"

using json = nlohmann::json;
using namespace std;



class Detector : public Target{
    public:
    /// Whether the fire detection module is currently active
    bool enabled = false;

    /// Callback for when activity state changes (for external notification)
    function<void(bool)> onStateChanged;

    Detector(const string& instance_name) {
        name = instance_name;
        // type_name = "fire_detector";
        // Subscribe to analitics:fire events
        // auto cmdKey = string(CommandType(CommandType::Analitic)._to_string()) + ":"s + DetectorType(DetectorType::Fire)._to_string();
        // supported_cmds = {std::move(cmdKey)};
    }

    /// Process event: handles type=="analitics", subtype=="fire"
    /// The event data contains "enabled" field that turns on/off module activity
    // void procEvent(const Command& cmd) override {
    //     cout << "FireDetector '" << name << "': received event "
    //          << cmd.toString() << endl;

    //     if(cmd.type._value != CommandType::Analitic){
    //         cout << __PRETTY_FUNCTION__ << " sckip cmd: " << cmd.toString() << endl;
    //         return;
    //     }

    //     const auto& ai_cmd = static_cast<const AnaliticCmd &>(cmd);
    //     if(ai_cmd.detector() !=  DetectorType(DetectorType::Fire)._to_string()){
    //         cout << __PRETTY_FUNCTION__ << " sckip cmd: " << cmd.toString() << endl;
    //         return;
    //     }

    //     const auto& state = ai_cmd.state();
    //     if(state.has_value()) setEnabled(state.value());
    // }

    /// Set module activity state
    void setEnabled(bool new_enabled) {
        if (enabled != new_enabled) {
            enabled = new_enabled;
            cout << name << ": activity "
                 << (enabled ? "ON (monitoring)" : "OFF (disabled)") << "\n";
            if (onStateChanged) {
                onStateChanged(enabled);
            }
        }
    }

    /// Check if module is active
    bool isEnabled() const {
        return enabled;
    }

    /// Simulate module work (called periodically when enabled)
    void work() {
        if (enabled) {
            cout << name << ": working — monitoring for fire...\n";
        } else {
            cout << name << ": inactive — skipping\n";
        }
    }

    json toJson() const override {
        auto j = Target::toJson();
        j["enabled"] = enabled;
        return j;
    }
};


/// Fire Detector Target example.
/// Target type: "fire_detector"
/// Handles events with type == "analitics", subtype == "fire"
/// Processing an event turns on|off module activity (enabled/disabled).
class FireDetector : public Detector {
public:
    FireDetector(const string& instance_name = "fire_detector_1")
    : Detector(instance_name){
        type_name = "fire_detector";
        // Subscribe to analitics:fire events
        auto cmdKey = string(RuleType(RuleType::Analitic)._to_string()) + ":"s + AnaliticCmdType(AnaliticCmdType::toggle)._to_string();
        supported_cmds = {std::move(cmdKey)};
    }

    /// Process event: handles type=="analitics", subtype=="fire"
    /// The event data contains "enabled" field that turns on/off module activity
    void procEvent(Command cmd) override {
        cout << "FireDetector '" << name << "': received event "
             << cmd.second.toString() << endl;

        const auto& rule = cmd.first;
        const auto& evn = cmd.second;
        if(rule.type._value != RuleType::Analitic){
            cout << __PRETTY_FUNCTION__ << " skip cmd: " << rule.toString() << endl;
            return;
        }

        const auto& ai_cmd = static_cast<const AnaliticCmd &>(rule);
        if(ai_cmd.detector() !=  DetectorType(DetectorType::Fire)._to_string()){
            cout << __PRETTY_FUNCTION__ << " skip cmd: " << rule.toString() << endl;
            return;
        }

        try{
            auto s_type = ai_cmd.subtype()._value;
            if(s_type == AnaliticCmdType::toggle){
                evn.data.at("state");
                setEnabled(evn.data.at("state"));    
            }
        }
        catch(const exception &ex){
            cout << __PRETTY_FUNCTION__ << ": " << ex.what() << endl;
        }
    }
};


class WeaponDetector : public Detector {
public:
    WeaponDetector(const string& instance_name = "weapon_detector_1")
    : Detector(instance_name){
        type_name = "weapon_detector";
        // Subscribe to analitics:fire events
        auto cmdKey = string(RuleType(RuleType::Analitic)._to_string()) + ":"s + DetectorType(DetectorType::Weapon)._to_string();
        supported_cmds = {std::move(cmdKey)};
    }

    /// Process event: handles type=="analitics", subtype=="fire"
    /// The event data contains "enabled" field that turns on/off module activity
    void procEvent(Command cmd) override {
        const auto& rule = cmd.first;
        const auto& evn = cmd.second;
        cout << "WeaponDetector '" << name << "': received event "
             << evn.toString() << endl;

        if(rule.type._value != RuleType::Analitic){
            cout << __PRETTY_FUNCTION__ << " sckip cmd: " << rule.toString() << endl;
            return;
        }

        const auto& ai_cmd = static_cast<const AnaliticCmd &>(rule);
        if(ai_cmd.detector() !=  DetectorType(DetectorType::Weapon)._to_string()){
            cout << __PRETTY_FUNCTION__ << " sckip cmd: " << rule.toString() << endl;
            return;
        }

        try{
            auto s_type = ai_cmd.subtype()._value;
            if(s_type == AnaliticCmdType::toggle){
                evn.data.at("state");
                setEnabled(evn.data.at("state"));    
            }
        }
        catch(const exception &ex){
            cout << __PRETTY_FUNCTION__ << ": " << ex.what() << endl;
        }
    }
};