#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "l_system_mesh.h"
#include "turtle_3d.h"
#include "utilities.h"
#include "l_system_3d.h"

#define DEFAULT_WINDOW_WIDTH (800)
#define DEFAULT_WINDOW_HEIGHT (600)
#define PI (3.1415926536)

ApplicationState* AllocateApplicationState(void) {
  ApplicationState *to_return = NULL;
  to_return = calloc(1, sizeof(*to_return));
  if (!to_return) return NULL;
  to_return->window_width = DEFAULT_WINDOW_WIDTH;
  to_return->window_height = DEFAULT_WINDOW_HEIGHT;
  to_return->aspect_ratio = ((float) to_return->window_width) /
    ((float) to_return->window_height);
  return to_return;
}

void FreeApplicationState(ApplicationState *s) {
  if (!s) return;
  if (s->mesh) DestroyLSystemMesh(s->mesh);
  if (s->turtle) DestroyTurtle3D(s->turtle);
  glDeleteBuffers(1, &(s->ubo));
  if (s->window) glfwDestroyWindow(s->window);
  memset(s, 0, sizeof(*s));
  free(s);
}

// Recomputes the scene's projection matrix based on s->aspect_ratio.
static void UpdateProjectionMatrix(ApplicationState *s) {
  glm_mat4_identity(s->shared_uniforms.projection);
  glm_perspective(45.0, s->aspect_ratio, 0.01, 100.0,
    s->shared_uniforms.projection);
}

static void FramebufferResizedCallback(GLFWwindow *window, int width,
    int height) {
  ApplicationState *s = (ApplicationState *) glfwGetWindowUserPointer(window);
  s->window_width = width;
  s->window_height = height;
  s->aspect_ratio = ((float) s->window_width) / ((float) s->window_height);
  glViewport(0, 0, width, height);
  UpdateProjectionMatrix(s);
}

// Sets up the GLFW window. Returns 0 on error.
static int SetupWindow(ApplicationState *s) {
  GLFWwindow *window = NULL;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  window = glfwCreateWindow(s->window_width, s->window_height, "3D L System",
    NULL, NULL);
  if (!window) {
    printf("Failed creating the GLFW window.\n");
    glfwTerminate();
    return 0;
  }
  glfwSetWindowUserPointer(window, s);
  glfwMakeContextCurrent(window);
  s->window = window;
  return 1;
}

// Allocates the ubo, but doesn't populate it with data yet. Returns 0 on
// error.
static int SetupUniformBuffer(ApplicationState *s) {
  glGenBuffers(1, &(s->ubo));
  glBindBuffer(GL_UNIFORM_BUFFER, s->ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(SharedUniforms), NULL,
    GL_STATIC_DRAW);
  glBindBufferRange(GL_UNIFORM_BUFFER, SHARED_UNIFORMS_BINDING, s->ubo, 0,
    sizeof(SharedUniforms));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  return CheckGLErrors();
}

static int Rotate90(Turtle3D *t) {
  return RotateTurtle(t, PI * 0.5);
}

// This generates the vertices for the L-system, and updates the mesh. Returns
// 0 on error.
static int GenerateVertices(ApplicationState *s) {
  Turtle3D *t = s->turtle;
  t->color[0] = 1;
  t->color[1] = 0;
  t->color[2] = 0;
  if (!MoveTurtleForward(t, 1.0)) return 0;
  if (!Rotate90(t)) return 0;
  t->color[0] = 0;
  t->color[1] = 1;
  if (!MoveTurtleForward(t, 1.0)) return 0;
  if (!Rotate90(t)) return 0;
  t->color[1] = 0;
  t->color[2] = 1;
  if (!MoveTurtleForward(t, 1.0)) return 0;
  if (!Rotate90(t)) return 0;
  t->color[0] = 1;
  t->color[1] = 1;
  if (!MoveTurtleForward(t, 1.0)) return 0;
  if (!PitchTurtle(t, PI * 0.5)) return 0;
  t->color[0] = 0.7;
  t->color[1] = 0.1;
  if (!MoveTurtleForward(t, 0.5)) return 0;
  if (!SetMeshVertices(s->mesh, t->vertices, t->vertex_count)) {
    printf("Failed setting vertices.\n");
    return 0;
  }
  return 1;
}

static int ProcessInputs(ApplicationState *s) {
  if (glfwGetKey(s->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(s->window, 1);
  }
  return 1;
}

static void UpdateCamera(ApplicationState *s) {
  vec3 position, target, up;
  float tmp;
  // TODO (eventually): Change camera based on user input; allow flying around.
  glm_mat4_identity(s->shared_uniforms.view);
  glm_vec3_zero(position);
  glm_vec3_zero(target);
  glm_vec3_zero(up);
  up[1] = 1.0;
  tmp = glfwGetTime() / 4.0;
  position[0] = sin(tmp) * 4.0;
  position[1] = 3.0;
  position[2] = cos(tmp) * 4.0;
  glm_lookat(position, target, up, s->shared_uniforms.view);
  glm_vec4(position, 0, s->shared_uniforms.camera_position);
}

static int RunMainLoop(ApplicationState *s) {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glClearColor(0, 0, 0, 1.0);
  while (!glfwWindowShouldClose(s->window)) {
    if (!ProcessInputs(s)) {
      printf("Error processing inputs.\n");
      return 0;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    UpdateCamera(s);
    glBindBuffer(GL_UNIFORM_BUFFER, s->ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SharedUniforms),
      (void *) &(s->shared_uniforms));
    if (!DrawMesh(s->mesh)) return 0;

    glfwSwapBuffers(s->window);
    glfwPollEvents();
    if (!CheckGLErrors()) {
      printf("Error drawing window.\n");
      return 0;
    }
  }
  return 1;
}

int main(int argc, char **argv) {
  int to_return = 0;
  ApplicationState *s = NULL;
  if (!glfwInit()) {
    printf("Failed initializing GLFW.\n");
    return 1;
  }
  s = AllocateApplicationState();
  if (!s) {
    printf("Failed allocating application state.\n");
    return 1;
  }
  if (!SetupWindow(s)) {
    printf("Failed setting up window.\n");
    FreeApplicationState(s);
    return 1;
  }
  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
    printf("Failed initializing GLAD.\n");
    to_return = 1;
    goto cleanup;
  }
  glViewport(0, 0, s->window_width, s->window_height);
  glfwSetFramebufferSizeCallback(s->window, FramebufferResizedCallback);
  if (!SetupUniformBuffer(s)) {
    printf("Failed setting up uniform buffer.\n");
    to_return = 1;
    goto cleanup;
  }
  UpdateProjectionMatrix(s);
  s->mesh = CreateLSystemMesh();
  if (!s->mesh) {
    printf("Failed initializing L-system mesh.\n");
    to_return = 1;
    goto cleanup;
  }

  if (!CheckGLErrors()) {
    printf("OpenGL errors detected during initialization.\n");
    to_return = 1;
    goto cleanup;
  }

  s->turtle = CreateTurtle3D();
  if (!s->turtle) {
    printf("Failed creating the \"turtle\" for drawing.\n");
    to_return = 1;
    goto cleanup;
  }
  if (!GenerateVertices(s)) {
    printf("Failed generating vertices.\n");
    to_return = 1;
    goto cleanup;
  }
  if (!RunMainLoop(s)) {
    printf("Application ended with an error.\n");
    to_return = 1;
  } else {
    printf("Everything done OK.\n");
  }
cleanup:
  FreeApplicationState(s);
  glfwTerminate();
  return to_return;
}
