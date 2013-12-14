#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <GLFW/glfw3.h>


#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define FXAA_SAMPLES 16

#define PI 3.14159265358979323846
#define DEG2RAD(x) ((x/180.0f)*PI)
#define RAD2DEG(x) ((x*180.0f)/PI)
#define DSIN(x) sin(DEG2RAD(x))
#define DCOS(x) cos(DEG2RAD(x))

#define max(x, y) (x>y?x:y)

#define TIMEDEL60 (1/(double)60)
#define FOV DEG2RAD(60.0f)

#define MOVEMENT_SPEED 4.0f     // units/sec

#define TURN_SPEED 120.0f       // degrees/sec
#define MOUSE_SENSITIVITY 0.05f

#define MAX_PROJECTILES 64
#define MAX_ENEMIES 32
#define NUM_BUILDINGS 4096

#define BUILDING_SEED 42


#define sleepytime(x) for(int i=0;i<x;i++){}    // usleep and microsleep are too finicky...
                                                // I apologize if you have a terrible computer

typedef struct {
    float x, y;
    float angle;
    bool alive;
    int obj_x, obj_y;
} Enemy;

typedef struct {
    float x, y;
    float speed;
    float angle;
    float alive_time;
    bool alive;
} Projectile;

typedef struct {
    float x, y;
    float x_, y_;
    float height;
} Building;


GLFWwindow *window;

double tv0, Tdel;

float rot_y;
float pos_x, pos_y, pos_z;
int width, height;
float ratio;
double cursor_x, cursor_y;

bool capture_cursor = true;

float movement_speed = MOVEMENT_SPEED;
float turn_speed = TURN_SPEED;
bool speed_increased = false;

Enemy enemies[MAX_ENEMIES];
Projectile projectiles[MAX_PROJECTILES];
Building buildings[NUM_BUILDINGS];
bool grid[200][200];

int numBuildings = 0;
int numProjectiles = 0;
int numEnemies = 0;

int building_seed = BUILDING_SEED;
int next_seed;

int ticks = 0;


void setup();
void render_setup();

void makeBuildings(int buildingCount);
void makeEnemy();

void step();
void moveEnemies();

void get_input();

void render();
void drawBuildings();

double getFPS();
void cleanup();

void error_callback(int error, const char *description);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);


int main(int argc, char *argv[])
{
    if ( argc > 1)
        building_seed = atoi(argv[1]);
    
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
    
    srand(building_seed);
    next_seed = rand();
    
    numBuildings = NUM_BUILDINGS;
    makeBuildings(numBuildings);

    
    rot_y = 0.0f;
    pos_x = 0.0f;
    pos_y = -8.0f;
    pos_z = 0.0f;

}

void render_setup()
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwGetFramebufferSize(window, &width, &height);
    ratio = width/(float)height;

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

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

