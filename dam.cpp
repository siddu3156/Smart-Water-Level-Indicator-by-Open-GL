#include <GL/glut.h>
#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <windows.h>
#include <mmsystem.h>

// --- Simulation State Variables ---
bool showTitleScreen = true;
float waterLevel = 180.0f;       // Min Y: 150.0f (Level 100), Max Y: 460.0f (Level 500)
bool fillingWater = false;       // Toggled by 'F' / 'f'
bool gateOpening = false;        // Controls whether gate is moving up
float gateHeight = 0.0f;         // Range: 0.0f (closed) to 70.0f (fully open)
bool alarmActive = false;        // Triggered at danger level (>400)
bool alarmMuted = false;         // Safety override toggle via 'A'/'a'
bool isNightMode = false;        // Toggled by 'N'/'D'
bool isRainMode = false;         // Toggled by 'T'/'t'
bool alarmSoundPlaying = false;  // Audio state tracker
float warningBannerAlpha = 0.0f; // Smooth fade-out of emergency text banner

// Transition system (Black cross-fade)
int transitionState = 0;         // 0: Idle, 1: Fading out title, 2: Fading in simulation
float transitionAlpha = 0.0f;     // 0.0 to 1.0

// Lightning effect states
bool lightningActive = false;
int lightningDuration = 0;

// Animation phases and offsets
float wavePhase = 0.0f;
float waterFlowOffset = 0.0f;
float cloudX[3] = { 150.0f, 600.0f, 1000.0f };
float cloudY[3] = { 680.0f, 720.0f, 650.0f };
float cloudSpeed[3] = { 0.4f, 0.2f, 0.3f };

// Blinking states
bool titleBlink = true;
bool alarmBlink = false;
float alarmGlowVal = 0.0f;
int blinkCounter = 0;

// --- Environmental Objects ---

// Rain droplet structure
struct RainDrop {
    float x, y;
    float speed;
    float length;
};
const int MAX_RAIN = 150;
RainDrop rainDrops[MAX_RAIN];

// Flying Bird structure
struct Bird {
    float x, y;
    float speed;
    float wingPhase;
    float size;
};
Bird birds[2];

// Water Splashing Particle System
struct Particle {
    float x, y;
    float vx, vy;
    float alpha;
    float size;
    bool active;
};
const int MAX_PARTICLES = 120;
Particle particles[MAX_PARTICLES];

// --- Day / Night / Rain Color Theme Helpers ---
struct Color {
    float r, g, b;
};

Color getSkyColorTop() {
    if (isRainMode) {
        return isNightMode ? Color{0.01f, 0.01f, 0.04f} : Color{0.15f, 0.16f, 0.18f};
    }
    return isNightMode ? Color{0.01f, 0.01f, 0.08f} : Color{0.15f, 0.45f, 0.85f};
}

Color getSkyColorBottom() {
    if (isRainMode) {
        return isNightMode ? Color{0.05f, 0.05f, 0.08f} : Color{0.32f, 0.34f, 0.38f};
    }
    return isNightMode ? Color{0.08f, 0.05f, 0.15f} : Color{0.6f, 0.8f, 1.0f};
}

Color getMountainColor1() {
    if (isRainMode) {
        return isNightMode ? Color{0.03f, 0.04f, 0.06f} : Color{0.15f, 0.22f, 0.18f};
    }
    return isNightMode ? Color{0.05f, 0.08f, 0.12f} : Color{0.18f, 0.38f, 0.22f};
}

Color getMountainColor2() {
    if (isRainMode) {
        return isNightMode ? Color{0.01f, 0.02f, 0.04f} : Color{0.10f, 0.16f, 0.12f};
    }
    return isNightMode ? Color{0.03f, 0.06f, 0.10f} : Color{0.12f, 0.32f, 0.16f};
}

Color getGroundColor() {
    if (isRainMode) {
        return isNightMode ? Color{0.02f, 0.04f, 0.02f} : Color{0.18f, 0.32f, 0.15f};
    }
    return isNightMode ? Color{0.04f, 0.08f, 0.04f} : Color{0.25f, 0.45f, 0.2f};
}

Color getDamColor() {
    return isNightMode ? Color{0.22f, 0.22f, 0.25f} : Color{0.52f, 0.52f, 0.54f};
}

Color getDamHighlightColor() {
    return isNightMode ? Color{0.32f, 0.32f, 0.35f} : Color{0.62f, 0.62f, 0.64f};
}

Color getDamShadowColor() {
    return isNightMode ? Color{0.12f, 0.12f, 0.15f} : Color{0.38f, 0.38f, 0.4f};
}

Color getWaterColor() {
    if (isRainMode) {
        return isNightMode ? Color{0.01f, 0.08f, 0.18f} : Color{0.05f, 0.3f, 0.5f};
    }
    return isNightMode ? Color{0.02f, 0.12f, 0.25f} : Color{0.0f, 0.4f, 0.75f};
}

Color getWaterSurfaceColor() {
    if (isRainMode) {
        return isNightMode ? Color{0.05f, 0.18f, 0.35f} : Color{0.1f, 0.5f, 0.7f};
    }
    return isNightMode ? Color{0.08f, 0.25f, 0.45f} : Color{0.0f, 0.7f, 0.95f};
}

// --- Drawing Helper Functions ---

void drawText(float x, float y, const std::string& text, void* font, float r, float g, float b, float alpha = 1.0f) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, alpha);
    glRasterPos2f(x, y);
    for (char c : text) {
        if (c == '\n') {
            y -= 22.0f; // Line spacing adjustment
            glRasterPos2f(x, y);
        } else {
            glutBitmapCharacter(font, c);
        }
    }
    glDisable(GL_BLEND);
}

void drawTextCentered(float y, const std::string& text, void* font, float r, float g, float b, float alpha = 1.0f) {
    float width = 0.0f;
    for (char c : text) {
        width += glutBitmapWidth(font, c);
    }
    float x = (1200.0f - width) / 2.0f;
    drawText(x, y, text, font, r, g, b, alpha);
}

void drawCircle(float cx, float cy, float r, int num_segments, float red, float green, float blue, float alpha = 1.0f) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(red, green, blue, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= num_segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
    glDisable(GL_BLEND);
}

void drawRect(float x1, float y1, float x2, float y2, float r, float g, float b, float alpha = 1.0f, bool fill = true) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, alpha);
    if (fill) {
        glBegin(GL_POLYGON);
    } else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
    glDisable(GL_BLEND);
}

void drawGradientRect(float x1, float y1, float x2, float y2, 
                      float r1, float g1, float b1, 
                      float r2, float g2, float b2, 
                      float a1 = 1.0f, float a2 = 1.0f, 
                      bool vertical = true) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_POLYGON);
    if (vertical) {
        glColor4f(r2, g2, b2, a2); // Bottom
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glColor4f(r1, g1, b1, a1); // Top
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
    } else {
        glColor4f(r1, g1, b1, a1); // Left
        glVertex2f(x1, y1);
        glColor4f(r2, g2, b2, a2); // Right
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glColor4f(r1, g1, b1, a1); // Left
        glVertex2f(x1, y2);
    }
    glEnd();
    glDisable(GL_BLEND);
}

// --- Dynamic Environment Objects Drawing Helpers ---

void drawBird(float x, float y, float wingPhase, float size) {
    float wingOffset = sinf(wingPhase) * size * 0.65f;
    glColor3f(0.04f, 0.04f, 0.06f); // dark bird silhouette
    glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x - size, y + wingOffset);
    glVertex2f(x, y);
    glVertex2f(x + size, y + wingOffset);
    glEnd();

    // Small head dot
    drawCircle(x, y, 2.0f, 6, 0.04f, 0.04f, 0.06f, 1.0f);
}

