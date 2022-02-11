#ifndef PARSE_CONFIG_H
#define PARSE_CONFIG_H
#include <stdint.h>
#include "turtle_3d.h"

// An arbitrary limit on the number of actions that can be taken by a single
// character. Can probably be increased without much consequence, but my
// assumption is that most L-system definitions won't have huge lists of
// actions for a single character.
#define MAX_ACTIONS_PER_CHAR (32)

// A struct to help read the config file line-by-line. Not intended for
// external use.
typedef struct {
  char *file_content;
  char **lines;
  uint32_t line_count;
  uint32_t current_line;
} ConfigFile;

void FreeConfigFile(ConfigFile *f);

// Keeps track of the contents with which we replace a single character.
typedef struct {
  // If this is nonzero, the replacement rule is actually used. This
  // differentiates replacements with a blank string (deletions) from
  // characters that should be left as-is.
  int used;
  // The number of characters with which the char is replaced.
  int length;
  // The actual replacement chars.
  const char *replacement;
} ReplacementRule;

// Keeps track of the actions taken when processing a character in a string.
typedef struct {
  // The number of actions to carry out.
  int length;
  // The functions used to move the turtle.
  TurtleInstruction instructions[MAX_ACTIONS_PER_CHAR];
  // The arguments to pass to each corresponding function.
  float args[MAX_ACTIONS_PER_CHAR];
} ActionRule;

// Tracks the rules for the L-system generation and drawing.
typedef struct {
  ConfigFile *f;
  // The initial string in the L-system.
  const char *init;
  // The replacement rules for each char in the ascii range.
  ReplacementRule replacements[128];
  // The action rules for each char in the ascii range.
  ActionRule actions[128];
} LSystemConfig;

// Loads and parses the config file at the given path, returning an
// LSystemConfig struct. Returns NULL if any error occurs.
LSystemConfig* LoadLSystemConfig(const char *path);

// Any resources associated with the given config. The pointer becomes invalid
// after this function is called.
void DestroyLSystemConfig(LSystemConfig *c);

#endif  // PARSE_CONFIG_H

