# Laser Duels — Assembly Directions

This guide covers the physical assembly, wiring, and initial setup of Laser Duels. It uses the **Arduino Mega 2560 build** in the Implementation Plan. The older project README mentions an Uno and keypad multiplexers; those are superseded here. The two keypads connect directly to the Mega, and no shift registers or keypad ICs are used.

> **Before powering anything:** Check the part labels and datasheets for the exact LCD, laser modules, phototransistors, servos, battery supplies, and driver components. The project files do not specify every module's connector order, current draw, resistor value, or supply rating, so do not guess those values.

## 1. Parts and tools

### Main components

- 1 Arduino Mega 2560-class board
- 2 4×4 matrix keypads
- 2 momentary push buttons
- 2 MG995R standard servos and character mounts
- 2 red laser modules and suitable driver stages
- 2 phototransistors, each with a suitable load resistor/divider circuit
- 1 16×2 LCD, either native parallel mode or fitted with an I2C backpack
- 1 piezo buzzer
- Regulated logic power for the Mega and low-current electronics
- Separate regulated servo supply rated for the combined transient current of both servos
- Appropriate laser-module supplies and driver components, as specified for the exact modules
- 1/8-inch MDF enclosure panels, wiring, connectors, fasteners, and suitable adhesive

### Useful tools

Screwdrivers, wire cutters/strippers, soldering equipment, multimeter, labels, and a computer with the Arduino IDE or compatible toolchain. A current-limited bench supply is useful for bring-up.

## 2. Mechanical assembly

1. **Cut and inspect the panels.** Cut the enclosure, main playing field, rear electrical housing, and two control pods from 1/8-inch MDF. The documented overall size is 18 × 13 × 5 inches; the playing field is 16 × 8 inches. Confirm all parts fit before gluing.
2. **Build the enclosure.** Interlock the puzzle-joint panels and secure the joints with suitable adhesive. Keep the rear electrical box accessible until wiring and testing are complete.
3. **Prepare the display and pods.** The rear panel has a 4 × 1.25-inch display opening. Each 4 × 2.3-inch control pod has a keypad cutout and a 1-inch-diameter button hole. Dry-fit the LCD, keypads, and buttons before final mounting.
4. **Mount the servos.** Secure one servo at each end of the field so each character can turn toward the opposing character without colliding with the enclosure or the other character. Leave access to the servo connector and horn.
5. **Mount the characters and optics.** Attach each laser to the character's front and its phototransistor to the back. In the starting, rear-facing position, each laser points outward, away from the opponent, and each rear sensor faces the opponent. Fit an opaque sensor hood/aperture to reduce room light without blocking the intended beam.
6. **Check the full movement.** With power disconnected, move each character through its intended travel by hand where the mechanism permits. Check that wiring cannot snag and that no mechanism is forced against a hard stop.
7. **Plan cable routing.** Route keypad, button, sensor, laser-control, and servo-signal leads beneath the field into the rear electrical compartment. Secure cables away from moving parts. Keep high-current servo supply/return wiring separate from sensitive sensor wiring where practical.
8. **Mount electronics.** Mount the Mega, LCD, buzzer, power distribution, and laser driver stages securely in the rear compartment. Keep the compartment open for testing and strain-relieve cables before closing it.

## 3. Mega pin assignments

Use the following proposed pin map in both the wiring and the sketch. Keep the two LCD options mutually exclusive. The keypad's actual row/column wire order is not standardized; identify it and record it before connecting.

| Component/function | Mega connection |
|---|---|
| Parallel LCD RS, Enable, D4, D5, D6, D7 | 22, 23, 24, 25, 26, 27 |
| I2C LCD SDA / SCL (alternate LCD build only) | SDA/pin 20 / SCL/pin 21 |
| Player 1 keypad rows / columns | Rows 30–33 / columns 34–37 |
| Player 2 keypad rows / columns | Rows 38–41 / columns 42–45 |
| Player 1 / Player 2 reflex buttons | 46 / 47 |
| Player 1 / Player 2 servo signal | 8 / 9 |
| Player 1 / Player 2 laser driver control | 10 / 11 |
| Piezo buzzer | 12 |
| Player 1 / Player 2 phototransistor divider output | A0 / A1 |

