# Laser Duels — Implementation Plan

This document is the complete implementation blueprint for the Arduino Mega version of Laser Duels. It describes the hardware interface, software responsibilities, data and state model, gameplay algorithms, integration sequence, and verification needed to write the full Arduino sketch. It intentionally specifies behavior rather than exact source-code syntax.

## 1. Scope and requirements

### 1.1 System

Build a two-player, standalone game controller for an Arduino Mega 2560-class board. It controls:

- Two directly wired 4×3 matrix keypads (one per player), with keys 0–9, `*`, and `#`.
- Two momentary reflex buttons.
- Two MG995R standard servos, one for each motorized character.
- Two 5 V red laser modules, one attached to each character.
- Two phototransistors, one mounted on the back of each character.
- One 16-column by 2-row blue/white LCD.
- One piezo buzzer.
- External power for the servos, and appropriate output drivers for the lasers.

The supplied project README describes an Uno and keypad multiplexing. Those details are superseded for this implementation: the controller is a Mega, and the keypads connect directly to Mega pins without shift registers or keypad ICs.

### 1.2 Required gameplay

- Both characters begin rear-facing, with their lasers aimed away from the opponent and their rear sensors exposed to the opponent.
- Both players receive the same numeric question.
- Players can enter and submit answers independently while the game continues.
- A correct answer fires that player's laser. An incorrect answer does not fire and does not impose a penalty or lockout.
- Each reflex-button press initiates a 180-degree turn toward the opponent for one second, then returns the character to its starting orientation. A three-second cooldown prevents retriggering.
- A point is awarded to a player whose laser is detected by the opponent's rear phototransistor.
- A simultaneous two-way hit awards no points and does not end the current round.
- The first player to five points wins. The winner and final scores remain displayed until a new match is requested.
- Questions are drawn from a mixture of built-in numeric trivia and randomly generated arithmetic.

### 1.3 Hardware-specific requirements

- Use the Adafruit assembled 16×2 LCD in its native parallel mode, or support an optional I2C backpack as a build-time wiring choice. The actual assembled LCD kit is normally a parallel HD44780-compatible display; I2C requires a separately fitted backpack.
- Use direct keypad matrix wiring, seven Mega pins per keypad (four rows and three columns).
- Recommend a robust, calibrated phototransistor circuit rather than relying on a bare digital input.
- Do not power MG995R servos from the Mega 5 V pin or USB. Use a separate, appropriately rated regulated servo supply and connect its ground to Mega ground.
- Do not drive laser modules directly from Mega GPIO pins unless the particular module's current specification and interface explicitly permit it. Use a suitable transistor/MOSFET driver stage.

## 2. Decisions, assumptions, and items to confirm

The following defaults make the system implementable without changing the stated game rules. Confirm or adjust them before finalizing gameplay and wiring.

| Topic | Recommended default | Why it is needed |
|---|---|---|
| Round transition | The first non-simultaneous confirmed hit awards one point and ends the round. After a short result display, the game returns both characters to rear-facing, clears transient input/fire state, chooses a new question, and resumes. | The README says to reset the round after a point but does not define the exact transition. |
| Simultaneous hits | Treat two opposite-side sensor hits occurring within a configurable 100 ms resolution window as simultaneous: award neither point and keep the same question/round active. A single hit is committed only after that window expires without an opposing hit. | "Simultaneously" needs a measurable software definition. The 100 ms value is a starting point for testing, not a sensor guarantee. |
| Laser pulse | Fire a bounded pulse (recommended starting value: 250 ms) for each valid correct submission, then turn the laser off. A later correct submission can fire again while the question is active. | The README says "fire" but does not define how long the laser remains on or whether a player can fire again. |
| Servo cooldown | Start the three-second cooldown when the servo returns to the rear-facing position. | This interprets "after rotating" conservatively and prevents a new action overlapping the return motion. |
| Question text | Keep arithmetic prompts short enough for the LCD. For longer trivia prompts, show a non-blocking sequence of question pages while leaving input and sensor processing active. | A 16×2 display cannot show every natural-language trivia question at once. |
| Input keys | Use a configurable key map. Recommended convention: digits enter an answer, `*` clears, and `#` submits. | The physical keypad legends/layout have not been specified. |
| New match | In the winner screen, either player's submit key starts a fresh match after a deliberate press; a short debounce and release are required. | The README allows restarting but does not define the restart control. |
| Trivia content | Begin with a curated compile-time question table of short prompts and integer answers. | Trivia topics, difficulty, and final question count were not specified. |

