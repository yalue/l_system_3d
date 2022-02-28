#include <stdint.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <glad/glad.h>
#include "l_system_mesh.h"
#include "parse_config.h"
#include "turtle_3d.h"

// Uniforms shared with all shaders. Must match the layout in
// shared_uniforms.glsl, and every field must be padded to four floats.
typedef struct {
  mat4 projection;
  mat4 view;
  vec4 camera_position;
  float size_scale;
  float geometry_thickness;
  float current_time;
  float pad[1];
} SharedUniforms;

// Maintains global data about the running program.
typedef struct {
  GLFWwindow *window;
  int window_width;
  int window_height;
  float aspect_ratio;
  double frame_start;
  double frame_duration;
  char *config_file_path;
  LSystemMesh *mesh;
  LSystemConfig *config;
  Turtle3D *turtle;
  GLuint ubo;
  SharedUniforms shared_uniforms;
  int key_pressed_tmp;
  uint32_t l_system_iterations;
  uint32_t l_system_length;
  uint8_t *l_system_string;
} ApplicationState;

