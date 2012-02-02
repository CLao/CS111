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
#include "alloc.h"

typedef struct dep_node *dep_node_t;
typedef struct dep_graph *dep_graph_t;

int
command_status (command_t c)
{
  return c->status;
}

/*Dependency graph node
 * Contains a pointer to a command, an array of pointers to
 * dependency nodes which must be executed before the current node's
 * command can be executed, and an array of pointers to dependency
 * nodes which depend on this one.
 * */
struct dep_node
{
	command_t c;
	struct dep_node_t* before;
	size_t bef_size;
	
	struct dep_node_t* after;
	size_t aft_size;
};

/*Dependency graph
 * Contains two arrays of dependency nodes;
 * one array of nodes which can be executed at any time,
 * and one array of nodes which have unresolved dependencies
 * */
struct dep_graph 
{
	// Array of executable nodes
	dep_node_t n_exec;
	size_t n_execSize;
	
	// Array of nodes with dependencies
	dep_node_t n_dep;
	size_t n_depSize;
};



/*make_dep_graph
 * Creates a dependency graph object from a command stream
 * */
 
dep_graph_t make_dep_graph (command_stream_t s)
{
	dep_graph_t return_d;
	return_d = checked_malloc (sizeof (struct dep_graph));
	return return_d;
}
 
 
/*execute_parallel
 * Time travel implementation of execute_command
 * Creates a dependency graph from the command stream
 * and uses it to execute commands in parallel if possible
 * */
int execute_command_r (command_t c, int time_travel);
void execute_parallel (command_stream_t s);

void execute_dep_graph (dep_graph_t d)
{	
	// while d has independent nodes
		// remove a node n from d
		// Fork
		
		// If child:
		// execute n
		//for each node m dependent on n
		//	pop n from m's dependency array
		//	if m's dependency array is empty
		//		insert m into d.n_exec;
	
		// If parent:
		//	recurse
}

void execute_parallel (command_stream_t s)
{
	dep_graph_t d;
	d = NULL;
	dep_node_t n;
	n = NULL;
	
	// Make the dependency graph
	d = make_dep_graph (s);
	
	// Execute the graph
	execute_dep_graph (d);
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
		int fidirect;
		int fodirect;
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
						fidirect = open(c->input, O_RDONLY | O_CREAT, 0666);						
						dup2(fidirect, 0);
						close(fidirect);
					}
					if (c->output!=NULL)
					{		
						fodirect = open(c->output, O_WRONLY |  O_TRUNC| O_CREAT, 0666);						
						dup2(fodirect, 1);
						close(fodirect);
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
					if (errorC) { exit(errorC); }//return errorC; }
					errorC = execute_command_r(c->u.command[1], 0);
					if (errorC) { exit(errorC); }//return errorC; }
					exit(0);				
					break;
				case OR_COMMAND:				
					errorC = execute_command_r(c->u.command[0], 0);
					if (errorC)
					{
						errorC = execute_command_r(c->u.command[1], 0);
						if (errorC) { exit (errorC); }//return errorC; }
						else exit(0);//return 0;
					}
					exit(0);
					//return 0;				
					break;
				case PIPE_COMMAND:				
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

							exit(0);
						}
						if(child2 == 0) //This child writes
						{

							close(fd[0]);
							dup2(fd[1], 1);
							if (c->u.command[0]->input!=NULL)
							{		
								fidirect = open(c->u.command[0]->input, O_WRONLY|O_CREAT|O_TRUNC, 0666);						
								dup2(fidirect, 0);
								close(fidirect);
							}
							argv = c->u.command[0]->u.word;
							childError = execvp(argv[0], argv);
							//if (errorStatus) { return errorStatus; } else return 0;
						}
					}
					if (child1==0) //This child reads
					{
						close(fd[1]);
						dup2(fd[0], 0);
						if (c->u.command[1]->output!=NULL)
							{		
								fodirect = open(c->u.command[1]->output, O_WRONLY|O_CREAT|O_TRUNC, 0666);						
								
								dup2(fodirect, 1);
								close(fodirect);
							}
						argv = c->u.command[1]->u.word;
						childError = execvp(argv[0], argv);
						//if (errorStatus) { return errorStatus; }else return 0;
					}				
					break;
					
					
			case SUBSHELL_COMMAND:
				if (c->input!=NULL)
				{		
					fidirect = open(c->input, O_RDONLY | O_CREAT, 0666);						
					dup2(fidirect, 0);
					close(fidirect);
				}
				if (c->output!=NULL)
				{		
					fodirect = open(c->output, O_WRONLY |  O_TRUNC| O_CREAT, 0666);						
					dup2(fodirect, 1);
					close(fodirect);
				}
				errorC = execute_command_r(c->u.subshell_command, time_travel);
				if (errorC) { return errorC; }
						else exit (0);
				break;
			default:
				error (3, 0, "No command type!");
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
