#ifndef TURTLE_3D_H
#define TURTLE_3D_H
#include <cglm/cglm.h>
#include "l_system_mesh.h"

// Holds the state of a 3D "turtle" that follows instructions relative to
// itself in 3D space.
typedef struct {
  // The turtle's current location.
  vec3 position;
  // The turtle's previous location. Can be arbitrary if there is no previous
  // position.
  vec3 prev_position;
  // The current with which the turtle draws lines.
  vec4 color;
  // This determines the turtle's current orientation. Must always be
  // normalized and orthogonal.
  vec3 forward;
  vec3 up;

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
} Turtle3D;

// TODO: add push and pop instructions that keep the turtle's position,
// previous position, orientation, and color on a stack.

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

#endif  // TURTLE_3D_H

