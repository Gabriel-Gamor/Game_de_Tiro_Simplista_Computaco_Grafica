
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#define M_PI 3.14159265358979323846
#include "stb_image.h"

#define FPS INT_MAX
#define noZoomSensibility 0.03f
#define zoomSensibility 0.02f

int rightClick = 0;
float timeRemaining = 30.0f;
int currentLevel = 1;
int misses = 0;
float playerX = -10.0f;
float playerZ = 0.0f;
const int width = 16 * 100;
const int height = 9 * 100;
float FOV = 85.0f;

void display();
void reshapeScreen(int w, int h);
void timer(int);
void draw();
void drawHUD();

bool paused = 0;
float camX = 0.0f;
float camY = 0.0f;
int last_mouse_x = -1;
int last_mouse_y = -1;
int targetCount = 0;
bool buffer[256];

typedef struct rgb
{
  float r, g, b;
} rgb;

typedef struct
{
  float x, y, z;
} Vec3;

typedef struct target
{
  rgb color;
  Vec3 position;
  float radius;
  struct target *next;
  struct target *prev;
} target;

target *firsttarget = NULL;

Vec3 vec3_sub(Vec3 a, Vec3 b)
{
  return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

float vec3_dot(Vec3 a, Vec3 b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_normalize(Vec3 v)
{
  float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  return (Vec3){v.x / length, v.y / length, v.z / length};
}

bool ray_intersects_sphere(Vec3 origin, Vec3 dir, target *targ, float radius)
{
  radius = radius + 0.1f;
  Vec3 target = targ->position;
  Vec3 L = vec3_sub(target, origin);
  float tca = vec3_dot(L, dir);
  float d2 = vec3_dot(L, L) - tca * tca;
  if (d2 <= radius * radius)
  {
    if (targ->prev != NULL)
    {
      targ->prev->next = targ->next;
    }
    else
    {
      firsttarget = targ->next;
    }
    if (targ->next != NULL)
    {
      targ->next->prev = targ->prev;
    }
    free(targ);
    targetCount--;
    return true;
  }
  return false;
}

void mouseMotion(int x, int y)
{
  if (paused)
  {
    glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
    return;
  }
  float sensibility = rightClick ? zoomSensibility : noZoomSensibility;

  glutSetCursor(GLUT_CURSOR_NONE);
  if (last_mouse_x >= 0 && last_mouse_y >= 0)
  {
    int dx = x - width / 2;
    int dy = y - height / 2;

    camX += dx * sensibility;
    camY += dy * sensibility;

    if (camY > 89.9f)
      camY = 89.9f;
    if (camY < -89.9f)
      camY = -89.9f;
  }

  last_mouse_x = x;
  last_mouse_y = y;
  glutWarpPointer(width / 2, height / 2);
}

void mouse(int button, int state, int x, int y)
{
  int intersected = 0;
  mouseMotion(x, y);

  if (state == GLUT_DOWN && button == GLUT_RIGHT_BUTTON)
  {
    rightClick = !rightClick;
    FOV = rightClick ? 30.0f : 85.0f;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, 16.0 / 9.0, 1, 75);
    glMatrixMode(GL_MODELVIEW);
  }

  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
  {
    Vec3 ray_origin = {playerX, 0.0f, playerZ};
    Vec3 ray_direction = {
        cos(camX * M_PI / 180.0f) * cos(-camY * M_PI / 180.0f),
        sin(-camY * M_PI / 180.0f),
        sin(camX * M_PI / 180.0f) * cos(-camY * M_PI / 180.0f)};
    ray_direction = vec3_normalize(ray_direction);

    target *current = firsttarget;
    while (current != NULL)
    {
      if (ray_intersects_sphere(ray_origin, ray_direction, current, current->radius))
      {
        intersected = 1;
      }
      current = current->next;
    }
    if (!intersected)
    {
      misses++;
    }
  }
}

void setupLighting()
{
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  GLfloat light0_pos[] = {0.0f, 0.0f, 0.0f, 1.0f};
  GLfloat light0_ambient[] = {0.35f, 0.35f, 0.35f, 1.0f};
  GLfloat light0_diffuse[] = {0.8f, 0.8f, 0.75f, 1.0f};
  GLfloat light0_specular[] = {0.5f, 0.5f, 0.4f, 1.0f};

  glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.9f);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.002f);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0002f);
}
int texWidth, texHeight, texChannels;
int texWidth2, texHeight2, texChannels2;
int texWidth3, texHeight3, texChannels3;
unsigned char *brickWall, *sodaCan, *woodTexturePattern;
void init()
{
  woodTexturePattern = stbi_load("wood.png", &texWidth3, &texHeight3, &texChannels3, 0);
  brickWall = stbi_load("brickwall.jpg", &texWidth, &texHeight, &texChannels, 0);
  sodaCan = stbi_load("sodacan.png", &texWidth2, &texHeight2, &texChannels2, 0);

  setupLighting();
  glEnable(GL_NORMALIZE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_POLYGON_SMOOTH);
  glShadeModel(GL_SMOOTH);

  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

  glEnable(GL_LIGHTING);

  glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
  glEnable(GL_DEPTH_TEST);
}