void drawTree(float x, float y, float height, float width) {
    // Wood Trunk (Brown)
    drawRect(x - width * 0.15f, y, x + width * 0.15f, y + height * 0.35f, 0.40f, 0.22f, 0.08f, 1.0f, true);

    // Layered Evergreen Canopy (Greens)
    float canopyBase = y + height * 0.28f;
    float canopyHeight = height * 0.72f;

    // Layer 1 (Dark Bottom)
    glBegin(GL_TRIANGLES);
    glColor3f(0.08f, 0.35f, 0.12f);
    glVertex2f(x - width * 0.5f, canopyBase);
    glVertex2f(x + width * 0.5f, canopyBase);
    glVertex2f(x, canopyBase + canopyHeight * 0.45f);

    // Layer 2 (Medium Middle)
    glColor3f(0.12f, 0.42f, 0.16f);
    glVertex2f(x - width * 0.4f, canopyBase + canopyHeight * 0.28f);
    glVertex2f(x + width * 0.4f, canopyBase + canopyHeight * 0.28f);
    glVertex2f(x, canopyBase + canopyHeight * 0.75f);

    // Layer 3 (Light Top)
    glColor3f(0.16f, 0.52f, 0.20f);
    glVertex2f(x - width * 0.3f, canopyBase + canopyHeight * 0.55f);
    glVertex2f(x + width * 0.3f, canopyBase + canopyHeight * 0.55f);
    glVertex2f(x, canopyBase + canopyHeight);
    glEnd();
}

void drawCloud(float x, float y) {
    float r = isRainMode ? 0.35f : 1.0f;
    float g = isRainMode ? 0.38f : 1.0f;
    float b = isRainMode ? 0.42f : 1.0f;
    if (isNightMode) {
        r *= 0.25f; g *= 0.25f; b *= 0.30f;
    }
    float alpha = (isNightMode || isRainMode) ? 0.65f : 0.90f;

    drawCircle(x, y, 32.0f, 12, r, g, b, alpha);
    drawCircle(x - 26.0f, y - 5.0f, 24.0f, 12, r, g, b, alpha);
    drawCircle(x + 26.0f, y - 5.0f, 24.0f, 12, r, g, b, alpha);
    drawCircle(x - 12.0f, y + 16.0f, 26.0f, 12, r, g, b, alpha);
    drawCircle(x + 12.0f, y + 16.0f, 26.0f, 12, r, g, b, alpha);
}

// --- Particle Splash Logic ---

void initParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
}

void spawnParticles() {
    if (gateHeight <= 2.0f) return;
    
    int count = 0;
    int limit = static_cast<int>(3.0f * (gateHeight / 70.0f)) + 1;
    for (int i = 0; i < MAX_PARTICLES && count < limit; i++) {
        if (!particles[i].active) {
            particles[i].active = true;
            // Splashing location at the foot of the spillway crest channel
            particles[i].x = 730.0f + (rand() % 35);
            particles[i].y = 110.0f + (rand() % 15);
            
            particles[i].vx = 1.8f + (rand() % 40) / 10.0f;
            particles[i].vy = 2.2f + (rand() % 45) / 10.0f;
            particles[i].alpha = 1.0f;
            particles[i].size = 3.0f + (rand() % 5);
            count++;
        }
    }
}

void updateParticles() {
    spawnParticles();
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vy -= 0.14f; // Gravity
            particles[i].alpha -= 0.028f; // Fade
            
            if (particles[i].alpha <= 0.0f || particles[i].y < 95.0f) {
                particles[i].active = false;
            }
        }
    }
}

void drawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            drawCircle(particles[i].x, particles[i].y, particles[i].size, 6, 1.0f, 1.0f, 1.0f, particles[i].alpha);
        }
    }
}

// --- Rain Setup and Physics ---

void initRain() {
    for (int i = 0; i < MAX_RAIN; i++) {
        rainDrops[i].x = rand() % 1200;
        rainDrops[i].y = rand() % 800;
        rainDrops[i].speed = 10.0f + (rand() % 6);
        rainDrops[i].length = 14.0f + (rand() % 8);
    }
}

void updateRain() {
    if (!isRainMode) return;
    
    for (int i = 0; i < MAX_RAIN; i++) {
        rainDrops[i].y -= rainDrops[i].speed;
        
        float collisionHeight = 100.0f;
        if (rainDrops[i].x < 570.0f) {
            collisionHeight = waterLevel; // Splashes on reservoir water
        } else if (rainDrops[i].x >= 570.0f && rainDrops[i].x < 770.0f) {
            collisionHeight = 100.0f; // Dam structure / spillway
        }
        
        if (rainDrops[i].y < collisionHeight) {
            // Reset to top
            rainDrops[i].y = 800.0f + (rand() % 40);
            rainDrops[i].x = rand() % 1200;
        }
    }
}

void drawRain() {
    if (!isRainMode) return;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glColor4f(0.7f, 0.85f, 1.0f, 0.45f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < MAX_RAIN; i++) {
        glVertex2f(rainDrops[i].x, rainDrops[i].y);
        glVertex2f(rainDrops[i].x, rainDrops[i].y - rainDrops[i].length);
    }
    glEnd();
    glDisable(GL_BLEND);
}

// --- System Resetter ---
void resetSystem() {
    waterLevel = 180.0f;
    fillingWater = false;
    gateOpening = false;
    gateHeight = 0.0f;
    alarmActive = false;
    alarmMuted = false;
    isRainMode = false;
    lightningActive = false;
    warningBannerAlpha = 0.0f;
    if (alarmSoundPlaying) {
        PlaySound(NULL, 0, 0);
        alarmSoundPlaying = false;
    }
    initParticles();
}

// --- Audio Manager ---
void updateAlarmSound() {
    if (alarmActive && !alarmMuted) {
        if (!alarmSoundPlaying) {
            PlaySound(TEXT("SystemHand"), NULL, SND_ALIAS | SND_ASYNC | SND_LOOP);
            alarmSoundPlaying = true;
        }
    } else {
        if (alarmSoundPlaying) {
            PlaySound(NULL, 0, 0);
            alarmSoundPlaying = false;
        }
    }
}

// --- Procedural Terrain Mathematics ---
float getHillHeight(float x) {
    return 100.0f + 25.0f * sinf(0.008f * x + 0.5f) + 10.0f * cosf(0.022f * x + 1.8f);
}

// --- TITLE SCREEN DRAWING ---
void drawTitleScreen();

// --- MAIN SIMULATION DRAWING FUNCTIONS ---
void drawEnvironment();
void drawDam();
void drawWater();
void drawSensor();
void drawServiceRoom();
void drawGate();
void drawHUD();

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (showTitleScreen) {
        drawTitleScreen();
    } else {
        drawEnvironment();
        drawDam();
        drawWater();
        drawSensor();
        drawServiceRoom();
        drawGate();
        drawHUD();
        drawRain(); // Overlay rain particles on top of scene
    }

    // Full-screen lightning flash overlays if active in rain mode
    if (isRainMode && lightningActive) {
        drawRect(0.0f, 0.0f, 1200.0f, 800.0f, 1.0f, 1.0f, 1.0f, 0.85f, true);
    }

    // Black screen overlay for cinematic fade transition
    if (transitionAlpha > 0.001f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, transitionAlpha);
        glBegin(GL_POLYGON);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(1200.0f, 0.0f);
        glVertex2f(1200.0f, 800.0f);
        glVertex2f(0.0f, 800.0f);
        glEnd();
        glDisable(GL_BLEND);
    }

    glutSwapBuffers();
}

// --- KEYBOARD FUNCTION CALLBACK ---
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case ' ':
            // Start fade transition
            if (showTitleScreen && transitionState == 0) {
                transitionState = 1;
            }
            break;
            
        case 'f':
        case 'F':
            if (!showTitleScreen) {
                fillingWater = !fillingWater;
            }
            break;
            
        case 'g':
        case 'G':
            if (!showTitleScreen) {
                gateOpening = !gateOpening;
            }
            break;
            
        case 'a':
        case 'A':
            if (!showTitleScreen) {
                alarmMuted = !alarmMuted; // Toggle alarm sound mute
            }
            break;
            
        case 'n':
        case 'N':
            isNightMode = true;
            break;
            
        case 'd':
        case 'D':
            isNightMode = false;
            break;

        case 't':
        case 'T':
            if (!showTitleScreen) {
                isRainMode = !isRainMode;
            }
            break;
            
        case 'r':
        case 'R':
            resetSystem();
            break;
            
        case 27: // ESC
            if (alarmSoundPlaying) {
                PlaySound(NULL, 0, 0);
            }
            exit(0);
            break;
    }
    glutPostRedisplay();
}