If these defaults are accepted, they can be used as initial configuration values and adjusted after physical testing. No blocking delay should be used to implement any of these timed behaviors.

## 3. Hardware interface and proposed pin map

This is a proposed pin map, not a substitute for checking the actual LCD backpack, servo connector, laser module, keypad cable order, and sensor polarity before applying power. Keep pin assignments centralized in one hardware-configuration section so they can be changed without editing game logic.

### 3.1 Proposed Mega pins

| Function | Mega pin(s) | Notes |
|---|---:|---|
| LCD I2C SDA / SCL (selected build) | 20 / 21 | Mega hardware I2C pins. Do not assign these to other functions. |
| Player 1 Adafruit 3×4 keypad | Connector order `COL2, ROW1, COL1, ROW4, COL3, ROW3, ROW2` to pins 22–28 | Row array `{23, 28, 27, 25}`, column array `{24, 22, 26}`; pin 29 is unused. |
| Player 2 Adafruit 3×4 keypad | Same connector order to pins 34–40 | Row array `{35, 40, 39, 37}`, column array `{36, 34, 38}`; pin 41 is unused. |
| Player 1 / Player 2 reflex buttons | 46 / 47 | Use internal pull-ups where compatible with the wiring; connect each switch to ground. |
| Player 1 / Player 2 servo signal | 8 / 9 | Servo library signal pins; they need not be PWM pins. |
| Player 1 / Player 2 laser driver input | 10 / 11 | GPIO controls an external driver; default output state must be OFF. |
| Piezo buzzer | 12 | Use a passive piezo for tone generation, or configure for the selected active buzzer. |
| Player 1 / Player 2 phototransistor divider output | A0 / A1 | Analog reads support baseline measurement and configurable thresholds. |
| Spare | Remaining pins | Preserve for diagnostics or later additions; do not silently repurpose assigned pins. |

The selected LCD build uses I2C on pins 20/21. Keypads use seven pins each: 22–28 for Player 1 and 34–40 for Player 2. Pins 29 and 41 are unused. The optional parallel LCD build is not compatible with this keypad pin map unless the keypad pins are reassigned.

### 3.2 Alternate LCD build — native parallel, 4-bit

This is not the selected build. Using the native parallel interface requires six GPIO pins and therefore requires changing the keypad pin assignment to avoid conflicts. Use the LCD's 16-pin header and the kit's contrast potentiometer. Connect LCD ground and logic power to the appropriate Mega ground and regulated logic supply; connect contrast through the supplied potentiometer; connect RS, Enable, and D4–D7 to six available pins. Tie RW to ground so the controller only writes to the LCD. Backlight pins must be wired with the current-limiting arrangement appropriate to the exact LCD module; do not assume the backlight includes a resistor unless its documentation confirms it.

The sketch's display adapter for this option must initialize a 16×2, 4-bit display, set a readable startup state, clear or overwrite stale characters when changing screens, and provide functions for score/status, question pages, round results, and winner display. The contrast potentiometer should be adjusted during assembly.

### 3.3 LCD build option B — I2C backpack

Fit a compatible I2C backpack to the LCD, connect its power and ground appropriately, and connect Mega SDA to pin 20 and SCL to pin 21. Confirm the backpack's voltage requirements, pin mapping, and I2C address. The address must be a configuration value (common addresses include 0x27 and 0x3F, but neither should be assumed without checking). The contrast trimmer on the backpack replaces the kit's parallel-mode potentiometer adjustment.

The display adapter must expose the same game-facing display operations as the parallel build. Select the implementation using a single build-time hardware option; do not maintain two copies of game logic.

### 3.4 Direct keypad wiring

Each Adafruit 3×4 matrix keypad has four row wires and three column wires in an interleaved connector order: `COL2, ROW1, COL1, ROW4, COL3, ROW3, ROW2`. Connect the seven connector contacts left-to-right to the assigned consecutive pin group. The sketch maps this order to the default key layout `1 2 3 / 4 5 6 / 7 8 9 / * 0 #`. Scan each keypad independently using a keypad library compatible with the selected Arduino toolchain, or implement an equivalent non-blocking matrix scan with proper row/column electrical configuration.