void keyboard(unsigned char key, int x, int y)
{
  buffer[key] = true;
  switch (key)
  {
  case 27:
    paused = !paused;
    break;
  }
  glutPostRedisplay();
}

void keyboardUp(unsigned char key, int x, int y)
{
  buffer[key] = false;
}

void spawnTargets(int numTargets)
{
  firsttarget = NULL;

  target *previousTargets = NULL;

  for (int i = 0; i < numTargets; i++)
  {
    target *newTarget = (target *)malloc(sizeof(target));
    if (newTarget != NULL)
    {
      int isBaloon = rand() % 2;
      newTarget->position.x = isBaloon ? (float)(rand() % 25) + 5.0f : 30.0f;

      newTarget->position.y = isBaloon ? (float)(rand() % 6) - 1.0f : -1.0f;

      newTarget->position.z = (float)(rand() % 26) - 5.0f;

      newTarget->radius = isBaloon ? 1.0f : 0.3f;
      newTarget->color.r = (float)rand() / (float)(RAND_MAX) * 0.9f;
      newTarget->color.g = (float)rand() / (float)(RAND_MAX) * 0.9f;
      newTarget->color.b = (float)rand() / (float)(RAND_MAX) * 0.9f;
      newTarget->next = NULL;
      newTarget->prev = previousTargets;

      if (previousTargets != NULL)
      {
        previousTargets->next = newTarget;
      }
      else
      {
        firsttarget = newTarget;
      }
      targetCount++;
      previousTargets = newTarget;
    }
  }
}

int main(int argc, char **argv)
{
  srand(time(NULL));

  spawnTargets(5);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(width, height);
  glutCreateWindow("Tiro ao Alvo");

  init();
  glutPassiveMotionFunc(mouseMotion);
  glutMotionFunc(mouseMotion);
  glutMouseFunc(mouse);
  glutDisplayFunc(display);
  glutReshapeFunc(reshapeScreen);
  glutTimerFunc(0, timer, 0);
  glutKeyboardFunc(keyboard);
  glutKeyboardUpFunc(keyboardUp);
  glutMainLoop();
  return 0;
}

