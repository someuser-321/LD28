#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <GLFW/glfw3.h>


#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define FXAA_SAMPLES 4

#define PI (3.14159265358979323846)
#define DEG2RAD(x) ((x/180.0f)*PI)
#define DSIN(x) (sin(DEG2RAD(x)))
#define DCOS(x) (cos(DEG2RAD(x)))

#define TIMEDEL60 (1/(double)60)
#define FOV DEG2RAD(60.0f)

#define MOVEMENT_SPEED 2.0f     // units/sec
#define MOVEMENT_MULTIPLIER 8

#define TURN_SPEED 120.0f       // degrees/sec
#define MOUSE_SENSITIVITY 0.05f


#define sleepytime(x) for(int i=0;i<x;i++){}


GLFWwindow *window;

double tv0, Tdel;

float rot_x, rot_y;
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

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "This is a window title...", NULL, NULL);
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    tv0 = glfwGetTime();

    rot_x = 30.0f;
    rot_y = 30.0f;
    pos_x = 0.0f;
    pos_y = 2.0f;
    pos_z = 4.0f;
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

    char title[64];
    sprintf(title, "This is a window title... @ %.1f FPS", (float)getFPS());
    glfwSetWindowTitle(window, title);
}

void get_input()
{
    if ( glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if ( glfwGetKey(window, 'W') == GLFW_PRESS ) {
        pos_x += DSIN(rot_y) * movement_speed * Tdel * DCOS(rot_x);
        pos_z -= DCOS(rot_y) * movement_speed * Tdel * DCOS(rot_x);
        pos_y -= DSIN(rot_x) * movement_speed * Tdel;
    }
    if ( glfwGetKey(window, 'S') == GLFW_PRESS ) {
        pos_x -= DSIN(rot_y) * movement_speed * Tdel * DCOS(rot_x);
        pos_z += DCOS(rot_y) * movement_speed * Tdel * DCOS(rot_x);
        pos_y += DSIN(rot_x) * movement_speed * Tdel;
    }
    if ( glfwGetKey(window, 'A') == GLFW_PRESS ) {
        pos_x -= DCOS(rot_y) * movement_speed * Tdel;
        pos_z -= DSIN(rot_y) * movement_speed * Tdel;
    }
    if ( glfwGetKey(window, 'D') == GLFW_PRESS ) {
        pos_x += DCOS(rot_y) * movement_speed * Tdel;
        pos_z += DSIN(rot_y) * movement_speed * Tdel;
    }
    if ( glfwGetKey(window, 'E') == GLFW_PRESS ) {
        if ( capture_cursor ) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            capture_cursor = false;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            capture_cursor = true;
        }

        sleepytime(31415926);
    }

    if ( glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ) {
        if ( rot_x > -90.0f )
            rot_x -= turn_speed * Tdel;
    }
    if ( glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ) {
        rot_y -= turn_speed * Tdel;
    }
    if ( glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ) {
        if ( rot_x < 90.0f )
            rot_x += turn_speed * Tdel;
    }
    if ( glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ) {
        rot_y += turn_speed * Tdel;
    }
    if ( glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ) {
        pos_y += movement_speed * Tdel;
    }
    if ( glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ) {
        pos_y -= movement_speed * Tdel;
    }
    if ( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && !speed_increased ) {
        movement_speed *= MOVEMENT_MULTIPLIER;
        speed_increased = true;
    } else if ( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE && speed_increased ) {
        movement_speed  = MOVEMENT_SPEED;
        speed_increased = false;
    }

    if ( capture_cursor ) {
        double x_pos, y_pos;
        glfwGetCursorPos(window, &x_pos, &y_pos);
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        rot_y += (x_pos - width/2) * MOUSE_SENSITIVITY;
        float rot_x_ = (y_pos - height/2) * MOUSE_SENSITIVITY;

        if ( rot_x + rot_x_ < 90.0f && rot_x + rot_x_ > -90.0f )
            rot_x += rot_x_;

        glfwSetCursorPos(window, width/2, height/2);
    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glPushMatrix();

    glRotatef(rot_x, 1.0f, 0.0f, 0.0f);
    glRotatef(rot_y, 0.0f, 1.0f, 0.0f);
    glTranslatef(-1.0f*pos_x, -1.0f*pos_y, -1.0f*pos_z);

    glBegin(GL_LINES);

        glColor3f( 1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(1.0f, 0.0f, 0.0f);

        glColor3f( 0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glColor3f( 0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.0f);

    glEnd();

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