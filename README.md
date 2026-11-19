# Digitally Controlled Linear Lab Power Supply (STM32F303)

> High-precision dual-channel digitally controlled linear laboratory power supply firmware based on the STM32F303 ARM Cortex-M4 microcontroller, featuring a 1 kHz closed-loop PI regulation algorithm, dynamic Safe Operating Area (SOA) thermal tracking, and LTspice simulation models.

**Project Status:** `Functional Prototype / Active` | **Core Technologies:** `C (C99)`, `STM32F303K8Tx`, `ARM Cortex-M4 (FPU/DSP @ 64 MHz)`, `STM32CubeIDE`, `STM32 HAL / CMSIS`, `LTspice XVII`, `I2C LCD`

---

## 1. Overview and Key Features

This project implements the embedded firmware and analog simulation models for a versatile dual-channel linear laboratory power supply. Built around the STM32F303K8 microcontroller, the system combines analog linear regulation (LM317 and pass transistor topologies) with real-time digital feedback, software calibration, and hardware protection.

### Key Capabilities
- **Dual Operating Modes per Channel:**
  - `MODE_POWER_LM317`: Regulated linear power output (up to 16 V) with active digital PI loop adjusting the LM317 reference voltage via on-chip 12-bit DAC.
  - `MODE_DAC_DIRECT`: Low-noise direct 0–3.3 V precision voltage output driven directly by the internal DAC buffer (for sensor testing and ADC stimulus).
- **1 kHz Deterministic Control Loop:**
  - Dedicated 1 ms execution interval (`Task_ControlLoop_1ms`) reading 12-bit ADC channels for terminal voltage and shunt drop, updating the PI controller with anti-windup integration.
- **Dynamic Safe Operating Area (SOA) Protection:**
  - Real-time thermal dissipation limit algorithm:
    $$\text{Limit}_{mV} = 16000\text{ mV} - \frac{13 \cdot I_{actual\_mA}}{2}$$
    Prevents transformer saturation and pass-element thermal runaway when high current is drawn at low output voltages.
- **Precision Measurement & Display Filtering:**
  - Low-side/high-side resistive divider sensing with fixed calibration constants ($R_{top}, R_{bot}, R_{shunt}, V_{ref}$).
  - Exponential moving average (EMA) 31:1 filter for rock-solid LCD UI readout without sluggishness in the fast inner regulation loop.
- **Ergonomic Rotary UI & Per-Channel Relays:**
  - Quadrature rotary encoder (TIM2 hardware interface) with cycle-based debounce.
  - User-selectable adjustment steps: **10 mV**, **100 mV**, and **1000 mV**.
  - Independent channel enable pushbuttons with physical relay cutoffs (`CH1_REL`, `CH2_REL`) and status LEDs.
- **Telemetry & Debugging:**
  - Non-blocking `printf` logging over Virtual COM Port (USART2 @ 38400 baud, 8N1).

---

## 2. System Architecture & Control Loop

