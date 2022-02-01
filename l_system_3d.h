#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <glad/glad.h>

// Maintains global data about the running program.
typedef struct {
  GLFWwindow *window;
  int window_width;
  int window_height;
  float aspect_ratio;
} ApplicationState;

// Allocates an ApplicationState struct and initializes its values to 0.
// Returns NULL on error.
ApplicationState* AllocateApplicationState(void);

// Frees an ApplicationState struct. The given pointer is no longer valid after
// this function returns.
void FreeApplicationState(ApplicationState *s);

