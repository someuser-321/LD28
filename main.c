#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <GLFW/glfw3.h>

#include "game.h"


#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define FXAA_SAMPLES 16

#define PI 3.14159265358979323846
#define DEG2RAD(x) ((x/180.0f)*PI)
#define RAD2DEG(x) ((x*180.0f)/PI)
#define DSIN(x) sin(DEG2RAD(x))
#define DCOS(x) cos(DEG2RAD(x))

#define TIMEDEL60 (1/(double)60)
#define FOV DEG2RAD(60.0f)

#define MOVEMENT_SPEED 4.0f     // units/sec

#define TURN_SPEED 120.0f       // degrees/sec
#define MOUSE_SENSITIVITY 0.05f


#define sleepytime(x) for(int i=0;i<x;i++){}


GLFWwindow *window;

double tv0, Tdel;

float rot_y;
float pos_x, pos_y, pos_z;

bool capture_cursor = true;

float movement_speed = MOVEMENT_SPEED;
float turn_speed = TURN_SPEED;
bool speed_increased = false;


void setup();
void render_setup();

void step();
void get_input();
void render();

double getFPS();
void cleanup();

void error_callback(int error, const char *description);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);


int main(int argc, char *argv[])
{
    glfwSetErrorCallback(error_callback);

    if ( !glfwInit() )
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_SAMPLES, FXAA_SAMPLES);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "LD28 - You only have one", NULL, NULL);
    if ( !window )
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    setup();
    render_setup();


    while ( !glfwWindowShouldClose(window) )
    {
        step();
        render();
    }


    cleanup();
    exit(EXIT_SUCCESS);
}

void setup()
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    glfwSetCursorPos(window, width/2, height/2);

    tv0 = glfwGetTime();

    rot_y = 0.0f;
    pos_x = 0.0f;
    pos_y = -8.0f;
    pos_z = 0.0f;
}

void render_setup()
{
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float ratio = width/(float)height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float near = 0.01;
    float far = 1000.0f;

    float top = tanf(FOV*0.5) * near;
    float bottom = -1*top;

    float left = ratio * bottom;
    float right = ratio * top;

    glFrustum(left, right, bottom, top, near, far);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

void step()
{
    glfwPollEvents();
    get_input(window);

    gametick();

    char title[64];
    sprintf(title, "LD28 - You only have one @ %.1f FPS", (float)getFPS());
    glfwSetWindowTitle(window, title);
}

void get_input()
{
    if ( glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if ( glfwGetKey(window, 'W') == GLFW_PRESS ) {
        pos_z += MOVEMENT_SPEED * Tdel;
    }
    if ( glfwGetKey(window, 'S') == GLFW_PRESS ) {
        pos_z -= MOVEMENT_SPEED * Tdel;
    }
    if ( glfwGetKey(window, 'A') == GLFW_PRESS ) {
        pos_x += MOVEMENT_SPEED * Tdel;
    }
    if ( glfwGetKey(window, 'D') == GLFW_PRESS ) {
        pos_x -= MOVEMENT_SPEED * Tdel;
    }
    if ( glfwGetKey(window, 'E') == GLFW_PRESS ) {
        capture_cursor = !capture_cursor;
        sleepytime(31415926);
    }

    if ( glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ) {
        shoot();
        pos_y -= MOVEMENT_SPEED * Tdel;
    }

    if ( capture_cursor ) {
        double x_pos, y_pos;
        glfwGetCursorPos(window, &x_pos, &y_pos);
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        rot_y = -1.0f*RAD2DEG(atan2(y_pos-height/2, x_pos-width/2));

        //glfwSetCursorPos(window, width/2, height/2);
    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glPushMatrix();    

    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glTranslatef(pos_x, pos_y, pos_z);

    glLineWidth(1.0f);
    glBegin(GL_LINES);

        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(1.0f, 0.0f, 0.0f);

        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.0f);

    glEnd();
    
    glBegin(GL_QUADS);
    
        glColor3f(0.8f, 0.8f, 0.8f);
        glVertex3f(-100.0f, -0.001f, -100.0f);
        glVertex3f(-100.0f, -0.001f,  100.0f);
        glVertex3f( 100.0f, -0.001f,  100.0f);
        glVertex3f( 100.0f, -0.001f, -100.0f);
    
    glEnd();
    
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    
        glColor3f(0.0f, 0.0f, 0.0f);
        for ( int i=-100 ; i<100 ; i++ ) {
            glVertex3f(i, 0.0f, -100.0f);
            glVertex3f(i, 0.0f,  100.0f);
        }
        for ( int j=-100 ; j<100 ; j++ ) {
            glVertex3f(-100.0f, 0.0f, j);
            glVertex3f(100.0f, 0.0f, j);
        }
    
    glEnd();
    
    glPushMatrix();
    
    glTranslatef(-pos_x, 0.0f, -pos_z);
    glRotatef(rot_y, 0.0f, 1.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    
        glColor3f(0.1f, 0.1f, 1.0f);
        glVertex3f(0.0f, 0.001f,  0.25f);
        glVertex3f(0.0f, 0.001f, -0.25f);
        glVertex3f(0.5f, 0.001f,  0.0f);
    
    glEnd();

    glPopMatrix();
    
    
    glPopMatrix();
    glfwSwapBuffers(window);
}

void cleanup()
{
    glfwTerminate();
}

void error_callback(int error, const char *description)
{
    fputs(description, stderr);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    render_setup();
}

double getFPS()
{
    double tv1 = glfwGetTime();
    Tdel = tv1 - tv0;
    tv0 = tv1;

    return 1/Tdel;
}