# CerePilot V1.50 Flight Controller

The CerePilot V1.50 is a flight controller by CerePi that combines an STM32H743
FMU with an NVIDIA Jetson carrier on a single board, so the autopilot and the
companion computer share one assembly and one power tree.

## Where to Buy

Available from CerePi: <https://www.cerepi.com>

## Features

- STM32H743VI, 480 MHz Cortex-M7, 2 MB flash
- BMI088 and ISM330DLC IMUs (accelerometer + gyroscope)
- BMM350 magnetometer
- BMP585 barometer
- SM9541 differential pressure sensor, -100 to +100 cmH2O
- u-blox MAX-M10N GNSS receiver on board
- 32 KB FM25V02A FRAM for parameter storage
- microSD card slot for logging
- 9 PWM outputs
- 2 CAN ports, each with a fitted 120 Ohm termination
- 5 serial ports, including an RS-232 port and a link to the on-board Jetson
- 1 external I2C port
- USB-C

## UART Mapping

| Serial | Port   | Default protocol | Connector |
|--------|--------|------------------|-----------|
| SERIAL0 | USB    | MAVLink2 | USB |
| SERIAL1 | UART7  | MAVLink2 (TELEM1) | J900 |
| SERIAL2 | UART8  | MAVLink2 | on-board Jetson |
| SERIAL3 | UART4  | GPS | on-board MAX-M10N |
| SERIAL4 | USART1 | MAVLink2 | J900, RS-232 levels |
| SERIAL5 | USART2 | RCIN | J900 |

SERIAL1 and SERIAL5 are brought out at 3.3 V logic levels. SERIAL4 carries
RS-232 signal levels from a TRS3221E transceiver and connects straight to
RS-232 equipment; 3.3 V devices belong on SERIAL1 or SERIAL5.

## Connector Pinout

Both connectors are 16-way with two mounting pins (17 and 18) tied to ground.

### J900 - telemetry, RC, RS-232, CAN2, USB

| Pin | Signal | Notes |
|-----|--------|-------|
| 1  | CAN2_L | 120 Ohm termination fitted on board |
| 2  | CAN2_H | |
| 3  | SBUS RX | RC input, SERIAL5 |
| 4  | SBUS TX | SERIAL5 |
| 5  | TELEM1 RX | SERIAL1 |
| 6  | TELEM1 TX | SERIAL1 |
| 7  | RS-232 RX | SERIAL4, RS-232 levels |
| 8  | RS-232 TX | SERIAL4, RS-232 levels |
| 9  | USB D- | |
| 10 | USB D+ | |
| 11-16 | GND | |

### J901 - PWM outputs, CAN1, external I2C, companion UART

| Pin | Signal | Notes |
|-----|--------|-------|
| 1  | GND | |
| 2  | SERVO1 | |
| 3  | SERVO2 | |
| 4  | SERVO3 | |
| 5  | SERVO4 | |
| 6  | SERVO5 | |
| 7  | SERVO6 | |
| 8  | SERVO7 | |
| 9  | SERVO8 | |
| 10 | SERVO9 | |
| 11 | SERIAL2 TX | shared with the on-board Jetson, see below |
| 12 | SERIAL2 RX | shared with the on-board Jetson, see below |
| 13 | I2C SDA | external I2C |
| 14 | I2C SCL | external I2C |
| 15 | CAN1_L | 120 Ohm termination fitted on board |
| 16 | CAN1_H | |

All signals on both connectors are at 3.3 V logic levels, except the RS-232
pair on J900, which is at RS-232 levels.

## Companion Computer

The on-board Jetson is wired to the FMU on **SERIAL2** (UART8), reaching the
Jetson's UART1. It defaults to MAVLink2 at 115200 baud, and no jumpers or
external wiring are needed.

This same signal pair is also brought out on J901 pins 11 and 12, so the link
has three endpoints: the FMU, the Jetson, and the connector. Leave those two
pins unconnected when using the on-board Jetson. Driving them from an external
device puts two transmitters on one line, and the Jetson will see both.

## RC Input

RC input is on **SERIAL5** (USART2), which defaults to `SERIAL5_PROTOCOL = 23`
(RCIN). The pad accepts SBUS from a standard receiver wired directly, and
half-duplex protocols such as CRSF and FPort are supported on the same pin.

## PWM Output

Nine PWM outputs, SERVO1 to SERVO9. Outputs sharing a timer must use the same
output rate and protocol:

| Outputs | Timer |
|---------|-------|
| 1, 2 | TIM3 |
| 3, 4 | TIM1 |
| 5    | TIM2 |
| 6, 7 | TIM4 |
| 8, 9 | TIM15 |

All nine support regular PWM, OneShot and DShot. The outputs are buffered by
level translators, which bidirectional DShot telemetry does not pass through,
so bidirectional DShot is not available on this board.

## GPIOs

Any PWM output not assigned to a motor or servo can be used as a GPIO by
setting its `SERVOn_FUNCTION` to -1. The pin numbers are:

| Output | Pin | | Output | Pin |
|--------|-----|-|--------|-----|
| SERVO1 | 50 | | SERVO6 | 55 |
| SERVO2 | 51 | | SERVO7 | 56 |
| SERVO3 | 52 | | SERVO8 | 57 |
| SERVO4 | 53 | | SERVO9 | 58 |
| SERVO5 | 54 | | | |

## OSD Support

The board has no on-board OSD. MSP DisplayPort and other serial OSD
protocols are supported on any spare serial port.

## Battery Monitoring

The board has no internal voltage or current sensing and expects an I2C smart
power module on the external I2C port. The defaults are set for an INA2xx
module:

- `BATT_MONITOR = 21`

Reboot after changing `BATT_MONITOR`, then set the module's parameters as
required.

## Board Voltage Monitoring

The 5 V input rail and the internal 3.3 V and 1.8 V regulator outputs are
measured and reported as the board voltage, so a failing regulator or a
sagging supply shows up in the logs and in `BATTERY_STATUS`.

## Airspeed

An SM9541-100C-D differential pressure sensor is fitted on an internal I2C bus
and is enabled by default:

- `ARSPD_TYPE = 21`
- `ARSPD_BUS = 1`

The sensor spans -100 to +100 cmH2O (about +/- 9.8 kPa) and reports its own
temperature. Connect the pitot and static lines to the two ports; with the
differential part either port may be the higher pressure.

## Compass

The BMM350 magnetometer is fitted on an internal I2C bus and enabled by
default. External compasses are supported on the external I2C port.

## Storage

Parameters are stored in FRAM. Logs are written to the microSD card, which
must be inserted before power-up to be detected.

## CAN

Two CAN ports, CAN1 on J901 and CAN2 on J900. Both have a 120 Ohm termination
resistor fitted on the board, so the CerePilot counts as one end of the bus.

Both drivers are enabled by default (`CAN_P1_DRIVER = 1`, `CAN_P2_DRIVER = 1`);
set the protocol you need with `CAN_D1_PROTOCOL` and `CAN_D2_PROTOCOL`.

## LEDs

Two status LEDs, green and blue, driven by the standard ArduPilot notify
patterns. The green LED also indicates bootloader activity.

## Loading Firmware

The board ships with the ArduPilot bootloader installed, so firmware is loaded
with `*.apj` files through a ground station or with:

    ./waf configure --board CerePilotV150
    ./waf copter --upload

To install or recover the bootloader itself, load `CerePilotV150_bl.hex` with
STM32CubeProgrammer over SWD (connect-under-reset, `-c port=SWD mode=UR`), or
use DFU.
