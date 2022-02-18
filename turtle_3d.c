#include <cglm/cglm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "l_system_mesh.h"
#include "turtle_3d.h"

// The number of vertices the turtle initially allocates space for.
#define INITIAL_TURTLE_CAPACITY (1024)

// The initial capacity of the turtle's position stack.
#define INITIAL_STACK_CAPACITY (32)

// The minimum spacing (squared) between two subsequent positions for them to
// be considered identical.
#define MIN_POSITION_DIST (1.0e-6)

#define PI (3.1415926536)

// Initializes the given stack of turtle positions. Returns 0 on error.
static int InitializePositionStack(PositionStack *s) {
  s->size = 0;
  s->buffer = (TurtlePosition *) calloc(INITIAL_STACK_CAPACITY,
    sizeof(TurtlePosition));
  if (!s->buffer) return 0;
  s->capacity = INITIAL_STACK_CAPACITY;
  return 1;
}

// Cleans up the stack of positions. Doesn't free s itself; just the buffer it
// wraps.
static void FreePositionStack(PositionStack *s) {
  free(s->buffer);
  memset(s, 0, sizeof(*s));
}

static int InitializeColorStack(ColorStack *s) {
  s->size = 0;
  s->buffer = (float *) calloc(INITIAL_STACK_CAPACITY, 4 * sizeof(float));
  if (!s->buffer) return 0;
  s->capacity = INITIAL_STACK_CAPACITY;
  return 1;
}

static void FreeColorStack(ColorStack *s) {
  free(s->buffer);
  memset(s, 0, sizeof(*s));
}

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
  if (!InitializePositionStack(&(to_return->position_stack))) {
    printf("Failed initializing stack of turtle positions.\n");
    free(to_return->vertices);
    free(to_return);
    return NULL;
  }
  if (!InitializeColorStack(&(to_return->color_stack))) {
    printf("Failed initializing stack of turtle colors.\n");
    free(to_return->vertices);
    free(to_return->position_stack.buffer);
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
  glm_vec3_zero(t->p.position);
  glm_vec3_zero(t->p.prev_position);
  // Face in positive X direction, with up towards positive Y.
  glm_vec3_zero(t->p.forward);
  t->p.forward[0] = 1;
  glm_vec3_zero(t->p.up);
  t->p.up[1] = 1;
  // The bounding cube is empty and ill-defined to begin with.
  glm_vec3_zero(t->min_bounds);
  glm_vec3_zero(t->max_bounds);
  // We don't reallocate the array or zero it out here so we can reuse the
  // turtle to draw another iteration without needing to clear all the
  // vertices.
  t->vertex_count = 0;
  // Clear the stack. As with the vertex array, don't free it, though.
  t->position_stack.size = 0;
  t->color_stack.size = 0;
}

void DestroyTurtle3D(Turtle3D *t) {
  if (!t) return;
  free(t->vertices);
  FreePositionStack(&(t->position_stack));
  FreeColorStack(&(t->color_stack));
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
    glm_scale_uni(model, MESH_CUBE_SIZE / max_axis);
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
static int IncreaseCapacityIfNeeded(Turtle3D *t) {
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
    printf("Unable to increase number of vertices: out of memory.\n");
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
  if (!IncreaseCapacityIfNeeded(t)) return 0;
  // If the two positions are too close together, then we'll just set the
  // direction vector to the turtle's current direction. Otherwise, the
  // direction in the line segment points from the previous point to the
  // current position.
  glm_vec3_sub(t->p.position, t->p.prev_position, direction);
  if (glm_vec3_norm2(direction) < MIN_POSITION_DIST) {
    glm_vec3_copy(t->p.forward, direction);
  } else {
    glm_vec3_normalize(direction);
  }
  v = t->vertices + t->vertex_count;
  glm_vec3_copy(t->p.prev_position, v->location);
  glm_vec3_copy(direction, v->forward);
  glm_vec3_copy(t->p.up, v->up);
  glm_vec4_copy(t->color, v->color);
  // The only difference between the two vertices in the line segment is the
  // position.
  *(v + 1) = *v;
  glm_vec3_copy(t->p.position, (v + 1)->location);
  t->vertex_count += 2;
  return 1;
}

// Updates min_bounds and max_bounds to contain the turtle's current position.
static void UpdateBounds(Turtle3D *t) {
  float *p = t->p.position;
  glm_vec3_copy(t->p.position, p);
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
  glm_vec3_copy(t->p.position, t->p.prev_position);
  glm_vec3_scale(t->p.forward, distance, change);
  glm_vec3_add(t->p.position, change, t->p.position);
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
  glm_vec3_rotate(t->p.forward, ToRadians(angle), t->p.up);
  return 1;
}

int PitchTurtle(Turtle3D *t, float angle) {
  vec3 right;
  glm_vec3_cross(t->p.forward, t->p.up, right);
  glm_vec3_rotate(t->p.up, ToRadians(angle), right);
  glm_vec3_rotate(t->p.forward, ToRadians(angle), right);
  return 1;
}

int RollTurtle(Turtle3D *t, float angle) {
  glm_vec3_rotate(t->p.up, ToRadians(angle), t->p.forward);
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

int PushTurtlePosition(Turtle3D *t, float ignored) {
  PositionStack *s = &(t->position_stack);
  TurtlePosition *new_buf = NULL;
  uint32_t new_cap = 0;
  // Expand capacity if necessary.
  if (s->size >= s->capacity) {
    new_cap = s->capacity * 2;
    if (new_cap < s->capacity) {
      printf("Turtle position stack overflow.\n");
      return 0;
    }
    new_buf = (TurtlePosition *) realloc(s->buffer, new_cap *
      sizeof(TurtlePosition));
    if (!new_buf) {
      printf("Failed expanding position stack.\n");
      return 0;
    }
    s->buffer = new_buf;
    s->capacity = new_cap;
  }
  s->buffer[s->size] = t->p;
  s->size++;
  return 1;
}

int PopTurtlePosition(Turtle3D *t, float ignored) {
  PositionStack *s = &(t->position_stack);
  if (s->size == 0) {
    printf("Turtle position stack is empty.\n");
    return 0;
  }
  t->p = s->buffer[s->size - 1];
  s->size--;
  return 1;
}

int PushTurtleColor(Turtle3D *t, float ignored) {
  ColorStack *s = &(t->color_stack);
  float *new_buf = NULL;
  float *v = NULL;
  uint32_t new_cap = 0;
  if (s->size >= s->capacity) {
    new_cap = s->capacity * 2;
    if (new_cap < s->capacity) {
      printf("Turtle color stack overflow.\n");
      return 0;
    }
    new_buf = (float *) realloc(s->buffer, new_cap * 4 * sizeof(float));
    if (!new_buf) {
      printf("Failed expanding color stack.\n");
      return 0;
    }
    s->buffer = new_buf;
    s->capacity = new_cap;
  }
  v = s->buffer + (4 * s->size);
  v[0] = t->color[0];
  v[1] = t->color[1];
  v[2] = t->color[2];
  v[3] = t->color[3];
  s->size++;
  return 1;
}

int PopTurtleColor(Turtle3D *t, float ignored) {
  ColorStack *s = &(t->color_stack);
  float *v = NULL;
  if (s->size == 0) {
    printf("Turtle color stack is empty.\n");
    return 0;
  }
  s->size--;
  v = s->buffer + (4 * s->size);
  t->color[0] = v[0];
  t->color[1] = v[1];
  t->color[2] = v[2];
  t->color[3] = v[3];
  return 1;
}

