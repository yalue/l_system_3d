// There are no shared uniforms so far. This file will contain them later.
layout(std140) uniform SharedUniforms {
  mat4 projection;
  mat4 view;
  vec4 camera_position;
} shared_uniforms;

