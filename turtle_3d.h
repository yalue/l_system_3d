#ifndef TURTLE_3D_H
#define TURTLE_3D_H
#include <cglm/cglm.h>
#include <stdint.h>
#include "l_system_mesh.h"

// The length per edge of the centered cube that the turtle's resulting mesh
// is scaled to fit into.
#define MESH_CUBE_SIZE (4.0)

// Holds the turtle's position and orientation information.
typedef struct {
  // The turtle's current location.
  vec3 position;
  // The turtle's previous location. Can be arbitrary if there is no previous
  // position.
  vec3 prev_position;
  // This determines the turtle's current orientation. Must always be
  // normalized and orthogonal.
  vec3 forward;
  vec3 up;
} TurtlePosition;

// Holds a stack of turtle positions.
typedef struct {
  // The number of positions currently in the stack.
  uint32_t size;
  // The max size of the buffer before it needs to be expanded.
  uint32_t capacity;
  // The actual buffer being used as a stack.
  TurtlePosition *buffer;
} PositionStack;

// Holds a stack of turtle colors.
typedef struct {
  uint32_t size;
  uint32_t capacity;
  // A buffer of (capacity * 4) floats
  float *buffer;
} ColorStack;

// Holds the state of a 3D "turtle" that follows instructions relative to
// itself in 3D space.
typedef struct {
  TurtlePosition p;
  // The current with which the turtle draws lines.
  vec4 color;

  // Defines a cube bounding the turtle's entire path so far. This *must* be
  // well-formed, i.e. each min component must be less than each max component.
  // The one exception to this is if no vertices have been output yet. Not
  // intended to be modified directly, instead it is modified by the
  // drawing functions.
  vec3 min_bounds;
  vec3 max_bounds;

  // The list of vertices generated by the turtle. Not intended to be modified
  // directly. (Instead use AppendSegment within drawing functions.)
  MeshVertex *vertices;
  uint32_t vertex_count;
  uint32_t vertex_capacity;
  PositionStack position_stack;
  ColorStack color_stack;
} Turtle3D;

// Allocates a new turtle, at position 0, 0, 0, with no vertices. Returns NULL
// on error. The turtle starts out facing right, with up in the positive Y
// direction.
Turtle3D* CreateTurtle3D(void);

// Resets the turtle to its original position (0, 0, 0) and orientation, facing
// right. Clears the list of all generated vertices.
void ResetTurtle3D(Turtle3D *t);

// Destroys the given turtle, freeing any resources and vertices. The given
// pointer is no longer valid after this returns.
void DestroyTurtle3D(Turtle3D *t);

// Sets the model and normal matrices for the turtle's vertices, and the
// location offset vector (to be added to each vertex location) that will
// center the mesh on 0, 0, 0. Returns 0 on error. Also returns the linear
// scale by which the model matrix scales up or down.
int SetTransformInfo(Turtle3D *t, mat4 model, mat3 normal, vec3 loc_offset,
    float *size_scale);

// The type used by all turtle-movement instructions. Some instructions may not
// use the floating-point parameter. All instructions must return 0 on error
// or nonzero on success.
typedef int (*TurtleInstruction)(Turtle3D *t, float param);

// Instrcts the turtle to move forward by the given distance, drawing a
// segment.
int MoveTurtleForward(Turtle3D *t, float distance);

// Moves the turtle forward by the given distance, but does not draw a segment.
int MoveTurtleForwardNoDraw(Turtle3D *t, float distance);

// Instructs the turtle to rotate about its upward axis by the given angle.
// Does not change the turtle's position. The angle is in degrees.
int RotateTurtle(Turtle3D *t, float angle);

// Instructs the turtle to rotate about its right-facing axis by the given
// angle. Does not change the turtle's position. The angle is in degrees.
int PitchTurtle(Turtle3D *t, float angle);

// Instructs the turtle to rotate about its forward-facing axis by the given
// angle. Does not change the turtle's position. The angle is in degrees.
int RollTurtle(Turtle3D *t, float angle);

// Set the corresponding channels in the turtle's current output color.
int SetTurtleRed(Turtle3D *t, float red);
int SetTurtleGreen(Turtle3D *t, float green);
int SetTurtleBlue(Turtle3D *t, float blue);
int SetTurtleAlpha(Turtle3D *t, float alpha);

// Saves the turtle's position on a stack. Returns 0 on error. The argument is
// ignored.
int PushTurtlePosition(Turtle3D *t, float ignored);

// Pops the turtle's position off the stack of positions. Returns 0 on error.
// It's an error if this instruction is issued when the stack is empty. The
// argument is ignored.
int PopTurtlePosition(Turtle3D *t, float ignored);

// Basically the same as PushTurtlePosition/PopTurtlePosition, but saves and
// restores the turtle's current color.
int PushTurtleColor(Turtle3D *t, float ignored);
int PopTurtleColor(Turtle3D *t, float ignored);

#endif  // TURTLE_3D_H

