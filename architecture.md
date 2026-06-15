Event-based system implemented and fully tested. All new files are in [`source/event_system/`](source/event_system/) — no existing files were edited (except [`CMakeLists.txt`](CMakeLists.txt) to add the `server_v2` target).

## Architecture Overview

```
Trigger (source) → EventQueue → Action (event set) → Target (handler)
```

### New Files Created

| File | Purpose |
|------|---------|
| [`event.hpp`](source/event_system/event.hpp) | Core `Event` struct with type/subtype/data, JSON serialization, composite routing key |
| [`target.hpp`](source/event_system/target.hpp) | `Target` base class + `TargetList` registry (system-only editable, not via API) |
| [`trigger.hpp`](source/event_system/trigger.hpp) | `Trigger` base class + `TriggerList` registry (not editable via API, lists by kind) |
| [`trigger.cpp`](source/event_system/trigger.cpp) | `Trigger::emitEvent()` and `setEventQueue()` implementations |
| [`action.hpp`](source/event_system/action.hpp) | `Action` (named event set), `TriggerActionLink`, `ActionList` (API-editable) |
| [`event_queue.hpp`](source/event_system/event_queue.hpp) | `EventQueue` — central event router with async worker thread |
| [`event_queue.cpp`](source/event_system/event_queue.cpp) | Event delivery, trigger→action pipeline, subscription management |
| [`api_handler.hpp`](source/event_system/api_handler.hpp) | `APIHandler` — httplib route registration and request handling |
| [`api_handler.cpp`](source/event_system/api_handler.cpp) | All API endpoint handlers (GET/POST/DELETE for triggers, targets, actions, links) |
| [`gpio_trigger.hpp`](source/event_system/gpio_trigger.hpp) | `GPIOTrigger` example — emits signal on every GPIO state change, with simulation mode |
| [`fire_detector.hpp`](source/event_system/fire_detector.hpp) | `FireDetector` example target — handles `analitics:fire` events, turns on/off activity |
| [`main_v2.cpp`](source/main_v2.cpp) | New entry point wiring all modules together with demo + HTTP server |

### API Endpoints (matching API2.json)

| Method | Path | Description | Access |
|--------|------|-------------|--------|
| GET | `/trigger` | Trigger list by kinds → `{"gpio":["in1","in2"]}` | Read-only |
| GET | `/target` | Target list with status | Read-only (system-only editable) |
| GET | `/action` | List all actions with cmds | Read |
| POST | `/action` | Create action with cmds | Write |
| DELETE | `/action?name=X` | Delete action | Write |
| GET | `/link` | List trigger-action links | Read |
| POST | `/link` | Link trigger to action (with optional condition) | Write |
| DELETE | `/link?trigger=X&action=Y` | Unlink trigger from action | Write |

### Verified Output

- **`GET /trigger`** → `{"gpio":["in1","in2"]}` — matches API2.json format
- **`GET /target`** → fire detectors with `enabled` state and `supported_events`
- **`POST /action`** → creates action, returns `{"status":"created"}`
- **`POST /link`** → links trigger to action, pipeline works end-to-end
- **Event pipeline**: GPIO state change → `EventQueue::processTriggerEvent` → finds linked actions → delivers cmds to subscribed targets → `FireDetector::procEvent` toggles activity

Build with: `cmake --build build --target server_v2`
Run with: `./build/server_v2`