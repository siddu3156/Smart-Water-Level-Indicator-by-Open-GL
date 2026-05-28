# SMART WATER LEVEL INDICATOR USING OPENGL

A highly realistic and animated **Computer Graphics and Visualization (BCG402)** project developed using **C/C++**, **OpenGL**, and **GLUT/freeGLUT**.

This project simulates a modern smart hydroelectric dam monitoring system with real-time water level detection, automatic gate control, warning systems, realistic environmental animation, and interactive controls.

---

# PROJECT FEATURES

* Realistic smart dam simulation
* Animated flowing water with wave effects
* Automatic water level detection system
* Water level indicator meter (100–500)
* Automatic gate opening system
* Real-time alarm and warning system
* Modern service/control room
* Animated clouds and birds
* Day/Night mode
* Rain mode (optional)
* Smooth OpenGL animations
* Interactive keyboard controls
* Realistic mountains, river, and environment

---

# TITLE SCREEN

The project contains a professional animated title page with:

* Gradient animated background
* Moving clouds
* Smooth water animation
* SPACE key transition to main simulation

---

# TECHNOLOGIES USED

* C++
* OpenGL
* GLUT/freeGLUT
* Computer Graphics Concepts
* Animation and Visualization

---

# SYSTEM FUNCTIONALITY

1. User fills water in reservoir
2. Water level increases gradually
3. Sensor continuously checks water level
4. Alarm activates at danger level
5. Gates open automatically
6. Water flows downstream
7. Water level reduces to safe state

---

# USER CONTROLS

| Key   | Function         |
| ----- | ---------------- |
| SPACE | Start Simulation |
| F     | Fill Water       |
| G     | Open Gate        |
| R     | Reset System     |
| N     | Night Mode       |
| D     | Day Mode         |
| T     | Rain Mode        |
| ESC   | Exit             |

---

# OPENGL CONCEPTS USED

* glBegin() / glEnd()
* glColor3f()
* glVertex2f()
* glTranslatef()
* glRotatef()
* glPushMatrix() / glPopMatrix()
* GLUT Timer Functions
* Double Buffering
* Polygon Rendering
* Real-Time Animation

---

# PROJECT STRUCTURE

```text id="71el9h"
Smart-Water-Level-Indicator-by-Open-GL/
│
├── dam.cpp
├── dam.exe
├── README.md
└── .vscode/
```

---

# HOW TO RUN

## Windows

Compile using CodeBlocks/Dev-C++/Visual Studio with:

```text id="dj90gw"
-lopengl32
-lglu32
-lfreeglut
```

---

## Linux

```bash id="7vw1q2"
g++ dam.cpp -lGL -lGLU -lglut
./a.out
```

---

# EXPECTED OUTPUT

The final output displays:

* Realistic dam environment
* Smart water monitoring system
* Animated water flow
* Sensor-based warning system
* Automatic gate opening
* Modern control room
* Interactive real-time simulation

---

# PROJECT OBJECTIVE

The main objective of this project is to demonstrate:

* OpenGL graphics programming
* Real-time animation
* Smart monitoring systems
* Event-driven simulation
* Interactive visualization techniques

---

# SUBJECT

Computer Graphics and Visualization (BCG402)

---

# LICENSE

This project is developed for academic and educational purposes.