Pins 30–37 and 38–45 are keypad pin groups. The row/column split above is a proposed convention: if the keypad cable order differs, record the actual order and use the same order in the sketch's keypad configuration. Do not connect a keypad to an assumed connector pinout.

## 4. Wiring directions

Make all wiring with power disconnected. Label both ends of each player's wires. Check every power rail and ground connection with a multimeter before connecting the Mega or powering a laser.

### A. Grounds and power distribution

1. Choose a regulated logic supply compatible with the Mega and the attached logic modules. Power the Mega using an input method supported by the exact board; do not apply an unknown voltage to its 5 V pin.
2. Power the two MG995R servos from a **separate regulated servo supply** meeting the manufacturer's voltage and combined peak-current requirements. Never power the servos from the Mega's 5 V regulator or USB.
3. Connect the servo supply ground to Mega ground at a suitable common-ground point. Connect laser-driver grounds and sensor/LCD grounds as required by their circuits. If using a non-isolated laser driver, its control reference must share the Mega ground.
4. Route servo current returns directly to the servo supply/common power point rather than through the Mega board or sensor ground wiring. Use wire and connectors rated for the expected current.
5. Do not connect separate positive supply rails together unless the circuit design explicitly requires it. Confirm the voltage and polarity at each connector before attaching a component.

### B. Keypads

For each 4×4 keypad, connect its four row conductors and four column conductors to its assigned eight-pin group:

- Player 1: four rows to pins 30–33 and four columns to pins 34–37.
- Player 2: four rows to pins 38–41 and four columns to pins 42–45.

Identify the row/column conductors using the keypad documentation or a continuity test. Label their order (for example, R1–R4 and C1–C4) and enter that exact order and key legend in the sketch. The two keypads are wired directly to the Mega; do not add shift registers or keypad ICs for this build.

### C. Reflex buttons

For each momentary button, connect one terminal to its assigned input (Player 1 pin 46; Player 2 pin 47) and the other terminal to Mega ground. Configure each pin as `INPUT_PULLUP` in the sketch; a pressed button then reads LOW. If the selected button or circuit requires external conditioning, follow its circuit design instead.

### D. Servos

For each MG995R:

1. Connect the signal lead to pin 8 (Player 1) or pin 9 (Player 2).
2. Connect the servo power and ground leads to the external servo supply, checking the servo's lead colors and connector polarity against its documentation.
3. Join the servo-supply ground to Mega ground as described above.
4. Calibrate each servo separately using the procedure below, first unloaded and then with the character installed.
5. Record the two calibrated endpoints for each player in the sketch. Do not assume a nominal 180-degree servo command is safe or mechanically possible. Stop if the linkage binds, buzzes under load, or reaches a hard stop.

#### Required servo calibration

The full game sketch intentionally refuses to start until its hardware configuration is confirmed, and it does not attach the servos in that state. Use this temporary bench sketch to find safe `Servo.write()` angle values before enabling the game.

