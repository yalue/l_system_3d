#version 330 core

in GS_OUT {
  vec3 frag_position;
  vec3 forward;
  vec3 up;
  vec3 right;
  vec4 color;
} fs_in;


out vec4 frag_color;

//INCLUDE_SHARED_UNIFORMS

void main() {
  frag_color = fs_in.color;
}
