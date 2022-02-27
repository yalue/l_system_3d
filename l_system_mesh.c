#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cglm/cglm.h>
#include <glad/glad.h>
#include "utilities.h"
#include "l_system_mesh.h"

static void DebugPrintVec3(float *v) {
  printf("(%.03f %.03f %.03f)", v[0], v[1], v[2]);
}

void DebugPrintVertex(MeshVertex *v) {
  printf("Mesh vertex: ");
  printf("position ");
  DebugPrintVec3(v->location);
  printf(", forward ");
  DebugPrintVec3(v->forward);
  printf(", up ");
  DebugPrintVec3(v->up);
  printf(", color ");
  DebugPrintVec3(v->color);
  printf(".\n");
}

// Loads and compiles a shader from the given file path. Returns 0 on error.
static GLuint LoadShader(const char *path, GLenum shader_type) {
  GLuint to_return = 0;
  GLint compile_result = 0;
  GLchar shader_log[512];
  char *shader_src_orig = NULL;
  char *shared_uniform_code = NULL;
  char *final_src = NULL;

  // First do some preprocessing to insert the common uniform definitions in
  // place of the special comment in the main shader source.
  shader_src_orig = ReadFullFile(path);
  if (!shader_src_orig) return 0;
  shared_uniform_code = ReadFullFile("./shared_uniforms.glsl");
  if (!shared_uniform_code) {
    free(shader_src_orig);
    return 0;
  }
  final_src = StringReplace(shader_src_orig, "//INCLUDE_SHARED_UNIFORMS\n",
    shared_uniform_code);
  if (!final_src) {
    printf("Failed preprocessing shader source code.\n");
    free(shader_src_orig);
    free(shared_uniform_code);
    return 0;
  }
  free(shader_src_orig);
  free(shared_uniform_code);
  shader_src_orig = NULL;
  shared_uniform_code = NULL;

  to_return = glCreateShader(shader_type);
  glShaderSource(to_return, 1, (const char**) &final_src, NULL);
  glCompileShader(to_return);
  free(final_src);
  final_src = NULL;

  // Check compilation success.
  memset(shader_log, 0, sizeof(shader_log));
  glGetShaderiv(to_return, GL_COMPILE_STATUS, &compile_result);
  if (compile_result != GL_TRUE) {
    glGetShaderInfoLog(to_return, sizeof(shader_log) - 1, NULL,
      shader_log);
    printf("Shader %s compile error:\n%s\n", path, shader_log);
    glDeleteShader(to_return);
    return 0;
  }
  if (!CheckGLErrors()) {
    glDeleteShader(to_return);
    return 0;
  }
  return to_return;
}

// Links and returns the shader program from the given source file names.
// The geometry_src_file can be NULL if a geometry shader isn't used. A
// vertex and fragment shader are always required.
static GLuint CreateShaderProgram(const char *vertex_src_file,
    const char *geometry_src_file, const char *fragment_src_file) {
  GLchar link_log[512];
  GLint link_result = 0;
  GLuint vertex_shader, geometry_shader, fragment_shader, to_return;

  vertex_shader = LoadShader(vertex_src_file, GL_VERTEX_SHADER);
  if (!vertex_shader) {
    printf("Couldn't load vertex shader.\n");
    return 0;
  }
  fragment_shader = LoadShader(fragment_src_file, GL_FRAGMENT_SHADER);
  if (!fragment_shader) {
    printf("Couldn't load fragment shader.\n");
    glDeleteShader(vertex_shader);
    return 0;
  }

  // Only load the geometry shader if requested.
  geometry_shader = 0;
  if (geometry_src_file != NULL) {
    geometry_shader = LoadShader(geometry_src_file, GL_GEOMETRY_SHADER);
    if (!geometry_shader) {
      printf("Couldn't load geometry shader.\n");
      glDeleteShader(vertex_shader);
      glDeleteShader(fragment_shader);
      return 0;
    }
  }

  to_return = glCreateProgram();
  glAttachShader(to_return, vertex_shader);
  if (geometry_shader) {
    glAttachShader(to_return, geometry_shader);
  }
  glAttachShader(to_return, fragment_shader);
  glLinkProgram(to_return);
  glDeleteShader(vertex_shader);
  glDeleteShader(geometry_shader);
  glDeleteShader(fragment_shader);
  glGetProgramiv(to_return, GL_LINK_STATUS, &link_result);
  memset(link_log, 0, sizeof(link_log));
  if (link_result != GL_TRUE) {
    glGetProgramInfoLog(to_return, sizeof(link_log) - 1, NULL, link_log);
    printf("GL program link error:\n%s\n", link_log);
    glDeleteProgram(to_return);
    return 0;
  }
  glUseProgram(to_return);
  if (!CheckGLErrors()) {
    glDeleteProgram(to_return);
    return 0;
  }
  return to_return;
}

