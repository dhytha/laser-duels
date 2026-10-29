# Laser Duels Arduino Sketch

The complete Arduino Mega 2560 game is in [LaserDuels.ino](./LaserDuels.ino), organized into four labeled chunks:

1. Hardware configuration, shared data types, and direct keypad scanning.
2. LCD output, trivia/arithmetic question selection, and answer entry.
3. Servo timing, bounded laser pulses, and phototransistor calibration/detection.
4. Round and match state machines, startup, and the non-blocking main loop.

## Before uploading

The sketch starts in a safe setup-error state because physical hardware values must be confirmed before motion or laser outputs are enabled.

1. Confirm the wiring against the project Assembly Directions.
2. Change the pin assignments and keypad layout in `Config` if the as-built wiring differs.
3. Calibrate each servo's rear-facing and opponent-facing positions in small steps. Replace `SERVO_REAR_DEGREES` and `SERVO_FACING_DEGREES`; do not assume the default endpoints are safe.
4. Measure phototransistor readings with the actual divider and lighting. Set `SENSOR_BRIGHTER_IS_HIGHER`, `SENSOR_HIT_DELTA`, and `SENSOR_REARM_DELTA`.
5. Verify the laser driver is hardware-default-OFF and set `LASER_DRIVER_ACTIVE_HIGH` to match it.
6. Only after those checks, set `HARDWARE_CONFIGURATION_CONFIRMED` to `true`.

The default display build uses an I2C backpack on Mega SDA/SCL pins 20/21 (`LCD_USE_I2C` is `1`). Install a compatible `LiquidCrystal_I2C` library and set `LCD_I2C_ADDRESS` to the address verified on the assembled backpack. Do not wire both LCD interfaces at once.

## Build and operation

Open the `LaserDuels` sketch in the Arduino IDE, select an Arduino Mega 2560-compatible board and port, then compile and upload. The sketch uses the Arduino `Servo` library and the compatible `LiquidCrystal_I2C` library.

- Numeric keys enter an answer; `*` clears it; `#` submits it.
- A correct answer triggers that player's bounded laser pulse; a sensor hit, not a correct answer alone, awards a point.
- Each reflex button requests a one-second turn. A three-second cooldown begins when that servo returns to its rear position.
- Either player's `#` starts a new match from the winner screen.

Test each input, sensor, servo, and laser driver independently before playing. Use an external regulated supply for both MG995R servos, share grounds as specified in the wiring guide, and keep every laser beam contained and away from people and reflective surfaces.

## Validation note

The validation record below applies to the earlier parallel-LCD configuration. The current I2C-LCD configuration still needs to be compiled with the installed `LiquidCrystal_I2C` library and tested with the backpack's verified address.

Static checks also passed for pin conflicts, keypad dimensions, trivia prompt/answer counts, numeric answer bounds, configured timing constants, absence of blocking `delay()` calls, safe-default laser setup, and arithmetic operand ranges. These checks do not replace compiling the optional I2C variant or testing the assembled game. Complete the documented hardware acceptance checks before enabling the game.
