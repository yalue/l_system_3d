#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include "utilities.h"

static void PrintGLErrorString(GLenum error) {
  switch (error) {
  case GL_NO_ERROR:
    printf("No OpenGL error");
    return;
  case GL_INVALID_ENUM:
    printf("Invalid enum");
    return;
  case GL_INVALID_VALUE:
    printf("Invalid value");
    return;
  case GL_INVALID_OPERATION:
    printf("Invalid operation");
    return;
  // GL_STACK_OVERFLOW is undefined; bug in GLAD
  case 0x503:
    printf("Stack overflow");
    return;
  // GL_STACK_UNDERFLOW is undefined; bug in GLAD
  case 0x504:
    printf("Stack underflow");
    return;
  case GL_OUT_OF_MEMORY:
    printf("Out of memory");
    return;
  default:
    break;
  }
  printf("Unknown OpenGL error: %d", (int) error);
}

int CheckGLErrors(void) {
  GLenum error = glGetError();
  if (error == GL_NO_ERROR) return 1;
  while (error != GL_NO_ERROR) {
    printf("Got OpenGL error: ");
    PrintGLErrorString(error);
    printf("\n");
    error = glGetError();
  }
  return 0;
}

char* ReadFullFile(const char *path) {
  long size = 0;
  char *to_return = NULL;
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("Failed opening %s: %s\n", path, strerror(errno));
    return NULL;
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    printf("Failed seeking end of %s: %s\n", path, strerror(errno));
    fclose(f);
    return NULL;
  }
  size = ftell(f);
  if (size < 0) {
    printf("Failed getting size of %s: %s\n", path, strerror(errno));
    fclose(f);
    return NULL;
  }
  if ((size + 1) < 0) {
    printf("File %s too big.\n", path);
    fclose(f);
    return NULL;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    printf("Failed rewinding to start of %s: %s\n", path, strerror(errno));
    fclose(f);
    return NULL;
  }
  // Use size + 1 to null-terminate the data.
  to_return = (char *) calloc(1, size + 1);
  if (!to_return) {
    printf("Failed allocating buffer to hold contents of %s.\n", path);
    fclose(f);
    return NULL;
  }
  if (fread(to_return, size, 1, f) < 1) {
    printf("Failed reading %s: %s\n", path, strerror(errno));
    fclose(f);
    free(to_return);
    return NULL;
  }
  fclose(f);
  return to_return;
}

char* StringReplace(const char *input, const char *match, const char *r) {
  char *input_pos = NULL;
  const char *prev_input_pos = NULL;
  char *output_pos = NULL;
  char *to_return = NULL;
  int occurrences = 0;
  int new_length = 0;
  int unchanged_length = 0;
  int original_length = strlen(input);
  int match_length = strlen(match);
  int sub_length = strlen(r);
  // Start by counting the number of occurrences so we can preallocate an
  // output buffer of the correct size.
  input_pos = strstr(input, match);
  while (input_pos != NULL) {
    occurrences++;
    input_pos += match_length;
    input_pos = strstr(input_pos, match);
  }
  if (occurrences == 0) return strdup(input);

  // Allocate the new string's memory, including space for the null char
  new_length = original_length - (occurrences * match_length) +
    (occurrences * sub_length);
  if (new_length < 0) return NULL;
  to_return = (char *) calloc(new_length + 1, sizeof(char));
  if (!to_return) return NULL;
  if (new_length == 0) return to_return;

  // Build the string with replacements.
  output_pos = to_return;
  prev_input_pos = input;
  input_pos = strstr(input, match);
  while (input_pos != NULL) {
    // Copy the part of the string before the replacement to the output.
    unchanged_length = ((uintptr_t) input_pos) - ((uintptr_t) prev_input_pos);
    memcpy(output_pos, prev_input_pos, unchanged_length);
    output_pos += unchanged_length;
    // Copy the replacement to the output.
    memcpy(output_pos, r, sub_length);
    output_pos += sub_length;
    // Advance past the matched string in the input and find the next
    // occurrence.
    input_pos += match_length;
    prev_input_pos = input_pos;
    input_pos = strstr(input_pos, match);
  }

  // Finally, copy any remaining input past the final match.
  unchanged_length = original_length - (((uintptr_t) prev_input_pos) -
    ((uintptr_t) input));
  memcpy(output_pos, prev_input_pos, unchanged_length);

  return to_return;
}

