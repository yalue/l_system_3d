#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_config.h"
#include "utilities.h"

// Frees the resources associated with the given file. After calling this, the
// pointer is no longer valid.
void FreeConfigFile(ConfigFile *f) {
  free(f->file_content);
  free(f->lines);
  memset(f, 0, sizeof(*f));
  free(f);
}

// Returns nonzero if c is a tab, space, or carriage return.
static int IsWhitespace(char c) {
  return (c == ' ') || (c == '\t') || (c == '\r');
}

// Returns a pointer to the first char in s that is non-whitespace. Only skips
// tabs and spaces; not newlines.
static char* SkipWhitespace(char *s) {
  while (IsWhitespace(*s)) s++;
  return s;
}

// Replaces any whitespace characters starting at s, and moving backwards.
// Stops either when a non-whitespace character is encountered, or the limit
// pointer is reached. Will not channge the char at the limit pointer. (Used to
// strip trailing spaces.)
static void ReplaceWhitespaceBackwards(char *s, char *limit) {
  while ((s != limit) && IsWhitespace(*s)) {
    *s = 0;
    s--;
  }
}

// Reads the file at the given path, and splits it into lines. Returns NULL on
// error, including if the file contains non-ASCII characters.
static ConfigFile *LoadConfigFile(const char *path) {
  ConfigFile *f = NULL;
  char *current = NULL;
  uint32_t line_index = 0;
  f = (ConfigFile *) calloc(1, sizeof(*f));
  if (!f) {
    printf("Error allocating internal config-file struct.\n");
    return NULL;
  }
  f->file_content = ReadFullFile(path);
  if (!f->file_content) {
    printf("Failed reading config file %s.\n", path);
    free(f);
    return NULL;
  }
  if (strlen(f->file_content) == 0) {
    printf("The config file %s was empty.\n", path);
    FreeConfigFile(f);
    return NULL;
  }
  // Split the file into lines. Start by counting the lines.
  current = f->file_content;
  while (*current != 0) {
    if (*current == '\n') f->line_count++;
    if (*current >= 127) {
      printf("The config file %s contains a non-ASCII character 0x%x.\n", path,
        (unsigned int) *current);
      FreeConfigFile(f);
      return NULL;
    }
    current++;
  }
  // We always end the file with a "line", which will be blank if the file ends
  // with a newline. It keeps it simpler to have at least one line.
  f->line_count++;
  f->lines = (char **) calloc(f->line_count, sizeof(char *));
  if (!f->lines) {
    printf("Failed allocating list of file lines.\n");
    FreeConfigFile(f);
    return NULL;
  }
  // Next, set the pointers to the start of each line.
  current = f->file_content;
  f->lines[0] = current;
  line_index = 1;
  while (*current != 0) {
    if (*current == '\n') {
      // Replace newlines with null chars to terminate the line strings.
      *current = 0;
      // Strip the trailing whitespace for the previous line.
      ReplaceWhitespaceBackwards(current - 1, f->lines[line_index - 1] - 1);
      // Skip leading whitespace on this line.
      f->lines[line_index] = SkipWhitespace(current + 1);
      line_index++;
    }
    current++;
  }
  return f;
}

// Gets the next line in the config file. Returns NULL if no more unread lines
// remain.
static char* GetNextLine(ConfigFile *f) {
  char *to_return = NULL;
  if (f->current_line >= f->line_count) return NULL;
  to_return = f->lines[f->current_line];
  f->current_line++;
  return to_return;
}

// Like GetNextLine, but skips blank or comment lines.
static char* GetNextNonBlankLine(ConfigFile *f) {
  char *line = GetNextLine(f);
  while (1) {
    if (line == NULL) return NULL;
    if (strlen(line) == 0) {
      line = GetNextLine(f);
      continue;
    }
    if (line[0] == '#') {
      line = GetNextLine(f);
      continue;
    }
    return line;
  }
  // Unreachable
  return NULL;
}