// --- TIMER FUNCTION CALLBACK ---
void timer(int value) {
    // 1. Cross-fade Screen transition state machine
    if (transitionState == 1) {
        transitionAlpha += 0.04f;
        if (transitionAlpha >= 1.0f) {
            transitionAlpha = 1.0f;
            showTitleScreen = false;
            transitionState = 2; // Transition screen switch over
        }
    } else if (transitionState == 2) {
        transitionAlpha -= 0.04f;
        if (transitionAlpha <= 0.0f) {
            transitionAlpha = 0.0f;
            transitionState = 0; // Completed
        }
    }

    // 2. Update wave phases
    wavePhase += 0.08f;
    if (wavePhase > 2.0f * 3.1415926f) wavePhase -= 2.0f * 3.1415926f;

    waterFlowOffset += 0.16f;
    if (waterFlowOffset > 20.0f) waterFlowOffset = 0.0f;

    // 3. Update Cloud positions
    for (int i = 0; i < 3; i++) {
        cloudX[i] += cloudSpeed[i];
        if (cloudX[i] > 1300.0f) {
            cloudX[i] = -100.0f;
            cloudY[i] = 630.0f + (rand() % 100);
        }
    }

    // 4. Update Birds horizontal flapping flight
    if (!isRainMode) {
        for (int i = 0; i < 2; i++) {
            birds[i].x += birds[i].speed;
            birds[i].wingPhase += 0.28f;
            if (birds[i].x > 1300.0f) {
                birds[i].x = -100.0f;
                birds[i].y = 540.0f + (rand() % 140);
            }
        }
    }

    // 5. Rain System particles and lightning triggers
    if (isRainMode) {
        updateRain();
        
        if (lightningActive) {
            lightningDuration--;
            if (lightningDuration <= 0) {
                lightningActive = false;
            }
        } else {
            if (rand() % 400 == 0) { // small random trigger
                lightningActive = true;
                lightningDuration = 3 + (rand() % 4); // flash for 3-6 frames
            }
        }
    }

    // 6. Blinking triggers
    blinkCounter++;
    if (blinkCounter % 20 == 0) {
        titleBlink = !titleBlink;
        alarmBlink = !alarmBlink;
    }

    if (alarmActive) {
        alarmGlowVal += 0.045f;
        if (alarmGlowVal > 1.0f) alarmGlowVal = 0.0f;
    }

    // 7. Water Inflow logic (Pumps and Storm impacts)
    float inflowRate = 0.0f;
    if (fillingWater) inflowRate += 0.65f;
    if (isRainMode) inflowRate += 0.22f; // automatic storm accumulation
    
    if (inflowRate > 0.0f && waterLevel < 460.0f) {
        waterLevel += inflowRate;
    }

    // 8. Smart sensor automatic check (Level >= 400 equivalent to Y >= 382.5f)
    if (waterLevel >= 382.5f) {
        alarmActive = true;
        gateOpening = true; // automatic release override
    } else {
        if (alarmActive) {
            alarmActive = false;
            gateOpening = false; // slide gate shut automatically when level drops below danger mark
            alarmMuted = false;  // Reset mute flag for the next danger state
        }
    }

    // 9. Audio Sound System continuous check
    updateAlarmSound();

    // 10. Smooth fade-out of emergency banner warning messages
    if (alarmActive) {
        warningBannerAlpha += 0.05f;
        if (warningBannerAlpha > 1.0f) warningBannerAlpha = 1.0f;
    } else {
        warningBannerAlpha -= 0.05f;
        if (warningBannerAlpha < 0.0f) warningBannerAlpha = 0.0f;
    }

    // 11. Sliding Gate physics
    if (gateOpening) {
        if (gateHeight < 70.0f) {
            gateHeight += 0.7f;
        }
    } else {
        if (gateHeight > 0.0f) {
            gateHeight -= 0.7f;
        }
    }

    // 12. Water Drainage physics when gate is discharging
    if (gateHeight > 1.0f) {
        float drainage = 0.85f * (gateHeight / 70.0f);
        if (waterLevel > 150.0f) {
            waterLevel -= drainage;
        }
    }

    // 13. Update water splash foam particles
    updateParticles();

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// --- INIT GL ENVIRONMENT ---
void init() {
    glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1200.0, 0.0, 800.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    initParticles();
    initRain();
    
    birds[0] = {150.0f, 620.0f, 1.6f, 0.0f, 13.0f};
    birds[1] = {650.0f, 670.0f, 1.3f, 1.8f, 10.0f};

    srand(static_cast<unsigned int>(time(NULL)));
}

// --- MAIN FUNCTION ---
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    
    glutInitWindowSize(1200, 800);
    glutInitWindowPosition(80, 40);
    glutCreateWindow("SMART WATER LEVEL INDICATOR - CGV PROJECT");

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}

// --- INCLUDE IMPLEMENTATION CODE OF DRAWING ROUTINES ---

void drawTitleScreen() {
    // 1. Sky Gradient Background (Dark Blue to Light Purple)
    drawGradientRect(0.0f, 0.0f, 1200.0f, 800.0f, 
                     0.01f, 0.06f, 0.20f, // Top
                     0.15f, 0.10f, 0.30f, // Bottom
                     1.0f, 1.0f, true);

    // 2. Stars
    srand(55);
    for (int i = 0; i < 45; i++) {
        float sx = rand() % 1200;
        float sy = 250 + (rand() % 550);
        float sz = 1.0f + (rand() % 15) / 10.0f;
        drawCircle(sx, sy, sz, 4, 1.0f, 1.0f, 1.0f, 0.6f);
    }

    // 3. Clouds
    for (int i = 0; i < 3; i++) {
        drawCloud(cloudX[i], cloudY[i] + 40.0f);
    }

    // 4. Center Minimalist Panel Board
    drawRect(220.0f, 180.0f, 980.0f, 620.0f, 0.02f, 0.04f, 0.10f, 0.85f, true); // Sleek slate panel
    drawRect(218.0f, 178.0f, 982.0f, 622.0f, 0.5f, 0.55f, 0.6f, 0.4f, false);    // Elegant border

    // 5. Clean, Centered Text Blocks (No glow, solid single colors, no separators)
    
    // Main Title: Simple, elegant white Times Roman font
    drawTextCentered(530.0f, "SMART WATER LEVEL INDICATOR", GLUT_BITMAP_TIMES_ROMAN_24, 1.0f, 1.0f, 1.0f);
    
    // Subtitle: Centered and simple light grey sans-serif
    drawTextCentered(485.0f, "Computer Graphics and Visualization (BCG402)", GLUT_BITMAP_HELVETICA_12, 0.85f, 0.85f, 0.85f);
    
    // Project Team section:
    drawTextCentered(410.0f, "DEVELOPED BY", GLUT_BITMAP_HELVETICA_12, 0.65f, 0.65f, 0.65f);
    
    // Student details (Centered, plain white, matching colors)
    drawTextCentered(365.0f, "Chinmay (4PM24CG010)", GLUT_BITMAP_HELVETICA_12, 1.0f, 1.0f, 1.0f);
    drawTextCentered(335.0f, "Nihaal N O (4PM24CG024)", GLUT_BITMAP_HELVETICA_12, 1.0f, 1.0f, 1.0f);
    drawTextCentered(305.0f, "Shankar D V (4PM24CG033)", GLUT_BITMAP_HELVETICA_12, 1.0f, 1.0f, 1.0f);
    drawTextCentered(275.0f, "Siddaling (4PM24CG039)", GLUT_BITMAP_HELVETICA_12, 1.0f, 1.0f, 1.0f);

    // 6. Water Waves at bottom of Title Screen
    glBegin(GL_QUAD_STRIP);
    for (float x = 0; x <= 1200.0f; x += 15.0f) {
        float y = 55.0f + 14.0f * sinf(0.015f * x + wavePhase);
        glColor4f(0.0f, 0.55f, 0.85f, 0.85f);
        glVertex2f(x, y);
        glColor4f(0.02f, 0.22f, 0.55f, 0.95f);
        glVertex2f(x, 0.0f);
    }
    glEnd();

    // 7. Water Reflection Animation of text strings inside the ripples
    float refOff = 3.5f * sinf(wavePhase * 1.5f);
    drawText(380.0f + refOff, 28.0f, "SMART WATER LEVEL INDICATOR", GLUT_BITMAP_HELVETICA_12, 0.0f, 0.35f, 0.60f);
    drawText(500.0f - refOff, 12.0f, "BCG402 CGV PROJECT", GLUT_BITMAP_HELVETICA_10, 0.0f, 0.25f, 0.50f);
}