1. Disconnect both laser modules from their supplies, remove the character/horn from the servo being tested, and leave the other servo disconnected. Keep the mechanism clear of hands and objects.
2. Connect one servo signal at a time to the pin being tested (8 for Player 1, 9 for Player 2). Power it from the separate regulated servo supply; connect that supply ground to Mega ground. Do not power it from USB or the Mega 5 V output.
3. Upload this temporary sketch to the Mega. It starts at the middle command of 90 degrees, and the Serial Monitor changes the command by only two degrees per `+` or `-` character. It deliberately limits test commands to 20–160 degrees; do not extend that range unless the servo and mechanism documentation and unloaded testing show it is safe.

   ```cpp
   #include <Servo.h>

   Servo testServo;
   const uint8_t SERVO_SIGNAL_PIN = 8;  // Use 9 for Player 2.
   int testAngle = 90;

   void setup() {
     Serial.begin(9600);
     testServo.attach(SERVO_SIGNAL_PIN);
     testServo.write(testAngle);
     Serial.println(F("Servo test: + or - moves 2 degrees; p prints angle."));
     Serial.println(testAngle);
   }

   void loop() {
     if (!Serial.available()) {
       return;
     }

     const char command = static_cast<char>(Serial.read());
     if (command == '+') {
       if (testAngle < 160) {
         testAngle += 2;
       }
       testServo.write(testAngle);
       Serial.println(testAngle);
     } else if (command == '-') {
       if (testAngle > 20) {
         testAngle -= 2;
       }
       testServo.write(testAngle);
       Serial.println(testAngle);
     } else if (command == 'p' || command == 'P') {
       Serial.println(testAngle);
     }
   }
   ```

4. Open Serial Monitor at 9600 baud. Send `+` or `-` (no line ending is easiest) one command at a time. Confirm the bare servo moves smoothly in both directions. Disconnect power immediately if it chatters continuously, stalls, heats up, or the supply voltage collapses.
5. Return the bare servo to 90 degrees. Turn off and disconnect servo power before attaching its horn. Fit the horn so the character's **rear-facing start orientation** is as close to the mechanical middle of its travel as possible. Secure the horn according to the servo/mount instructions.
6. Restore power and use two-degree steps to identify the rear-facing and opponent-facing positions. Record the angle for each orientation. The opponent-facing position should turn the character toward the other player; do not force a full 180-degree rotation if the physical mount does not safely allow it.
7. Test both endpoints repeatedly with the character attached. Leave clearance at both ends of travel; do not use an angle that makes the linkage hit a hard stop or makes the servo buzz under load. If the character cannot reach both orientations safely, change the mount/linkage rather than forcing the servo.
8. Repeat for the other player. Since the two characters face opposite directions, their recorded facing angles may move in opposite numeric directions.
9. Enter the measured values in `SERVO_REAR_DEGREES` and `SERVO_FACING_DEGREES` in `LaserDuels.ino`, keeping Player 1 values at index 0 and Player 2 values at index 1. Re-upload the full game only after all hardware configuration values have been checked.

The test sketch is temporary and is not the game firmware. Re-upload `LaserDuels.ino` after recording the endpoints.

### E. Laser modules and driver stages

**Do not connect a laser module directly to a Mega GPIO pin.** Pins 10 and 11 are logic control signals only.

1. Install one driver stage per laser module. Select the transistor/MOSFET, gate/base resistor, protection parts (if required), and supply arrangement according to the actual module's current, voltage, and driver requirements.
2. Connect Player 1's driver control input to Mega pin 10 and Player 2's to pin 11. Wire each module's power and switched path according to the driver design and module documentation.
3. Provide a hardware default-OFF condition, such as a suitable pull-down on a MOSFET gate, so reset or a floating control pin cannot turn a laser on.
4. Connect grounds as required by the driver. Verify the driver is OFF with the module disconnected before connecting the laser.
5. Route and align the beam only with the enclosure secured and a safe beam stop in place. Never aim at people, eyes, or reflective surfaces. Keep the beam path contained and use the lowest practical power.

### F. Phototransistors

Each phototransistor needs a load resistor/divider circuit that produces a voltage within the Mega analog-input range. The correct resistor value and collector/emitter arrangement depend on the exact phototransistor and supply; use its datasheet and verify the circuit before connecting it.