void DestroyLSystemConfig(LSystemConfig *c) {
  if (c->f) FreeConfigFile(c->f);
  memset(c, 0, sizeof(*c));
  free(c);
}

// Returns nonzero if c is a non-whitespace, printable, ASCII character that
// isn't a '#'.
static int IsValidLSystemChar(char c) {
  // All non-printable and whitespace ascii characters fall at or below ' '
  return (c > ' ') && (c < 127) && (c != '#');
}

// If s starts with the given token (followed by whitespace), this returns a
// pointer to the first character past the token and the following whitespace.
// Note that this will return an empty string (pointer to a null char) if the
// token takes up the entirety of s. Returns NULL if s doesn't start with the
// token, or if the token isn't followed by whitespace.
static char* ConsumeToken(const char *token, char *s) {
  int token_length = strlen(token);
  if (token_length == 0) return s;
  if (!s) return NULL;
  s = SkipWhitespace(s);
  // Return now if s doesn't start with the token.
  if (strstr(s, token) != s) return NULL;
  s += token_length;
  // Make sure the char after the token is whitespace or NULL.
  if (*s && !IsWhitespace(*s)) return NULL;
  s = SkipWhitespace(s);
  return s;
}

// Consumes input lines until the "actions" line is encountered. (If this
// returns successfully, input will be at the first line past "actions".)
// Returns 0 on error. Fills in c->replacements and c->init.
static int ParseReplacementRules(LSystemConfig *config) {
  char *current_line = NULL;
  char *replacement = NULL;
  char *tmp = NULL;
  uint8_t c;
  int init_found = 0;
  while (1) {
    replacement = NULL;
    current_line = GetNextNonBlankLine(config->f);
    if (!current_line) {
      printf("The config file didn't contain an \"actions\" line.\n");
      return 0;
    }
    // Check if we're on the "init" line.
    tmp = ConsumeToken("init", current_line);
    if (tmp) {
      if (init_found) {
        printf("The config file contains a duplicate \"init\" on line %d.\n",
          config->f->current_line);
        return 0;
      }
      if (strlen(tmp) <= 0) {
        printf("The config file's \"init\" string is empty.\n");
        return 0;
      }
      init_found = 1;
      config->init = tmp;
      continue;
    }
    // Check if we're on the "actions" line.
    tmp = ConsumeToken("actions", current_line);
    if (tmp) {
      if (strlen(tmp) > 0) {
        printf("The config contains extra text on the \"actions\" line.\n");
        return 0;
      }
      break;
    }
    // Finally, parse a replacement rule.
    c = current_line[0];
    if (!IsValidLSystemChar(c)) {
      printf("Line %d of the config: invalid char to replace (0x%x).\n",
        (int) config->f->current_line, (unsigned int) c);
      return 0;
    }
    if (config->replacements[c].used) {
      printf("Replacement rule for %c redefined on line %d of the config.\n",
        c, (int) config->f->current_line);
      return 0;
    }
    replacement = SkipWhitespace(current_line + 1);
    config->replacements[c].used = 1;
    config->replacements[c].length = strlen(replacement);
    config->replacements[c].replacement = replacement;
  }
  if (!init_found) {
    printf("The config file didn't contain an \"init\" line.\n");
    return 0;
  }
  return 1;
}

// Attempts to parse a float arg from string s. The arg must be preceded only
// by whitespace, and followed only by whitespace and/or a null char. Returns 0
// if a float can't be parsed in such a manner. Otherwise, sets *arg to the
// float and returns 1.
static int ParseFloatArg(char *s, float *arg) {
  char *end = s;
  float result;
  *arg = 0;
  s = SkipWhitespace(s);
  result = strtof(s, &end);
  if (s == end) {
    printf("Failed parsing floating-point number.\n");
    return 0;
  }
  end = SkipWhitespace(end);
  if (*end != 0) {
    printf("Got extra text following a number.\n");
    return 0;
  }
  *arg = result;
  return 1;
}