Keypad events must be debounced and reported as key transitions/events rather than repeatedly interpreted as new presses while a key is held. Do not use keypad keys as interrupts. Verify the actual key labels against the configured mapping before gameplay.

### 3.5 Buttons

Wire each momentary switch between its Mega input and ground and enable an internal pull-up, unless the selected switch wiring/electrical design requires an external pull-up. A pressed button then reads as active-low. Debounce in software and detect the transition from released to pressed so a held button starts only one rotation. A press received during rotation or cooldown is ignored and cannot queue a future turn.

### 3.6 MG995R servo power and mechanical endpoints

Power both MG995R servos from a separate regulated supply with the voltage and peak-current capacity recommended by the servo manufacturer. Two MG995R units can draw substantial current, especially during startup, reversal, or a stall; size wiring and supply for combined transient load. Never use the Mega's onboard 5 V regulator or USB as the servo power source. Connect the servo supply ground, Mega ground, and driver grounds together at a suitable common ground point. Keep high-current servo return paths from disturbing the Mega's sensor and logic ground.

Connect each servo's signal wire to its assigned Mega pin and its power/ground wires to the external servo supply with polarity checked. Attach the characters and horns with power disconnected. Determine safe rear-facing and opponent-facing servo pulse endpoints by incremental bench testing; do not assume the physical mechanism can safely rotate exactly 180 degrees or that the servo's nominal range matches the mechanism. Configure separate calibrated endpoint values per player if the mounts differ. Avoid commands that force the mechanism against a hard stop, and mechanically secure the servo and character against wobble.

The software must attach/control the servo only through the servo abstraction, initialize both to calibrated rear-facing positions, and track their logical orientations. It must not infer exact physical position feedback: standard MG995R servos provide no position confirmation to this design.

### 3.7 Laser drivers and eye safety

Use a driver stage appropriate to the laser module's current and electrical characteristics: typically a logic-level N-channel MOSFET or suitable transistor, with any required gate/base resistor and protection components per the module/driver design. Connect the module's supply and return according to its rating and switch its return or supply using the designed driver. Tie grounds together as required by the driver. Set both laser outputs OFF during reset, startup, error handling, game-over, and all idle states. A hardware pull-down or equivalent default-off design is recommended so a reset or floating GPIO cannot turn a laser on.

Use the lowest practical power and a mechanically contained beam path. Never aim a laser at a person's face or eyes, and avoid reflective surfaces. Align and test with the enclosure secured and the beam directed into a safe beam stop. Ensure the laser is enabled only during the configured, bounded firing pulse.

### 3.8 Phototransistor recommendation

For each rear-facing phototransistor, use a resistor-divider/load-resistor circuit that converts received light into a voltage readable by an analog input. Select the resistor value based on the actual phototransistor and supply so both ambient-light and laser-hit readings remain within the Mega's analog input range. Connect the divider output to A0 or A1 and share the appropriate ground. Verify whether brighter illumination makes the ADC value rise or fall; do not hardcode a polarity assumption before measuring the circuit.

Mount each sensor facing backward, shielded from room light as practical, with a small opaque optical hood/aperture to reduce side light and direct exposure to the opponent's beam. A wavelength-appropriate optical filter may improve rejection of room lighting, if available. The hood must not block the intended beam over the full playable area.

At startup and between matches, sample each sensor with the lasers off to estimate a baseline. Use a configurable hit threshold relative to that baseline, a minimum consecutive-sample or elapsed-time requirement to reject noise, and hysteresis/re-arm logic to prevent one continuous illumination from generating repeated hits. Recheck the sensor behavior under expected room lighting. The analog threshold and validation time must be tuning parameters, not unexplained constants.

## 4. Software structure

Implement one Arduino sketch (or a small set of source/header files if the project grows) with clear module boundaries. Avoid dynamic allocation and blocking waits. Use fixed-size arrays and bounded string buffers appropriate to the Mega's memory.

### 4.1 Hardware/configuration layer

Owns all pin assignments, LCD build choice, keypad row/column order, key mapping, servo endpoint calibration, laser pulse duration, sensor thresholds, simultaneous-hit window, score limit, and arithmetic bounds. Use named configuration constants. Validate values where possible at startup and report configuration problems through a visible error indication; never silently enable a laser or servo with invalid configuration.