1. Mount Player 1's sensor on the back of Player 1's character and connect its divider output to A0. Mount Player 2's sensor on the back of Player 2's character and connect its output to A1.
2. Connect each divider to the appropriate logic supply and ground, following the selected circuit. Make the sensor ground common with Mega ground.
3. Shield each sensor from ambient and side light with its hood. Ensure the hood still admits the opponent's beam throughout the intended playing area.
4. Run the temporary serial test below to read both analog inputs. With the laser modules disconnected, record each sensor's range under the expected room lighting. Then, with the enclosure secured and a safe beam stop in place, test one laser at a time and record the sensor's readings while illuminated. Determine whether the analog value rises or falls.

   ```cpp
   const uint8_t SENSOR_PINS[2] = {A0, A1};
   uint32_t lastReport = 0;

   void setup() {
     Serial.begin(9600);
   }

   void loop() {
     if (static_cast<uint32_t>(millis() - lastReport) >= 250) {
       lastReport = millis();
       Serial.print(F("P1 A0="));
       Serial.print(analogRead(SENSOR_PINS[0]));
       Serial.print(F("  P2 A1="));
       Serial.println(analogRead(SENSOR_PINS[1]));
     }
   }
   ```

5. In `LaserDuels.ino`, set `SENSOR_BRIGHTER_IS_HIGHER` to match the measured direction. Set `SENSOR_HIT_DELTA` above normal ambient variation but comfortably below the smallest measured laser-hit change. Set `SENSOR_REARM_DELTA` above ambient noise and below `SENSOR_HIT_DELTA`, so the sensor can re-arm after the beam is removed. These are ADC-count differences from the automatically measured startup baseline, not absolute ADC values.
6. Do not start a match if readings are near 0 or 1023, unstable, or indistinguishable between ambient light and a valid hit. The game sketch rejects startup readings near the ADC rails or with a calibration spread above `SENSOR_MAX_CALIBRATION_SPREAD` (default 180 counts); correct the lighting/circuit problem rather than raising that limit without measurements. If the measured hit change is too small to leave a reliable gap above ambient noise, improve the sensor circuit, shielding, or alignment.

The temporary sensor test is for setup only; re-upload the game sketch afterward. During normal startup and match restart, the game samples the ambient sensor baseline for about 1.8 seconds with lasers off. It does **not** automatically select or adjust the hit/re-arm thresholds; those remain the measured values configured in the sketch.

### G. LCD (choose one wiring option)

#### Option 1: Native parallel LCD, 4-bit mode

Use the LCD's 16-pin header and the supplied contrast potentiometer, if present.

| LCD signal | Connection |
|---|---|
| VSS / GND | Ground |
| VDD / logic power | Supply specified for the LCD |
| VO / contrast | Center/wiper of contrast potentiometer; potentiometer ends to the LCD's specified logic supply and ground |
| RW | Ground (write-only) |
| RS | Mega pin 22 |
| Enable (E) | Mega pin 23 |
| D4, D5, D6, D7 | Mega pins 24, 25, 26, 27, respectively |
| Backlight | Wire per the exact LCD module documentation |

Do not assume the backlight has a built-in current-limiting resistor. Confirm the module's backlight requirements before applying power. Initialize the display as a 16-column, 2-row, 4-bit LCD in the sketch.

#### Option 2: LCD with I2C backpack

This option requires a compatible I2C backpack fitted to the LCD; it is not the native parallel wiring.

| Backpack signal | Connection |
|---|---|
| GND | Ground |
| VCC | Supply compatible with both backpack and LCD |
| SDA | Mega SDA / pin 20 |
| SCL | Mega SCL / pin 21 |

Confirm the backpack voltage requirements, connector mapping, and I2C address. Set the verified address in the sketch; do not assume a common address. Use the backpack's contrast trimmer. Do not also wire the parallel LCD interface to the Mega.

### H. Piezo buzzer

Connect the buzzer control lead to Mega pin 12 and its other lead to ground, observing polarity if the selected buzzer is polarized. Confirm that the buzzer's current is safe for direct GPIO drive; use an appropriate driver if it is not. Configure the sketch for the actual part (passive piezo or active buzzer).

