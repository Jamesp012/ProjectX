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

#include <gtest/gtest.h>
#include <random>
#include <tuple>

#include "../src/DebouncedLDR.h"

namespace {

/*---------------------------------------------------------------------------*/

class TestingScript
{
public:
    struct ScriptPoint
    {
        uint16_t _reading;
        uint16_t _expected;

        ScriptPoint(uint16_t reading, uint16_t expected)
            : _reading(reading)
            , _expected(expected)
        { }
    };

    ScriptPoint const* _script_points;
    std::string _name;
    int _num_points;

    TestingScript(char const* name, ScriptPoint const script_points[], int num_points)
        : _script_points(script_points)
        , _name(name)
        , _num_points(num_points)
    { }

    void execute(DebouncedLDR& ldr)
    {
        for (int i = 0; i < _num_points; ++i)
        {
            ScriptPoint const& sp = _script_points[i];
            SCOPED_TRACE(_name + " i:" + std::to_string(i));
            EXPECT_EQ(sp._expected, ldr.update(sp._reading));
        }
    }
};

using ScriptPoint = TestingScript::ScriptPoint;

/*---------------------------------------------------------------------------*/

#define ARRAY_SIZE(A) sizeof(A)/sizeof(A[0])
#define RUN_SCRIPT(NAME,LDR) TestingScript(#NAME, NAME ## _script, ARRAY_SIZE(NAME ## _script)).execute(LDR);

class TestDebouncedLDR : public testing::Test
{
protected:
    std::mt19937 _rng;

    void SetUp() override
    {
        _rng.seed(123456);
    }
};

TEST_F(TestDebouncedLDR, TestInitialState)
{
    DebouncedLDR ldr(1024, 1, 0);
    EXPECT_EQ(0, ldr.light_level());
}

TEST_F(TestDebouncedLDR, TestPassthrough)
{
    DebouncedLDR ldr(65535, 65535, 0);

    ScriptPoint passthrough_script[] = {
        { 0, 0 },
        { 1, 1 },
        { 2, 2 },
        { 3, 3 },
        { 1024, 1024 },
        { 65535, 65535 },
    };

    RUN_SCRIPT(passthrough, ldr);
}

TEST_F(TestDebouncedLDR, TestScaling)
{
    {
        // Two light levels with no latching
        DebouncedLDR ldr(65535, 1, 0);

        ScriptPoint two_levels_script[] = {
            { 0, 0 },
            { 65535 - (65535 / 2) - 1, 0 },
            { 65535 - (65535 / 2), 1 },
            { 65535 - (65535 / 2) - 1, 0 },
            { 65535 - (65535 / 2), 1 },
            { 65535 - 1, 1 },
            { 65535, 1 },
        };

        RUN_SCRIPT(two_levels, ldr);
    }

    {
        // Four light levels with no latching
        DebouncedLDR ldr(1023, 3, 0);

        ScriptPoint four_levels_script[] = {
            { 0, 0 },
            { 255, 0 },
            { 256, 1 },
            { 511, 1 },
            { 512, 2 },
            { 767, 2 },
            { 768, 3 },
            { 1023, 3 },
            { 1024, 3 },
            { 65535, 3 },
        };

        RUN_SCRIPT(four_levels, ldr);
    }
}

TEST_F(TestDebouncedLDR, TestLatching)
{
    DebouncedLDR ldr(1023, 1, 100);

    // Check that light level latches until reading falls below noise threshold.
    ScriptPoint latching_script[] = {
        { 0, 0 },
        { 511, 0 },
        { 512, 1 },
        { 511, 1 },
        { uint16_t(512 - ldr.noise()), 1 },
        { uint16_t(512 - ldr.noise() - 1), 0 },
        { 1023, 1 },
        { 512, 1 },
        { 511, 0 },
        { 512, 0 },
    };

    RUN_SCRIPT(latching, ldr);
}

TEST_F(TestDebouncedLDR, TestDecimalization)
{
    // 10-bit reading range with 10 logical levels
    DebouncedLDR ldr(1023, 9, 5);

    ScriptPoint decimalization_script[] = {
        { 0, 0 },
        { 102, 0 },
        { 103, 1 },
        { 102, 1 },
        { 101, 1 },
        { 100, 1 },
        { 99, 1 },
        { 98, 1 },
        { 97, 0 },
        { 98, 0 },
        { 99, 0 },
        { 100, 0 },
        { 101, 0 },
        { 102, 0 },
        { 103, 1 },
    };

    RUN_SCRIPT(decimalization, ldr);
}
/*---------------------------------------------------------------------------*/

} // anonymous namespace
