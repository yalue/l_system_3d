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

//INCLUDE_SHARED_UNIFORMS

void main() {
  vec3 right = gs_in[0].right;
  vec3 forward = gs_in[0].forward;
  vec3 bottom = gs_in[0].position;
  vec3 top = gs_in[0].position;
  // NOTE: verify this is in the correct order.
  mat4 projView = shared_uniforms.projection * shared_uniforms.view;
  // The normal vectors don't change.
  gs_out.forward = forward;
  gs_out.up = gs_in[0].up;
  gs_out.right = right;
  //gs_out.color = gs_in[1].color;
  gs_out.color = vec4(0.5, 0.3, 1.0, 1.0);

  float half_rect_width = 0.5 * shared_uniforms.geometry_thickness *
    shared_uniforms.size_scale;

  // Bottom left
  gs_out.frag_position = bottom - (right * half_rect_width);
  gl_Position = projView * vec4(gs_out.frag_position, 1.0);
  EmitVertex();
  // Bottom right
  gs_out.frag_position = bottom + (right * half_rect_width);
  gl_Position = projView * vec4(gs_out.frag_position, 1.0);
  EmitVertex();
  // Top right
  gs_out.frag_position = top + (right * half_rect_width);
  gl_Position = projView * vec4(gs_out.frag_position, 1.0);
  EmitVertex();
  // Top left
  gs_out.frag_position = top - (right * half_rect_width);
  gl_Position = projView * vec4(gs_out.frag_position, 1.0);
  EmitVertex();
  EndPrimitive();
}