void draw()
{
  glCullFace(GL_BACK);
  glPushMatrix();
  glDisable(GL_LIGHTING);

  glTranslatef(-10.0f, 5.0f, 18.0f);
  GLUquadric *lightSphere = gluNewQuadric();
  glLineWidth(5.0f);
  glBegin(GL_LINES);

  glColor3f(0.0, 0.0, 0.0);
  glVertex3f(0.0f, 0.0f, 0.0f);
  glVertex3f(0.0f, 30.0f, 0.0f);
  glEnd();
  glPushMatrix();
  glColor3f(0.0f, 0.0f, 0.0f);
  glTranslatef(0.0f, 0.4f, 0.0f);
  gluSphere(lightSphere, 0.3f, 10, 10);
  glPopMatrix();
  glColor3f(1.0f, 1.0f, 0.9f);
  gluSphere(lightSphere, 0.4f, 10, 10);
  gluDeleteQuadric(lightSphere);
  glEnable(GL_LIGHTING);
  glPopMatrix();
  glCullFace(GL_FRONT);

  GLfloat material_specular[] = {0.9f, 0.9f, 0.9f, 1};
  GLfloat material_shininess[] = {128.0f};

  glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, material_shininess);

  glEnable(GL_TEXTURE_2D);
  glColor3f(0.8, 0.8, 0.8);

  GLuint texture;
  glGenTextures(1, &texture);

  unsigned char texture_data[2][2][4] =
      {
          205, 133, 63, 255, 244, 164, 96, 255,
          244, 164, 96, 255, 205, 133, 63, 255};

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glScalef(1.0f, 2.0f, 2.0f);
  glMatrixMode(GL_MODELVIEW);

  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(-40.0, -5.0, -40.0);
  glTexCoord2f(20.0, 0.0);
  glVertex3f(40.0, -5.0, -40.0);
  glTexCoord2f(20.0, 25.0);
  glVertex3f(40.0, -5.0, 40.0);
  glTexCoord2f(0.0, 25.0);
  glVertex3f(-40.0, -5.0, 40.0);

  glEnd();

  glCullFace(GL_BACK);
  glPushMatrix();

  GLfloat material_specular2[] = {0.0f, 0.0f, 0.0f, 0.0f};
  GLfloat material_shininess2[] = {128.0f};
  glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular2);
  glMaterialfv(GL_FRONT, GL_SHININESS, material_shininess2);

  glEnable(GL_TEXTURE_2D);

  GLuint wallTexture;

  glGenTextures(1, &wallTexture);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glScalef(0.2f, 0.2f, 0.2f);
  glMatrixMode(GL_MODELVIEW);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, brickWall);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(35.0f, -5.0f, -10.0f);
  glTexCoord2f(0.0, 5.0);
  glVertex3f(35.0f, 25.0f, -10.0f);
  glTexCoord2f(5.0, 5.0);
  glVertex3f(-25.0f, 25.0f, -10.0f);
  glTexCoord2f(5.0, 0.0);
  glVertex3f(-25.0f, -5.0f, -10.0f);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(35.0f, -5.0f, 25.0f);
  glTexCoord2f(0.0, 5.0);
  glVertex3f(35.0f, 25.0f, 25.0f);
  glTexCoord2f(5.0, 5.0);
  glVertex3f(35.0f, 25.0f, -25.0f);
  glTexCoord2f(5.0, 0.0);
  glVertex3f(35.0f, -5.0f, -25.0f);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(-25.0f, -5.0f, 25.0f);
  glTexCoord2f(0.0, 5.0);
  glVertex3f(-25.0f, 25.0f, 25.0f);
  glTexCoord2f(5.0, 5.0);
  glVertex3f(35.0f, 25.0f, 25.0f);
  glTexCoord2f(5.0, 0.0);
  glVertex3f(35.0f, -5.0f, 25.0f);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(-15.0f, -5.0f, -25.0f);
  glTexCoord2f(0.0, 5.0);
  glVertex3f(-15.0f, 25.0f, -25.0f);
  glTexCoord2f(5.0, 5.0);
  glVertex3f(-15.0f, 25.0f, 25.0f);
  glTexCoord2f(5.0, 0.0);
  glVertex3f(-15.0f, -5.0f, 25.0f);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(-25.0f, 8.0f, -25.0f);
  glTexCoord2f(0.0, 5.0);
  glVertex3f(35.0f, 8.0f, -25.0f);
  glTexCoord2f(5.0, 5.0);
  glVertex3f(35.0f, 8.0f, 25.0f);
  glTexCoord2f(5.0, 0.0);
  glVertex3f(-25.0f, 8.0f, 25.0f);

  glEnd();

  glTranslatef(54.0f, -1.3f, 0.0f);

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_2D);

  GLuint woodTexture;
  glGenTextures(1, &woodTexture);

  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glScalef(10.0f, 30.0f, 10.0f);
  glMatrixMode(GL_MODELVIEW);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth3, texHeight3, 0, GL_RGB, GL_UNSIGNED_BYTE, woodTexturePattern);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  glVertex3f(-25.0f, .0f, -10.0f);
  glTexCoord2f(0.0, 1.0);
  glVertex3f(-25.0f, .0f, 25.0f);
  glTexCoord2f(1.0, 1.0);
  glVertex3f(-15.0f, .0f, 25.0f);
  glTexCoord2f(1.0, 0.0);
  glVertex3f(-15.0f, .0f, -10.0f);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(-56.0f, -25.0f, -10.0f);
  glTexCoord2f(0.0, 1.0);
  glVertex3f(-56.0f, -25.0f, 25.0f);
  glTexCoord2f(1.0, 1.0);
  glVertex3f(-56.0f, .0f, 25.0f);
  glTexCoord2f(1.0, 0.0);
  glVertex3f(-56.0f, .0f, -10.0f);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(-56.0f, .0f, -10.0f);
  glTexCoord2f(0.0, 1.0);
  glVertex3f(-56.0f, .0f, 25.0f);
  glTexCoord2f(1.0, 1.0);
  glVertex3f(-54.0f, .0f, 25.0f);
  glTexCoord2f(1.0, 0.0);
  glVertex3f(-54.0f, .0f, -10.0f);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(-25.0f, -25.0f, -10.0f);
  glTexCoord2f(0.0, 1.0);
  glVertex3f(-25.0f, -25.0f, 25.0f);
  glTexCoord2f(1.0, 1.0);
  glVertex3f(-25.0f, .0f, 25.0f);
  glTexCoord2f(1.0, 0.0);
  glVertex3f(-25.0f, .0f, -10.0f);

  glEnd();
  glPopMatrix();
  glCullFace(GL_FRONT);
  glColor3f(1.0, 1.0, 0.0);

  GLfloat material_specular3[] = {0.9f, 0.9f, 0.9f, 0.9f};
  GLfloat material_shininess3[] = {100.0f};
  glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular3);
  glMaterialfv(GL_FRONT, GL_SHININESS, material_shininess3);

  GLUquadric *quad = gluNewQuadric();

  GLuint sodaCanTexture;
  glGenTextures(1, &sodaCanTexture);
  glBindTexture(GL_TEXTURE_2D, sodaCanTexture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth2, texHeight2, 0, GL_RGBA, GL_UNSIGNED_BYTE, sodaCan);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  glTranslatef(0.2f, 0.1f, 0.0f);
  glScalef(1.0f, -1.0f, 1.0f);
  glMatrixMode(GL_MODELVIEW);

  target *current = firsttarget;
  while (current != NULL)
  {
    glColor3f(current->color.r, current->color.g, current->color.b);
    glPushMatrix();

    if (current->radius == 0.3f)
    {

      glTranslatef(current->position.x, current->position.y, current->position.z);
      glTranslatef(0.0f, -current->radius, 0.0f);
      glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
      glCullFace(GL_BACK);

      glEnable(GL_TEXTURE_2D);

      gluQuadricTexture(quad, GL_TRUE);

      gluCylinder(quad, 0.25f, 0.25f, 0.75f, 10, 10);

      glDisable(GL_TEXTURE_2D);
      glCullFace(GL_FRONT);
    }
    if (current->radius == 1.0f)
    {
      glTranslatef(current->position.x, current->position.y, current->position.z);
      glScalef(0.9f, 1.15f, 0.9f);
      glCullFace(GL_BACK);
      glRotatef(90.0f, 0.0f, -1.0f, 0.0f);
      gluSphere(quad, current->radius, 32, 32);
      glLineWidth(2.0f);
      glBegin(GL_LINES);

      glVertex3f(0, 0, 0);
      glVertex3f(0, -30.0f, 0);

      glEnd();
      glCullFace(GL_FRONT);
    }
    glPopMatrix();
    current = current->next;
  }

  gluDeleteQuadric(quad);

  drawHUD();
  glFlush();
}
void drawHUD()
{
  void *font = GLUT_BITMAP_TIMES_ROMAN_24;
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glColor3f(1.0, 1.0, 1.0);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glDisable(GL_DEPTH_TEST);

  timeRemaining < 5.0f ? glColor3f(1.0, 0.0, 0.0) : glColor3f(1.0, 1.0, 1.0);

  glRasterPos2f((width / 2) - 100, height - 40.0f);
  char text[50];

  snprintf(text, sizeof(text), "Tempo Restante: %.1f", timeRemaining);

  for (const char *c = text; *c != '\0'; c++)
  {
    glutBitmapCharacter(font, *c);
  }

  glColor3f(1.0, 1.0, 1.0);

  glRasterPos2f(10.0f, height - 40.0f);

  snprintf(text, sizeof(text), "Nivel: %d", currentLevel);
  for (const char *c = text; *c != '\0'; c++)
  {
    glutBitmapCharacter(font, *c);
  }

  glRasterPos2f(1500.0f, height - 40.0f);

  snprintf(text, sizeof(text), "Erros: %d", misses);
  for (const char *c = text; *c != '\0'; c++)
  {
    glutBitmapCharacter(font, *c);
  }

  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glDisable(GL_DEPTH_TEST);
  glLoadIdentity();
  glTranslatef(00.0f, 00.0f, -50.0f);

  float scale = 1.0f;

  scale = (rightClick == 1) ? 0.4f : 1.0f;
  glLineWidth(3.0f);

  glBegin(GL_LINES);
  glVertex3f(-0.9f * scale, 0.0f, 0.0f);
  glVertex3f(-0.4f * scale, 0.0f, 0.0f);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(0.0f, -0.9f * scale, 0.0f);
  glVertex3f(0.0f, -0.4f * scale, 0.0f);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(0.4f * scale, 0.0f, 0.0f);
  glVertex3f(0.9f * scale, 0.0f, 0.0f);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(0.0f, 0.4f * scale, 0.0f);
  glVertex3f(0.0f, 0.9f * scale, 0.0f);
  glEnd();
  glLineWidth(1.0f);

  glEnable(GL_LIGHTING);

  glPopMatrix();

  glLoadIdentity();
  glEnable(GL_DEPTH_TEST);
}
void handleMovement()
{
  if (paused)
  {
    return;
  }

  float directionWS = 0.0f;
  float directionAD = 0.0f;

  if (buffer['w'])
  {
    directionWS += 1.0f;
  }
  if (buffer['s'])
  {
    directionWS -= 1.0f;
  }
  if (buffer['d'])
  {
    directionAD += 1.0f;
  }
  if (buffer['a'])
  {
    directionAD -= 1.0f;
  }
  static int lastTime = 0;
  int currentTime = glutGet(GLUT_ELAPSED_TIME);
  float deltaTime = (currentTime - lastTime) / 1000.0f;
  lastTime = currentTime;
  if (directionWS != 0.0f && directionAD != 0.0f)
  {
    directionWS *= sqrt(2.0f) / 2.0f;
    directionAD *= sqrt(2.0f) / 2.0f;
  }
  playerX += 15 * deltaTime * directionWS * cos(camX * M_PI / 180.0f);
  playerZ += 15 * deltaTime * directionWS * sin(camX * M_PI / 180.0f);
  playerX += 15 * deltaTime * directionAD * cos((camX * M_PI / 180.0f) + M_PI / 2.0f);
  playerZ += 15 * deltaTime * directionAD * sin((camX * M_PI / 180.0f) + M_PI / 2.0f);
}