```mermaid
flowchart TD
    subgraph HardwarePower["⚡ Power Stage (Hardware)"]
        MAINS["230V AC Mains"] --> XFMR["Transformer (24V AC)"]
        XFMR --> RECT["Bridge Rectifier & 2200uF Filter"]
        RECT --> LM317["LM317 Linear Regulator"]
        LM317 --> SHUNT["Current Shunt (1.15 Ohm)"]
        SHUNT --> RELAY["Output Disconnect Relay (PB4 / PB5)"]
        RELAY --> VOUT["Output Terminals (0-16V DC)"]
    end

    subgraph FeedbackSensors["🔍 Feedback & Sensing"]
        PRE_V["Pre-shunt Divider"] --> ADC_A["STM32 ADC2 (Channel Current)"]
        VOUT --> POST_V["Post-shunt Divider"] --> ADC_V["STM32 ADC1 (Terminal Voltage)"]
    end

    subgraph DigitalControl["🧠 STM32F303 Digital Control Core"]
        ADC_V --> PROCESS_ADC["PowerControl_ProcessADC()<br/>Ohm's Law & EMA Display Filter"]
        ADC_A --> PROCESS_ADC
        
        PROCESS_ADC --> SOA["PowerControl_UpdateSOALimit()<br/>Dynamic SOA Thermal Calculation"]
        
        TARGET["Target Voltage<br/>(Rotary Encoder Input)"] --> PI["1 kHz PI Controller<br/>Kp=10, Ki=2 + Anti-Windup"]
        SOA -.->|Dynamic Ceiling| PI
        PROCESS_ADC -->|Actual V/I| PI
        
        PI --> DAC["12-bit DAC1 (PA4 / PA5)"]
    end

    subgraph UI_Peripherals["🖥️ Human-Machine Interface"]
        ENC["Rotary Encoder (TIM2)"] --> TASK_INPUTS["Task_Inputs_20ms()<br/>Step: 10mV / 100mV / 1V"]
        BTNS["Mode & Channel Buttons"] --> TASK_INPUTS
        TASK_INPUTS --> RELAY
        TASK_INPUTS --> TARGET
        PROCESS_ADC --> TASK_UI["Task_UI_100ms()<br/>LiquidCrystal I2C 16x2 / 20x4"]
    end

    DAC -->|Control Voltage (0-3.3V)| LM317
```

---

## 3. Hardware Pinout & Peripheral Mapping

| Pin | Peripheral / Function | Type | Description |
| :--- | :--- | :--- | :--- |
| **PA0** | `TIM2_CH1` | Input | Rotary encoder Phase A |
| **PA1** | `TIM2_CH2` | Input | Rotary encoder Phase B |
| **PA2** | `USART2_TX` | Output (VCP) | Debug UART transmit (38400 baud) |
| **PA3** | `GPIO_Input` (Pull-Up) | Input | Rotary encoder pushbutton (step cycle: 10mV / 100mV / 1V) |
| **PA4** | `DAC1_OUT1` | Analog Out | Channel 1 analog control output (to LM317 adjust network) |
| **PA5** | `DAC1_OUT2` | Analog Out | Channel 2 analog control output |
| **PA6** | `ADC1_IN10` | Analog In | Channel 1 terminal voltage feedback (voltage divider) |
| **PA7** | `ADC2_IN15` | Analog In | Channel 2 terminal voltage feedback (voltage divider) |
| **PB0** | `ADC1_IN11` | Analog In | Channel 1 pre-shunt voltage (for differential current sensing) |
| **PB1** | `ADC2_IN12` | Analog In | Channel 2 pre-shunt voltage |
| **PA8** | `GPIO_Input` (Pull-Up) | Input | Mode toggle pushbutton (`MODE_POWER_LM317` / `MODE_DAC_DIRECT`) |
| **PA9** | `GPIO_Input` (Pull-Up) | Input | Channel selection toggle pushbutton (CH1 / CH2 UI focus) |
| **PA10** | `GPIO_Input` (Pull-Up) | Input | Channel 1 output enable/disable toggle |
| **PA11** | `GPIO_Input` (Pull-Up) | Input | Channel 2 output enable/disable toggle |
| **PB3** | `GPIO_Output` | Output | Channel 1 status LED |
| **PA12** | `GPIO_Output` | Output | Channel 2 status LED |
| **PB4** | `GPIO_Output` | Output | Channel 1 power relay control |
| **PB5** | `GPIO_Output` | Output | Channel 2 power relay control |
| **PB6 / PB7** | `I2C1_SCL / SDA` | I/O | LiquidCrystal I2C alpha-numeric display bus |

---

## 4. LTspice Simulation Models

