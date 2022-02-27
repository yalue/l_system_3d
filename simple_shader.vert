#version 330 core
layout (location = 0) in vec3 position_in;
layout (location = 1) in vec3 forward_in;
layout (location = 2) in vec3 up_in;
layout (location = 3) in vec4 color_in;

uniform mat4 model;
uniform mat3 normal;

// Added to the location of each vertex to center the overall mesh on 0,0,0.
uniform vec3 location_offset;

out VS_OUT {
  vec3 frag_position;
  vec3 forward;
  vec3 up;
  vec3 right;
  vec4 color;
} vs_out;

// This will be replaced with the contents of shared_uniforms.glsl
//INCLUDE_SHARED_UNIFORMS

void main() {
  gl_Position = shared_uniforms.projection * shared_uniforms.view * model *
    vec4(position_in + location_offset, 1.0);
  vs_out.color = color_in;
  vs_out.forward = normal * forward_in;
  vs_out.up = normal * up_in;
  vs_out.right = normalize(cross(vs_out.forward, vs_out.up));

  // Used to prevent the normal matrix from being optimized out. (I don't want
  // to have to ignore the error when looking for its uniform location.)
  if (normal[0][0] == 0.0001) {
    vs_out.color[0] *= 0.9999;
  }

  vs_out.frag_position = vec3(model * vec4(position_in + location_offset, 1.0));
}
