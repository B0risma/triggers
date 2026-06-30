# API Reference — Event-Driven Control System

Base URL: `http://<host>:<port>`
Content-Type: `text/json`

---

## 1. Triggers (Read-Only)

### GET `/trigger`

List all triggers in the system, grouped by trigger kind.

**Response 200** — Trigger list keyed by kind:
```json
{
  "GPIO": [
    {"name": "in1", "event": "GPIO:in1"},
    {"name": "in2", "event": "GPIO:in2"}
  ],
  "VirtSwitch": [
    {"name": "sw1", "event": "VirtSwitch:sw1"}
  ],
  "Analitic": [
    {"name": "Fire", "event": "Analitic:Fire"},
    {"name": "Weapon", "event": "Analitic:Weapon"}
  ],
  "Shedule": [
    {"name": "day_schedule", "event": "Shedule:day_schedule"}
  ]
}
```

**Response 500** — Internal error:
```json
{"error": "error message"}
```

---

## 2. Actions (CRUD)

### GET `/action`

List all defined actions.

**Response 200** — Object keyed by action name:
```json
{
  "act_set1": {
    "name": "act_set1",
    "rules": [
      {
        "rule_type": "Toggle",
        "target": "Fire",
        "target_type": "Analitic",
        "detector": "Fire"
      }
    ]
  },
  "act_set2": {
    "name": "act_set2",
    "rules": [
      {
        "rule_type": "Preset",
        "target": "",
        "target_type": "Video",
        "preset_on": "preset1",
        "preset_off": "preset2"
      }
    ]
  }
}
```

### GET `/action?range=1`

Get the schema/range of valid rule types and their fields.

**Response 200** — Rule type range schema:
```json
{
  "Analitic": {
    "Toggle": {
      "detector": ["Fire", "Weapon"]
    }
  },
  "Video": {
    "Preset": {
      "preset_on": "string",
      "preset_off": "string"
    }
  },
  "Alarm": {
    "Toggle": {
      "target": ["WhiteLight", "RedBlue"]
    },
    "OneShot": {
      "target": ["WhiteLight", "RedBlue", "Sound"]
    }
  },
  "target": ["Invalid", "Video", "Analitic", "Alarm"],
  "type": ["Toggle", "OneShot", "Number", "Preset"]
}
```

### POST `/action`

Create a new action with one or more rules.

**Request body:**
```json
{
  "name": "my_action",
  "rules": [
    {
      "rule_type": "Toggle",
      "target_type": "Analitic",
      "target": "Fire",
      "detector": "Fire"
    }
  ]
}
```

**Response 201** — Created:
```json
{
  "status": "created",
  "name": "my_action"
}
```

**Response 400** — Bad request (invalid data):
```json
{"error": "error description"}
```

**Response 500** — Server error:
```json
{"error": "no action registry"}
```

### DELETE `/action?name=<action_name>`

Delete an action by name. Also removes all links referencing this action.

**Response 200** — Deleted:
```json
{
  "status": "deleted",
  "name": "my_action"
}
```

**Response 400** — Bad request:
```json
{"error": "error description"}
```

---

## 3. Trigger-Action Links (CRUD)

### GET `/link`

List all trigger-to-action links.

**Response 200** — Array of link objects:
```json
[
  {"event": "GPIO:in1", "action": "act_set1"},
  {"event": "Analitic:Fire", "action": "act_fire_on"},
  {"event": "Shedule:day_schedule", "action": "act_night_mode"}
]
```

### POST `/link`

Create a link between a trigger and an action.

**Request body:**
```json
{
  "event": "GPIO:in1",
  "action": "act_set1"
}
```

**Response 200** — Linked:
```json
{
  "status": "linked",
  "Source": "GPIO:in1",
  "action": "act_set1"
}
```

**Response 400** — Bad request:
```json
{"error": "error description"}
```

### DELETE `/link?trigger=<trigger_key>&action=<action_name>`

Remove a trigger-action link.

**Request example:**
```
DELETE /link?trigger=GPIO:in1&action=act_set1
```

**Response 200** — Unlinked:
```json
{
  "status": "unlinked",
  "trigger": "GPIO:in1",
  "action": "act_set1"
}
```

**Response 400** — Bad request:
```json
{"error": "error description"}
```

---

## 4. Virtual Switches (CRUD)

### POST `/switch`

Create a new virtual switch.

**Request body:**
```json
{
  "name": "sw1",
  "state": true
}
```

**Response 200** — Created:
```json
{"status": "created"}
```

### PATCH `/switch`

Update a virtual switch state (triggers state change event).

**Request body:**
```json
{
  "name": "sw1",
  "state": false
}
```