void drawEnvironment() {
    Color skyT = getSkyColorTop();
    Color skyB = getSkyColorBottom();
    drawGradientRect(0.0f, 0.0f, 1200.0f, 800.0f, 
                     skyT.r, skyT.g, skyT.b, 
                     skyB.r, skyB.g, skyB.b, 
                     1.0f, 1.0f, true);

    if (isNightMode) {
        srand(202);
        for (int i = 0; i < 65; i++) {
            float sx = rand() % 1200;
            float sy = 280 + (rand() % 520);
            float sz = 1.0f + (rand() % 15) / 10.0f;
            drawCircle(sx, sy, sz, 4, 1.0f, 1.0f, 0.9f, 0.75f);
        }
        
        drawCircle(1050.0f, 680.0f, 32.0f, 20, 0.95f, 0.95f, 0.88f, 1.0f);
        drawCircle(1062.0f, 685.0f, 29.0f, 20, skyT.r, skyT.g, skyT.b, 1.0f);
    } else if (!isRainMode) {
        drawCircle(1050.0f, 680.0f, 38.0f, 24, 1.0f, 0.95f, 0.15f, 1.0f);
        glLineWidth(2.0f);
        glColor4f(1.0f, 0.90f, 0.0f, 0.55f);
        glBegin(GL_LINES);
        for (int i = 0; i < 12; i++) {
            float angle = i * (2.0f * 3.1415926f / 12.0f);
            float x1 = 1050.0f + 46.0f * cosf(angle);
            float y1 = 680.0f + 46.0f * sinf(angle);
            float x2 = 1050.0f + 68.0f * cosf(angle);
            float y2 = 680.0f + 68.0f * sinf(angle);
            glVertex2f(x1, y1);
            glVertex2f(x2, y2);
        }
        glEnd();
    }

    // Background Distant Mountains (Procedural with Haze)
    glBegin(GL_QUAD_STRIP);
    for (float x = 0.0f; x <= 1200.0f; x += 10.0f) {
        float y = 180.0f + 110.0f * sinf(0.003f * x + 1.2f) + 40.0f * cosf(0.009f * x + 0.5f) + 15.0f * sinf(0.025f * x);
        
        Color peakColor = getMountainColor2();
        float blend = (y - 100.0f) / 350.0f;
        if (blend > 1.0f) blend = 1.0f;
        if (blend < 0.0f) blend = 0.0f;
        
        glColor3f(peakColor.r * blend + skyB.r * (1.0f - blend),
                  peakColor.g * blend + skyB.g * (1.0f - blend),
                  peakColor.b * blend + skyB.b * (1.0f - blend));
        glVertex2f(x, y);
        
        glColor3f(skyB.r, skyB.g, skyB.b);
        glVertex2f(x, 100.0f);
    }
    glEnd();

    // Midground Mountains (Procedural)
    glBegin(GL_QUAD_STRIP);
    for (float x = 0.0f; x <= 1200.0f; x += 10.0f) {
        float y = 130.0f + 70.0f * sinf(0.005f * x + 3.1f) + 30.0f * cosf(0.014f * x + 2.1f) + 12.0f * sinf(0.035f * x + 0.8f);
        
        Color peakColor = getMountainColor1();
        float blend = (y - 100.0f) / 250.0f;
        if (blend > 1.0f) blend = 1.0f;
        if (blend < 0.0f) blend = 0.0f;
        
        glColor3f(peakColor.r * blend + skyB.r * (0.8f - 0.8f * blend),
                  peakColor.g * blend + skyB.g * (0.8f - 0.8f * blend),
                  peakColor.b * blend + skyB.b * (0.8f - 0.8f * blend));
        glVertex2f(x, y);
        
        glColor3f(skyB.r * 0.9f, skyB.g * 0.9f, skyB.b * 0.9f);
        glVertex2f(x, 100.0f);
    }
    glEnd();

    // Foreground Hills (Procedural)
    glBegin(GL_QUAD_STRIP);
    for (float x = 0.0f; x <= 1200.0f; x += 10.0f) {
        float y = getHillHeight(x);
        
        Color hillColor = getGroundColor();
        glColor3f(hillColor.r * 1.15f, hillColor.g * 1.15f, hillColor.b * 1.1f);
        glVertex2f(x, y);
        
        glColor3f(hillColor.r * 0.85f, hillColor.g * 0.85f, hillColor.b * 0.8f);
        glVertex2f(x, 100.0f);
    }
    glEnd();

    // Scattered Small Pine Trees on the Hills (Procedural rooting)
    drawTree(40.0f, getHillHeight(40.0f), 45.0f, 25.0f);
    drawTree(120.0f, getHillHeight(120.0f), 55.0f, 30.0f);
    drawTree(220.0f, getHillHeight(220.0f), 50.0f, 28.0f);
    drawTree(310.0f, getHillHeight(310.0f), 60.0f, 32.0f);
    drawTree(420.0f, getHillHeight(420.0f), 48.0f, 26.0f);
    drawTree(490.0f, getHillHeight(490.0f), 52.0f, 28.0f);
    
    drawTree(795.0f, getHillHeight(795.0f), 50.0f, 28.0f);
    drawTree(860.0f, getHillHeight(860.0f), 45.0f, 25.0f);
    drawTree(915.0f, getHillHeight(915.0f), 55.0f, 30.0f);

    if (!isRainMode) {
        drawBird(birds[0].x, birds[0].y, birds[0].wingPhase, birds[0].size);
        drawBird(birds[1].x, birds[1].y, birds[1].wingPhase, birds[1].size);
    }

    for (int i = 0; i < 3; i++) {
        drawCloud(cloudX[i], cloudY[i]);
    }

    Color groundC = getGroundColor();
    drawRect(0.0f, 0.0f, 1200.0f, 100.0f, groundC.r, groundC.g, groundC.b, 1.0f, true);
    
    drawRect(0.0f, 100.0f, 570.0f, 110.0f, groundC.r * 1.15f, groundC.g * 1.15f, groundC.b * 1.10f, 1.0f, true);
    drawRect(770.0f, 100.0f, 1200.0f, 110.0f, groundC.r * 1.15f, groundC.g * 1.15f, groundC.b * 1.10f, 1.0f, true);

    // Trees framing the relocated control room on Right Bank
    drawTree(935.0f, 100.0f, 95.0f, 48.0f);
    drawTree(1145.0f, 100.0f, 105.0f, 55.0f);
    drawTree(1185.0f, 100.0f, 85.0f, 45.0f);
}