### 4.2 Input layer

Responsibilities:

- Poll both keypads and translate physical events into player-specific digit, clear, submit, or unsupported-key actions.
- Debounce both reflex buttons and generate one press event per physical press.
- Keep each player's answer buffer independent, bounded, and null-terminated if text is used.
- Reject extra digits when the answer buffer is full and provide visible/audio feedback.
- Prevent input overflow, malformed values, and accidental repeated submissions caused by a held key.

The keypad mapping must be in one location. Define whether a leading minus sign is permitted; recommended initial arithmetic constraints make answers nonnegative, so negative entry can be disabled. Clear must erase the current player's entry only. Submit must parse the complete buffer as an integer, reject an empty or malformed entry, compare it with the current answer, and then clear or retain the buffer according to the agreed UX.

### 4.3 Question provider

Represent a question as a fixed-size record containing:

- A short display prompt or prompt-page identifier.
- The correct integer answer.
- A category identifier (trivia or arithmetic).
- Optional difficulty or generation metadata for testing/debugging.

Provide two question sources:

1. A compile-time table of curated numeric trivia prompts and answers. Keep wording short, factual, and appropriate for one or more 16-character display pages. Do not store dynamically allocated strings.
2. A random arithmetic generator. Select an operation from an explicitly configured set; generate operands within documented bounds; ensure the result fits the answer representation and is solvable as an integer. For division, generate a nonzero divisor and a dividend that divides exactly. For subtraction, either constrain operands to nonnegative results or explicitly support signed answers and a minus-key path.

Choose trivia versus arithmetic using a configurable weighting. Seed the pseudo-random generator once at startup using a changing, noncritical source such as an unconnected analog input sampled before sensor calibration, optionally mixed with elapsed startup timing. This is for variety only, not security. Prevent immediate question repeats where practical (for trivia, avoid selecting the previous table index; for arithmetic, avoid identical prompt/operand combinations).

Do not regenerate the answer independently for each player. Both players must be checked against the same active question and answer.

### 4.4 Display layer

Provide a display adapter with the same API regardless of parallel or I2C hardware. It should support initialization, fixed-width line output, score/status updates, question paging, feedback messages, and winner display.

The primary layout should keep both scores visible, for example a compact first line with player labels and scores, and use the second line for a shortened question or current input/status. When natural-language trivia requires multiple pages, advance pages using a non-blocking timer while continuing to process inputs, servo timers, laser pulses, and sensor events. Avoid blocking LCD operations or frequent full-screen clears that cause flicker. Always pad/overwrite a line so remnants of longer prior text do not remain visible.

Display states should include at minimum: initializing/calibrating, ready/current question, answer accepted or incorrect feedback, point awarded, simultaneous-hit/no-point feedback, match won, restart prompt, and hardware/configuration error.

### 4.5 Servo/reflex controller

Maintain a separate state record for each player:

- Calibrated rear-facing and opponent-facing servo endpoints.
- Current action phase: ready/rear-facing, turning/aiming, returning, or cooldown.
- Timestamp of phase start and next allowed activation.
- Debounced button state.

On a valid press while ready, command the opponent-facing endpoint and start the one-second exposure timer. When it expires, command the rear-facing endpoint and start the cooldown timer. On cooldown expiry, permit another press. Handle `millis()` rollover safely by using elapsed-time comparisons. Service both players independently in every main-loop iteration; one player must never block the other.

The timing implementation must leave the character aimed at the opponent for the specified one-second interval. If motion time is included or excluded from that interval, measure behavior during bench testing and document the chosen interpretation. The recommended interpretation is one second after the turn command, with the three-second cooldown measured from the return command.

### 4.6 Laser controller

For each player, track whether a pulse is active and its shutoff timestamp. On a correct submission during an active round, turn on that player's driver output and schedule shutoff after the configured pulse duration. Do not use a delay to hold the laser on. A repeated correct submission must not extend an existing pulse indefinitely; either ignore it while that player's pulse is active or restart a bounded pulse, and choose one policy consistently. Recommended policy: ignore a new trigger while that player's pulse is already active.

Always force both laser outputs off when a pulse expires, when the round ends, when a match ends, and during reset/initialization. An incorrect answer must never alter laser state.

### 4.7 Sensor and hit-resolution layer

