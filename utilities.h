#ifndef OPENGL_TUTORIAL_UTILITIES_H
#define OPENGL_TUTORIAL_UTILITIES_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// Returns 0 if any OpenGL errors are detected. Otherwise, prints the errors
// and returns nonzero.
int CheckGLErrors(void);

// Reads the file with the entire given name to a NULL-terminated buffer of
// bytes. Returns NULL on error. The caller is responsible for freeing the
// returned buffer when it's no longer needed.
uint8_t* ReadFullFile(const char *path);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // OPENGL_TUTORIAL_UTILITIES_H
