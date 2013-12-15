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
#define min(x, y) (x<y?x:y)

#define TIMEDEL60 (1/(double)60)
#define FOV DEG2RAD(60.0f)

#define MOVEMENT_SPEED 4.0f     // units/sec

#define TURN_SPEED 120.0f       // degrees/sec
#define MOUSE_SENSITIVITY 0.05f

#define MAX_PROJECTILES 4096
#define MAX_ENEMIES 1024
#define NUM_BUILDINGS 2048

#define PROJECTILE_SPEED 8.0f
#define ENEMY_SPEED 1.0f

#define BUILDING_SEED 42


#define sleepytime(x) for(int i=0;i<x;i++){}    // usleep and microsleep are too finicky...
                                                // I apologize if you have a terrible computer

typedef struct {
    float x, y;
    float angle;
    int direction;
    bool alive;
} Enemy;

typedef struct {
    float x, y;
    float angle;
    bool alive;
    float alive_time;
} Projectile;

typedef struct {
    float x, y;
    float x_, y_;
    float height;
} Building;

typedef struct {
    float x, y;
    float score;
} Objective;


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
int currentProjectile = 0;

Objective objective;
float score = 20.0f;

float enemy_speed = ENEMY_SPEED;
float initial_enemy_speed = ENEMY_SPEED;

int building_seed = BUILDING_SEED;
int next_seed;

int ticks = 0;


void setup();
void render_setup();

void makeBuildings(int buildingCount);
void makeEnemies();
void addProjectile(float x, float y);

void step();
void moveEnemies();
void moveProjectiles();

void get_input();

void render();
void drawBuildings();

void DIE(char *message);

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
    
    /*numBuildings = 4;
    buildings[0].x = buildings[0].y = 2;
    buildings[0].x_ = buildings[0].y_ = 3;
    buildings[0].height = 8;
    
    grid[100+2][100+2] = true;

    buildings[1].x = buildings[1].y = -2;
    buildings[1].x_ = buildings[1].y_ = -1;
    buildings[1].height = 8;
    
    grid[100-2][100-2] = true;
    
    buildings[2].x = -2;
    buildings[2].y = 2;
    buildings[2].x_ = -1;
    buildings[2].y_ = 3;
    buildings[2].height = 8;
    
    grid[100-2][100+2] = true;
    
    buildings[3].x = 2;
    buildings[3].y = -2;
    buildings[3].x_ = 3;
    buildings[3].y_ = -1;
    buildings[3].height = 8;
    
    grid[100+2][100-2] = true;*/
    
    do {
        objective.x = rand()%200-100;
        objective.y = rand()%200-100;
        objective.score = objective.x + objective.y;
    } while ( grid[(int)objective.x][(int)objective.y] );
    
    printf("Objective: x=%.2f, y=%.2f\n", objective.x, objective.y);
    
    numEnemies = 1;
    for ( int i=0 ; i<MAX_ENEMIES ; i++ )
    {
        enemies[i].alive = false;
    }
    
    
    rot_y = 0.0f;
    pos_x = 0.0f;
    pos_y = 8.0f;
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
        DIE("You fell off the edge and died. That was clever.");
    }
    
    if ( ticks%5 == 0)
        makeEnemies();
    if ( ticks%250 == 0 )
        enemy_speed++;
    
    ticks++;
    
    moveEnemies();
    moveProjectiles();
    
    if ( fabs(pos_x - objective.x + 5) < 5.0f && fabs(pos_y - objective.y + 5) < 5.0f )
    {
        DIE("You got to the objective. What an extraordinarily adequate performance.");
        score += objective.score;
        initial_enemy_speed *= 1.5f;
        enemy_speed = initial_enemy_speed;
    }

    char title[256];
    snprintf(title, 64, "LD28 - You only have one @ %.1f FPS", (float)getFPS());
    glfwSetWindowTitle(window, title);
}

