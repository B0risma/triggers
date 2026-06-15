#pragma once
#include <string>
#include <iostream>
#include <functional>
#include "event_system/command.hpp"
#include "json.hpp"
#include "target.hpp"
#include "event.hpp"

using json = nlohmann::json;
using namespace std;

/// Fire Detector Target example.
/// Target type: "fire_detector"
/// Handles events with type == "analitics", subtype == "fire"
/// Processing an event turns on|off module activity (enabled/disabled).
class FireDetector : public Target {
public:
    /// Whether the fire detection module is currently active
    bool enabled = false;

    /// Callback for when activity state changes (for external notification)
    function<void(bool)> onStateChanged;

    FireDetector(const string& instance_name = "fire_detector_1") {
        name = instance_name;
        type_name = "fire_detector";
        // Subscribe to analitics:fire events
        supported_events = {"analitics:fire"};
    }

    /// Process event: handles type=="analitics", subtype=="fire"
    /// The event data contains "enabled" field that turns on/off module activity
    void procEvent(const Command& evn) override {
        cout << "FireDetector '" << name << "': received event "
             << evn.toString() << endl;

        if (evn.type == "analitics" && evn.subtype == "fire") {
            // Check if event data has "enabled" field
            if (evn.data.contains("enabled")) {
                bool new_state = evn.data["enabled"];
                setEnabled(new_state);
            }
            // Also check "detector" field for identification
            if (evn.data.contains("detector")) {
                cout << "FireDetector '" << name << "': detector source is '"
                     << evn.data["detector"] << "'\n";
            }
        } else if (evn.type == "gpio") {
            // GPIO state change can also affect fire detector
            // (e.g., a physical alarm button triggers it)
            if (evn.data.contains("state")) {
                bool gpio_state = evn.data["state"];
                setEnabled(gpio_state);
                cout << "FireDetector '" << name << "': GPIO triggered, state="
                     << gpio_state << "\n";
            }
        }
    }

    /// Set module activity state
    void setEnabled(bool new_enabled) {
        if (enabled != new_enabled) {
            enabled = new_enabled;
            cout << "FireDetector '" << name << "': activity "
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
            cout << "FireDetector '" << name << "': working — monitoring for fire...\n";
        } else {
            cout << "FireDetector '" << name << "': inactive — skipping\n";
        }
    }

    json toJson() const override {
        auto j = Target::toJson();
        j["enabled"] = enabled;
        return j;
    }
};
