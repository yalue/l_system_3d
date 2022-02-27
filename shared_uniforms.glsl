layout(std140) uniform SharedUniforms {
  mat4 projection;
  mat4 view;
  vec4 camera_position;
  float size_scale;
  float geometry_thickness;
  float pad[2];
} shared_uniforms;