void get_input()
{
    if ( glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if ( glfwGetKey(window, 'W') == GLFW_PRESS ) {
        if ( !grid[(int)(pos_x+100)][(int)(pos_z-0.15f+100)] )
            pos_z -= MOVEMENT_SPEED * Tdel;
    }
    if ( glfwGetKey(window, 'S') == GLFW_PRESS ) {
        if ( !grid[(int)(pos_x+100)][(int)ceil(pos_z+0.15f+100-1)] )
            pos_z += MOVEMENT_SPEED * Tdel;
    }
    if ( glfwGetKey(window, 'A') == GLFW_PRESS ) {
        if ( !grid[(int)(pos_x-0.15f+100)][(int)(pos_z+100)] )
            pos_x -= MOVEMENT_SPEED * Tdel;
    }
    if ( glfwGetKey(window, 'D') == GLFW_PRESS ) {
        if ( !grid[(int)ceil(pos_x+0.15f+100-1)][(int)(pos_z+100)] )
            pos_x += MOVEMENT_SPEED * Tdel;
    }
    if ( glfwGetKey(window, 'E') == GLFW_PRESS ) {
        capture_cursor = !capture_cursor;
        sleepytime(31415926);
    }
    if ( glfwGetKey(window, 'R') == GLFW_PRESS ) {
        DIE("Creating new level.");
        sleepytime(31415926);
    }
    
    if ( glfwGetKey(window, 'Q') == GLFW_PRESS ) {
        printf("x=%.2f, y=%.2f, z=%.2f\n", pos_x, pos_y, pos_z);
    }
    
    if ( glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ) {
        if ( ticks%5 == 0 )
            addProjectile(pos_x+cosf(DEG2RAD(-rot_y))/4, pos_z+sinf(DEG2RAD(-rot_y))/4);
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
    glTranslatef(-pos_x, -pos_y, -pos_z);
    
    
    /*glLineWidth(1.0f);
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

    glEnd();*/

    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(objective.x, 1.0f, objective.y);
        glVertex3f(objective.x+10, 1.0f, objective.y+10);
        glVertex3f(objective.x+10, 1.0f, objective.y+10);
        glVertex3f(objective.x+10, 1.0f, objective.y+10);
    glEnd();

    glColor3f(0.8f, 0.8f, 0.8f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
        glColor3f(5.0f/max(pos_y, 5), 5.0f/max(pos_y, 5), 5.0f/max(pos_y, 5));
        for ( int i=-100 ; i<100 ; i++ ) {
            glVertex3f(i, -0.1f, -100.0f);
            glVertex3f(i, -0.1f,  100.0f);
        }
        for ( int j=-100 ; j<100 ; j++ ) {
            glVertex3f(-100.0f, -0.1f, j);
            glVertex3f(100.0f, -0.1f, j);
        }
    glEnd();
    
    glBegin(GL_QUADS);
    
        glColor3f(1.0f, 0.0f, 0.0f);
        for ( int i=0 ; i<MAX_ENEMIES ; i++ )
        {
            if ( enemies[i].alive )
            {
                float x = enemies[i].x;
                float y = enemies[i].y;

                glPushMatrix();
                glRotatef(enemies[i].angle, 0.0f, 1.0f, 0.0f);
                glVertex3f(-0.075f + x, 0.0f, -0.075f + y);
                glVertex3f(-0.075f + x, 0.0f,  0.075f + y);
                glVertex3f( 0.075f + x, 0.0f,  0.075f + y);
                glVertex3f( 0.075f + x, 0.0f, -0.075f + y);
                glPopMatrix();
            }
        }
    glEnd();
    
    glPointSize(2.0f);
    glBegin(GL_POINTS);
        for ( int i=0 ; i<numProjectiles ; i++ )
        {
            if ( projectiles[i].alive )
            {
                float x = projectiles[i].x;
                float y = projectiles[i].y;
                float d = projectiles[i].alive_time;
               
                glColor3f(1.0f, 1.0f/d, 1.0f/d);
                glVertex3f(x, 0.0f, y);
            }
            
        }
    glEnd();
    
    glBegin(GL_QUADS);
        drawBuildings();
    glEnd();
    
    glPushMatrix();
    
    glTranslatef(pos_x, 0.0f, pos_z);
    glPushMatrix();
    glRotatef(rot_y, 0.0f, 1.0f, 0.0f);
    
    glColor3f(0.4f, 0.6f, 1.0f);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
    glBegin(GL_QUADS);
        glVertex3f(-0.075f, 0.0f, -0.075f);
        glVertex3f(-0.075f, 0.0f,  0.075f);
        glVertex3f( 0.075f, 0.0f,  0.075f);
        glVertex3f( 0.075f, 0.0f, -0.075f);
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
        tmp.x = rand()%200 - 100;
        tmp.y = rand()%200 - 100;
        
        if ( tmp.x < 5 && tmp.x > -5 && tmp.y < 5 && tmp.y > -5)
            continue;
        
        int size = rand()%190/10+1;
        int ds = 1;

        if ( size > 10 )
            ds = 2;
        if ( size > 15 )
            ds = 4;
        
        tmp.height = size;
        tmp.x_ = (tmp.x + ds) > 100 ? 100 : (tmp.x + ds);
        tmp.y_ = (tmp.y + ds) > 100 ? 100 : (tmp.y + ds);
        
        for ( int i=floor(tmp.x) ; i<floor(tmp.x_) ; i++ ) {
            for ( int j=floor(tmp.y) ; j<floor(tmp.y_) ; j++ ) {
                grid[i+100][j+100] = true;
            }
        }
        
        buildings[i] = tmp;
    }
}

void makeEnemies()
{
    int i;
    for ( i=0 ; i<MAX_ENEMIES ; i++ )
    {
        if ( !enemies[i].alive )
        {
            int x = (int)pos_x + (rand()%6 + 4) * -1*((rand()%2)*2-1);
            int y = (int)pos_z + (rand()%6 + 4) * -1*((rand()%2)*2-1);

            enemies[i].x = x;
            enemies[i].y = y;
            enemies[i].direction = rand()%4;
            enemies[i].alive = true;

            break;
        }
    }
}

void addProjectile(float x, float y)
{
    Projectile tmp;
    tmp.x = x;
    tmp.y = y;
    tmp.angle = -rot_y;
    tmp.alive = true;
    tmp.alive_time = 0.0f;
    
    projectiles[currentProjectile] = tmp;
    currentProjectile = (currentProjectile+1) % MAX_PROJECTILES;
    numProjectiles = min(numProjectiles+1, MAX_PROJECTILES);
}

void moveProjectiles()
{
    for ( int i=0 ; i<numProjectiles ; i++ )
    {
        if ( projectiles[i].alive )
        {
            projectiles[i].x += cosf(DEG2RAD(projectiles[i].angle)) * PROJECTILE_SPEED * Tdel;
            projectiles[i].y += sinf(DEG2RAD(projectiles[i].angle)) * PROJECTILE_SPEED * Tdel;
            
            int tmpx, tmpy;

            tmpx = (int)(projectiles[i].x+100);
            tmpy = (int)(projectiles[i].y+100);
            
            if ( grid[tmpx][tmpy] )
            {
                projectiles[i].alive = false;
            }
            if ( fabs(projectiles[i].x - pos_x) < 0.075f && fabs(projectiles[i].y - pos_z) < 0.075f )
            {
                DIE("You walked right into that bullet. Well done. I bet your parents are proud.");
                break;
            }
            
            for ( int j=0 ; j<MAX_ENEMIES ; j++ )
            {
                if ( enemies[j].alive )
                {
                    if ( fabs(projectiles[i].x - enemies[j].x) < 0.1f && fabs(projectiles[i].y - enemies[j].y) < 0.1f ) {
                        enemies[j].alive = false;
                        break;
                    }
                }
            }
            
            projectiles[i].alive_time += Tdel;
        }
    }
}

void moveEnemies()
{
    for ( int i=0 ; i<MAX_ENEMIES ; i++ )
    {
        if ( enemies[i].alive )
        {
            switch ( enemies[i].direction ) {
                case 0:
                    enemies[i].x += enemy_speed * Tdel;
                    break;
                case 1:
                    enemies[i].y += enemy_speed * Tdel;
                    break;
                case 2:
                    enemies[i].x -= enemy_speed * Tdel;
                    break;
                default:
                    enemies[i].y -= enemy_speed * Tdel;
                    break;
            }
            
            if ( fabs(enemies[i].x - pos_x) > 32 || fabs(enemies[i].y - pos_z) > 32 ) {
                enemies[i].alive = false;
                break;
            }
            if ( fabs(enemies[i].x) > 98 || fabs(enemies[i].y) > 98 ) {
                enemies[i].alive = false;
                break;
            }
            if ( grid[(int)enemies[i].x+100][(int)enemies[i].y+100] ) {
                enemies[i].alive = false;
                break;
            }
            
            if ( fabs(enemies[i].x - pos_x) < 0.1f && fabs(enemies[i].y - pos_z) < 0.1f )
            {
                DIE("You just walked into that enemy. He gave you a big hug. Now you are dead.");
            }
        }
    }
}

void DIE(char *message)
{
    printf("%s\n", message);
    building_seed = next_seed;
    next_seed = rand();
    makeBuildings(numBuildings);
    pos_x = pos_z = 0;
    pos_y = 8.0f;
    
    for ( int i=0 ; i<MAX_PROJECTILES ; i++ )
        projectiles[i].alive = false;
    for ( int i=0 ; i<MAX_ENEMIES ; i++ )
        enemies[i].alive = false;
    
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
        glVertex3f(x, -0.1f,  y);
        glVertex3f(x, height, y);
        glVertex3f(x, height, y_);
        glVertex3f(x, -0.1f,  y_);
    
        glVertex3f(x, -0.1f,  y_);
        glVertex3f(x, height, y_);
        glVertex3f(x_, height, y_);
        glVertex3f(x_, -0.1f,  y_);
    
        glVertex3f(x_, -0.1f,  y);
        glVertex3f(x_, height, y);
        glVertex3f(x_, height, y_);
        glVertex3f(x_, -0.1f,  y_);
    
        glVertex3f(x, -0.1f,  y);
        glVertex3f(x, height, y);
        glVertex3f(x_, height, y);
        glVertex3f(x_, -0.1f,  y);
        
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
        
        glVertex3f(x, -0.1f, y);//
        glVertex3f(x, height, y);
        glVertex3f(x, -0.1f, y_);//
        glVertex3f(x, height, y_);
        glVertex3f(x_, -0.1f, y_);//
        glVertex3f(x_, height, y_);
        glVertex3f(x_, -0.1f, y);//
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