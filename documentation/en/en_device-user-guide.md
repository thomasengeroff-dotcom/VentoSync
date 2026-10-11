# 📘 VentoSync – On-Device User Guide

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_device-user-guide.md)

This guide covers **only the operation directly on the ventilation unit**: the three buttons, the nine
indicator LEDs and the blink codes. Setup, Home Assistant and the web dashboard are described in the other
documents (see [Further Reading](#-further-reading)).

Mode names are given in German as they appear on the device's Home Assistant and dashboard UI
(e.g. *Smart-Automatik*, *Wärmerückgewinnung*).

<p align="center">
  <img src="../../images/Ventomax%20V-WRG-1/PXL_20260128_232625674.jpg" alt="VentoSync control panel on the ventilation unit" width="500" />
</p>

---

## 📑 Contents

1. [Quick Reference](#-quick-reference)
2. [Control Panel Overview](#-control-panel-overview)
3. [The Buttons](#-the-buttons)
4. [Operating Modes](#-operating-modes)
5. [Fan Level (1–10)](#-fan-level-110)
6. [Reading the LEDs](#-reading-the-leds)
7. [Master LED Error and Warning Codes](#-master-led-error-and-warning-codes)
8. [Child Lock](#-child-lock)
9. [Several Units in One Room](#-several-units-in-one-room)
10. [Power-Up, Restart and Power Failure](#-power-up-restart-and-power-failure)
11. [States the Unit Enters on Its Own](#-states-the-unit-enters-on-its-own)
12. [Troubleshooting](#-troubleshooting)
13. [Safety Notes](#️-safety-notes)

---

## ⚡ Quick Reference

| I want to … | How |
| :--- | :--- |
| switch the room's ventilation **off or on** | Press the **Power button** briefly |
| change the **operating mode** | Press the **Mode button (M)** briefly – each press moves to the next mode |
| ventilate **more or less** | Press the **Level button (+)** briefly (one level up) or hold it (levels cycle) |
| **lock / unlock the buttons** (child lock) | Hold the **Mode button** for about **5 seconds** |
| **restart** the unit | Hold the **Power button** for more than **10 seconds** |
| **see the current state** while the LEDs are dark | Press a button – or wait until something changes in the room |

> [!IMPORTANT]
> **All settings apply to the whole room.** If several VentoSync units are set up in the same room, all of
> them follow a change, no matter which unit you operate. Supply and exhaust air always stay balanced.

---

## 🖐️ Control Panel Overview

The original VentoMaxx control panel is retained:

- **3 buttons:** Power (I/O), Mode (M), Level (+)
- **9 green LEDs:**

| LED | Count | Purpose |
| :--- | :---: | :--- |
| **Power LED** | 1 | Ready indicator. Bright after an interaction, then permanently dim (10 %). |
| **Master LED** (centre) | 1 | Glows dimly on the room's **Master unit** (device no. 1); shows errors and warnings by blink code on **every** unit. |
| **Mode LED left** ("WRG") | 1 | Shows the operating mode together with the right mode LED. |
| **Mode LED right** ("VEN") | 1 | Shows the operating mode together with the left mode LED. |
| **Level LEDs** | 5 | Bar graph for fan level 1–10. |

> [!NOTE]
> **A dark panel is normal.** 30 seconds after the last interaction the mode and level LEDs switch off and
> the Power LED dims to 10 % ("stealth mode") so the unit does not disturb at night. The unit keeps running
> unchanged.

---

## 🔘 The Buttons

### Power (I/O) – room on/off

| Action | Effect |
| :--- | :--- |
| **Short press** (up to 10 s) | Room **on**: switches to **Aus** (off – fans stop). <br>Room **off**: returns to the **last used mode** (factory default: Smart-Automatik). |
| **Very long press** (over 10 s) | **Restarts** the unit. Mode and settings are kept. Useful if the unit stops responding or keeps reporting a connection error. |

> In **Aus** the sensors, Wi-Fi and the link to the other units stay active – the unit keeps measuring, only
> the fans stop. There is no sleep or power-saving state.

### Mode (M) – select operating mode

| Action | Effect |
| :--- | :--- |
| **Short press** | Moves to the next mode:<br>**Smart-Automatik → Wärmerückgewinnung → Durchlüften → Stoßlüftung → Aus → Smart-Automatik …** |
| **Hold for about 5 s** | Toggles the **child lock** (see [Child Lock](#-child-lock)). The mode is **not** changed. |

### Level (+) – fan strength

| Action | Effect |
| :--- | :--- |
| **Short press** | One level up (1 → 2 → … → 10 → 1). |
| **Hold** | After about 0.75 s the levels cycle automatically (about 2 levels per second): up to 10, then back down to 1, and so on. **Release** when the desired level is reached. |

Good to know:

- In **Aus** the Level button does nothing – switch on first.
- During the first **10 seconds after power-up / restart** the Level button is ignored.
- In **Smart-Automatik** the Level button overrides the automatic for **60 minutes** by default (see
  [Overriding the level in Smart-Automatik](#overriding-the-level-in-smart-automatik)). For a permanently
  fixed level choose **Wärmerückgewinnung**.

> [!TIP]
> Every button press takes effect **immediately**, even while the panel is dark – there is no "wake-up press".
> If you only want to see the state without changing anything, check Home Assistant or the web dashboard.

---

## 🔄 Operating Modes

| # | Mode | Left mode LED (WRG) | Right mode LED (VEN) | What happens |
| :-: | :--- | :---: | :---: | :--- |
| 1 | **Smart-Automatik** *(default)* | 🟢 **pulses slowly** (fast: level overridden) | ⚫ | Ventilates on its own with heat recovery and sets the level by CO2 and humidity. |
| 2 | **Wärmerückgewinnung** (heat recovery) | 🟢 on | ⚫ | Alternating operation with heat recovery at the fixed level you choose. |
| 3 | **Durchlüften** (cross-ventilation) | 🟢 on | 🟢 on | Continuous airflow in one direction (e.g. to cool down in summer). Ends after **30 minutes** by default. |
| 4 | **Stoßlüftung** (boost ventilation) | ⚫ | 🟢 on | 15 minutes of strong one-way ventilation, then a 105-minute pause – repeating. |
| 5 | **Aus** (off) | ⚫ | ⚫ | Fans stopped. Only the Power LED is lit. |

### 1. Smart-Automatik (recommended)

- Runs alternating heat-recovery operation at a low base level (factory default level 2).
- When **CO2** or **humidity** rises, the unit raises the level slowly and smoothly (at most one level every
  10 seconds; up to level 7 by default) and lowers it again once the air improves.
- If it is noticeably cooler outside than inside in summer, the automatic may switch to **Durchlüften** on
  its own. The left mode LED **keeps pulsing** – the unit stays in Smart-Automatik.
- Units without their own sensors also respond, because they adopt the demand of the other units in the room.

#### Overriding the level in Smart-Automatik

For "more / less air right now" (cooking, visitors, a quiet night) simply press the **Level button**:

- The chosen level applies **for 60 minutes to the whole room**; the automatic pauses meanwhile. The duration
  can be set between 30 and 300 minutes in Home Assistant (*Smart-Automatik: Übersteuerung Dauer*). Afterwards it
  continues smoothly from that level (one level every 10 s).
- The left mode LED **pulses fast** (instead of slowly) while the override runs.
- Another press selects the next level and restarts the countdown.
- **End it early:** change the mode or select Smart-Automatik again.
- The override may lie below or above the configured automatic limits. **Safety comes first:** while the air
  conditioner runs (climate coordination) at most its maximum level is used; with very high CO2 (factory
  default 1500 ppm) or a mold risk the automatic may still ventilate harder.
- The same happens when the level is changed in Smart-Automatik in Home Assistant or the web dashboard.
- The override is not stored permanently: a restarted unit adopts it from the other units of the room; if all
  units restart at once (power failure), it ends.

### 2. Wärmerückgewinnung (heat recovery)

- The air direction changes regularly: 70 s (level 1) to 50 s (level 10) per direction. The ceramic core keeps
  the heat in the room.
- At every direction change the fan slows down and speeds up again smoothly (about 5 s each) – a short
  moment of silence is normal.
- The level stays fixed until you change it.

### 3. Durchlüften (cross-ventilation)

- No direction changes: one unit draws outdoor air in, its partner unit blows room air out.
- **Time limit:** by default cross-ventilation ends after **30 minutes**; the room then switches to
  **Wärmerückgewinnung** automatically (the right mode LED goes off). The duration (0–120 min, 0 = unlimited)
  is set in Home Assistant or the web dashboard.
- No heat recovery – use only briefly in winter.

### 4. Stoßlüftung (boost ventilation)

- Every 2 hours: **15 minutes of ventilation** in one direction at the set level, then a **105-minute pause**
  (fans stopped). Every second run blows in the opposite direction.
- **Standstill is normal here:** most of the time the fans are stopped although the mode LED is lit.
- The strength of each run is set with the **Level button**. Choose a high level for a strong boost.
- Runs until another mode is selected.

### 5. Aus (off)

- The fans of **all units in the room** stop. Sensors and radio link stay active.
- Switch on with the **Power button** (back to the last mode) or the **Mode button** (starts with Smart-Automatik).

---

## 📶 Fan Level (1–10)

The 10 levels are shown on the **5 level LEDs**. Each LED stands for two levels:
**dim = odd level**, **bright = even level**.

| Level | LED 1 | LED 2 | LED 3 | LED 4 | LED 5 |
| :-: | :-: | :-: | :-: | :-: | :-: |
| 1 | ◐ | ○ | ○ | ○ | ○ |
| 2 | ● | ○ | ○ | ○ | ○ |
| 3 | ● | ◐ | ○ | ○ | ○ |
| 4 | ● | ● | ○ | ○ | ○ |
| 5 | ● | ● | ◐ | ○ | ○ |
| 6 | ● | ● | ● | ○ | ○ |
| 7 | ● | ● | ● | ◐ | ○ |
| 8 | ● | ● | ● | ● | ○ |
| 9 | ● | ● | ● | ● | ◐ |
| 10 | ● | ● | ● | ● | ● |

● bright · ◐ dim (about 20 %) · ○ off

Good to know:

- The LEDs show the **target level** immediately. The fan ramps there smoothly within a few seconds.
- In Smart-Automatik the LEDs show the level **chosen by the automatic** – or, during an override, the chosen
  level (capped to the climate coordination maximum while it is active).
- If a **presence adjustment** is configured in Home Assistant, the fan runs a little stronger or weaker
  while presence is detected; the LEDs keep showing the base level.
- In **Aus** all level LEDs are dark.

---

## 🔆 Reading the LEDs

### Normal display

| Situation | Power | Master | Mode LEDs | Level LEDs |
| :--- | :---: | :---: | :---: | :---: |
| After an interaction (for 30 s) | bright | Master unit: dim on · others: off | mode (see above) | level |
| Idle (after 30 s) | dim (10 %) | Master unit: dim on · others: off | off | off |
| Mode **Aus** | bright, then dim | as above | off | off |
| Power-up / restart (about 3 s) | all LEDs bright (LED self-test) | | | |

- The **maximum LED brightness** (5–100 %, default 80 %) is set in Home Assistant or the web dashboard and
  applies to the whole room.
- When someone changes the mode or level on another unit in the room, **all units in the room light up
  together** and show the new state for 30 seconds.

### Short flashes of all LEDs

| Signal | Meaning |
| :--- | :--- |
| All LEDs flash **2× slowly** | Child lock was **switched on or off** (confirmation). |
| All LEDs flash **3× quickly** | Button is **locked** – the child lock is active, the press was ignored. |

---

## 🚨 Master LED Error and Warning Codes

Faults and special states are shown by the **centre Master LED** with a repeating blink pattern. The codes
appear on **every** unit (not only the Master) and regardless of whether the panel is currently dimmed.

| Pattern | Meaning | Cause | What to do |
| :--- | :--- | :--- | :--- |
| **2× long** (0.5 s each), then 5 s pause | **Lost connection to a partner unit** | A unit of the same room has not reported for more than 3 minutes (e.g. no power, defective, out of range). Only active when "ESP-NOW Peerprüfung" (peer check) is switched on in Home Assistant (on by default); at the earliest 3 minutes after start-up. | Check that all units of the room have power (Power LED lit?). Restart the affected unit if necessary (Power > 10 s). If a unit is permanently out of service, switch off the peer check in Home Assistant. |
| **3× long** (0.5 s each), then 5 s pause | **Wi-Fi connection lost** | The unit has been connected to neither Wi-Fi nor Home Assistant for more than 30 seconds. | Check router / Wi-Fi. Ventilation **continues normally**; coordination between the units of the room also works without Wi-Fi. If the unit cannot rejoin the Wi-Fi at all, it opens its own hotspot for reconfiguration (see [Troubleshooting](#-troubleshooting)). |
| **4× very fast** (0.1 s each), then 2 s pause, **full brightness** | **Overheating warning** | The temperature inside the unit is **50–60 °C** (power supply limit range). | Find the heat source (direct sun on the indoor unit, heating, covered vents). The warning clears automatically below 48 °C. **At 60 °C the unit shuts down for protection** (see below). |
| **Slow pulse** (1 s on, 2 s off) | **Window open – ventilation paused** | Home Assistant reports an open window in the room; after 5 s all fans of the room stop. The pulse is shown for only about **30 seconds** (then off so it does not disturb at night) – the pause itself continues. | Close the window – ventilation resumes automatically in the selected mode. |
| **Permanently dim on** | **No error** – this unit is the room's **Master** | – | – |
| **Off** | **No error** (non-Master unit) | – | – |

**Priority:** if several states occur at once, the LED shows only the most important one:
partner unit (2×) → Wi-Fi (3×) → overheating (4×) → window open.

### Emergency shutdown on overheating (above 60 °C)

When the temperature inside the unit reaches **60 °C**, the fan is stopped and the unit switches itself
**completely off for 60 minutes**. During this time **all LEDs are dark** and the buttons do not respond.
Afterwards the unit restarts on its own (LED self-test). If it is still too hot, the cycle repeats.

> [!WARNING]
> A unit that repeatedly goes completely dark may be overheating. Remove the cause or consult a qualified
> electrician. **Never open the housing while it is live.**

---

## 🔒 Child Lock

The child lock disables **all three buttons** on the unit. Home Assistant and the web dashboard remain fully
usable.

| Action | How | Feedback |
| :--- | :--- | :--- |
| **Enable** | Hold the **Mode button** for about **5 seconds** | all LEDs flash **2×** |
| **Disable** | Hold the **Mode button** again for about **5 seconds** | all LEDs flash **2×** |
| Press a button while locked | – | all LEDs flash **3× quickly**, nothing happens |

- The Mode button can still be held for 5 seconds while locked – this is the only way to release the lock on
  the unit itself.
- On the unit you can only tell whether the lock is on by pressing a button briefly (3× flash = locked).
  Alternatively: the "Kindersicherung" switch in Home Assistant.
- The lock survives restarts and power failures.
- The child lock applies **per unit**, not to the whole room.
- **Exception:** the restart via the Power button (> 10 s) also works while the lock is active.

---

## 🔗 Several Units in One Room

Ventilation units work in pairs (one blows in, the other blows out). VentoSync units of the same room
coordinate directly by radio – even without a Wi-Fi router.

- **Operating any one unit is enough:** mode, level and on/off are applied to all units of the room.
- All units of the room **light up together** when something changes.
- The unit with **number 1** is the **Master** (Master LED dimly on). In Smart-Automatik it sets the level, and
  it sets the timing of Stoßlüftung.
- If a unit fails, the others keep running; with the peer check enabled the Master LED shows **2×**.

---

## 🔌 Power-Up, Restart and Power Failure

1. After power-up (or a restart) **all LEDs light up brightly for about 3 seconds** (self-test).
2. The panel then shows the current mode and level and dims after 30 s.
3. During the first 10 seconds the Level button is ignored.
4. Mode, level and child lock survive restarts and power failures. A restarting unit adopts the state of the
   other units in the room.

---

## 🌙 States the Unit Enters on Its Own

Some changes do not come from the buttons but from Home Assistant or the automatic. How to recognise them
on the unit:

| State | Visible on the unit | Note |
| :--- | :--- | :--- |
| **Window open** (window guard) | Fans stopped, Master LED pulses for about 30 s | Resumes automatically once the window is closed. Individual units may be excluded from the lock in Home Assistant. |
| **Durchlüften timer expired** | Right mode LED off, left on → Wärmerückgewinnung | Normal, see [Durchlüften](#3-durchlüften-cross-ventilation). |
| **Summer cooling** in Smart-Automatik | Constant airflow, left mode LED keeps pulsing | Normal, the automatic returns to heat recovery by itself. |
| **Vacation mode** (via Home Assistant) | By default Stoßlüftung at level 1 (right mode LED, level LED 1 dim) | Ended via Home Assistant; the unit then restores the previous mode. |
| **Air conditioning active** (climate coordination) | Smart-Automatik ventilates at most at level 3 | With very high CO2 or humidity the unit still ramps up. |
| **Overheating** | Master LED 4× fast, or unit completely dark | See [Emergency shutdown](#emergency-shutdown-on-overheating-above-60-c). |

> The **filter change reminder** (after about 1 year of runtime or 3 years) is **not** shown on the unit; it is
> reported in Home Assistant.

---

## 🛠️ Troubleshooting

| Problem | Possible cause | Solution |
| :--- | :--- | :--- |
| Panel dark, only the Power LED dimly lit | Normal idle state after 30 s | Not a fault. |
| **All** LEDs dark, including the Power LED | No power, or emergency shutdown due to overheating (up to 60 min) | Check the circuit breaker. If overheated, wait and remove the cause. |
| Fan stopped, mode LED lit | Stoßlüftung pause, window open, or brief direction change (Wärmerückgewinnung) | Normal. |
| Fan stopped, both mode LEDs off | Mode **Aus** | Press the Power button briefly. |
| Level button does not respond | Mode **Aus**, unit just started (< 10 s) or child lock (3× flash) | Switch on / wait briefly / release the lock. |
| Level jumps back in Smart-Automatik | The automatic sets the level itself | For a fixed level choose **Wärmerückgewinnung**. |
| Durchlüften ends "by itself" | 30-minute timer expired | Normal; change the duration in Home Assistant / web dashboard (0 = unlimited). |
| All LEDs flash 3× on every press | Child lock active | Hold the Mode button for 5 s. |
| Another unit in the room changes too | Room-wide coordination | Normal and intended. |
| Master LED keeps blinking 3× | No Wi-Fi | Check the router. If the Wi-Fi was changed: on your phone, look for the Wi-Fi **"VentoSync Hotspot"** (password `ventosync`), connect and enter your home Wi-Fi in the window that opens (otherwise `192.168.4.1`). |
| Master LED keeps blinking 2× | Partner unit not reporting | Check / restart the partner unit. |
| Unit does not respond at all | Software hang | Hold the Power button > 10 s (restart). If that does not help: switch the circuit breaker off and on again. |

---

## ⚠️ Safety Notes

> [!CAUTION]
> The unit contains a power supply connected to **230 V mains voltage**. Open the housing only when it is
> disconnected (circuit breaker off). Work on the mains connection must be carried out by a **qualified
> electrician** only.

- Do not cover the ventilation openings.
- Do not keep operating a unit that repeatedly shuts down due to overheating (Master LED 4× or completely
  dark) without finding the cause.
- VentoSync is an independent open-source controller and is not affiliated with VentoMaxx GmbH.

---

## 📚 Further Reading

- [Control Panel Operation (technical details)](en_control-panel-operation.md)
- [Operating Modes & Program Logic](en_operating-modes.md)
- [Child Lock](en_child-lock-implementation.md)
- [Window Guard Setup](en_window-guard-ha-setup.md)
- [Vacation Mode Setup](en_vacation-mode-ha-setup.md)
- [Local Web Dashboard](en_local-web-dashboard.md)
- [Home Assistant Entities](en_home-assistant-entities.md)