## 5. Initial setup and bring-up

Bring the hardware up in stages. Keep the laser modules disconnected until their driver outputs have been checked. The project sketch initially displays a setup error and refuses to attach the servos until its hardware-confirmation flag is enabled; perform servo and sensor calibration with the temporary test sketches above first.

1. **Inspect wiring.** With all power disconnected, confirm pin labels, connector polarity, supply voltages, common grounds, servo power separation, and that no positive rail is shorted to ground.
2. **Identify each keypad.** Use its documentation or a continuity test to record which conductors are rows and columns, and the key legends associated with each row/column crossing. The sketch's default keypad layout is `1 2 3 A / 4 5 6 B / 7 8 9 C / * 0 # D`; change `KEYPAD_LAYOUT` and the row/column pin arrays if the physical keypad differs. The scanner accepts one key at a time per keypad, so do not hold multiple keys on one keypad.
3. **Finish bench calibration.** Use the servo calibration and sensor serial-test procedures above. Record both endpoints, sensor direction, ambient variation, and laser-hit readings.
4. **Configure and compile.** Open `LaserDuels.ino` in the Arduino IDE, choose an Arduino Mega 2560-compatible board and port, and compile with the `Servo` and `LiquidCrystal` libraries. The sketch defaults to a parallel LCD. If using I2C, set `LCD_USE_I2C` to `1`, install a compatible `LiquidCrystal_I2C` library, and enter the measured I2C address.
5. **Set the hardware values.** In the `Config` section of `LaserDuels.ino`, enter the actual servo endpoints, sensor direction and thresholds, and laser-driver active level. Check the pin map and keypad layout against the wiring. Only then change `HARDWARE_CONFIGURATION_CONFIRMED` from `false` to `true` and upload the game.
6. **Check safe startup with lasers disconnected.** Power the Mega and logic electronics, then the external servo supply. Verify the LCD shows the calibration screen and both characters return to their rear-facing positions without binding. If it shows a setup error, keep lasers disconnected and correct the indicated sensor/configuration problem.
7. **Test the inputs and display.** Verify the score/question display works; check both keypads' digits, `*` clear, and `#` submit behavior; and verify each button turns only its own character. A held key or button should produce only one event. Adjust contrast as needed.
8. **Recheck sensors in game startup.** With lasers off and room lighting unchanged, allow the game's 1.8-second baseline calibration to finish. Test one safely contained laser at a time and verify that a hit on Player 1's rear sensor is attributed to Player 2, and vice versa. If readings fail calibration or hit detection, disable the lasers and retune the sensor setup.
9. **Test the laser drivers separately.** First verify their default-OFF state with laser modules disconnected. Then connect and test one module at a time, with the enclosure secured and a safe beam stop in place. Confirm each correct submission makes only its own bounded pulse. Never look into a beam or aim it toward anyone.
10. **Run an end-to-end acceptance test.** Confirm wrong answers do not fire, correct answers alone do not award points, optical hits award a point to the correct player, simultaneous hits do not score, a scored round generates a new question, and the first player to five wins. Test restart with `#` at the winner screen.
11. **Close and document.** After all checks pass, secure covers and strain-relieve cables. Label each player's keypad, button, servo, laser driver, and sensor. Record final pin assignments, LCD mode/address, supply ratings, servo endpoints, sensor readings/thresholds, and any changes from the suggested timing values.

## 6. Before each use

- Set the characters to their calibrated rear-facing starting positions.
- Confirm sensor hoods and beam paths are clear and aimed safely.
- Confirm the servo supply and logic supply are connected to the correct inputs.
- Check that both lasers are OFF before starting a match.
- Recalibrate or recheck sensors if room lighting or the enclosure has changed.
- Keep the game supervised; never direct a laser toward a person's face or eyes.
