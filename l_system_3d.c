#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "l_system_3d.h"
#include "utilities.h"

#define DEFAULT_WINDOW_WIDTH (800)
#define DEFAULT_WINDOW_HEIGHT (600)

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
  if (s->window) glfwDestroyWindow(s->window);
  memset(s, 0, sizeof(*s));
  free(s);
}

static void FramebufferResizedCallback(GLFWwindow *window, int width,
    int height) {
  ApplicationState *s = (ApplicationState *) glfwGetWindowUserPointer(window);
  s->window_width = width;
  s->window_height = height;
  s->aspect_ratio = ((float) s->window_width) / ((float) s->window_height);
  glViewport(0, 0, width, height);
}

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

static int ProcessInputs(ApplicationState *s) {
  if (glfwGetKey(s->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(s->window, 1);
  }
  return 1;
}

static int RunMainLoop(ApplicationState *s) {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glClearColor(0.3f, 0.05f, 0.5f, 1.0f);
  while (!glfwWindowShouldClose(s->window)) {
    if (!ProcessInputs(s)) {
      printf("Error processing inputs.\n");
      return 0;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

  if (!CheckGLErrors()) {
    printf("OpenGL errors detected during initialization.\n");
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
