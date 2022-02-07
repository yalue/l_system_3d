#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <glad/glad.h>
#include "l_system_mesh.h"
#include "turtle_3d.h"

// Uniforms shared with all shaders. Must match the layout in
// shared_uniforms.glsl, and every field must be padded to four floats.
typedef struct {
  mat4 projection;
  mat4 view;
  vec4 camera_position;
} SharedUniforms;

// Maintains global data about the running program.
typedef struct {
  GLFWwindow *window;
  int window_width;
  int window_height;
  float aspect_ratio;
  LSystemMesh *mesh;
  Turtle3D *turtle;
  GLuint ubo;
  SharedUniforms shared_uniforms;
} ApplicationState;

// Allocates an ApplicationState struct and initializes its values to 0.
// Returns NULL on error.
ApplicationState* AllocateApplicationState(void);

// Frees an ApplicationState struct. The given pointer is no longer valid after
// this function returns.
void FreeApplicationState(ApplicationState *s);