// Attempts to parse an action starting with the given token on the given line.
// Returns 0 if the line doesn't start with the token, -1 if the error is
// fatal, or 1 if the action was parsed and added OK.
static int TryParseAction(LSystemConfig *config, const char *token,
    char *line, uint8_t c, TurtleInstruction n) {
  char *next = NULL;
  ActionRule *a = NULL;
  float arg;
  next = ConsumeToken(token, line);
  // The line just didn't start with the token; not a fatal error.
  if (!next) return 0;
  if (!ParseFloatArg(next, &arg)) {
    printf("Failed parsing arg for \"%s\" on line %d of the config.\n",
      token, config->f->current_line);
    return -1;
  }
  a = config->actions + c;
  if (a->length >= MAX_ACTIONS_PER_CHAR) {
    printf("Too many actions defined for char %c. The limit is %d.\n",
      c, MAX_ACTIONS_PER_CHAR);
    return -1;
  }
  a->instructions[a->length] = n;
  a->args[a->length] = arg;
  a->length++;
  return 1;
}

// Parses the action rules from the config file. Expects to be on the line
// immediately following the line containing "actions". Returns 0 on error.
static int ParseActionRules(LSystemConfig *config) {
  char *action_names[] = {
    "move_forward",
    "move_forward_nodraw",
    "rotate",
    "yaw",
    "pitch",
    "roll",
    "set_color_r",
    "set_color_g",
    "set_color_b",
    "set_color_a",
    "push_position",
    "pop_position",
    "push_color",
    "pop_color",
  };
  TurtleInstruction action_fns[] = {
    MoveTurtleForward,
    MoveTurtleForwardNoDraw,
    RotateTurtle,
    RotateTurtle,
    PitchTurtle,
    RollTurtle,
    SetTurtleRed,
    SetTurtleGreen,
    SetTurtleBlue,
    SetTurtleAlpha,
    PushTurtlePosition,
    PopTurtlePosition,
    PushTurtleColor,
    PopTurtleColor,
  };
  char *current_line = NULL;
  uint8_t current_char = 0;
  int possible_action_count = sizeof(action_fns) / sizeof(TurtleInstruction);
  int result, i;
  while (1) {
    current_line = GetNextNonBlankLine(config->f);
    if (!current_line) break;
    if (strlen(current_line) == 1) {
      // We're switching chars.
      current_char = current_line[0];
      if (!IsValidLSystemChar(current_char)) {
        printf("Got invalid char on line %d of the config.\n",
          config->f->current_line);
        return 0;
      }
      if (config->actions[current_char].length != 0) {
        printf("Actions for char %c redefined starting on line %d.\n",
          current_char, config->f->current_line);
        return 0;
      }
      continue;
    }
    if (current_char == 0) {
      printf("Got non-char in the config file's actions sections before any "
        "char was specified (line %d).\n", config->f->current_line);
      return 0;
    }
    // We're not looking at a char so we must be looking at an instruction.
    for (i = 0; i < possible_action_count; i++) {
      result = TryParseAction(config, action_names[i], current_line,
        current_char, action_fns[i]);
      if (result < 0) return 0;
      if (result == 0) continue;
      if (result > 0) break;
      printf("Got invalid action at line %d of the config.\n",
        config->f->current_line);
      return 0;
    }
  }
  return 1;
}

LSystemConfig* LoadLSystemConfig(const char *path) {
  LSystemConfig *to_return = NULL;
  to_return = (LSystemConfig *) calloc(1, sizeof(*to_return));
  if (!to_return) {
    printf("Failed allocating L-system config data.\n");
    return NULL;
  }
  to_return->f = LoadConfigFile(path);
  if (!to_return->f) {
    DestroyLSystemConfig(to_return);
    return NULL;
  }
  if (!ParseReplacementRules(to_return)) {
    DestroyLSystemConfig(to_return);
    return NULL;
  }
  if (!ParseActionRules(to_return)) {
    DestroyLSystemConfig(to_return);
    return NULL;
  }
  return to_return;
}
