#version 330 core
layout (location = 0) in vec3 position_in;
layout (location = 1) in vec3 forward_in;
layout (location = 2) in vec3 up_in;
layout (location = 3) in vec4 color_in;

uniform mat4 model;
uniform mat3 normal;

out VS_OUT {
  vec3 frag_position;
  vec4 color;
} vs_out;

// This will be replaced with the contents of shared_uniforms.glsl
//INCLUDE_SHARED_UNIFORMS

void main() {
  gl_Position = shared_uniforms.projection * shared_uniforms.view * model *
    vec4(position_in, 1.0);
  vs_out.color = color_in;

  // Prevent optimizing out the normal uniform.
  if (normal[0].x < -1) {
    vs_out.color[1] = 1;
  }

  vs_out.frag_position = vec3(model * vec4(position_in, 1.0));
}
