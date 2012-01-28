// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdlib.h>

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

	if(!time_travel)
	{
		int error;
		char *buffer;
		switch(c->type){
		case SIMPLE_COMMAND: 
			error = system(*(c->u.word));
			break;
		case SUBSHELL_COMMAND:
			execute_command(*(c->u.command), time_travel);
			break;
		default:
			execute_command(c->u.command[0],time_travel);
			execute_command(c->u.command[1],time_travel);
			break;
			
		}
	}
  //error (1, 0, "command execution not yet implemented");
}
