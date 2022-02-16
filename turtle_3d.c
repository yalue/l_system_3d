#include <cglm/cglm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "l_system_mesh.h"
#include "turtle_3d.h"

// The number of vertices the turtle initially allocates space for.
#define INITIAL_TURTLE_CAPACITY (1024)

// The minimum spacing (squared) between two subsequent positions for them to
// be considered identical.
#define MIN_POSITION_DIST (1.0e-6)

#define PI (3.1415926536)

Turtle3D* CreateTurtle3D(void) {
  Turtle3D *to_return = NULL;
  to_return = (Turtle3D *) calloc(1, sizeof(*to_return));
  if (!to_return) {
    printf("Failed allocating Turtle3D struct.\n");
    return NULL;
  }
  to_return->vertices = (MeshVertex *) calloc(INITIAL_TURTLE_CAPACITY,
    sizeof(MeshVertex));
  if (!to_return->vertices) {
    printf("Failed allocating the turtle's vertex array.\n");
    free(to_return);
    return NULL;
  }
  to_return->vertex_capacity = INITIAL_TURTLE_CAPACITY;
  ResetTurtle3D(to_return);
  return to_return;
}

void ResetTurtle3D(Turtle3D *t) {
  // Opaque white color
  glm_vec4_one(t->color);
  // Start at the origin
  glm_vec3_zero(t->position);
  glm_vec3_zero(t->prev_position);
  // Face in positive X direction, with up towards positive Y.
  glm_vec3_zero(t->forward);
  t->forward[0] = 1;
  glm_vec3_zero(t->up);
  t->up[1] = 1;
  // The bounding cube is empty and ill-defined to begin with.
  glm_vec3_zero(t->min_bounds);
  glm_vec3_zero(t->max_bounds);
  // We don't reallocate the array or zero it out here so we can reuse the
  // turtle to draw another iteration without needing to clear all the
  // vertices.
  t->vertex_count = 0;
}

void DestroyTurtle3D(Turtle3D *t) {
  if (!t) return;
  free(t->vertices);
  memset(t, 0, sizeof(*t));
  free(t);
}

static float Max3(float a, float b, float c) {
  if (a > b) {
    if (a >= c) return a;
    return c;
  }
  if (b >= c) return b;
  return c;
}

static void ModelToNormalMatrix(mat4 model, mat3 normal) {
  mat4 dst;
  glm_mat4_inv(model, dst);
  glm_mat4_transpose(dst);
  glm_mat4_pick3(dst, normal);
}

int SetTransformInfo(Turtle3D *t, mat4 model, mat3 normal, vec3 loc_offset) {
  float dx, dy, dz, max_axis;
  dx = t->max_bounds[0] - t->min_bounds[0];
  dy = t->max_bounds[1] - t->min_bounds[1];
  dz = t->max_bounds[2] - t->min_bounds[2];
  if ((dx < 0) || (dy < 0) || (dz < 0)) {
    printf("Mesh bounds (deltas %f, %f, %f) not well-formed.\n", dx, dy, dz);
    return 0;
  }
  // Center the model and make it at most 2 units wide in any axis. Start by
  // computing the amount to add to each vertex to center the mesh.
  loc_offset[0] = -(dx / 2) - t->min_bounds[0];
  loc_offset[1] = -(dy / 2) - t->min_bounds[1];
  loc_offset[2] = -(dz / 2) - t->min_bounds[2];
  max_axis = Max3(dx, dy, dz);
  glm_mat4_identity(model);
  if (max_axis > 0) {
    glm_scale_uni(model, 2.0 / max_axis);
  }
  ModelToNormalMatrix(model, normal);
  // TODO (eventually): If I get less stupid, combine the loc_offset
  // translation into the model matrix. This should obviously be possible, but
  // I just am not good enough to know how.
  return 1;
}

// Checks if the internal array has space for two more vertices (another line
// segment). If not, this attempts to reallocate the turtle's internal array of
// vertices, doubling its capacity. Returns 0 on error.
static int IncreateCapacityIfNeeded(Turtle3D *t) {
  void *new_buffer = NULL;
  uint32_t new_capacity;
  uint32_t required_capacity = t->vertex_count + 2;
  if (required_capacity < t->vertex_count) {
    printf("Vertex capacity overflow: too many vertices.\n");
    return 0;
  }
  if (required_capacity <= t->vertex_capacity) return 1;
  new_capacity = t->vertex_capacity * 2;
  if (new_capacity < t->vertex_capacity) {
    printf("Vertex capacity overflow: too many vertices.\n");
    return 0;
  }
  new_buffer = realloc(t->vertices, new_capacity * sizeof(MeshVertex));
  if (!new_buffer) {
    printf("Unable to increate number of vertices: out of memory.\n");
    return 0;
  }
  t->vertices = (MeshVertex *) new_buffer;
  t->vertex_capacity = new_capacity;
  return 1;
}