Sample both analog sensors frequently and consistently. For each sensor, track a calibrated baseline, filtered/latest reading, candidate-hit start time, confirmed-hit state, and re-arm state. Use a simple moving average or bounded consecutive-sample test only if needed; avoid a filter slow enough to miss the laser pulse or a turn window.

Map the sensor physically hit to the opposing shooter: a hit on Player 1's rear sensor is a hit by Player 2; a hit on Player 2's rear sensor is a hit by Player 1. Convert confirmed sensor events into player-attributed hit events and pass them to a round-resolution function. The sensor itself does not identify a laser; attribution relies on one rear sensor per character and the known opposing geometry.

When the first confirmed hit arrives, record its player and timestamp but do not immediately award a point until the simultaneous-hit window expires. If the opposite player's hit is also confirmed within the window, clear both pending hit events, award no point, show simultaneous-hit feedback, and continue the same round. If no opposite hit arrives before expiry, award the pending player's point and end the round. Ignore or safely discard later sensor activity after a round has been resolved until the next round is armed.

After a no-point simultaneous event, the same question remains active. Require sensors to return below their re-arm threshold before accepting a new event from that sensor, so one continuous laser exposure cannot produce repeated points.

### 4.8 Game/match controller

Use an explicit finite-state model rather than a single deeply nested loop. Suggested top-level states:

1. **BOOT** — establish safe output levels immediately; initialize serial diagnostics only if desired; initialize display, keypads, buttons, servos, laser drivers, sensors, and buzzer.
2. **CALIBRATE** — keep lasers off and servos rear-facing; sample sensor baselines; display calibration status. If readings are invalid or saturated, show an actionable error and do not start a match automatically.
3. **READY/ROUND_ACTIVE** — display scores and active question; process both players' answer and button events; service servo and laser timers; sample sensors; resolve hits.
4. **ROUND_RESULT** — show point/no-point feedback for a short non-blocking interval. If a point was scored, either transition to match won or start the next round. If simultaneous hits occurred, return directly to the same active question.
5. **MATCH_WON** — stop lasers, return both characters rear-facing when safe, show winner and final scores, ignore normal answer submissions, and wait for the restart gesture.
6. **ERROR** — outputs remain safe; show/report the error and do not claim a match is running. Recovery must be deliberate (for example reset/restart), not a silent fallback.

Keep match scores as bounded integers. Increment exactly once for a resolved hit, never for a correct answer alone. After each increment, compare with the configured score limit of five. When the limit is reached, declare the winner, stop all firing, reset transient states, and display final scores.

### 4.9 Main-loop scheduling

The main loop repeatedly services short, non-blocking tasks in a predictable order:

1. Read/debounce buttons and keypad events.
2. Advance both servo state machines.
3. Advance laser shutoff timers.
4. Sample/filter phototransistors and create sensor events.
5. Resolve pending/simultaneous hits and update game state.
6. Update question page and feedback timers.
7. Refresh only display content that has changed.
8. Emit optional concise diagnostic information when enabled.

No task may use a long delay, wait in a loop for a key, or prevent the other player's input/sensor from being processed. All timers use elapsed-time comparisons that remain correct across the Arduino clock counter rollover.

## 5. Gameplay procedures in detail

### 5.1 Startup and calibration

1. Set laser control pins to their hardware-safe OFF state as early as possible.
2. Initialize the LCD and show a startup message.
3. Initialize keypad/button inputs and servo control.
4. Move both characters to their individually calibrated rear-facing endpoints.
5. Keep lasers off and sample each phototransistor in the expected ambient environment.
6. Check that analog readings are not pinned at an ADC rail and are stable enough for threshold calculation. If not, show a sensor/setup error and keep the game inactive.
7. Seed the pseudo-random generator, set both scores to zero, select the first shared question, initialize timers and answer buffers, and enter the active-round state.

Calibration should allow normal room lighting while requiring a laser-level change sustained for the configured validation period. If automatic thresholds prove unreliable under the actual enclosure and lighting, add a documented manual threshold configuration; do not silently learn a threshold while a laser is firing.

### 5.2 Answer-entry behavior

Each player's keypad is independent. A digit appends to that player's buffer if capacity allows. Clear empties only that player's buffer. Submit with an empty buffer gives feedback and does nothing else. A nonempty integer is compared with the current question's answer:

- Incorrect: show brief player-specific incorrect feedback; leave the active question/round unchanged; do not lock the player out; do not fire a laser.
- Correct: show brief accepted feedback; trigger that player's bounded laser pulse; continue listening for sensor hits. Correctness alone never awards a point.

Input handling and feedback must not pause the other player. Define whether to clear the answer after every submission; recommended behavior is clear after both correct and incorrect submission so a retry starts with a fresh entry.

### 5.3 Round resolution

The round remains active until a non-simultaneous confirmed hit or a match-ending score. A player may rotate and submit answers concurrently. A player can score only if their laser pulse is detected by the opponent's rear sensor. A sensor event from a continuous exposure is one event, not repeated scoring opportunities.

On a single resolved hit:

1. Turn both lasers off immediately.
2. Award one point to the shooter mapped from the hit sensor.
3. Display the updated score and point result.
4. If that player has reached five, enter match-won state.
5. Otherwise, after the brief result interval, return both characters to rear-facing, clear answer and hit state, choose a new question, and begin the next round.

On simultaneous hits:

1. Turn off active laser pulses after their configured pulse interval (or immediately if the game policy chooses to cancel them at detection).
2. Award no points.
3. Show simultaneous-hit feedback.
4. Keep the same question active and continue the round after sensor re-arming.

### 5.4 Match restart

After a winner is declared, keep both lasers off and keep the final score visible. Accept only the configured restart gesture. Starting a new match resets scores, answer buffers, active pulses, sensor event state, pending hit resolution, cooldowns as appropriate, and question history; returns both servos to rear-facing; recalibrates sensors if needed; and generates a new shared first question.

## 6. Data, limits, and memory considerations

- Use fixed-size answer buffers sized for the maximum permitted answer digits plus terminator.
- Define an integer type and range suitable for all configured arithmetic answers and scores. Check arithmetic generation for overflow before storing a result.
- Keep the trivia table in program memory if its size warrants it, using the platform's supported constant-data mechanism.
- Avoid heap allocation, dynamic `String` concatenation in frequently executed paths, and unbounded input.
- Bound LCD lines to 16 display characters and format output into fixed-size buffers.
- Keep diagnostic output disabled or concise in normal play; serial logging must never block timing-critical work.
- Define explicit maxima for generated operands, answer length, trivia count, key debounce interval, sensor sample interval, hit-confirm time, and simultaneous-hit window.

## 7. Build and implementation sequence

### Phase 1 — Confirm and record wiring

- Confirm the exact Mega variant, LCD interface selection, keypad row/column order, key legend, phototransistor part/polarity, laser module current/interface, servo supply, and physical servo endpoints.
- Produce a wiring diagram matching the proposed pin map, including a common-ground plan, external servo power, laser drivers, sensor dividers, and LCD contrast/backlight.
- Label both players' keypads, sensors, laser drivers, and servo leads.

### Phase 2 — Safe hardware bring-up

- Verify Mega startup and safe laser-off behavior with lasers disconnected.
- Test LCD initialization in the selected mode and verify full 16×2 clearing/overwriting.
- Test each keypad's row/column mapping and each button's debounced press/release.
- Test phototransistor analog readings in ambient light and under a safely contained laser beam; record baseline, polarity, and usable threshold range.
- Test one MG995R at a time with an external supply and unloaded horn, then calibrate safe endpoints; add the second servo only after supply and ground stability are verified.
- Test each laser driver independently with the beam safely terminated.

### Phase 3 — Independent device services

- Implement and verify the display adapter for both documented build options (the selected build is enabled at compile time).
- Implement keypad parsing, per-player answer buffers, clear/submit behavior, and overflow feedback.
- Implement button debounce and two independent non-blocking servo state machines.
- Implement bounded laser pulses and safe output defaults.
- Implement sensor sampling, baseline/threshold tuning, event confirmation, and re-arming.

### Phase 4 — Question and scoring logic

- Add the fixed trivia table and validate every answer against its prompt.
- Add bounded arithmetic generators for each selected operation and test their edge cases.
- Add weighted question selection, shared question state, and non-immediate-repeat behavior.
- Add scoring, five-point win detection, and score display.

### Phase 5 — Integrate the round state machine

- Integrate answer checks, laser pulses, sensor-to-shooter mapping, and hit resolution.
- Add simultaneous-hit handling and the configured round-result transition.
- Add winner display and restart gesture.
- Verify that both players' input and timers continue to work concurrently under every state.