The repository includes two comprehensive LTspice circuit simulation files:
- **`Voltage-src.asc`:** Explores Darlington and discrete power pass transistor stages (TIP122, BD140, TIP2955) driven by op-amp feedback loops, transient response under stepping load currents, and stability analysis.
- **`Voltage-src-2.asc`:** Full AC-DC rectification and LM317 regulation model. Simulates mains transformer secondary (50 Hz AC sinusoidal source), diode bridge rectification (1N4007), bulk electrolytic filter capacitance (2200 µF), and variable DC loading with parametric voltage DAC injection.

---

## 5. Directory Structure

```text
.
├── Voltage-src.asc                # LTspice simulation model (discrete pass transistors & op-amps)
├── Voltage-src-2.asc              # LTspice simulation model (AC transformer, rectifier & LM317 stage)
├── Voltage-source/                # STM32CubeIDE project root
│   ├── .project                   # Eclipse / STM32CubeIDE project definition
│   ├── .cproject                  # Eclipse CDT build configuration
│   ├── .mxproject                 # STM32CubeMX metadata
│   ├── Voltage-source.ioc         # Graphical STM32CubeMX pinout & clock tree configuration
│   ├── STM32F303K8TX_FLASH.ld     # GNU GCC linker script for STM32F303K8 (64 KB Flash, 16 KB SRAM)
│   ├── Core/
│   │   ├── Inc/
│   │   │   ├── power_control.h    # Power channel structures, calibration parameters & PI declarations
│   │   │   ├── liquidcrystal_i2c.h # I2C character LCD driver header
│   │   │   ├── main.h             # Pin assignments, peripheral handles & global definitions
│   │   │   └── stm32f3xx_it.h     # Interrupt service routine headers
│   │   ├── Src/
│   │   │   ├── main.c             # Multi-rate scheduler (1ms, 20ms, 100ms tasks), hardware inits & UI
│   │   │   ├── power_control.c    # 1 kHz PI regulation loop, Ohm's law current calc & SOA protection
│   │   │   ├── liquidcrystal_i2c.c # I2C HD44780 display driver implementation
│   │   │   ├── stm32f3xx_hal_msp.c # Low-level peripheral clocks and GPIO initialization
│   │   │   ├── system_stm32f3xx.c # ARM CMSIS clock tree configuration
│   │   │   └── u8g2/              # Modular monochrome graphics library (u8g2 v2.x)
│   │   └── Startup/
│   │       └── startup_stm32f303k8tx.s # Vector table and GCC assembly startup
│   └── Drivers/                   # CMSIS Core and STM32F3xx HAL firmware library
├── .gitignore                     # Build artifacts, logs and Eclipse workspace ignores
└── README.md                      # Project documentation
```

---

## 6. Build and Flashing Instructions

### Prerequisites
- **IDE:** [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) (version 1.14.0 or newer) or standalone `arm-none-eabi-gcc` toolchain with `make`.
- **Hardware Programmer:** ST-LINK/V2, ST-LINK/V3, or onboard ST-Link on NUCLEO-F303K8.
- **Simulation:** [LTspice](https://www.analog.com/en/resources/design-tools-and-calculators/ltspice-simulator.html) XVII or newer.

### Building in STM32CubeIDE
1. Launch STM32CubeIDE and choose a fresh workspace.
2. Select **File -> Import... -> General -> Existing Projects into Workspace**.
3. Point the root directory to `Voltage-source/` and click **Finish**.
4. Right-click the project, choose **Build Project** (configuration `Debug` or `Release`).
5. Connect your ST-Link to SWD pins (PA13 SWDIO, PA14 SWCLK, GND, 3V3) and click **Run -> Debug** or **Run**.

---

## 7. Known Limitations and Roadmap

- **Constant-Current (CC) Closed Loop:** Currently, current limiting is governed by dynamic SOA foldback protection. A dedicated constant-current inner control loop for battery charging and LED driver compliance testing is planned for the next firmware iteration.
- **Graphical Display Migration:** The `u8g2` graphics engine is integrated into the source tree; transitioning the display task from character LCD to an SPI/I2C 128x64 OLED (SSD1306) will provide live voltage/current graphing and oscilloscope-style output waveform rendering.
