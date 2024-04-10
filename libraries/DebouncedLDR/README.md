
# DebouncedLDR

This library implements a class that applies quantization and hysteresis to the
analog readings of a Light Dependent Resistor (LDR) so that applications can
work with logical light levels instead of physical light readings. Using this
library, applications can easily avoid the flickering effects that can happen
with simple thresholding.

## Theory of operation

The `DebouncedLDR` class does not do any of the actual GPIO manipulation. The
client application is expected to call `DebouncedLDR::update` frequently
(i.e. every few milliseconds) with the latest reading from the analog pin. The
`update` method returns the current logical light level.

The light level value returned by `update` represents a logical light level
between zero and the maximum level passed to the instance's constructor.  The
`noise` parameter to the constructor is used to define "meaningful" changes in
the LDR reading, a higher `noise` value requires a larger change in the readings
to cause a change in the reported light level, and a `noise` value of zero will
do no debouncing of the readings.

## Testing

This library includes unit tests that can be run on a host system (not on the
Arduino) using CMake. From the top-level of the library repo, these commands
should run the tests:

```
cd tests
cmake -S . -B build
cmake --build build && (cd build; ctest --output-on-failure)
```

The first `cmake` command above only needs to be run once, the second command
can be run each time the source is changed to check the test status.