### Phase 6 — Tune and document the assembled system

- Tune servo endpoints and verify the one-second turn and three-second cooldown physically.
- Tune sensor threshold, validation, hysteresis, and hit window across expected room-light conditions and playable distances.
- Confirm every laser pulse is bounded and every reset/error/win path turns both lasers off.
- Record final pin assignments, LCD mode/address, keypad mappings, supply ratings, measured calibration values, and any changed default gameplay timings in the project documentation.

## 8. Verification and acceptance tests

### 8.1 Initialization and safety

- Power-up, reset, and brownout do not leave either laser on.
- Invalid/missing sensor readings prevent a false "ready" state and produce a visible error.
- The Mega remains stable while both MG995R servos start, stop, and reverse on the external supply.
- LCD startup and error screens work in the chosen LCD build; no LCD operation blocks gameplay.

### 8.2 Inputs and answer handling

- Both keypads independently enter the intended digits and special keys.
- A held key or button creates one event, not repeated entries/rotations.
- Clear, submit, empty answer, maximum-length answer, overflow, and malformed input behave as specified.
- Wrong answers never fire lasers and can be retried immediately.
- Correct answers fire only the submitting player's laser for a bounded duration.
- Simultaneous keypad/button activity does not drop the other player's events.

### 8.3 Servo timing

- Each button press commands the correct player's turn.
- The character returns to its calibrated rear-facing position after the configured one-second interval.
- A button press during rotation/cooldown is ignored and does not queue.
- A new press is accepted after the configured cooldown.
- Both servos can operate at once without blocking sensing, input, or laser shutoff.
- Mechanical stops are not forced and no cable/character collision occurs through the full motion.

### 8.4 Sensor and scoring

- Ambient light alone does not produce a hit after calibration.
- A contained opponent laser pulse produces one confirmed hit at the rear sensor over the intended range and angle.
- Light on one sensor maps to the opposite shooter; the mapping is verified with labels.
- One non-simultaneous hit awards exactly one point to the correct player.
- A correct answer without a sensor hit awards no point.
- A laser striking a sensor while the opponent is turned away/mid-rotation can be detected; a blocked sensor does not score.
- Two hits inside the simultaneous window award zero points and retain the same question.
- Two hits outside the window resolve according to the first-hit rule.
- Sustained illumination cannot cause repeated points; the sensor must re-arm before another hit.

### 8.5 Question, round, and match behavior

- Trivia questions and answers match, fit or page correctly on the LCD, and both players receive the same question.
- Arithmetic addition, subtraction, multiplication, and exact division (if enabled) produce valid results within configured bounds.
- Generator edge cases never divide by zero, overflow, or create an unenterable answer.
- After a scored non-winning hit, the correct score is retained, transient state is cleared, and a new question begins.
- The first player to five points wins; the score cannot increment again after match end.
- Winner and final scores remain visible until the restart action.
- Restart resets the match cleanly and does not accidentally reuse an active pulse or pending sensor hit.

### 8.6 Timing and soak

- Run both players' keypads, buttons, servos, sensors, and laser pulses concurrently for an extended play session.
- Verify no delay-based pause causes missed input or missed sensor events.
- Exercise timer rollover behavior using a test harness or shortened test intervals.
- Confirm the sensor sampling and hit-confirm time detect the shortest configured laser pulse with margin.

## 9. Final implementation checklist

- [ ] All Mega pins are declared once and match the as-built wiring.
- [ ] Exactly one LCD wiring mode is selected, while game display behavior stays identical across modes.
- [ ] Both keypads are wired directly and mapped to the correct players.
- [ ] Servo supply is external, adequately rated, and shares a designed common ground with the Mega.
- [ ] Laser modules use safe driver circuits and default-off hardware behavior.
- [ ] Phototransistor divider polarity and thresholds are measured and documented.
- [ ] All time-based behavior is non-blocking.
- [ ] Both players can act concurrently.
- [ ] Numeric trivia and arithmetic share a single question/answer interface.
- [ ] Points require a confirmed optical hit, not merely a correct answer.
- [ ] Simultaneous hits, round reset, laser pulse, cooldown, and restart policies are configured and tested.
- [ ] Match ends at five points and displays final scores.
- [ ] Verification tests pass on the assembled mechanical system, not only in a desktop simulation.
