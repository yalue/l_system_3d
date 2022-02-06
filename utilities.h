#ifndef OPENGL_TUTORIAL_UTILITIES_H
#define OPENGL_TUTORIAL_UTILITIES_H
#ifdef __cplusplus
extern "C" {
#endif

// Returns 0 if any OpenGL errors are detected. Otherwise, prints the errors
// and returns nonzero.
int CheckGLErrors(void);

// Reads the file with the entire given name to a NULL-terminated buffer of
// bytes. Returns NULL on error. The caller is responsible for freeing the
// returned buffer when it's no longer needed.
char* ReadFullFile(const char *path);

// Replaces all instances of the string match in input with r. Returns a new
// null-terminated string, or NULL on error. The returned string must be freed
// by the caller when no longer needed. Even if no replacements are made, this
// will return a new copy of the input string.
char* StringReplace(const char *input, const char *match, const char *r);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // OPENGL_TUTORIAL_UTILITIES_H