void drawDam() {
    Color damMain = getDamColor();
    Color damHi = getDamHighlightColor();
    Color damSh = getDamShadowColor();

    // 1. Reservoir Retaining Wall Pillar (Left side of opening, X: 550 to 570, Y: 100 to 480)
    drawRect(550.0f, 100.0f, 570.0f, 480.0f, damMain.r, damMain.g, damMain.b, 1.0f, true);
    drawRect(550.0f, 100.0f, 553.0f, 480.0f, damHi.r, damHi.g, damHi.b, 1.0f, true); // highlights

    // 2. Curved Ogival Spillway concrete face (X: 570 to 740)
    // The spillway slopes down in a beautiful parabolic curve from (570, 340) to (740, 100).
    glBegin(GL_QUAD_STRIP);
    for (float x = 570.0f; x <= 740.0f; x += 10.0f) {
        float f = (x - 570.0f) / 170.0f; // 0.0 to 1.0
        float y = 100.0f + 240.0f * (1.0f - f * f); // parabolic curve
        
        glColor3f(damMain.r * (1.0f - 0.3f * f) + damHi.r * 0.1f,
                  damMain.g * (1.0f - 0.3f * f) + damHi.g * 0.1f,
                  damMain.b * (1.0f - 0.3f * f) + damHi.b * 0.1f);
        glVertex2f(x, y);
        
        glColor3f(damSh.r, damSh.g, damSh.b);
        glVertex2f(x, 100.0f);
    }
    glEnd();

    // Concrete layer lines on the spillway curve face
    glLineWidth(1.0f);
    glColor3f(damSh.r * 0.7f, damSh.g * 0.7f, damSh.b * 0.7f);
    glBegin(GL_LINES);
    for (float x = 580.0f; x < 740.0f; x += 30.0f) {
        float f = (x - 570.0f) / 170.0f;
        float y = 100.0f + 240.0f * (1.0f - f * f);
        glVertex2f(x, y);
        glVertex2f(x, 100.0f);
    }
    for (float y_line = 130.0f; y_line < 340.0f; y_line += 40.0f) {
        float f_val = sqrtf((340.0f - y_line) / 240.0f);
        float x_line = 570.0f + 170.0f * f_val;
        glVertex2f(570.0f, y_line);
        glVertex2f(x_line, y_line);
    }
    glEnd();

    // 3. Vertical Buttress Support Walls along the spillway slope
    float buttressXs[2] = { 640.0f, 700.0f };
    for (int i = 0; i < 2; i++) {
        float bx1 = buttressXs[i];
        float bx2 = bx1 + 15.0f;
        
        glBegin(GL_POLYGON);
        glColor3f(damMain.r * 1.1f, damMain.g * 1.1f, damMain.b * 1.1f); // light side
        glVertex2f(bx1, 100.0f);
        glVertex2f(bx1, 340.0f - 100.0f * i); // sloped top
        glVertex2f(bx2, 340.0f - 100.0f * i - 20.0f);
        glVertex2f(bx2, 100.0f);
        glEnd();
        
        glBegin(GL_POLYGON);
        glColor3f(damSh.r * 0.8f, damSh.g * 0.8f, damSh.b * 0.8f); // shadow side
        glVertex2f(bx2, 100.0f);
        glVertex2f(bx2, 340.0f - 100.0f * i - 20.0f);
        glVertex2f(bx2 + 3.0f, 340.0f - 100.0f * i - 25.0f);
        glVertex2f(bx2 + 3.0f, 100.0f);
        glEnd();
    }

    // 4. Spillway opening recess (X: 570 to 620, Y: 340 to 440)
    drawRect(570.0f, 340.0f, 620.0f, 440.0f, 0.15f, 0.15f, 0.18f, 1.0f, true);

    // 5. Gate Pillars (X: 550 to 570 and X: 620 to 640, Y: 440 to 480)
    drawRect(550.0f, 440.0f, 570.0f, 480.0f, damMain.r, damMain.g, damMain.b, 1.0f, true);
    drawRect(620.0f, 440.0f, 640.0f, 480.0f, damMain.r, damMain.g, damMain.b, 1.0f, true);
    drawRect(620.0f, 440.0f, 623.0f, 480.0f, damHi.r, damHi.g, damHi.b, 1.0f, true); // highlights

    // 6. Bridge Top Walkway (X: 550 to 650, Y: 470 to 480)
    drawRect(550.0f, 470.0f, 650.0f, 480.0f, damHi.r, damHi.g, damHi.b, 1.0f, true);

    // 7. Right Dam Mass Concrete Structure (X: 640 to 770)
    glBegin(GL_POLYGON);
    glColor3f(damMain.r, damMain.g, damMain.b);
    glVertex2f(640.0f, 480.0f);
    glVertex2f(655.0f, 480.0f);
    glVertex2f(770.0f, 100.0f);
    glVertex2f(640.0f, 100.0f);
    glEnd();
    
    glBegin(GL_POLYGON);
    glColor3f(damHi.r, damHi.g, damHi.b);
    glVertex2f(637.0f, 480.0f);
    glVertex2f(640.0f, 480.0f);
    glVertex2f(640.0f, 100.0f);
    glVertex2f(637.0f, 100.0f);
    glEnd();

    // 8. Top Walkway Railing (Handrails)
    glLineWidth(1.5f);
    glColor3f(0.85f, 0.85f, 0.9f);
    glBegin(GL_LINES);
    for (float rx = 552.0f; rx <= 648.0f; rx += 12.0f) {
        glVertex2f(rx, 480.0f);
        glVertex2f(rx, 492.0f);
    }
    glVertex2f(550.0f, 492.0f); glVertex2f(650.0f, 492.0f);
    glVertex2f(550.0f, 486.0f); glVertex2f(650.0f, 486.0f);
    glEnd();

    // 9. Streetlights/Walkway Safety Lights
    float lightXs[3] = { 560.0f, 600.0f, 640.0f };
    for (int i = 0; i < 3; i++) {
        float lx = lightXs[i];
        
        glLineWidth(2.0f);
        glColor3f(0.2f, 0.2f, 0.22f);
        glBegin(GL_LINES);
        glVertex2f(lx, 480.0f); glVertex2f(lx, 498.0f); // vertical pole
        glVertex2f(lx, 498.0f); glVertex2f(lx - 4.0f, 498.0f); // arm
        glEnd();
        
        drawCircle(lx - 4.0f, 497.0f, 2.0f, 6, 0.95f, 0.95f, 0.3f, 1.0f);
        
        if (isNightMode) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBegin(GL_TRIANGLES);
            glColor4f(1.0f, 0.95f, 0.5f, 0.35f);
            glVertex2f(lx - 4.0f, 497.0f);
            glColor4f(1.0f, 0.95f, 0.5f, 0.0f);
            glVertex2f(lx - 16.0f, 480.0f);
            glVertex2f(lx + 8.0f, 480.0f);
            glEnd();
            glDisable(GL_BLEND);
        }
    }

    // 10. Concrete joint lines for the mass concrete block on the right
    glLineWidth(1.0f);
    glColor3f(damSh.r * 0.9f, damSh.g * 0.9f, damSh.b * 0.9f);
    glBegin(GL_LINES);
    for (float h = 150.0f; h < 480.0f; h += 50.0f) {
        float x_slope = 640.0f - 130.0f * (h - 480.0f) / 380.0f;
        glVertex2f(640.0f, h);
        glVertex2f(x_slope, h);
    }
    glEnd();
}

