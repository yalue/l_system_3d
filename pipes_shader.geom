#version 330 core
layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
  vec3 position;
  vec3 forward;
  vec3 up;
  vec3 right;
  vec4 color;
} gs_in[];

out GS_OUT {
  vec3 frag_position;
  vec3 forward;
  vec3 up;
  vec3 right;
  vec4 color;
} gs_out;

uniform mat4 model;
uniform mat3 normal;

//INCLUDE_SHARED_UNIFORMS

// Returns a value that oscillates between 0 and 1 based on time.
float timeOffset() {
  return (sin(shared_uniforms.current_time * 3) + 1.0) * 0.5;
}

// Takes a forward-facing vector and returns a vector that points to its right,
// along the camera plane.
vec3 cameraRight(vec3 position, vec3 forward) {
  vec3 to_camera = shared_uniforms.camera_position.xyz - position;
  return normalize(cross(forward, to_camera));
}

void main() {
  gs_out.forward = normalize(normal * gs_in[0].forward);
  gs_out.up = normalize(normal * gs_in[0].up);
  gs_out.right = normalize(normal * gs_in[0].right);
  gs_out.color = gs_in[1].color;

  vec4 frag_pos0 = model * vec4(gs_in[0].position, 1);
  vec3 camera_right0 = cameraRight(frag_pos0.xyz, gs_out.forward);
  vec4 frag_pos1 = model * vec4(gs_in[1].position, 1);
  vec3 camera_right1 = cameraRight(frag_pos1.xyz, gs_out.forward);
  mat4 projView = shared_uniforms.projection * shared_uniforms.view;
  float half_width = shared_uniforms.geometry_thickness *
    shared_uniforms.size_scale * 0.5;

  // Bottom left
  vec3 pos_tmp = frag_pos0.xyz - (half_width * camera_right0);
  gs_out.frag_position = pos_tmp;
  gl_Position = projView * vec4(pos_tmp, 1);
  EmitVertex();

  // Bottom right
  pos_tmp = frag_pos0.xyz + (half_width * camera_right0);
  gs_out.frag_position = pos_tmp;
  gl_Position = projView * vec4(pos_tmp, 1);
  EmitVertex();

  // Top left
  pos_tmp = frag_pos1.xyz - (half_width * camera_right1);
  gs_out.frag_position = pos_tmp;
  gl_Position = projView * vec4(pos_tmp, 1);
  EmitVertex();

  // Top right
  pos_tmp = frag_pos1.xyz + (half_width * camera_right1);
  gs_out.frag_position = pos_tmp;
  gl_Position = projView * vec4(pos_tmp, 1);
  EmitVertex();
  EndPrimitive();
}
