#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "l_system_mesh.h"
#include "parse_config.h"
#include "turtle_3d.h"
#include "utilities.h"
#include "l_system_3d.h"

#define DEFAULT_WINDOW_WIDTH (800)
#define DEFAULT_WINDOW_HEIGHT (600)
#define DEFAULT_FPS (60.0)

static ApplicationState* AllocateApplicationState(void) {
  ApplicationState *to_return = NULL;
  to_return = calloc(1, sizeof(*to_return));
  if (!to_return) return NULL;
  to_return->window_width = DEFAULT_WINDOW_WIDTH;
  to_return->window_height = DEFAULT_WINDOW_HEIGHT;
  to_return->aspect_ratio = ((float) to_return->window_width) /
    ((float) to_return->window_height);
  to_return->frame_duration = 1.0 / DEFAULT_FPS;
  return to_return;
}

static void FreeApplicationState(ApplicationState *s) {
  if (!s) return;
  if (s->mesh) DestroyLSystemMesh(s->mesh);
  if (s->turtle) DestroyTurtle3D(s->turtle);
  if (s->config) DestroyLSystemConfig(s->config);
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

// This generates the vertices for the L-system, and updates the mesh. Returns
// 0 on error.
static int GenerateVertices(ApplicationState *s) {
  Turtle3D *t = s->turtle;
  ActionRule *r = NULL;
  int result;
  uint32_t char_index, inst_index;
  uint8_t c;
  ResetTurtle3D(t);
  for (char_index = 0; char_index < s->l_system_length; char_index++) {
    c = s->l_system_string[char_index];
    r = s->config->actions + c;
    for (inst_index = 0; inst_index < r->length; inst_index++) {
      result = r->instructions[inst_index](t, r->args[inst_index]);
      if (!result) {
        printf("Failed running instruction %d for char %c.\n", (int) inst_index,
          (char) c);
        return 0;
      }
    }
  }

  if (!SetMeshVertices(s->mesh, t->vertices, t->vertex_count)) {
    printf("Failed setting vertices.\n");
    return 0;
  }
  if (!SetTransformInfo(s->turtle, s->mesh->model, s->mesh->normal,
    s->mesh->location_offset)) {
    printf("Failed getting transform matrices.\n");
    return 0;
  }
  return 1;
}

static float ToMB(uint64_t bytes) {
  float tmp = (float) bytes;
  return tmp / (1024.0 * 1024.0);
}

// Iterates the L-system exactly once. Returns 0 on error. Does not update the
// mesh.
static int IncreaseIterations(ApplicationState *s) {
  uint32_t new_length = 0;
  ReplacementRule *r = NULL;
  uint8_t *new_buffer = NULL;
  uint8_t *dst = NULL;
  uint8_t *loc = s->l_system_string;
  uint8_t c;
  // First iterate over the string to pre-calcuate the size of the buffer we'll
  // need.
  while (*loc) {
    c = *loc;
    loc++;
    r = s->config->replacements + c;
    if (!r->used) {
      new_length++;
      continue;
    }
    new_length += r->length;
  }
  // +1 to ensure a null terminator.
  new_buffer = (uint8_t *) calloc(1, new_length + 1);
  if (!new_buffer) {
    printf("Failed allocating new %f MB L-system string.\n", ToMB(new_length));
    return 0;
  }
  // Iterate over the string again, this time populating the new buffer.
  dst = new_buffer;
  loc = s->l_system_string;
  while (*loc) {
    c = *loc;
    loc++;
    r = s->config->replacements + c;
    if (!r->used) {
      // Keep the same char if no replacement was defined.
      *dst = c;
      dst++;
      continue;
    }
    memcpy(dst, r->replacement, r->length);
    dst += r->length;
    continue;
  }
  free(s->l_system_string);
  s->l_system_string = new_buffer;
  s->l_system_length = new_length;
  s->l_system_iterations++;
  return 1;
}

// Reduces the L-system iterations by one. Unfortunately, this is implemented
// by recomputing the entire thing. Does nothing if we're already at 0
// iterations.
static int DecreaseIterations(ApplicationState *s) {
  uint32_t target_iterations, i;
  if (s->l_system_iterations == 0) {
    printf("Can't decrease iterations. Already at 0 iterations.\n");
    return 1;
  }
  target_iterations = s->l_system_iterations - 1;
  free(s->l_system_string);
  s->l_system_string = (uint8_t *) strdup(s->config->init);
  if (!s->l_system_string) {
    printf("Failed copying the initial L-system string.\n");
    return 0;
  }
  s->l_system_length = strlen(s->config->init);
  s->l_system_iterations = 0;
  for (i = 0; i < target_iterations; i++) {
    if (!IncreaseIterations(s)) return 0;
  }
  return 1;
}

// Reloads the config file from ./config.txt. If this fails, then we'll just
// print a message and return. (The config can be faulty at runtime, but we
// won't start the program unless it's OK.)
static void ReloadConfig(ApplicationState *s) {
  LSystemConfig *new_config = NULL;
  LSystemConfig *old_config = s->config;
  new_config = LoadLSystemConfig("./config.txt");
  if (!new_config) {
    printf("Failed loading new config file.\n");
    return;
  }
  s->config = new_config;
  DestroyLSystemConfig(old_config);
  printf("Config updated OK.\n");
}

static void PrintMemoryUsage(ApplicationState *s) {
  float vbo_size_mb = ToMB(sizeof(MeshVertex) * s->mesh->vertex_count);
  printf("L-system size is now %.02f MB.\n", ToMB(s->l_system_length));
  printf("Drawing %u vertices, taking %.02f MB.\n",
    (unsigned) s->mesh->vertex_count, vbo_size_mb);
}

static int ProcessInputs(ApplicationState *s) {
  int pressed;
  if (glfwGetKey(s->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(s->window, 1);
    return 1;
  }
  // s->key_pressed_tmp is used to prevent counting one press multiple times,
  // and to prevent up and down from being pressed together.
  pressed = glfwGetKey(s->window, GLFW_KEY_UP) == GLFW_PRESS;
  if (!s->key_pressed_tmp && pressed) {
    // Nothing pressed -> up pressed
    s->key_pressed_tmp = GLFW_KEY_UP;
    if (!(IncreaseIterations(s) && GenerateVertices(s))) return 0;
    PrintMemoryUsage(s);
  } else if ((s->key_pressed_tmp == GLFW_KEY_UP) && !pressed) {
    // Up pressed -> up released
    s->key_pressed_tmp = 0;
  }
  pressed = glfwGetKey(s->window, GLFW_KEY_DOWN) == GLFW_PRESS;
  if (!s->key_pressed_tmp && pressed) {
    // Nothing pressed -> down pressed
    s->key_pressed_tmp = GLFW_KEY_DOWN;
    if (!(DecreaseIterations(s) && GenerateVertices(s))) return 0;
    PrintMemoryUsage(s);
  } else if ((s->key_pressed_tmp == GLFW_KEY_DOWN) && !pressed) {
    // Down pressed -> down released
    s->key_pressed_tmp = 0;
  }
  pressed = glfwGetKey(s->window, GLFW_KEY_R) == GLFW_PRESS;
  if (!s->key_pressed_tmp && pressed) {
    // Nothing pressed -> R pressed
    s->key_pressed_tmp = GLFW_KEY_R;
    ReloadConfig(s);
    if (!GenerateVertices(s)) return 0;
  } else if ((s->key_pressed_tmp == GLFW_KEY_R) && !pressed) {
    // R pressed -> R released
    s->key_pressed_tmp = 0;
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
  position[0] = sin(tmp) * 5.0;
  position[1] = 2.0;
  position[2] = cos(tmp) * 5.0;
  glm_lookat(position, target, up, s->shared_uniforms.view);
  glm_vec4(position, 0, s->shared_uniforms.camera_position);
}

// Sleeps for s number of seconds.
static void SleepSeconds(double s) {
  struct timespec t;
  t.tv_sec = (uint64_t) s;
  t.tv_nsec = (s - ((double) t.tv_sec)) * 1e9;
  nanosleep(&t, NULL);
}

static int WaitNextFrame(ApplicationState *s) {
  double elapsed = glfwGetTime() - s->frame_start;
  if (elapsed < s->frame_duration) {
    SleepSeconds(s->frame_duration - elapsed);
  }
  return 1;
}

static int RunMainLoop(ApplicationState *s) {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glClearColor(0, 0, 0, 1.0);
  while (!glfwWindowShouldClose(s->window)) {
    s->frame_start = glfwGetTime();
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
    if (!WaitNextFrame(s)) return 0;
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
  s->config = LoadLSystemConfig("./config.txt");
  if (!s->config) {
    printf("Error parsing ./config.txt.\n");
    to_return = 1;
    goto cleanup;
  }
  printf("Config loaded OK!\n");
  s->l_system_string = (uint8_t *) strdup(s->config->init);
  if (!s->l_system_string) {
    printf("Error initializing L-system string.\n");
    to_return = 1;
    goto cleanup;
  }
  s->l_system_length = strlen(s->config->init);
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