void applyCamMatrix()
{
  handleMovement();
  if (playerX < -13.0f)
    playerX = -13.0f;
  if (playerX > -3.2f)
    playerX = -3.2f;
  if (playerZ < -6.9f)
    playerZ = -6.9f;
  if (playerZ > 22.5f)
    playerZ = 22.5f;

  gluLookAt(playerX, 0, playerZ,
            playerX + cos(camX * M_PI / 180.0f) * cos(-camY * M_PI / 180.0f),
            sin(-camY * M_PI / 180.0f),
            playerZ + sin(camX * M_PI / 180.0f) * cos(-camY * M_PI / 180.0f),
            0.0f, 1.0f, 0.0f);
}

void gameOver()
{
  currentLevel = 1;
  misses = 0;
  targetCount = 0;
  timeRemaining = 30.0f;
  free(firsttarget);
  spawnTargets(5);
}
void gameLogic()
{
  if ((misses >= 3) || (timeRemaining <= 0.0f))
  {
    gameOver();
  }

  if (targetCount == 0)
  {
    misses = 0;
    currentLevel++;
    timeRemaining = fmax(5.0f, 15.0f - (int)currentLevel / 4.0f);
    spawnTargets(5 + currentLevel);
  }

  static int lastTime = 0;
  int currentTime = glutGet(GLUT_ELAPSED_TIME);
  float deltaTime = (currentTime - lastTime) / 1000.0f;
  lastTime = currentTime;
  if (!paused)
    timeRemaining -= deltaTime;
}
void display()
{
  gameLogic();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  applyCamMatrix();

  draw();
  glMatrixMode(GL_MODELVIEW);
  glutSwapBuffers();
}

void reshapeScreen(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(FOV, 16.0 / 9.0, 1, 75);
  glMatrixMode(GL_MODELVIEW);
}

void timer(int value)
{
  glutPostRedisplay();
  glutTimerFunc(1000 / FPS, timer, 0);
}
