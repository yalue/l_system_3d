// Holds information about the L-system's mesh that we render.
#ifndef L_SYSTEM_MESH_H
#define L_SYSTEM_MESH_H
#include <stdint.h>
#include <cglm/cglm.h>
#include <glad/glad.h>

// The binding point for the shared uniform block.
#define SHARED_UNIFORMS_BINDING (0)

// Defines a single vertex within the mesh.
typedef struct {
  float location[3];
  float pad1;
  // Points in the direction of the next vertex. May be 0 if this is at the
  // end of a sequence of lines.
  float direction[3];
  float pad2;
  // Points "downward"; must be orthogonal to the direction. Basically just
  // gives a consistent way to orient geometry.
  float orientation[3];
  float pad3;
  // The color of the vertex.
  float color[4];
} MeshVertex;

// Holds information about a full mesh to render.
typedef struct {
  // We copy the vertices into the buffer only when SetMeshVertices is called.
  uint32_t vertex_count;
  // OpenGL stuff needed for drawing this mesh.
  GLuint shader_program;
  GLuint vao;
  GLuint vbo;
  // The model and normal matrices used when drawing this mesh.
  // TODO: Try to just make these identity matrices? Or perhaps use these to
  // scale the model to fit in a 1x1x1 cube and sit on the floor?
  mat4 model;
  mat3 normal;
  GLint model_uniform_index;
  GLint normal_uniform_index;
} LSystemMesh;

// Allocates and initializes a new L-system mesh. Returns NULL on error. The
// mesh is initially empty. Destroy the returned mesh using DestroyLSystemMesh
// when it's no longer needed.
LSystemMesh* CreateLSystemMesh(void);

// Destroys the given mesh, freeing associated memory. The given mesh pointer
// is no longer valid after this returns.
void DestroyLSystemMesh(LSystemMesh *m);

// Updates the vertices to render in the mesh. Returns 0 on error. The list of
// vertices should specify *lines*; i.e. this should be a list of pairs of
// vertices.
int SetMeshVertices(LSystemMesh *m, MeshVertex *vertices, uint32_t count);

// Draws the mesh. Returns 0 on error, including any GL errors if they occur.
int DrawMesh(LSystemMesh *m);

#endif  // L_SYSTEM_MESH_H
