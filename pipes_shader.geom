#version 330 core
layout (lines) in;
layout (triangle_strip, max_vertices = 3) out;

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

  mat4 projView = shared_uniforms.projection * shared_uniforms.view;
  vec4 pos_tmp = model * vec4(gs_in[0].position, 1);
  vec3 frag_pos_start = pos_tmp.xyz;
  gs_out.frag_position = frag_pos_start;;
  gl_Position = projView * pos_tmp;
  EmitVertex();

  pos_tmp = model * vec4(gs_in[1].position, 1);
  vec3 frag_pos_end = pos_tmp.xyz;
  gs_out.frag_position = frag_pos_end;
  gl_Position = projView * pos_tmp;
  EmitVertex();

  // TODO (next): Continue the same thing, except put points to the left and
  // right of both endpoints rather than a single triangle point to the left of
  // the midpoint.

  // The third vertex is in the "up" direction from the midpoint of the
  // original segment.
  vec3 dir = gs_in[1].position - gs_in[0].position;
  vec3 midpoint = gs_in[0].position + dir * 0.5;
  pos_tmp = model * vec4(midpoint, 1);
  vec3 plane_right = cameraRight(pos_tmp.xyz, gs_out.forward);
  pos_tmp.xyz = pos_tmp.xyz - plane_right * (length(frag_pos_end -
    frag_pos_start) * shared_uniforms.geometry_thickness);
  gs_out.frag_position = pos_tmp.xyz;
  gl_Position = projView * pos_tmp;
  EmitVertex();
  EndPrimitive();
}
