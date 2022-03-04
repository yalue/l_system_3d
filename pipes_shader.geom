#version 330 core
layout (lines) in;
layout (line_strip, max_vertices = 4) out;

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

void main() {
  gs_out.forward = normal * gs_in[0].forward;
  gs_out.up = normal * gs_in[0].up;
  gs_out.right = normal * gs_in[0].right;
  gs_out.color = gs_in[1].color;

  mat4 projView = shared_uniforms.projection * shared_uniforms.view;
  vec4 pos_tmp = model * vec4(gs_in[0].position, 1);
  pos_tmp += normalize(vec4(gs_out.forward, 1.0)) * timeOffset();
  gs_out.frag_position = pos_tmp.xyz;
  gl_Position = projView * pos_tmp;
  EmitVertex();

  pos_tmp = model * vec4(gs_in[1].position, 1);
  pos_tmp += normalize(vec4(gs_out.forward, 1.0)) * timeOffset();
  gs_out.frag_position = pos_tmp.xyz;
  gl_Position = projView * pos_tmp;
  EmitVertex();
  EndPrimitive();

  // Add a second primitive: a line starting at each midpoint and pointing in
  // the "up" direction.
  vec3 dir = gs_in[1].position - gs_in[0].position;
  vec3 midpoint = gs_in[0].position + dir * 0.5;
  pos_tmp = model * vec4(midpoint, 1.0);
  gs_out.frag_position = pos_tmp.xyz;
  gl_Position = projView * pos_tmp;
  EmitVertex();
  pos_tmp = model * vec4(midpoint + gs_out.up * (length(dir) * 0.5), 1);
  gs_out.frag_position = pos_tmp.xyz;
  gl_Position = projView * pos_tmp;
  EmitVertex();
  EndPrimitive();
}