void drawWater() {
    Color wSurf = getWaterSurfaceColor();
    Color wBase = getWaterColor();
    Color damMain = getDamColor();

    // 1. LEFT RESERVOIR WATER
    glBegin(GL_QUAD_STRIP);
    for (float x = 0.0f; x <= 570.0f; x += 10.0f) {
        float y = waterLevel + 6.5f * sinf(0.026f * x + wavePhase);
        if (y < 100.0f) y = 100.0f;
        
        glColor4f(wSurf.r, wSurf.g, wSurf.b, 0.85f);
        glVertex2f(x, y);
        
        glColor4f(wBase.r, wBase.g, wBase.b, 0.95f);
        glVertex2f(x, 100.0f);
    }
    glEnd();

    drawGradientRect(0.0f, 100.0f, 570.0f, 110.0f, wSurf.r, wSurf.g, wSurf.b, wBase.r, wBase.g, wBase.b, 0.35f, 0.0f, true);

    // 1.1 Wavy reflection of the concrete dam on the reservoir water surface
    float reflectionWidth = 60.0f;
    float refAlpha = 0.22f + 0.08f * sinf(wavePhase);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_POLYGON);
    glColor4f(damMain.r * 0.7f, damMain.g * 0.7f, damMain.b * 0.7f, refAlpha);
    glVertex2f(550.0f - reflectionWidth, waterLevel);
    glColor4f(damMain.r * 0.7f, damMain.g * 0.7f, damMain.b * 0.7f, 0.0f);
    glVertex2f(550.0f - reflectionWidth * 2.0f, waterLevel - 15.0f);
    glVertex2f(550.0f, waterLevel - 15.0f);
    glColor4f(damMain.r * 0.7f, damMain.g * 0.7f, damMain.b * 0.7f, refAlpha);
    glVertex2f(550.0f, waterLevel);
    glEnd();
    glDisable(GL_BLEND);

    // 2. SPILLWAY FLOW CASCADE (Only if gate is open) - Parabolic Curve Flow!
    if (gateHeight > 0.5f) {
        float thickness = 16.0f * (gateHeight / 70.0f);
        
        glBegin(GL_QUAD_STRIP);
        for (float f = 0.0f; f <= 1.0f; f += 0.05f) {
            float x = 570.0f + f * 170.0f;
            float y = 100.0f + 240.0f * (1.0f - f * f); // matches ogival spillway curve
            
            float wave = 2.8f * sinf(0.12f * x - waterFlowOffset);
            float yTop = y + thickness + wave;
            
            glColor4f(wSurf.r * 1.15f, wSurf.g * 1.15f, wSurf.b * 1.15f, 0.92f); // Splash shine
            glVertex2f(x, yTop);
            
            glColor4f(wBase.r, wBase.g, wBase.b, 0.96f);
            glVertex2f(x, y);
        }
        glEnd();
        
        // Foam highlight line
        glLineWidth(2.5f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.70f);
        glBegin(GL_LINE_STRIP);
        for (float f = 0.0f; f <= 1.0f; f += 0.05f) {
            float x = 570.0f + f * 170.0f;
            float y = 100.0f + 240.0f * (1.0f - f * f);
            float wave = 2.8f * sinf(0.12f * x - waterFlowOffset + 1.5f);
            glVertex2f(x, y + (8.0f * (gateHeight / 70.0f)) + wave);
        }
        glEnd();
    }

    // 3. DOWNSTREAM WATER (Right, X: 740 to 1200)
    float downstreamHeight = 110.0f + 14.0f * (gateHeight / 70.0f);
    glBegin(GL_QUAD_STRIP);
    for (float x = 740.0f; x <= 1200.0f; x += 15.0f) {
        float waveVal = 3.0f * sinf(0.03f * x + wavePhase * 1.6f);
        if (gateHeight > 5.0f) {
            waveVal = (4.0f + (rand() % 4)) * sinf(0.05f * x + wavePhase * 2.2f); // Turbulent
        }
        float y = downstreamHeight + waveVal;
        
        glColor4f(wSurf.r, wSurf.g, wSurf.b, 0.85f);
        glVertex2f(x, y);
        glColor4f(wBase.r, wBase.g, wBase.b, 0.95f);
        glVertex2f(x, 100.0f);
    }
    glEnd();

    drawParticles();
}

void drawServiceRoom() {
    Color damSh = getDamShadowColor();
    
    // Platform Foundation
    drawRect(970.0f, 100.0f, 1130.0f, 115.0f, damSh.r, damSh.g, damSh.b, 1.0f, true);
    drawRect(970.0f, 100.0f, 1130.0f, 115.0f, 0.1f, 0.1f, 0.1f, 1.0f, false);
    
    // pathway / road from dam walkway to station foundation
    drawRect(770.0f, 100.0f, 970.0f, 110.0f, damSh.r * 0.9f, damSh.g * 0.9f, damSh.b * 0.9f, 1.0f, true);
    glLineWidth(1.0f);
    glColor3f(0.3f, 0.3f, 0.35f);
    glBegin(GL_LINES);
    for (float rx = 775.0f; rx < 970.0f; rx += 25.0f) {
        glVertex2f(rx, 110.0f); glVertex2f(rx, 120.0f);
    }
    glVertex2f(770.0f, 120.0f); glVertex2f(970.0f, 120.0f);
    glEnd();

    // Main Control Room Structure (Modern architecture)
    drawRect(980.0f, 115.0f, 1120.0f, 185.0f, 0.82f, 0.82f, 0.85f, 1.0f, true);
    drawRect(980.0f, 115.0f, 1120.0f, 185.0f, 0.2f, 0.2f, 0.25f, 1.0f, false);

    // Cantilevered Roof
    glBegin(GL_POLYGON);
    glColor3f(0.20f, 0.20f, 0.24f);
    glVertex2f(970.0f, 185.0f);
    glVertex2f(1130.0f, 185.0f);
    glVertex2f(1120.0f, 200.0f);
    glVertex2f(980.0f, 200.0f);
    glEnd();

    // Secure Metal Door
    drawRect(990.0f, 115.0f, 1015.0f, 155.0f, 0.35f, 0.2f, 0.1f, 1.0f, true);
    drawCircle(1011.0f, 135.0f, 1.5f, 6, 0.9f, 0.9f, 0.1f, 1.0f); // knob
    
    // Large Window with internal terminal screens glowing
    drawRect(1030.0f, 130.0f, 1110.0f, 165.0f, 0.1f, 0.15f, 0.2f, 1.0f, true); // panoramic glass pane
    if (isNightMode) {
        // Glowing screens inside
        drawRect(1040.0f, 135.0f, 1070.0f, 155.0f, 0.0f, 0.8f, 0.8f, 0.8f, true); // cyan screen
        drawRect(1075.0f, 135.0f, 1100.0f, 155.0f, 0.0f, 0.9f, 0.2f, 0.8f, true); // green radar screen
    } else {
        // Shine/Reflection on glass during the day
        glBegin(GL_POLYGON);
        glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
        glVertex2f(1030.0f, 130.0f);
        glVertex2f(1060.0f, 130.0f);
        glVertex2f(1080.0f, 165.0f);
        glVertex2f(1050.0f, 165.0f);
        glEnd();
    }
    glColor3f(0.2f, 0.2f, 0.25f);
    drawRect(1030.0f, 130.0f, 1110.0f, 165.0f, 0.2f, 0.2f, 0.25f, 1.0f, false); // frame
    glBegin(GL_LINES);
    glVertex2f(1070.0f, 130.0f); glVertex2f(1070.0f, 165.0f);
    glEnd();

    // Outdoor street lamp above door
    drawCircle(1002.5f, 165.0f, 3.0f, 8, 0.9f, 0.9f, 0.9f, 1.0f);
    drawRect(1001.0f, 165.0f, 1004.0f, 168.0f, 0.1f, 0.1f, 0.1f, 1.0f, true);
    if (isNightMode) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.95f, 0.7f, 0.25f); // bright yellow spotlight cone
        glVertex2f(1002.5f, 165.0f);
        glColor4f(1.0f, 0.95f, 0.7f, 0.0f); // fade out at bottom bank
        glVertex2f(970.0f, 115.0f);
        glVertex2f(1035.0f, 115.0f);
        glEnd();
        glDisable(GL_BLEND);
    }

    // High-tech satellite communication antenna on roof
    glLineWidth(2.0f);
    glColor3f(0.3f, 0.3f, 0.35f);
    glBegin(GL_LINES);
    glVertex2f(1105.0f, 200.0f); glVertex2f(1105.0f, 215.0f); // pole
    glEnd();
    // Dish curve
    glBegin(GL_LINE_STRIP);
    for (float a = 0.0f; a <= 3.14159f; a += 0.3f) {
        glVertex2f(1105.0f + 6.0f * cosf(a), 215.0f + 4.0f * sinf(a));
    }
    glEnd();

    // External warning indicator leds on building wall
    drawRect(1020.0f, 120.0f, 1026.0f, 140.0f, 0.1f, 0.1f, 0.12f, 1.0f, true);
    drawCircle(1023.0f, 135.0f, 1.2f, 6, 0.2f, 1.0f, 0.2f, 1.0f); // green normal
    drawCircle(1023.0f, 130.0f, 1.2f, 6, 1.0f, 0.9f, 0.1f, 1.0f); // yellow warning
    drawCircle(1023.0f, 125.0f, 1.2f, 6, 1.0f, 0.1f, 0.1f, alarmActive ? (alarmBlink ? 1.0f : 0.3f) : 0.1f); // red emergency blink

    // Alarm Siren Beacon on Roof
    drawRect(1043.0f, 200.0f, 1057.0f, 205.0f, 0.3f, 0.3f, 0.32f, 1.0f, true); // beacon base
    drawCircle(1050.0f, 209.0f, 4.5f, 12, 0.85f, 0.15f, 0.15f, 1.0f); // red bulb

    if (alarmActive) {
        float glowRadius = 15.0f + 20.0f * alarmGlowVal;
        drawCircle(1050.0f, 209.0f, glowRadius, 16, 1.0f, 0.1f, 0.1f, 0.35f * (1.0f - alarmGlowVal)); // rotating glow pulse
        
        glLineWidth(1.5f);
        glColor4f(1.0f, 0.1f, 0.1f, 0.55f * (1.0f - alarmGlowVal));
        glBegin(GL_LINES);
        for (int i = 0; i < 8; i++) {
            float angle = i * (2.0f * 3.1415926f / 8.0f) + (wavePhase * 0.45f);
            glVertex2f(1050.0f, 209.0f);
            glVertex2f(1050.0f + glowRadius * 1.6f * cosf(angle), 209.0f + glowRadius * 1.6f * sinf(angle));
        }
        glEnd();
    }
}

