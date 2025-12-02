**Pong Project**

**Ameen Shaikh**

**Project Overview**

For my project, I implemented a two-player version of the classic game Pong, which simulates table tennis. Each player controls a paddle and attempts to hit a moving ball; missing the ball causes the player to lose the round.

User input is handled through a Python script running on a PC. The script detects key presses and transmits them to an ESP32 microcontroller over Bluetooth. One player uses the W/S keys while the other uses the Up/Down arrow keys. The ESP32 operates as a Bluetooth server that receives these inputs and runs the game logic.

The game logic on the ESP32 is structured using FreeRTOS, where I created three separate tasks:

-	Input Task: Reads Bluetooth input and updates the paddle positions.
  
-	Game Logic Task: Updates the ball position, detects collisions, determines scoring events, and updates the     game state.
  
-	SPI Transmission Task: Sends the ball, paddle, and score data to the FPGA via SPI.
  
These tasks are timed so the game operates at 60 FPS.

The FPGA is responsible for graphics output. I implemented an SPI slave module on the FPGA to receive game-state data from the ESP32, and I designed a VGA synchronizer/renderer that draws all game objects (paddles, ball, scores) at 60 FPS on a connected monitor.

**Problems Encountered**

A couple of issues occurred during development.

One major challenge involved the FPGA’s SPI slave. My initial approach used the microcontroller-provided SCLK as an actual clock for sequential logic. Although this functioned in simulation, it caused problems in practice, including unstable behavior during register resets and general electrical reliability issues. I resolved this by switching to the FPGA’s internal system clock and using oversampling logic to correctly capture the SPI signals.

Another issue occurred in the Python input script. The library I used could not register multiple simultaneous key presses, meaning only one player could press a key at any given time. This made gameplay less responsive, and improving this behavior would require switching to a different input-handling library.

**Project Components**

The components used in this project were:

  -	ESP32 microcontroller
  
  -	Breadboard
  
  -	Three jumper wires
  
  -	BASYS-3 FPGA board
  
  -	VGA cable
  
  -	VGA-compatible monitor
  
  -	PC with Bluetooth capability
  
The ESP32 is mounted on the breadboard, and the three jumper wires connect its SPI pins to GPIO pins on the BASYS-3. The BASYS-3 outputs VGA signals directly to the monitor. The PC runs the Python script that transmits user input over Bluetooth to the ESP32.

**Similarities and Differences from Real Embedded Systems**

At a gameplay level, this project resembles many typical microcontroller Pong implementations. However, the key difference is the use of an FPGA for graphics rendering. Rather than relying on the microcontroller to generate VGA signals, I offloaded this responsibility entirely to the FPGA.

This architecture introduces significant advantages. The FPGA provides true hardware-level parallelism and precise timing control, resulting in extremely stable VGA output with very little jitter compared to microcontroller-only designs. For simple games like Pong, both approaches can work, but for more complex games, a microcontroller would struggle to maintain accurate video timing while also handling game logic.

This division of responsibilities, microcontroller for logic, FPGA for graphics, is similar to many real embedded systems that partition tasks across specialized hardware.

**Potential Improvements**

If given more time, I would first address the input-handling limitation by using a Python library capable of registering multiple simultaneous key presses. This would make gameplay significantly smoother.
I would also consider enhancing the gameplay experience by adding features such as power-ups (e.g. paddle size changes or temporary speed boosts). Finally, I would like to implement direct USB keyboard support on the FPGA so that players could plug in a keyboard and play without running the Python script on a PC.

