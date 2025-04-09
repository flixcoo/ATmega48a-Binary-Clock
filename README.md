# 🕒 ATmega48A - Binary Clock ⏱️

![AVR](https://img.shields.io/badge/AVR-ATmega48A-blue)
![License](https://img.shields.io/badge/License-MIT-green)
![Platform](https://img.shields.io/badge/Platform-Embedded-red)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen)

> A compact binary clock implementation using ATmega48A microcontroller with power-saving features and brightness control

## 🛠️ Hardware Configuration

### 📍 Pin Configuration
<img src="atmega_layout.png" width="500" alt="ATmega48A Pinout Diagram">

### 💡 LED Indicators
| Function  | Bit | Pin  |
|-----------|-----|------|
| **Hours** | 2⁰  | `PD7` |
|           | 2¹  | `PD6` |
|           | 2²  | `PD5` |
|           | 2³  | `PD4` |
|           | 2⁴  | `PD3` |
| **Minutes** | 2⁰ | `PC5` |
|            | 2¹ | `PC4` |
|            | 2² | `PC3` |
|            | 2³ | `PC2` |
|            | 2⁴ | `PC1` |
|            | 2⁵ | `PC0` |

### 🔘 Buttons
- **Button 1**: `PB0`
- **Button 2**: `PB1`
- **Button 3**: `PD2`

### 🕰️ Time & Measurements
-  **Clock crystal**: `PB6` + `PB7`
-  **Timing measurement**: `PD0`

## ✨ Features

### 🔌 Initialization
- ⚡ Power-on sequence with LED chase animation
- 🕛 Default start time: 12:00
- ⏳ Auto-sleep after 2.5 minutes inactivity

### 🎛️ Button Functions
| Combination | Action |
|-------------|--------|
| **Button 1** | Toggle power saving mode |
| **Button 2** | ➕ Increment hours (0→23) |
| **Button 3** | ➕ Increment minutes (0→59) |
| **Button 1 & 2** | ⚙️ Timing test mode |
| **Button 2 & 3** | 💡 Cycle brightness (5 levels) |

### ⚙️ Advanced Features
<details>
<summary><strong>🔧 Timing Measurement Mode</strong></summary>

- All LEDs light up for 2s on activation
- `PD0` toggles every second
- Buttons disabled during test
- Exit by pressing 1+2 again (3x LED blink)
</details>

<details>
<summary><strong>🔋 Power Saving Mode</strong></summary>

- All LEDs turned off
- MCU in power-save mode
- Wake via `PD0` interrupt or Button 1
- Full button functionality maintained
</details>

<details>
<summary><strong>🌓 Brightness Control</strong></summary>

- 5 adjustable brightness levels
- PWM-controlled LED intensity
- Settings persist through power cycles
- Cycle through levels with 2+3 button combo
</details>

## 🎬 Demonstration

### Boot Sequence
![boot_animation](https://github.com/user-attachments/assets/477c446e-d2a7-4ab1-aa5f-4784977676b6)

```c
// Boot sequence in the code
void startup_sequence() {
    for (int i = 7; i >= 3; i--) {
        PORTD |= (1 << i);
        _delay_ms(50);
        PORTD &= ~(1 << i);
    }

    for (int i = 3; i < 8; i++) {
        PORTD |= (1 << i);
        _delay_ms(50);
        PORTD &= ~(1 << i);
    }

    for (int i = 5; i >= 0; i--) {
        PORTC |= (1 << i);
        _delay_ms(50);
        PORTC &= ~(1 << i);
    }

    for (int i = 0; i < 6; i++) {
        PORTC |= (1 << i);
        _delay_ms(50);
        PORTC &= ~(1 << i);
    }

    _delay_ms(300);
}
```

## 📜 License

This project is licensed under the MIT License - see the LICENSE file for details.

---

> Developed with ❤️ during my studies at HTWK Leipzig
> 🚀 Feel free to contribute or fork this project!
