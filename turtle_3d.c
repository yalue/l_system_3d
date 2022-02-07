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

Turtle3D* CreateTurtle3D(void) {
  Turtle3D *to_return = NULL;
  to_return = (Turtle3D *) calloc(1, sizeof(*to_return));
  if (!to_return) {
    printf("Failed allocating Turtle3D struct.\n");
    return NULL;
  }
  // Start out as opaque white
  glm_vec4_one(to_return->color);
  to_return->forward[0] = 1.0;
  to_return->up[1] = 1.0;
  to_return->vertices = (MeshVertex *) calloc(INITIAL_TURTLE_CAPACITY,
    sizeof(MeshVertex));
  if (!to_return->vertices) {
    printf("Failed allocating the turtle's vertex array.\n");
    free(to_return);
    return NULL;
  }
  to_return->vertex_capacity = INITIAL_TURTLE_CAPACITY;
  return to_return;
}

void DestroyTurtle3D(Turtle3D *t) {
  if (!t) return;
  free(t->vertices);
  memset(t, 0, sizeof(*t));
  free(t);
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

int MoveTurtleForward(Turtle3D *t, float distance) {
  vec3 change;
  glm_vec3_copy(t->position, t->prev_position);
  glm_vec3_scale(t->forward, distance, change);
  glm_vec3_add(t->position, change, t->position);
  UpdateBounds(t);
  return AppendSegment(t);
}

int RotateTurtle(Turtle3D *t, float angle) {
  glm_vec3_rotate(t->forward, angle, t->up);
  return 1;
}

int PitchTurtle(Turtle3D *t, float angle) {
  vec3 right;
  glm_vec3_cross(t->forward, t->up, right);
  glm_vec3_rotate(t->up, angle, right);
  glm_vec3_rotate(t->forward, angle, right);
  return 1;
}
