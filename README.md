# Laser Duels

**Laser Duels** is a two-player, head-to-head physical arcade game that combines fast-paced numeric trivia with timing and physical reflex mechanics. Players compete to solve numeric questions, time their character movements, and hit their opponent’s sensor with a laser beam.

---

## Table of Contents
1. [Game Overview](#game-overview)
2. [Game Procedure & Rules](#game-procedure--rules)
3. [Hardware & Components](#hardware--components)
4. [Enclosure & Mechanical Specifications](#enclosure--mechanical-specifications)
5. [System Architecture & Software Flow](#system-architecture--software-flow)
6. [Assembly Instructions](#assembly-instructions)

---

## Game Overview

Each player controls a motorized character mounted on a servo motor. The characters are stationed on opposite sides of an open playing field. 

* **Front of Character**: Equipped with a $650\text{ nm}$, $5\text{ V}$, $5\text{ mW}$ red laser diode.
* **Back of Character**: Equipped with a phototransistor optical sensor.
* **Player Controls**: Each player has a control pod consisting of a push button (reflex action) and a numeric keypad (answer entry).
* **Game Display**: An LED/LCD display mounted on the rear electrical panel displays questions, scores, and match outcomes.

---

## Game Procedure & Rules

### 1. Match Setup
* Both character structures start in a **rear-facing orientation** (lasers pointed outward away from the playing box; phototransistors exposed to the opponent).
* A question with a single numeric answer is generated and displayed on the central display screen.

### 2. Player Actions
During a round, players perform two distinct actions concurrently:
* **Reflex Button (Rotate)**: Pressing the push button activates the servo motor, rotating the player's character $180^\circ$ to face the opponent for $1\text{ second}$ before automatically returning to the starting position. After rotating, a $3\text{-second}$ cooldown timer prevents immediate re-rotation.
* **Numeric Keypad (Shoot)**: Players enter their numeric answer on their keypad and press enter.

### 3. Firing & Scoring Mechanics
* **Correct Answer**: Immediately triggers the player's laser diode to fire.
* **Incorrect Answer**: Fails to fire the laser. *Note: Incorrect inputs do not trigger a lockout timer*, allowing the player to immediately re-enter their answer.
* **Scoring a Point**: A point is awarded if a player fires their laser and successfully strikes the exposed **phototransistor** on the back of the opponent's character (requiring the opponent to be turned away or mid-rotation).
* **Simultaneous Hits**: If both players turn and hit each other simultaneously, no points are awarded and the round continues.
* **Winning the Match**: The first player to reach the programmed score limit wins the game. Final scores are displayed on the screen, and players may restart a new match.

---

## Hardware & Components

### Bill of Materials (BOM)

| Component | Quantity | Description |
| :--- | :--- | :--- |
| **Microcontroller** | 1 | Arduino Uno |
| **Display** | 1 | LCD / LED Screen |
| **Actuators** | 2 | Micro Servo Motors (e.g., SG90) |
| **Emitters** | 2 | $650\text{ nm}$, $5\text{ V}$, $5\text{ mW}$ Red Laser Diodes |
| **Sensors** | 2 | Phototransistors |
| **User Input** | 2 | $4 \times 3$ Matrix Keypads (0–9, *, #) |
| **User Input** | 2 | Momentary Push Buttons |
| **Audio** | 1 | Piezo Speaker / Buzzer |
| **Power Supply** | 1 | Portable Battery Pack with Switch |
| **Enclosure** | - | $1/8\text{ in}$ Medium-Density Fiberboard (MDF) |
| **Multiplexing** | 2 | Shift Registers / Keypad ICs |

---

## Enclosure & Mechanical Specifications

The housing is designed to sit on a standard table for seated or standing gameplay. All panel joints feature laser-cut puzzle geometry held together with high-strength adhesive.

```
+-------------------------------------------------------+
|                 REAR ELECTRICAL BOX                   |
|              [  LED / LCD Display  ]                  |
+-------------------------------------------------------+
|                                                       |
|   [Servo 1 / Laser]               [Servo 2 / Laser]   |
|   (Player 1 Side)                 (Player 2 Side)     |
|                                                       |
|                 MAIN PLAYING FIELD                    |
|                                                       |
+-------------------------------------------------------+
|  [Pod 1]                                   [Pod 2]    |
|  (Button + Numpad)                       (Button + Numpad)
+-------------------------------------------------------+
```

### Key Dimensions
* **Overall Dimensions**: $18\text{ in (L)} \times 13\text{ in (W)} \times 5\text{ in (H)}$
* **Main Playing Field**: $16\text{ in} \times 8\text{ in}$
* **Rear Housing Panel**: $5\text{ in}$ total height ($2\text{ in}$ depth) with a $4\text{ in} \times 1.25\text{ in}$ display opening cut $0.625\text{ in}$ from the top edge.
* **Control Pods**: Two $4\text{ in} \times 2.3\text{ in}$ front modules containing a $1.0\text{ in}$ diameter hole for the push button and a $1.6\text{ in} \times 2.3\text{ in}$ cutout for the keypad.

---

## System Architecture & Software Flow

### Wiring Overview
* **Control Inputs**: Keypads route through shift registers to save digital IO pins on the Arduino Uno. Push buttons connect to digital interrupt pins.
* **Sensors**: Phototransistors connect to analog/digital input pins configured for ambient light calibration and rapid detection.
* **Outputs**: Servos connect to PWM-capable outputs. Lasers, piezo speaker, and display connect via standard digital output lines.
* **Cable Routing**: All wiring runs underneath the playing field floor into the back electrical enclosure.

### Execution Flowchart

```
                 +-------------------+
                 |    Start Match    |
                 +---------+---------+
                           |
                           v
              +-------------------------+
              | Generate Question Bank  |
              | Reset Servos to Origin  |
              +------------+------------+
                           |
                           v
              +-------------------------+
              | Display Question on Screen |
              +------------+------------+
                           |
            +--------------+--------------+
            |                             |
            v                             v
  [ Player Press Button ]       [ Player Input Answer ]
            |                             |
            v                             v
  Rotate Servo 180°             Is Answer Correct?
  Wait 1 Sec -> Turn Back       /                \
  Apply 3s Cooldown            No                 Yes
                               |                   |
                         (Allow Retry)        Fire Laser
                                                   |
                                                   v
                                          Check Phototransistors
                                                   |
                                         Did Laser Hit Sensor?
                                        /                     \
                                      No                       Yes
                                       |                        |
                                (Loop/Continue)          Add Point to Winner
                                                                |
                                                      Score Limit Reached?
                                                     /                    \
                                                   No                      Yes
                                                   |                        |
                                            (Reset Round)            Display Winner
```

---

## Assembly Instructions

1. **Laser Cutting**: Cut the enclosure walls, main playing field, and control pods out of MDF using puzzle-joint templates.
2. **Structural Framing**: Assemble the main field and rear electrical housing using superglue along interlocking joints.
3. **Control Pod Assembly**: Mount keypads and push buttons into the designated pod cutouts.
4. **Character Mounts**: Connect servo horn bases underneath the field floor. Attach the laser diode to the front face and the phototransistor to the rear face of each character structure.
5. **Electrical Wiring**:
   * Route pod wires beneath the main floor into the rear electrical compartment.
   * Connect servos, phototransistors, and lasers to the Arduino Uno per the circuit schematic.
   * Mount the LCD display and piezo speaker into the rear electrical box.
6. **Code Deployment**: Upload the target sketch to the Arduino Uno via USB, power the system using the integrated battery box, and perform sensor calibration for room lighting conditions.
