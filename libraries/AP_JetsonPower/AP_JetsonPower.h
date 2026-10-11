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
#pragma once

#include "AP_JetsonPower_config.h"

#if AP_JETSONPOWER_ENABLED

#include <AP_Param/AP_Param.h>

/*
  power-up sequencing for a carrier board holding an NVIDIA Jetson module
 */
class AP_JetsonPower
{
public:
    AP_JetsonPower();

    CLASS_NO_COPY(AP_JetsonPower);

    static AP_JetsonPower *get_singleton() { return _singleton; }

    void init();
    void update();

    static const struct AP_Param::GroupInfo var_info[];

private:
    enum class State : uint8_t {
        HELD_OFF = 0,   // module powered down, waiting out the power-good delay
        POWERED,        // POWER_EN asserted, waiting before releasing reset
        RUNNING,        // reset released, module booting or running
        DISABLED,       // no usable pin configuration
    };

    // the control pins drive inverting buffers, so a high output holds the
    // module off and a low output releases it
    void hold(int8_t pin) const;
    void release(int8_t pin) const;

    AP_Int8 _enable_pin;
    AP_Int8 _reset_pin;
    AP_Int8 _shutdown_pin;
    AP_Int16 _power_ms;
    AP_Int16 _reset_ms;

    uint32_t _state_ms;
    State _state;
    bool _shutdown_seen;

    static AP_JetsonPower *_singleton;
};

namespace AP {
    AP_JetsonPower *jetson_power();
};

#endif  // AP_JETSONPOWER_ENABLED