void step()
{
    glfwPollEvents();
    get_input(window);
    
    if ( pos_x > 100 || pos_x < -100 || pos_z > 100 || pos_z < -100 )
    {
        printf("You fell off the edge and died. Good job.\n");
        building_seed = next_seed;
        next_seed = rand();
        makeBuildings(numBuildings);
        pos_x = pos_z = 0;
        pos_y = -8.0f;
    }
    
    if ( ticks%10 == 0)
        makeEnemy();
    ticks++;
    
    moveEnemies();

    char title[64];
    snprintf(title, 64, "LD28 - You only have one @ %.1f FPS", (float)getFPS());
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
    if ( glfwGetKey(window, 'R') == GLFW_PRESS ) {
        pos_x = 9001.0f;
        sleepytime(31415926);
    }

    if ( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ) {
        pos_y -= pos_y * MOVEMENT_SPEED * Tdel / 10.0f;
    }
    if ( glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ) {
        pos_y += pos_y * MOVEMENT_SPEED * Tdel / 10.0f;
    }

    if ( capture_cursor ) {
        glfwGetCursorPos(window, &cursor_x, &cursor_y);
        rot_y = -1.0f*RAD2DEG(atan2(cursor_y-height/2, cursor_x-width/2));
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

    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_LINES);
        for ( int i=-100 ; i<100 ; i++ ) {
            glVertex3f(i, 0.0f, -100.0f);
            glVertex3f(i, 0.0f,  100.0f);
        }
        for ( int j=-100 ; j<100 ; j++ ) {
            glVertex3f(-100.0f, 0.0f, j);
            glVertex3f(100.0f, 0.0f, j);
        }
    glEnd();
    
    glBegin(GL_QUADS);
    
        glColor3f(1.0f, 0.0f, 0.0f);
        for ( int i=0 ; i<numEnemies ; i++ )
        {
            float x = enemies[i].x;
            float y = enemies[i].y;

            glPushMatrix();
            glRotatef(enemies[i].angle, 0.0f, 1.0f, 0.0f);
            glVertex3f(-0.05f + x, 0.0f, -0.05f + y);
            glVertex3f(-0.05f + x, 0.0f,  0.05f + y);
            glVertex3f( 0.05f + x, 0.0f,  0.05f + y);
            glVertex3f( 0.05f + x, 0.0f, -0.05f + y);
            glPopMatrix();
        }
        
        for ( int i=0 ; i<numProjectiles ; i++ )
        {
            float x = projectiles[i].x;
            float y = projectiles[i].y;
            float d = projectiles[i].alive_time;
               
            glColor3f(1.0f, 1.0f/d, 1.0f/d);
            
            glPushMatrix();
            glRotatef(projectiles[i].angle, 0.0f, 1.0f, 0.0f);
            glVertex3f(-0.05f + x, 0.0f, -0.05f + y);
            glVertex3f(-0.05f + x, 0.0f,  0.05f + y);
            glVertex3f( 0.05f + x + d, 0.0f,  0.05f + y + d);
            glVertex3f( 0.05f + x + d, 0.0f, -0.05f + y + d);
            glPopMatrix();
            
        }
        
        drawBuildings();
    
    glEnd();
    
    glPushMatrix();
    
    glTranslatef(-pos_x, 0.0f, -pos_z);
    glPushMatrix();
    glRotatef(rot_y, 0.0f, 1.0f, 0.0f);
    
    glColor3f(0.4f, 0.6f, 1.0f);
    glBegin(GL_QUADS);
        glVertex3f(-0.05f, 0.0f, -0.05f);
        glVertex3f(-0.05f, 0.0f,  0.05f);
        glVertex3f( 0.05f, 0.0f,  0.05f);
        glVertex3f( 0.05f, 0.0f, -0.05f);
    glEnd();
    glPopMatrix();
    
    // FIX ME!!! ARRRRRRRRRRRRRRRRRRRRRRGH!!!!!!
    glTranslatef((cursor_x-width/2)/width*12.5f/ratio, 0.0f, (cursor_y-height/2)/height*12.5f/ratio);

    glPointSize(5.0f);
    glBegin(GL_POINTS);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();

    glPopMatrix();


    glPopMatrix();
    glfwSwapBuffers(window);
}

void makeBuildings(int buildingCount)
{
    for ( int i=0 ; i<200 ; i++ ) {
        for ( int j=0 ; j<200 ; j++ ) {
            grid[i][j] = false;
        }
    }
    
    for ( int i=0 ; i<buildingCount ; i++ )
    {
        Building tmp;
        tmp.x = (rand()%2000)/10.0f - 100.0f;
        tmp.y = (rand()%2000)/10.0f - 100.0f;
        //tmp.x_ = tmp.x + 1.0f;
        //tmp.y_ = tmp.y + 1.0f;
        tmp.x_ = tmp.x + 1; //rand()%2 ? (rand()%100)/-1000.0f : (rand()%100)/1000.0f;
        tmp.y_ = tmp.y + 1; //rand()%2 ? (rand()%100)/-1000.0f : (rand()%100)/1000.0f;
        tmp.height = (rand()%80)/10.0f + 0.1f;
        
        grid[(int)tmp.x_+100][(int)tmp.y_+100] = true;
        
        buildings[i] = tmp;
    }
}