// Appends a line segment from the turtle's previous position to its current
// position to the turtle's path.
static int AppendSegment(Turtle3D *t) {
  MeshVertex *v = NULL;
  vec3 direction;
  if (!IncreateCapacityIfNeeded(t)) return 0;
  // If the two positions are too close together, then we'll just set the
  // direction vector to the turtle's current direction. Otherwise, the
  // direction in the line segment points from the previous point to the
  // current position.
  glm_vec3_sub(t->position, t->prev_position, direction);
  if (glm_vec3_norm2(direction) < MIN_POSITION_DIST) {
    glm_vec3_copy(t->forward, direction);
  } else {
    glm_vec3_normalize(direction);
  }
  v = t->vertices + t->vertex_count;
  glm_vec3_copy(t->prev_position, v->location);
  glm_vec3_copy(direction, v->forward);
  glm_vec3_copy(t->up, v->up);
  glm_vec4_copy(t->color, v->color);
  // The only difference between the two vertices in the line segment is the
  // position.
  *(v + 1) = *v;
  glm_vec3_copy(t->position, (v + 1)->location);
  t->vertex_count += 2;
  return 1;
}

// Updates min_bounds and max_bounds to contain the turtle's current position.
static void UpdateBounds(Turtle3D *t) {
  float *p = t->position;
  glm_vec3_copy(t->position, p);
  if (p[0] > t->max_bounds[0]) {
    t->max_bounds[0] = p[0];
  }
  if (p[0] < t->min_bounds[0]) {
    t->min_bounds[0] = p[0];
  }
  if (p[1] > t->max_bounds[1]) {
    t->max_bounds[1] = p[1];
  }
  if (p[1] < t->min_bounds[1]) {
    t->min_bounds[1] = p[1];
  }
  if (p[2] > t->max_bounds[2]) {
    t->max_bounds[2] = p[2];
  }
  if (p[2] < t->min_bounds[2]) {
    t->min_bounds[2] = p[2];
  }
}

int MoveTurtleForwardNoDraw(Turtle3D *t, float distance) {
  vec3 change;
  glm_vec3_copy(t->position, t->prev_position);
  glm_vec3_scale(t->forward, distance, change);
  glm_vec3_add(t->position, change, t->position);
  UpdateBounds(t);
  return 1;
}

int MoveTurtleForward(Turtle3D *t, float distance) {
  if (!MoveTurtleForwardNoDraw(t, distance)) return 0;
  return AppendSegment(t);
}

static float ToRadians(float degrees) {
  return degrees * (PI / 180.0);
}

int RotateTurtle(Turtle3D *t, float angle) {
  glm_vec3_rotate(t->forward, ToRadians(angle), t->up);
  return 1;
}

int PitchTurtle(Turtle3D *t, float angle) {
  vec3 right;
  glm_vec3_cross(t->forward, t->up, right);
  glm_vec3_rotate(t->up, ToRadians(angle), right);
  glm_vec3_rotate(t->forward, ToRadians(angle), right);
  return 1;
}

int RollTurtle(Turtle3D *t, float angle) {
  glm_vec3_rotate(t->up, ToRadians(angle), t->forward);
  return 1;
}

static float ClampColor(float c) {
  if (c <= 0.0) return 0;
  if (c >= 1.0) return 1.0;
  return c;
}

int SetTurtleRed(Turtle3D *t, float red) {
  t->color[0] = ClampColor(red);
  return 1;
}

int SetTurtleGreen(Turtle3D *t, float green) {
  t->color[1] = ClampColor(green);
  return 1;
}

int SetTurtleBlue(Turtle3D *t, float blue) {
  t->color[2] = ClampColor(blue);
  return 1;
}

int SetTurtleAlpha(Turtle3D *t, float alpha) {
  t->color[3] = ClampColor(alpha);
  return 1;
}
