#version 330 core

in VS_OUT {
  vec3 frag_position;
  vec4 color;
} fs_in;

out vec4 frag_color;

// This will be replaced with the content of shared_uniforms.glsl
//INCLUDE_SHARED_UNIFORMS

void main() {
  frag_color = fs_in.color;
}
