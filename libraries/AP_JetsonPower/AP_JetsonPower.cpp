/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
  power-up sequencing for a carrier board holding an NVIDIA Jetson module.

  The module needs POWER_EN asserted only once its input rail is good, and
  SYS_RESET* released only after it has had time to run its own power
  sequence. Jetson Orin NX/Nano design guide DG-10931 asks for at least 80ms
  between the two.

  Both control lines are expected to drive inverting buffers, so the board
  holds them high from reset to keep the module down, and this library
  releases them in turn once the autopilot is running. SHUTDOWN_REQ* is an
  output of the module and is only read.
 */

#include "AP_JetsonPower.h"

#if AP_JETSONPOWER_ENABLED

#include <AP_HAL/AP_HAL.h>
#include <GCS_MAVLink/GCS.h>

extern const AP_HAL::HAL &hal;

#ifndef AP_JETSONPOWER_POWER_MS_DEFAULT
#define AP_JETSONPOWER_POWER_MS_DEFAULT 50
#endif
#ifndef AP_JETSONPOWER_RESET_MS_DEFAULT
#define AP_JETSONPOWER_RESET_MS_DEFAULT 100
#endif
#ifndef AP_JETSONPOWER_EN_PIN_DEFAULT
#define AP_JETSONPOWER_EN_PIN_DEFAULT -1
#endif
#ifndef AP_JETSONPOWER_RST_PIN_DEFAULT
#define AP_JETSONPOWER_RST_PIN_DEFAULT -1
#endif
#ifndef AP_JETSONPOWER_SHDN_PIN_DEFAULT
#define AP_JETSONPOWER_SHDN_PIN_DEFAULT -1
#endif

const AP_Param::GroupInfo AP_JetsonPower::var_info[] = {

    // @Param: EN_PIN
    // @DisplayName: Jetson enable pin
    // @Description: GPIO pin driving the module POWER_EN buffer. Set to -1 to disable the sequence.
    // @Values: -1:Disabled
    // @RebootRequired: True
    // @User: Advanced
    AP_GROUPINFO("EN_PIN", 1, AP_JetsonPower, _enable_pin, AP_JETSONPOWER_EN_PIN_DEFAULT),

    // @Param: RST_PIN
    // @DisplayName: Jetson reset pin
    // @Description: GPIO pin driving the module SYS_RESET buffer. Set to -1 to leave reset alone.
    // @Values: -1:Disabled
    // @RebootRequired: True
    // @User: Advanced
    AP_GROUPINFO("RST_PIN", 2, AP_JetsonPower, _reset_pin, AP_JETSONPOWER_RST_PIN_DEFAULT),

    // @Param: SHDN_PIN
    // @DisplayName: Jetson shutdown request pin
    // @Description: GPIO pin the module drives low to request an orderly shutdown. Set to -1 if not connected.
    // @Values: -1:Disabled
    // @RebootRequired: True
    // @User: Advanced
    AP_GROUPINFO("SHDN_PIN", 3, AP_JetsonPower, _shutdown_pin, AP_JETSONPOWER_SHDN_PIN_DEFAULT),

    // @Param: PWR_MS
    // @DisplayName: Jetson power enable delay
    // @Description: Delay from startup before POWER_EN is asserted, allowing the module input rail to settle.
    // @Units: ms
    // @Range: 0 5000
    // @User: Advanced
    AP_GROUPINFO("PWR_MS", 4, AP_JetsonPower, _power_ms, AP_JETSONPOWER_POWER_MS_DEFAULT),

    // @Param: RST_MS
    // @DisplayName: Jetson reset release delay
    // @Description: Delay from POWER_EN before SYS_RESET is released. The Jetson design guide requires at least 80ms.
    // @Units: ms
    // @Range: 80 5000
    // @User: Advanced
    AP_GROUPINFO("RST_MS", 5, AP_JetsonPower, _reset_ms, AP_JETSONPOWER_RESET_MS_DEFAULT),

    AP_GROUPEND
};

AP_JetsonPower *AP_JetsonPower::_singleton;

AP_JetsonPower::AP_JetsonPower()
{
    AP_Param::setup_object_defaults(this, var_info);
    _singleton = this;
}

void AP_JetsonPower::hold(int8_t pin) const
{
    if (pin >= 0) {
        hal.gpio->pinMode(pin, HAL_GPIO_OUTPUT);
        hal.gpio->write(pin, 1);
    }
}

void AP_JetsonPower::release(int8_t pin) const
{
    if (pin >= 0) {
        hal.gpio->write(pin, 0);
    }
}

void AP_JetsonPower::init()
{
    if (_enable_pin < 0) {
        _state = State::DISABLED;
        return;
    }

    // the board holds these from reset; take them over without changing level
    hold(_enable_pin);
    hold(_reset_pin);

    if (_shutdown_pin >= 0) {
        hal.gpio->pinMode(_shutdown_pin, HAL_GPIO_INPUT);
    }

    _state_ms = AP_HAL::millis();
    _state = State::HELD_OFF;
}

void AP_JetsonPower::update()
{
    const uint32_t now_ms = AP_HAL::millis();

    switch (_state) {
    case State::DISABLED:
        return;

    case State::HELD_OFF:
        if (now_ms - _state_ms < (uint32_t)_power_ms.get()) {
            return;
        }
        release(_enable_pin);
        _state_ms = now_ms;
        _state = State::POWERED;
        return;

    case State::POWERED:
        if (now_ms - _state_ms < (uint32_t)_reset_ms.get()) {
            return;
        }
        release(_reset_pin);
        _state_ms = now_ms;
        _state = State::RUNNING;
        GCS_SEND_TEXT(MAV_SEVERITY_INFO, "Jetson: powered up");
        return;

    case State::RUNNING:
        break;
    }

    // the module pulls this low to say it is shutting itself down. Report it
    // once; powering the module back down is left to the operator, as doing
    // it automatically could cut the companion computer during flight.
    if (_shutdown_pin >= 0 && !_shutdown_seen && hal.gpio->read(_shutdown_pin) == 0) {
        _shutdown_seen = true;
        GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "Jetson: shutdown requested");
    }
}

namespace AP {

AP_JetsonPower *jetson_power()
{
    return AP_JetsonPower::get_singleton();
}

};

#endif  // AP_JETSONPOWER_ENABLED