void drawGate() {
    float currentGateY1 = 340.0f + gateHeight;
    float currentGateY2 = 410.0f + gateHeight;

    drawGradientRect(570.0f, currentGateY1, 620.0f, currentGateY2,
                     0.38f, 0.42f, 0.48f, // Top slate steel
                     0.18f, 0.22f, 0.28f, // Bottom
                     1.0f, 1.0f, true);
    
    glLineWidth(2.0f);
    glColor3f(0.1f, 0.12f, 0.16f);
    glBegin(GL_LINES);
    for (float gy = currentGateY1 + 12.0f; gy < currentGateY2; gy += 16.0f) {
        glVertex2f(570.0f, gy);
        glVertex2f(620.0f, gy);
    }
    glEnd();

    for (float gy = currentGateY1 + 4.0f; gy < currentGateY2; gy += 12.0f) {
        drawCircle(572.5f, gy, 1.2f, 6, 0.08f, 0.08f, 0.1f, 0.8f);
        drawCircle(617.5f, gy, 1.2f, 6, 0.08f, 0.08f, 0.1f, 0.8f);
    }

    drawRect(566.0f, 340.0f, 570.0f, 470.0f, 0.1f, 0.1f, 0.12f, 0.9f, true);
    drawRect(620.0f, 340.0f, 624.0f, 470.0f, 0.1f, 0.1f, 0.12f, 0.9f, true);

    glLineWidth(1.5f);
    glColor3f(0.12f, 0.12f, 0.14f);
    glBegin(GL_LINES);
    glVertex2f(568.0f, 470.0f); glVertex2f(570.0f, currentGateY2);
    glVertex2f(622.0f, 470.0f); glVertex2f(620.0f, currentGateY2);
    glEnd();

    drawCircle(568.0f, 470.0f, 4.5f, 12, 0.28f, 0.28f, 0.3f, 1.0f);
    drawCircle(622.0f, 470.0f, 4.5f, 12, 0.28f, 0.28f, 0.3f, 1.0f);
}

