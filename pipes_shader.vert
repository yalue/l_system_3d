#version 330 core
layout (location = 0) in vec3 position_in;
layout (location = 1) in vec3 forward_in;
layout (location = 2) in vec3 up_in;
layout (location = 3) in vec4 color_in;

// Added to the location of each vertex to center the overall mesh on 0,0,0.
uniform vec3 location_offset;

out VS_OUT {
  vec3 position;
  vec3 forward;
  vec3 up;
  vec3 right;
  vec4 color;
} vs_out;

//INCLUDE_SHARED_UNIFORMS

void main() {
  vs_out.position = position_in + location_offset;
  vs_out.forward = forward_in;
  vs_out.up = up_in;
  vs_out.color = color_in;
  vs_out.right = normalize(cross(forward_in, up_in));
}
