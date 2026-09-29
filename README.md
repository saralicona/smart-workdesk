# Context-Aware Smart Workstation (Physical-to-Digital HCI System)

A physical-to-digital desk ecosystem inspired by Apple’s Focus Modes. The system evaluates ambient environmental data (light and sound) against explicit user intentions (`FOCUS`, `SOCIAL`, `RELAX`) to dynamically adapt task lighting via PWM and surface non-intrusive distraction alerts through a WebSerial digital dashboard.

---

## System Overview & Architecture



The workstation utilizes a decoupled **Interface Separation Strategy**:
1. **Local Physical UI (128x64 I2C OLED & Pushbuttons):** Provides tactile mode selection and immediate operational status on the physical desktop.
<img width="752" height="411" alt="image" src="https://github.com/user-attachments/assets/712dc00e-6663-411c-9888-a8316d709d11" />


2. **Digital GUI (HTML/JS WebSerial Dashboard):** Surfaces real-time telemetry streaming, latched acoustic distraction alerts, and session reflection prompts without cluttering the physical environment.# smart-workdesk
<img width="1917" height="541" alt="displau" src="https://github.com/user-attachments/assets/8ec98a53-17b9-4713-8d65-1fcd9afffed9" />