// Sets *index to the index of the named uniform in s->shader_program. Returns
// 0 and prints a message on error.
static int UniformIndex(GLuint program, const char *name, GLint *index) {
  *index = glGetUniformLocation(program, name);
  if (*index < 0) {
    printf("Failed getting location of uniform %s.\n", name);
    return 0;
  }
  return 1;
}

// Loads the shaders for the model, and looks up uniform indices. Returns 0 on
// error.
static int SetupShaderProgram(LSystemMesh *m, const char *vertex_src,
    const char *geometry_src, const char *fragment_src) {
  GLuint p;
  GLuint block_index;
  p = CreateShaderProgram(vertex_src, geometry_src, fragment_src);
  if (!p) return 0;
  m->shader_program = p;
  if (!UniformIndex(p, "model", &(m->model_uniform_index))) return 0;
  if (!UniformIndex(p, "normal", &(m->normal_uniform_index))) return 0;
  if (!UniformIndex(p, "location_offset",
    &(m->location_offset_uniform_index))) {
    return 0;
  }
  block_index = glGetUniformBlockIndex(p, "SharedUniforms");
  if (block_index == GL_INVALID_INDEX) {
    printf("Failed getting index of shared uniform block.\n");
    CheckGLErrors();
    return 0;
  }
  glUniformBlockBinding(p, block_index, SHARED_UNIFORMS_BINDING);
  return CheckGLErrors();
}

int SwitchRenderingModes(LSystemMesh *m) {
  glDeleteProgram(m->shader_program);
  m->shader_program = 0;
  if (m->using_geometry_shader) {
    m->using_geometry_shader = 0;
    return SetupShaderProgram(m, "simple_shader.vert", NULL,
      "simple_shader.frag");
  }
  m->using_geometry_shader = 1;
  return SetupShaderProgram(m, "pipes_shader.vert", "pipes_shader.geom",
    "pipes_shader.frag");
}

LSystemMesh* CreateLSystemMesh(void) {
  LSystemMesh *m = NULL;

  m = (LSystemMesh *) calloc(1, sizeof(*m));
  if (!m) {
    printf("Failed allocating CPU-side mesh struct.\n");
    return NULL;
  }

  glGenVertexArrays(1, &(m->vao));
  glBindVertexArray(m->vao);
  glGenBuffers(1, &(m->vbo));
  glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
  // Setting up the location, direction, orientation, and color attributes
  // (respectively).
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
    (void *) offsetof(MeshVertex, location));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
    (void *) offsetof(MeshVertex, forward));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
    (void *) offsetof(MeshVertex, up));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
    (void *) offsetof(MeshVertex, color));
  glEnableVertexAttribArray(3);
  if (!CheckGLErrors()) {
    printf("Failed setting up mesh object.\n");
    DestroyLSystemMesh(m);
    return NULL;
  }
  if (!SetupShaderProgram(m, "simple_shader.vert", NULL,
    "simple_shader.frag")) {
    DestroyLSystemMesh(m);
    return NULL;
  }
  glm_mat4_identity(m->model);
  glm_mat3_identity(m->normal);
  return m;
}

void DestroyLSystemMesh(LSystemMesh *m) {
  if (!m) return;
  glDeleteBuffers(1, &(m->vbo));
  glDeleteVertexArrays(1, &(m->vao));
  glDeleteProgram(m->shader_program);
  memset(m, 0, sizeof(*m));
  free(m);
}

int SetMeshVertices(LSystemMesh *m, MeshVertex *vertices, uint32_t count) {
  glBindVertexArray(m->vao);
  glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
  glBufferData(GL_ARRAY_BUFFER, count * sizeof(MeshVertex), vertices,
    GL_STATIC_DRAW);
  m->vertex_count = count;
  return CheckGLErrors();
}

int DrawMesh(LSystemMesh *m) {
  glUseProgram(m->shader_program);
  glBindVertexArray(m->vao);
  glUniformMatrix4fv(m->model_uniform_index, 1, GL_FALSE, (float *) m->model);
  glUniformMatrix3fv(m->normal_uniform_index, 1, GL_FALSE,
    (float *) m->normal);
  glUniform3fv(m->location_offset_uniform_index, 1,
    (float *) m->location_offset);
  glDrawArrays(GL_LINES, 0, m->vertex_count);
  return CheckGLErrors();
}
