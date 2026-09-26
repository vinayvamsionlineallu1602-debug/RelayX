# RelayX — Portable Wi-Fi Relay

> **Connectivity, beyond the edge.**

RelayX is a compact, battery-powered Wi-Fi relay concept designed to extend existing Wi-Fi connectivity into **hard-to-reach, temporary, and remote locations**.

The project focuses on a simple deployment experience:

**Drop in → Power on → Extend**

The proposed MVP uses an ESP32 to connect to an existing Wi-Fi network and create a separate Wi-Fi access point for nearby users and devices.

---

## 📌 Project Overview

Wi-Fi coverage can become weak or unavailable because of:

* Distance from the main router
* Walls and physical obstacles
* Changing environments
* Temporary locations
* Outdoor deployments

RelayX explores a portable alternative for situations where permanent networking infrastructure may not be practical.

The initial target environments include:

* College campuses
* Student events
* Temporary workspaces
* Construction sites
* Farms and semi-rural locations
* Warehouses
* Outdoor venues

These target customers and use cases are defined in the project presentation.

---

# 🎯 Problem

### Wi-Fi ends before the work does.

Existing Wi-Fi coverage can degrade with distance and physical obstacles, creating weak or dead zones.

RelayX is designed around the idea of placing a small relay device between the main router and users:

```text
        EXISTING ROUTER
              │
              │ Wi-Fi
              ▼
        ┌─────────────┐
        │   RelayX    │
        │   ESP32     │
        └──────┬──────┘
               │
               │ Extended Wi-Fi
               ▼
       ┌───────────────┐
       │ Users/Devices │
       └───────────────┘
```

The PPT identifies events, campuses, farms, outdoor venues, and construction environments as example situations where coverage can become a problem.

---

# 💡 Proposed Solution

RelayX is designed as a portable Wi-Fi relay that:

1. Connects to an existing Wi-Fi network.
2. Creates a new local Wi-Fi access point.
3. Forwards network traffic through the existing connection.
4. Provides a placement indicator based on upstream signal strength.
5. Operates from a rechargeable battery.
6. Can be deployed without a SIM card.

### MVP Features

* 🔋 Rechargeable battery
* 🔌 USB-C power
* 📶 Wi-Fi relay
* 📍 Placement guidance
* ⚡ Simple setup
* 📡 No SIM required

These are the proposed MVP characteristics in the project presentation.

---

# 🏗️ System Architecture

```text
                 INTERNET
                     │
                     ▼
              ┌─────────────┐
              │ Main Router │
              └──────┬──────┘
                     │
                Existing Wi-Fi
                     │
```
   
