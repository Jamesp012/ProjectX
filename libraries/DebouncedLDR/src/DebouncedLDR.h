/*
    Copyright 2023 Zach Vonler <zvonler@gmail.com>

    This file is part of DebouncedLDR.

    DebouncedLDR is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    DebouncedLDR is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    DebouncedLDR.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef debounced_ldr_h
#define debounced_ldr_h

#ifdef UNIT_TESTING
#include <algorithm>
#include <cmath>
#include <cstdint>
#define floor std::floor
#define min std::min
#define max std::max
#else
#include <Arduino.h>
#endif

/*---------------------------------------------------------------------------*/

/**
 * Represents an LDR with latching light levels to prevent flicker.
 */
class DebouncedLDR {
public:
    using Reading = uint16_t;
    using Level = uint16_t;

    /**
     * Creates an instance that will map readings in the range [0, max_reading]
     * to light level values in the range [0, max_level], and will require the
     * reading to change by more than noise to cause a light level change.
     */
    DebouncedLDR(Reading max_reading, Level max_level, Reading noise);

    /**
     * Adds the reading and returns the current level.
     */
    Level update(Reading reading);

    Level light_level() const { return _level; }

    Reading max_reading() const { return _max_reading; }
    Level max_level() const { return _max_level; }
    Reading noise() const { return _noise; }

private:
    Reading _max_reading;
    Level _max_level;
    Reading _noise;
    float _level_width;
    Level _level = 0;
    Reading _last_reading_on_change = 0;
};

/*---------------------------------------------------------------------------*/

#endif
