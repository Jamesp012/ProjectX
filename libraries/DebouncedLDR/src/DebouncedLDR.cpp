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

#include "DebouncedLDR.h"

/*---------------------------------------------------------------------------*/

DebouncedLDR::DebouncedLDR(Reading max_reading, Level max_level, Reading noise)
    : _max_reading(max_reading)
    , _max_level(max_level)
    , _noise(noise)
    , _level_width(_max_reading / (_max_level + 1.0f))
{
}

DebouncedLDR::Level
DebouncedLDR::update(Reading reading)
{
    Level new_level;
    if (reading >= _max_reading) {
        new_level = _max_level;
    } else {
        new_level = floor(reading / _level_width);
    }

    if (new_level != _level) {
        Reading diff = max(reading, _last_reading_on_change) - min(reading, _last_reading_on_change);
        if (diff > _noise) {
            _level = new_level;
            _last_reading_on_change = reading;
        }
    }

    return _level;
}

/*---------------------------------------------------------------------------*/
