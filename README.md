# GPS Tracker

![Banner](assets/banner.png)

A real-time GPS tracking web dashboard built with ESP32 + SIM7600, integrated with Blynk IoT and Fonnte WhatsApp API. Accessible via browser and installable as a PWA on Android.

🌐 **Live Demo:** [gpstrack88.nexalab.my.id](https://gpstrack88.nexalab.my.id)
📲 **Install Guide:** [gpstrack88.nexalab.my.id/download](https://gpstrack88.nexalab.my.id/download)

---

## Features

- 📍 **Live GPS Tracking** — Real-time vehicle location on an interactive map
- 🛡️ **Geofence** — Set a safe zone and get alerted when the vehicle exits
- 💬 **WhatsApp Alert** — Send vehicle location directly to WhatsApp via Fonnte API
- 🔊 **Buzzer Control** — Remotely trigger the buzzer to deter theft
- ⚡ **Relay Control** — Remotely cut the engine via relay
- 📡 **Status Monitor** — Monitor internet, GPS lock, and signal strength in real-time

---

## Tech Stack

### Hardware
| Component | Description |
|-----------|-------------|
| ESP32 | Main microcontroller |
| SIM7600 | 4G LTE modem for GPS + internet |
| Relay | Remote engine cut-off |
| Buzzer | Anti-theft alert |

### Software
| Technology | Description |
|------------|-------------|
| Blynk IoT | Cloud platform for data & control |
| Fonnte API | WhatsApp notification gateway |
| Leaflet.js | Interactive map rendering |
| Bootstrap 5 | UI framework |
| Vercel | Web hosting & deployment |
| Cloudflare | DNS management |

---

## Project Structure

```
GPS TRACKER
├── index.html          # Main dashboard
├── script.js           # Blynk API logic & map
├── style.css           # Dashboard styling
├── manifest.json       # PWA configuration
├── download/
│   └── index.html      # Install landing page
└── assets/
    ├── logo_1.png      # App icon
    └── banner.png      # README banner
```

---

## Blynk Virtual Pins

| Pin | Name | Description |
|-----|------|-------------|
| V0 | Geofence Toggle | Enable/disable geofence |
| V1 | Buzzer | Remote buzzer control |
| V2 | Relay | Remote engine cut-off |
| V3 | Tracking | Send location via WhatsApp |
| V4 | Latitude | GPS latitude value |
| V5 | Longitude | GPS longitude value |
| V6 | Signal | Modem signal quality |
| V7 | Geofence Status | Active/inactive status |
| V8 | Internet Status | GPRS connection status |

---

## How It Works

1. ESP32 reads GPS coordinates from SIM7600 via AT commands
2. Data is sent to Blynk cloud every 30 seconds
3. Web dashboard fetches data from Blynk API every 3 seconds
4. If geofence is active and vehicle exits the zone, a WhatsApp alert is sent via Fonnte
5. User can control relay and buzzer remotely from the dashboard

---

## Install as Mobile App (PWA)

No APK needed. Install directly from your browser:

1. Open [gpstrack88.nexalab.my.id](https://gpstrack88.nexalab.my.id) in your browser (Chrome, Safari, etc.) on Android or iOS
2. Tap the menu button (⋮ on Android / Share icon on iOS)
3. Select **"Add to Home Screen"**
4. Done — GPS Tracker icon appears on your home screen

---

## Author

**Muhamad Nara Utama** (41422110037)

---

## License

© 2026 Muhamad Nara Utama. All rights reserved.
This project is created for academic purposes.
Unauthorized use, copying, or distribution is not permitted.