**Response 200** — Updated:
```json
{"status": "updated"}
```

### DELETE `/switch?name=<switch_name>`

Delete a virtual switch.

```
DELETE /switch?name=sw1
```

**Response 200** — Deleted:
```json
{"status": "deleted"}
```

---

## 5. Action Creation Samples (by target type)

### 5.1 Analitic — Toggle fire detector

```json
{
  "name": "act_fire_toggle",
  "rules": [
    {
      "rule_type": "Toggle",
      "target_type": "Analitic",
      "target": "Fire",
      "detector": "Fire"
    }
  ]
}
```

### 5.2 Analitic — Toggle weapon detector

```json
{
  "name": "act_weapon_toggle",
  "rules": [
    {
      "rule_type": "Toggle",
      "target_type": "Analitic",
      "target": "Weapon",
      "detector": "Weapon"
    }
  ]
}
```

### 5.3 Video — Preset command

```json
{
  "name": "act_camera_preset",
  "rules": [
    {
      "rule_type": "Preset",
      "target_type": "Video",
      "target": "",
      "preset_on": "preset1",
      "preset_off": "preset2"
    }
  ]
}
```

### 5.4 Alarm — Toggle WhiteLight

```json
{
  "name": "act_light_toggle",
  "rules": [
    {
      "rule_type": "Toggle",
      "target_type": "Alarm",
      "target": "WhiteLight"
    }
  ]
}
```

### 5.5 Alarm — Toggle RedBlue

```json
{
  "name": "act_flash_toggle",
  "rules": [
    {
      "rule_type": "Toggle",
      "target_type": "Alarm",
      "target": "RedBlue"
    }
  ]
}
```

### 5.6 Alarm — OneShot Sound (siren)

```json
{
  "name": "act_siren",
  "rules": [
    {
      "rule_type": "OneShot",
      "target_type": "Alarm",
      "target": "Sound"
    }
  ]
}
```

### 5.7 Alarm — OneShot WhiteLight (flash)

```json
{
  "name": "act_light_flash",
  "rules": [
    {
      "rule_type": "OneShot",
      "target_type": "Alarm",
      "target": "WhiteLight"
    }
  ]
}
```

### 5.8 Multiple rules in one action

```json
{
  "name": "act_full_alarm",
  "rules": [
    {
      "rule_type": "Toggle",
      "target_type": "Analitic",
      "target": "Fire",
      "detector": "Fire"
    },
    {
      "rule_type": "Toggle",
      "target_type": "Analitic",
      "target": "Weapon",
      "detector": "Weapon"
    },
    {
      "rule_type": "OneShot",
      "target_type": "Alarm",
      "target": "Sound"
    },
    {
      "rule_type": "Toggle",
      "target_type": "Alarm",
      "target": "RedBlue"
    }
  ]
}
```

---

## 6. Virtual Switch Lifecycle Example

### 6.1 Create switch

```
POST /switch
Content-Type: text/json

{"name": "sw1", "state": true}
```

### 6.2 Link switch trigger to an action

```
POST /link
Content-Type: text/json

{"event": "VirtSwitch:sw1", "action": "act_fire_toggle"}
```

### 6.3 Toggle switch state (fires event)

```
PATCH /switch
Content-Type: text/json

{"name": "sw1", "state": false}
```

### 6.4 Delete switch

```
DELETE /switch?name=sw1
```

---

## 7. Complete Workflow Example

### Step 1 — Create actions

```
POST /action
{"name": "act_fire_toggle", "rules": [{"rule_type":"Toggle","target_type":"Analitic","target":"Fire","detector":"Fire"}]}

POST /action
{"name": "act_siren", "rules": [{"rule_type":"OneShot","target_type":"Alarm","target":"Sound"}]}

POST /action
{"name": "act_cam_preset", "rules": [{"rule_type":"Preset","target_type":"Video","target":"","preset_on":"preset1","preset_off":"preset2"}]}
```

### Step 2 — Link triggers to actions

```
POST /link
{"event": "Analitic:Fire", "action": "act_fire_toggle"}

POST /link
{"event": "Analitic:Fire", "action": "act_siren"}

POST /link
{"event": "Analitic:Fire", "action": "act_cam_preset"}
```

### Step 3 — Now when a Fire analytics event is emitted, three actions execute:
1. Fire detector toggles monitoring on
2. Siren sounds (one-shot)
3. Camera switches to preset position

---

## Error Responses Summary

| HTTP Status | Meaning |
|-------------|---------|
| 200 | Success |
| 400 | Bad request — invalid JSON, missing required fields |
| 500 | Internal server error — registry not initialized |

All error responses follow the format:
```json
{"error": "human-readable description"}