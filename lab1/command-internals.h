// UCLA CS 111 Lab 1 command internals

#include <stddef.h>

enum command_type
  {
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
  };

// Data associated with a command.
struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or 0 if none.
  char *input;
  char *output;

  union
  {
    // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
    struct command *command[2];

    // for SIMPLE_COMMAND:
    char **word;

    // for SUBSHELL_COMMAND:
    struct command *subshell_command;
  } u;
};

// Command stream data.
struct command_stream
{
	// Index of current position in the stream.
	unsigned position;
	
	// Array of C strings. Each string is a token.
	char** tokens;
	
	// Current number of tokens.
	size_t numTokens;
	
	// Current allocated size of token array.
	size_t sizeTokens;
	
	// Whether the stream has been read into command form or not (bool).
	int done;
	
	// The entire script stored in a single command.
	command_t script;
	
	// The entire script stored as multiple commands.
	command_t* cArray;
	
	// The length of the command array.
	size_t cLen;
	
	// The size (Bytes) of the command array.
	size_t cSize;
};
