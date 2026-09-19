# 🌀 Ventilation Group Component

This folder contains the core ESPHome custom component for managing decentralized VentoSync ventilation units. It implements high-level group coordination, hardware sensor abstraction, and the finite state machine (FSM) for synchronized push-pull heat recovery.

## 📄 File Overview

| File | Description |
| :--- | :--- |
| **`ventilation_group.h`** | **Group Coordinator (`VentilationController`)**: Manages local device state, Master/Slave roles, ESP-NOW v9 mesh communication, LRU peer discovery, Window Guard contact evaluation, and hardware sensor routing (Fan PWM, RPM tacho, Board/NTC/SCD41 temperature). |
| **`ventilation_state_machine.h` / `.cpp`** | **Deterministic Cycle Engine (`VentilationStateMachine`)**: Pure logic state machine governing direction cycles, half-cycle timing (default 70 s), smooth 5-second soft ramps (`RAMP_DURATION_MS`), and operating modes (`MODE_OFF`, `MODE_ECO_RECOVERY`, `MODE_VENTILATION`, `MODE_STOSSLUEFTUNG`). |
| **`__init__.py`** | **ESPHome Python Generator**: Registers the `VentilationController` component and `GlobalsComponent` in ESPHome, exposing YAML configuration options (`room_id`, `device_id`, `floor_id`, `is_phase_a`, fan outputs, and sensor bindings). |

## ⚙️ Key Mechanisms

- **Deterministic State Machine**: Calculates `HardwareState` (fan enable, airflow direction, and linear ramp factor $[0.0 \dots 1.0]$) at sub-second precision to ensure soft direction reversals without acoustic noise.
- **Push-Pull Phase Assignment**: Units in Phase A blow inwards while units in Phase B exhaust outwards, alternating simultaneously every half-cycle (70 seconds).
- **Stoßlüftung Sub-Cycles**: Implements an automated 2-hour burst ventilation cycle (15 minutes active boost with alternating airflow, followed by a 105-minute energy-saving pause).
- **ESP-NOW Wireless Mesh**: Communicates with `network_sync.h` using binary packets (`VentilationPacket`, v9 protocol) to mirror master operating mode and target fan level across the room.