void drawHUD() {
    // 1. Control Center Dashboard (Top-Left, X: 30 to 450, Y: 540 to 770) - 230px height
    drawRect(30.0f, 540.0f, 450.0f, 770.0f, 0.03f, 0.05f, 0.12f, 0.78f, true); 
    drawRect(28.0f, 538.0f, 452.0f, 772.0f, 0.0f, 0.85f, 0.85f, 0.85f, false);    // Cyan border
    
    drawText(50.0f, 740.0f, "SMART WATER MONITORING HUB", GLUT_BITMAP_HELVETICA_18, 0.0f, 1.0f, 1.0f);
    drawText(50.0f, 725.0f, "--------------------------------------------------------", GLUT_BITMAP_HELVETICA_12, 0.0f, 0.85f, 0.85f);

    int meterVal = static_cast<int>(100.0f + (waterLevel - 150.0f) * 400.0f / 310.0f);
    if (meterVal < 100) meterVal = 100;
    if (meterVal > 500) meterVal = 500;
    
    int levelPercent = static_cast<int>((waterLevel - 150.0f) * 100.0f / 310.0f);
    if (levelPercent < 0) levelPercent = 0;
    if (levelPercent > 100) levelPercent = 100;
    
    std::string levelStr = "Water level: " + std::to_string(meterVal) + " / 500 units   (" + std::to_string(levelPercent) + " %)";
    
    std::string fillingStr = "Water Inflow: ";
    if (isRainMode) {
        fillingStr += fillingWater ? "STORMWATER + PUMPS (MAX)" : "RAIN STORMWATER RUNOFF";
    } else {
        fillingStr += fillingWater ? "FILLING INFLOW (PUMPS ON)" : "IDLE (NATURAL POOL)";
    }
    
    std::string gateStr = "Release Gates: ";
    if (gateHeight >= 68.0f) gateStr += "AUTOMATIC SAFETY DISCHARGE";
    else if (gateHeight <= 1.0f) gateStr += "CLOSED / RETRACTED";
    else gateStr += "OPENING DISCHARGE (" + std::to_string(static_cast<int>(gateHeight*100.0f/70.0f)) + "%)";

    std::string alarmStr = "Safety Alarm: ";
    if (alarmMuted) alarmStr += "MUTED (SAFETY OVERRIDE)";
    else alarmStr += alarmActive ? "ACTIVE (HIGH WATER RELEASE)" : "SYSTEM SECURE";

    std::string modeStr = "Operation Mode: ";
    if (meterVal >= 400) {
        modeStr += "AUTOMATIC DISCHARGE RELEASE";
    } else {
        modeStr += "MANUAL CONTROL MODE";
    }

    std::string weatherStr = "Weather System: ";
    weatherStr += isRainMode ? "HEAVY STORM (RAIN ON)" : "CLEAR SKIES";
    if (isNightMode) weatherStr += " - NIGHT TIME";
    else weatherStr += " - DAY TIME";

    drawText(50.0f, 695.0f, levelStr, GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(50.0f, 670.0f, fillingStr, GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(50.0f, 645.0f, gateStr, GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(50.0f, 620.0f, alarmStr, GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(50.0f, 595.0f, modeStr, GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(50.0f, 570.0f, weatherStr, GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);

    // Labels for LED Bulbs
    drawText(315.0f, 685.0f, "SAFE", GLUT_BITMAP_HELVETICA_10, 0.2f, 0.9f, 0.2f);
    drawText(315.0f, 655.0f, "WARNING", GLUT_BITMAP_HELVETICA_10, 0.9f, 0.85f, 0.1f);
    drawText(315.0f, 625.0f, "DANGER", GLUT_BITMAP_HELVETICA_10, 1.0f, 0.2f, 0.2f);

    // 2. Keyboard Control Guide Panel (Top-Right, X: 820 to 1170, Y: 540 to 770) - 230px height
    drawRect(820.0f, 540.0f, 1170.0f, 770.0f, 0.03f, 0.05f, 0.12f, 0.78f, true);
    drawRect(818.0f, 538.0f, 1172.0f, 772.0f, 0.0f, 0.85f, 0.85f, 0.85f, false);
    
    drawText(840.0f, 740.0f, "SYSTEM INTERFACE KEYS", GLUT_BITMAP_HELVETICA_18, 0.0f, 1.0f, 1.0f);
    drawText(840.0f, 725.0f, "--------------------------------------------------------", GLUT_BITMAP_HELVETICA_12, 0.0f, 0.85f, 0.85f);

    drawText(840.0f, 695.0f, "F / f     - Toggle Water Inflow (Fill)", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(840.0f, 675.0f, "G / g     - Toggle Spillway Gates Manually", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(840.0f, 655.0f, "A / a     - Toggle Mute Safety Alarm Siren", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(840.0f, 635.0f, "D / d     - Switch to Day Theme", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(840.0f, 615.0f, "N / n     - Switch to Night Theme (Glows)", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(840.0f, 595.0f, "T / t     - Toggle Heavy Storm (Rain Mode)", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(840.0f, 575.0f, "R / r     - Reset System Simulation State", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText(840.0f, 555.0f, "ESC       - Exit Simulation Program", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);

    // 3. Smooth fading Warning emergency banner
    if (warningBannerAlpha > 0.001f) {
        float blinkFactor = alarmBlink ? 1.0f : 0.6f;
        float currentAlpha = warningBannerAlpha * blinkFactor;
        
        drawRect(400.0f, 495.0f, 800.0f, 545.0f, 0.25f, 0.0f, 0.0f, currentAlpha * 0.92f, true);
        drawRect(398.0f, 493.0f, 802.0f, 547.0f, 1.0f, 0.1f, 0.1f, currentAlpha, false);
        drawText(415.0f, 513.0f, "EMERGENCY: WATER EXCEEDS 400! AUTOMATIC RELEASE!", GLUT_BITMAP_HELVETICA_12, 1.0f, 0.25f, 0.25f, currentAlpha);
    }
}

void drawSensor() {
    // 1. Vertical Measuring scale marked 100 to 500
    // Bar coordinates X: 538 to 546, Y: 150 to 460
    drawRect(538.0f, 150.0f, 546.0f, 460.0f, 0.12f, 0.12f, 0.14f, 0.85f, true);
    drawRect(538.0f, 150.0f, 546.0f, 460.0f, 0.65f, 0.65f, 0.65f, 0.85f, false);

    // Color Danger Zone (Level 400 to 500 -> Y: 382.5 to 460) with red background glow
    drawRect(539.0f, 382.5f, 545.0f, 460.0f, 0.8f, 0.1f, 0.1f, 0.35f, true);

    // Tick lines drawing programmatically
    glColor4f(0.95f, 0.95f, 0.95f, 0.75f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int lvl = 100; lvl <= 500; lvl += 10) {
        float y = 150.0f + (lvl - 100.0f) * 310.0f / 400.0f;
        float tickWidth = (lvl % 50 == 0) ? 6.0f : 3.0f;
        glVertex2f(538.0f, y);
        glVertex2f(538.0f + tickWidth, y);
    }
    glEnd();

    // Scale numeric labels
    drawText(508.0f, 146.0f, "100", GLUT_BITMAP_HELVETICA_10, 0.9f, 0.9f, 0.9f);
    drawText(508.0f, 223.5f, "200", GLUT_BITMAP_HELVETICA_10, 0.9f, 0.9f, 0.9f);
    drawText(508.0f, 301.0f, "300", GLUT_BITMAP_HELVETICA_10, 0.9f, 0.9f, 0.9f);
    drawText(508.0f, 378.5f, "400", GLUT_BITMAP_HELVETICA_10, 1.0f, 0.4f, 0.4f); // danger marker start
    drawText(508.0f, 456.0f, "500", GLUT_BITMAP_HELVETICA_10, 1.0f, 0.2f, 0.2f);

    // Floating pointer indicator at waterLevel Y
    float pointerY = waterLevel;
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.15f, 0.15f);
    glVertex2f(536.0f, pointerY);
    glVertex2f(525.0f, pointerY + 5.0f);
    glVertex2f(525.0f, pointerY - 5.0f);
    glEnd();

    drawRect(495.0f, pointerY - 7.0f, 525.0f, pointerY + 7.0f, 0.1f, 0.1f, 0.12f, 0.85f, true);
    
    // Numeric meter reading
    int meterVal = static_cast<int>(100.0f + (waterLevel - 150.0f) * 400.0f / 310.0f);
    if (meterVal < 100) meterVal = 100;
    if (meterVal > 500) meterVal = 500;
    
    std::string meterStr = std::to_string(meterVal);
    drawText(498.0f, pointerY - 4.0f, meterStr, GLUT_BITMAP_HELVETICA_10, 1.0f, 1.0f, 1.0f);

    // 2. Realistic Industrial Water Level Sensor Device
    // Location: physically attached to the right of the scale at Y ≈ 382.5f (danger line)
    // Sensor Box housing: X: 546 to 562, Y: 374 to 391
    drawGradientRect(546.0f, 374.0f, 562.0f, 391.0f,
                     0.35f, 0.35f, 0.38f, // top steel
                     0.20f, 0.20f, 0.22f, // bottom metal
                     1.0f, 1.0f, true);
    drawRect(546.0f, 374.0f, 562.0f, 391.0f, 0.1f, 0.1f, 0.12f, 1.0f, false); // border

    // Top metal highlight
    glLineWidth(1.0f);
    glColor3f(0.6f, 0.6f, 0.65f);
    glBegin(GL_LINES);
    glVertex2f(547.0f, 390.0f); glVertex2f(561.0f, 390.0f);
    glEnd();

    // Sensor probe / pointer head extending left to the 400 danger mark
    drawRect(540.0f, 381.5f, 546.0f, 383.5f, 0.5f, 0.5f, 0.52f, 1.0f, true); // probe metal arm
    drawCircle(539.0f, 382.5f, 1.5f, 6, 0.85f, 0.65f, 0.1f, 1.0f); // gold contact tip

    // Warning Light connection conduit (armored wire curving down)
    glLineWidth(2.0f);
    glColor3f(0.08f, 0.08f, 0.1f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(558.0f, 374.0f);
    glVertex2f(562.0f, 362.0f);
    glVertex2f(568.0f, 350.0f);
    glVertex2f(570.0f, 342.0f);
    glEnd();

    // LED Indicator Light on the Sensor Box (glows Green normally, blinks Red when high)
    if (meterVal >= 400) {
        if (alarmBlink) {
            drawCircle(554.0f, 382.5f, 3.5f, 12, 1.0f, 0.15f, 0.15f, 1.0f); // bright red led
            drawCircle(554.0f, 382.5f, 7.0f, 12, 1.0f, 0.15f, 0.15f, 0.45f); // red glow aura
        } else {
            drawCircle(554.0f, 382.5f, 3.5f, 12, 0.4f, 0.0f, 0.0f, 1.0f); // dim red led
        }
    } else {
        drawCircle(554.0f, 382.5f, 3.5f, 12, 0.2f, 1.0f, 0.2f, 1.0f); // bright green
        drawCircle(554.0f, 382.5f, 6.0f, 12, 0.2f, 1.0f, 0.2f, 0.3f); // green aura
    }

    // 3. LED Bulbs Panel drawing inside HUD (Left dashboard)
    bool isLow = (meterVal < 280);
    bool isMed = (meterVal >= 280 && meterVal < 400);
    bool isHigh = (meterVal >= 400);

    float ledX = 390.0f;
    
    // Green (Safe)
    drawCircle(ledX, 690.0f, 8.0f, 16, 0.1f, 0.28f, 0.1f, 1.0f);
    if (isLow) {
        drawCircle(ledX, 690.0f, 6.0f, 16, 0.2f, 1.0f, 0.2f, 1.0f);
        drawCircle(ledX, 690.0f, 10.0f, 16, 0.2f, 1.0f, 0.2f, 0.35f);
    }
    
    // Yellow (Warning)
    drawCircle(ledX, 660.0f, 8.0f, 16, 0.28f, 0.28f, 0.1f, 1.0f);
    if (isMed) {
        drawCircle(ledX, 660.0f, 6.0f, 16, 1.0f, 0.92f, 0.1f, 1.0f);
        drawCircle(ledX, 660.0f, 10.0f, 16, 1.0f, 0.92f, 0.1f, 0.35f);
    }

    // Red (Danger)
    drawCircle(ledX, 630.0f, 8.0f, 16, 0.28f, 0.1f, 0.1f, 1.0f);
    if (isHigh) {
        if (alarmBlink) {
            drawCircle(ledX, 630.0f, 6.0f, 16, 1.0f, 0.15f, 0.15f, 1.0f);
            drawCircle(ledX, 630.0f, 12.0f, 16, 1.0f, 0.15f, 0.15f, 0.45f);
        } else {
            drawCircle(ledX, 630.0f, 6.0f, 16, 0.65f, 0.1f, 0.1f, 1.0f);
        }
    }
}
