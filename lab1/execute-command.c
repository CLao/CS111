// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

int
command_status (command_t c)
{
  return c->status;
}

/* execute_command_r
 * Recursive implementation of execute_command.
 * Returns 0 if execution succeded, else
 * returns an error code < 0.
 */
int execute_command_r (command_t c, int time_travel)
{
	if(!time_travel)
	{
		int errorStatus;
		pid_t pid;
		pid_t andPid;
		int status;
		int andStatus;
		char** argv;
		int seq;
		int errorC;
		errorC = 0;
		int fd[2] = {0, 1};
		pid = fork();
		if(pid < 0)
		{
			error (1, 0, "Fork error");
		}
		if (pid > 0)
		{
			if(waitpid(pid, &status, 0) < 0)
			{
				return status;
				error(status, 0, "Child Process sdFailed");
			}
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			{
				return status;
				error(status, 0, "Child Process Failed");
			}
			
			return 0;
		}
		if (pid == 0)
		{
			switch(c->type){
				case SIMPLE_COMMAND: 
					argv = c->u.word;
					errorStatus = execvp(c->u.word[0], argv);
					break;
				case SEQUENCE_COMMAND:
					execute_command_r(c->u.command[0], 0);
					if(c->u.command[1] != NULL)
						execute_command_r(c->u.command[1], 0);
					break;
				case AND_COMMAND:				
					errorC = execute_command_r(c->u.command[0], 0);
					if (errorC) { return errorC; }
					errorC = execute_command_r(c->u.command[1], 0);
					if (errorC) { return errorC; }
					//if(c->u.command[1]!=NULL)
					//	execute_command(c->u.command[1], 0);
						
					/*andPid = fork();
					if(andPid>0){
						if(waitpid(andPid, &andStatus, 0) < 0)
							error(1,0,"a&&b: a failedssss");
						else if (WEXITSTATUS(andStatus) != 0)
							error(1,0, "a&&b: a failed");
						else
						{
							printf("whrtetgd");
							execute_command(c->u.command[1],0);
						}
					}
					else if(andPid == 0)
						execute_command(c->u.command[0],0);
					else if(andPid <0)
						error(1, 0, "AND failed to fork");*/				
					break;
				case OR_COMMAND:				
					errorC = execute_command_r(c->u.command[0], 0);
					if (errorC)
					{
						errorC = execute_command_r(c->u.command[1], 0);
						if (errorC) { return errorC; }
						else return 0;
					}
					return 0;
					/*andPid = fork();
					if(andPid>0){
						if(waitpid(andPid, &andStatus, 0) < 0)
							error(1,0,"a||b: a failedssss");
						else if (WEXITSTATUS(andStatus) != 0){
							error(1,0, "a||b: a failed");
							execute_command(c->u.command[1],0);
						}
						else
						{
							printf("whrtetgd");
							execute_command(c->u.command[1],0);
						}
					}
					else if(andPid == 0)
						execute_command(c->u.command[0],0);
					else if(andPid <0)
						error(1, 0, "AND failed to fork");*/				
					break;
				case PIPE_COMMAND:
					//NOT CORRECT IMPLEMENTATION					
					pipe(fd);
					
					
			case SUBSHELL_COMMAND:
				execute_command(*(c->u.command), time_travel);
				break;
			default:
			//	execute_command(c->u.command[0],time_travel);
			//	execute_command(c->u.command[1],time_travel);
				break;
			}	
		}
	}
  else error (1, 0, "Time travel command execution not yet implemented");
  
  return 0;
}

void
execute_command (command_t c, int time_travel)
{
	int status;
	status = execute_command_r (c, time_travel);
	
	if (!status) { error(status, 0, "Child Process Failed"); }
}
