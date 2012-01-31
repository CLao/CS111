// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>

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
		int childError;
		pid_t pid;
		pid_t andPid;
		int status;
		int child1;
		int child2;
		int childstatus;
		int andStatus;
		char** argv;
		int seq;
		int fdirect;
		int errorC;
		errorC = 0;
		int fd[2];
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
					if (c->input!=NULL)
					{		
					fdirect = open(c->input, O_RDONLY|O_CREAT|O_TRUNC);						
						pipe(fd);
						dup2(fd[0], fdirect);
						dup2(fd[1], 1);
					}
					if (c->output!=NULL)
					{		
					fdirect = open(c->output, O_WRONLY|O_CREAT|O_TRUNC);						
						pipe(fd);
						dup2(fd[0], 0);
						dup2(fd[1], fdirect);
					}
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
					break;
				case PIPE_COMMAND:
					//NOT CORRECT IMPLEMENTATION					
					pipe(fd);
					
					
					child1 = fork();
					if ( child1 > 0)
					{
						child2 = fork();
						if (child2 > 0)
						{
							close(fd[0]);
							close(fd[1]);
							if (waitpid(child1, &childstatus, 0)<0){
								return childstatus;
								error(childstatus, 0, "Child Process sdFailed");
							}
							if (!WIFEXITED(childstatus) || WEXITSTATUS(childstatus) != 0)

							{
								return childstatus;
								error(childstatus, 0, "Child Process Failedsadsadsa");
							}
							waitpid(child2, &childstatus, WNOHANG);
							//error(1, 0, "I don't even");
							exit(0);
						}
						if(child2 == 0) //This child writes
						{
							//error(1,0,"FAT ALBERT");
							close(fd[0]);
							dup2(fd[1], 1);
							argv = c->u.command[0]->u.word;
							childError = execvp(argv[0], argv);
							//if (errorStatus) { return errorStatus; } else return 0;
						}
					}
					if (child1==0) //This child reads
					{
						//error(1, 0, "HEY HEY HEY");
						close(fd[1]);
						dup2(fd[0], 0);
						argv = c->u.command[1]->u.word;
						childError = execvp(argv[0], argv);
						//if (errorStatus) { return errorStatus; }else return 0;
					}				
					break;
					
					
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
	//printf("%d", status);
	if (status!= 0) { error(status, 0, "Child Process Failed"); }
}
