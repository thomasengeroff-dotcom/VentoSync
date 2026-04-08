# 🌀 Ventilation Group Component

This folder contains the core ESPHome custom component for managing a group of VentoSync units. It implements the high-level coordination and state management required for synchronized push-pull ventilation.

## 📄 File Overview

| File | Description |
| :--- | :--- |
| **`ventilation_group.h`** | **Group Coordinator**: Manages the collective state of all units assigned to the same room. Handles Master/Slave roles, ESP-NOW synchronization, and unified demand processing. |
| **`ventilation_state_machine.h` / `.cpp`** | **Cycle Logic**: Implements the finite state machine (FSM) for the ventilation cycles. Handles phases like `SUPPLY_AIR`, `EXHAUST_AIR`, `OFF`, and the logic for ramping speeds between phases. |
| **`__init__.py`** | **ESPHome Integration**: The Python bridge that registers this component within the ESPHome framework, allowing it to be configured via YAML (e.g., setting the Room ID and Phase). |

## ⚙️ Key Mechanisms

- **State Machine**: Ensures that direction changes are synchronized and that speed ramps are respected to protect hardware and minimize noise.
- **Master Authority**: In most room groups, the device with the lowest ID (usually ID 1) acts as the Master to provide the timing pulses for the alternating phases.
- **ESP-NOW Integration**: Communicates with the `network_sync.h` helper to propagate state changes across the group.