void makeEnemy()
{
    int i;
    for ( i=0 ; i<numEnemies ; i++ ) {
        if ( !enemies[i].alive ) {
            int x = (int)pos_x + rand()%16 - 8;
            int y = (int)pos_y + rand()%16 - 8;
    
            enemies[i].x = x;
            enemies[i].y = y;
            enemies[i].angle = RAD2DEG(atan2(pos_y-y, pos_x-x));
            enemies[i].alive = true;
        }
    }
}

void moveEnemies()
{
    for ( int i=0 ; i<numEnemies ; i++ )
    {
        if ( enemies[i].alive )
        {
            float x = enemies[i].x;
            float y = enemies[i].y;
            float angle = enemies[i].angle;
            
            if ( (angle < 45.0f && angle > -45.0f) || (angle > 135.0f && angle < -135.0f) ) {
                //movesideways
                enemies[i].x -= (pos_x - x)/4 * Tdel;
            } else {
                //moveupdown
                enemies[i].y -= (pos_y - y)/4 * Tdel;
            }
            
            enemies[i].angle = RAD2DEG(atan2(pos_y-y, pos_x-x));
        }
    }
}

void drawBuildings()
{
    glBegin(GL_QUADS);
    for ( int i=0 ; i<numBuildings ; i++ ) {
        float x = buildings[i].x;
        float y = buildings[i].y;
        float x_ = buildings[i].x_;
        float y_ = buildings[i].y_;
        float height = buildings[i].height;
        
        glColor3f(0.5f, 0.5f, 0.5f);
        glVertex3f(x, 0.0f,  y);
        glVertex3f(x, height, y);
        glVertex3f(x, height, y_);
        glVertex3f(x, 0.0f,  y_);
    
        glVertex3f(x, 0.0f,  y_);
        glVertex3f(x, height, y_);
        glVertex3f(x_, height, y_);
        glVertex3f(x_, 0.0f,  y_);
    
        glVertex3f(x_, 0.0f,  y);
        glVertex3f(x_, height, y);
        glVertex3f(x_, height, y_);
        glVertex3f(x_, 0.0f,  y_);
    
        glVertex3f(x, 0.0f,  y);
        glVertex3f(x, height, y);
        glVertex3f(x_, height, y);
        glVertex3f(x_, 0.0f,  y);
        
        glColor3f(0.45f, 0.45f, 0.45f);
        glVertex3f(x , height,  y);
        glVertex3f(x , height, y_);
        glVertex3f(x_, height, y_);
        glVertex3f(x_, height,  y);
    }
    glEnd();
    
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    for ( int i=0 ; i<numBuildings ; i++ ) {
        float x = buildings[i].x;
        float y = buildings[i].y;
        float x_ = buildings[i].x_;
        float y_ = buildings[i].y_;
        float height = buildings[i].height;
        
        glVertex3f(x, height, y);//
        glVertex3f(x_, height, y);
        glVertex3f(x, height, y);//
        glVertex3f(x, height, y_);
        glVertex3f(x, height, y_);//
        glVertex3f(x_, height, y_);
        glVertex3f(x_, height, y);//
        glVertex3f(x_, height, y_);
        
        glVertex3f(x, 0.0f, y);//
        glVertex3f(x, height, y);
        glVertex3f(x, 0.0f, y_);//
        glVertex3f(x, height, y_);
        glVertex3f(x_, 0.0f, y_);//
        glVertex3f(x_, height, y_);
        glVertex3f(x_, 0.0f, y);//
        glVertex3f(x_, height, y);
    }
    glEnd();
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