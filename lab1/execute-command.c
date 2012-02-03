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
#include "string.h"

typedef struct dep_node* dep_node_t;
typedef struct dep_graph *dep_graph_t;

int debugmode = 0;

int
command_status (command_t c)
{
  return c->status;
}

int mem_need_grow (void* ptr, size_t* len, size_t obSize, size_t mem)
{
	if (obSize * (*len + 1) > mem)
	{
		return 1;
	}
	return 0;
}

//NOTE: all dependency graph objects re-use command pointers that
// already exist and get freed independently of their use in the
// dependency graph.

/*Dependency graph node
 * Contains a pointer to a command, an array of pointers to
 * dependency nodes which must be executed before the current node's
 * command can be executed, and an array of pointers to dependency
 * nodes which depend on this one.
 * */
struct dep_node
{
	command_t c;
	dep_node_t* before;
	size_t bef_size;
	
	dep_node_t* after;
	size_t aft_size;

	char** in;
	char** out;
	char** args;
};

// Initialze a node with a command_t
void init_node (dep_node_t n, command_t c)
{
	n->c = c;
	n->before = checked_malloc (sizeof (dep_node_t));
	n->bef_size = 0;
	n->after = checked_malloc (sizeof (dep_node_t));
	n->aft_size = 0;
	n->in = checked_malloc (sizeof (char *)*512);
	n->out = checked_malloc (sizeof (char*)*512);
	n->args = checked_malloc (sizeof (char*)*512);
}

void destroy_node (dep_node_t n)
{
	// Free all pointers and stuff
}

/*Dependency graph
 * Contains two arrays of dependency nodes pointers;
 * one array of nodes which can be executed at any time,
 * and one array of nodes which have unresolved dependencies
 * */
struct dep_graph 
{
	// Array of executable nodes pointers
	dep_node_t* exec;
	size_t execSize;
	size_t execMem;
	
	// Array of node pointers with dependencies
	dep_node_t* dep;
	size_t depSize;
	size_t depMem;
};

// Adds a dependency node to the graph
int dep_graph_add (dep_graph_t d, dep_node_t n, int whichArray)
{
	switch (whichArray)
	{
	case 0: // exec
		if (d->execMem < (sizeof (dep_node_t) * (d->execSize + 1)))
			{ d->exec = checked_grow_alloc (d->exec, &(d->execMem)); }
		d->exec[d->execSize] = n;
		d->execSize++;
		break;
		
	case 1: // dep
		if (d->depMem < (sizeof (dep_node_t) * (d->depSize + 1)))
			{ d->dep = checked_grow_alloc (d->dep, &(d->depMem)); }
		d->dep[d->depSize] = n;
		d->depSize++;
		break;
	default:
			return -1;
	}
	
	return 0;
}

// Deletes AND FREES an executable node from the dependency graph
int remove_e (dep_graph_t d, size_t epos)
{
	if (epos >= d->execSize) { return -1; }
	free (d->exec[epos]);
	size_t it;
	for (it = epos; it < (d->execSize - 1); it++)
	{
		d->exec[it] = d->exec[it + 1];
	}
	d->execSize--;
	
	return 0;
}

// Moves a node from the array of executable nodes to the array of
//	nodes with dependencies
int move_e_to_d (dep_graph_t d, size_t epos)
{
	if (epos >= d->execSize) { return -1; }
	
	int err = dep_graph_add (d, d->exec[epos], 1);
	d->depSize++;
	
	size_t it;
	for (it = epos; it < (d->execSize - 1); it++)
	{
		d->exec[it] = d->exec[it + 1];
	}
	
	d->depSize--;
	
	return 0;
}

// Moves a node from the array of nodes with dependencies to the array of
//	independent nodes
int move_d_to_e (dep_graph_t d, size_t dpos)
{
	if (dpos >= d->depSize) { return -1; }
	
	int err = dep_graph_add (d, d->dep[dpos], 0);
	d->execSize++;
	
	size_t it;
	free(d->dep[dpos]);
	for (it = dpos; it < (d->depSize - 1); it++)
	{
		d->dep[it] = d->dep[it + 1];
	}
	
	d->depSize--;
	
	return 0;
}

// Initialze dependency graph
void init_dep_graph (dep_graph_t d)
{
	d->exec = checked_malloc (sizeof (dep_node_t));
	d->dep = checked_malloc (sizeof (dep_node_t));
	d->execSize = 0;
	d->execMem = sizeof (dep_node_t);
	d->depSize = 0;
	d->depMem = sizeof (dep_node_t);;
}

/*make_dep_graph
 * Creates a dependency graph object from a command stream
 * */
char** find_args(command_t c, size_t* aSize)
{
	char** args;
	char** args2;
	size_t a1Mem;
	a1Mem = (sizeof (char*) * 512);
	size_t a2Mem;
	a2Mem = (sizeof (char*));
	args = checked_malloc(a1Mem);
	//args2 = checked_malloc(a2Mem);
	size_t a1Size;
	a1Size = 0;
	size_t a2Size;
	a2Size = 0;
	int pos = 0;
	int innerPos = 1;
	switch (c->type){
    		case SEQUENCE_COMMAND:
			args = find_args(c->u.command[0], aSize); 
			break;
		case SIMPLE_COMMAND:
			while (c->u.word[innerPos] != NULL)
			{
				if (mem_need_grow (args, &a1Size, sizeof (char*), a1Mem))
					{ args = checked_grow_alloc (args, &a1Size); }
				args[pos] = c->u.word[innerPos]; pos++; innerPos++; a1Size++;
			}
			break;
		case SUBSHELL_COMMAND: break;
		default: 
			args = find_args(c->u.command[0], &a1Size);
			args2 = find_args(c->u.command[1], &a2Size);
			innerPos = 0;
			while(args[pos]!=NULL)
			{
				pos++;
			}       
			pos++;
			while(args2[innerPos]!=NULL)
			{
				args[pos] = args2[innerPos];
				pos++; innerPos++;
			}
			break;   
	}
	*aSize = a1Size;
	return args;
}

char** find_I(command_t c)
{
	char** args;
	char** args2;
	args = checked_malloc((sizeof(char*))*512);
	args2 = checked_malloc((sizeof(char*))*512);
	size_t a1Size;
	size_t a2Size;
	int pos = 0;
	int innerPos = 0;
	switch (c->type){
    		case SEQUENCE_COMMAND:
			args = find_args(c->u.command[0], &a1Size); 
			break;
		case SIMPLE_COMMAND:
			if (c->input != NULL)
			{args[pos] = c->input;pos++;}
			//if (c->output != NULL)
			//{args[pos] = c->output;pos++;}
			break;
		case SUBSHELL_COMMAND: break;
		default: 
			args = find_args(c->u.command[0], &a1Size);
			args2 = find_args(c->u.command[1], &a2Size);
			innerPos = 0;
			while(args[pos]!=NULL)
			{
				pos++;
			}       
			pos++;
			while(args2[innerPos]!=NULL)
			{
				args[pos] = args2[innerPos];
				pos++; innerPos++;
			}       
			break;   
	}
	return args;
}

char** find_O(command_t c)
{
	char** args;
	char** args2;
	args = checked_malloc((sizeof(char*))*512);
	args2 = checked_malloc((sizeof(char*))*512);
	size_t a1Size;
	size_t a2Size;
	int pos = 0;
	int innerPos = 0;
	switch (c->type){
    		case SEQUENCE_COMMAND:
			args = find_args(c->u.command[0], &a1Size); 
			break;
		case SIMPLE_COMMAND:
			//if (c->input != NULL)
			//{args[pos] = c->input;pos++;}
			if (c->output != NULL)
			{args[pos] = c->output;pos++;}
			break;
		case SUBSHELL_COMMAND: break;
		default: 
			args = find_args(c->u.command[0], &a1Size);
			args2 = find_args(c->u.command[1], &a2Size);
			innerPos = 0;
			while(args[pos]!=NULL)
			{
				pos++;
			}       
			pos++;
			while(args2[innerPos]!=NULL)
			{
				args[pos] = args2[innerPos];
				pos++; innerPos++;
			}       
			break;   
	}
	return args;
}


dep_graph_t make_dep_graph (command_stream_t s)
{

	dep_graph_t ret_d;
	ret_d = NULL;
	size_t it;
	it = 0;
	size_t iter = 0;
	command_t comm;
	ret_d = checked_malloc (sizeof (struct dep_graph));
	init_dep_graph (ret_d);

	char** args;
	args = checked_malloc((sizeof(char*))*512);
	char** I;
	I = checked_malloc((sizeof(char*))*512);
	char** O;
	O = checked_malloc((sizeof(char*))*512);

	int position = 0;
	int innerpos = 0;
	for (it = 0; it < s->cLen; it++)
	{			
		comm = s->cArray[it];
		dep_node_t n;
		n = checked_malloc (sizeof (struct dep_node));
		init_node(n, comm);
		while(iter < ret_d->execSize || iter < ret_d->depSize)
		{	
			size_t aSize = 0;
			args = find_args(comm, &aSize);
			I = find_I(comm);
			O = find_O(comm);
			n->in = I;
			n->out = O;
			n->args = args;
			//printf("iter: %d\n", iter);
			//error(1,0,"dick");
			//if(comm->input!= NULL)
			//	printf("%s\n", comm->input);

			if(O != NULL && iter < ret_d->execSize){//printf("O\n");
				while(O[position]!=NULL){
					while(ret_d->exec[iter]->in[innerpos]!=NULL){
						if(strcmp(ret_d->exec[iter]->in[innerpos], O[position])==0)
						{
							n->before[n->bef_size] = ret_d->exec[iter];
							n->bef_size +=1;
						}
						innerpos+=1;
					}
					position +=1;
				}
			}
			
			if(O != NULL && iter < ret_d->depSize){//printf("O\n");
				while(O[position]!=NULL){//printf("O2\n");
					while(ret_d->dep[iter]->in[innerpos]!=NULL){
						if(strcmp(ret_d->dep[iter]->in[innerpos], O[position])==0)
						{
							n->before[n->bef_size] = ret_d->dep[iter];
							n->bef_size +=1;
						}
						innerpos+=1;
					}
					position +=1;
				}
			}

			if(I != NULL && iter < ret_d->depSize){
				while(I[position]!=NULL){
					while(ret_d->dep[iter]->out[innerpos]!=NULL){
						if(strcmp(ret_d->dep[iter]->out[innerpos], I[position])==0)
						{
							n->before[n->bef_size] = ret_d->dep[iter];
							n->bef_size +=1;
						}
						innerpos+=1;
					}
					position +=1;
				}
			}
			if(I != NULL && iter < ret_d->execSize){
				while(I[position]!=NULL){
					while(ret_d->exec[iter]->out[innerpos]!=NULL){
						if(strcmp(ret_d->exec[iter]->out[innerpos], I[position])==0)
						{
							n->before[n->bef_size] = ret_d->exec[iter];
							n->bef_size +=1;
						}
						innerpos+=1;
					}
					position +=1;
				}
			}

			if(args != NULL && iter < ret_d->depSize){
				while(args[position]!=NULL){
					while(ret_d->dep[iter]->out[innerpos]!=NULL){
						if(strcmp(ret_d->dep[iter]->out[innerpos], args[position])==0)
						{
							n->before[n->bef_size] = ret_d->dep[iter];
							n->bef_size +=1;
						}
						innerpos+=1;
					}
					position +=1;
				}
			}
			
			if(args != NULL && iter < ret_d->execSize){
				while(args[position]!=NULL){
					while(ret_d->exec[iter]->out[innerpos]!=NULL){
						if(strcmp(ret_d->exec[iter]->out[innerpos], args[position])==0)
						{
							n->before[n->bef_size] = ret_d->exec[iter];
							n->bef_size +=1;
						}
						innerpos+=1;
					}
					position +=1;
				}
			}

			/*if(comm->input != NULL && iter < ret_d->depSize){
				if(ret_d->dep[iter]->c->output == comm->input)
				{
					n->before[n->bef_size] = ret_d->dep[iter];
					n->bef_size+=1;
					//ret_d->dep[iter]->after[ret_d->dep[iter]->aft_size] = n;
					//ret_d->dep[iter]->aft_size++;
				}
			}*/
		position = 0;
		innerpos = 0;
		iter +=1;
		}

		if(n->bef_size > 0){if (debugmode) printf("befsize = %zu\n", n->bef_size);
			dep_graph_add(ret_d, n, 1);}
		else{ if (debugmode) printf("befsize = %zu\n", n->bef_size);
			dep_graph_add(ret_d, n, 0);}
		iter = 0;
		
		//TODO:
		// Determine which nodes already seen depend on this one.
		// Look through the independent nodes.
			// If a match is found, add this node as a dependency
			// And move it to the node with dependencies list
		// Look through the nodes with dependencies.
			// If a match is found, add this node as a dependency

		
	}
	
	return ret_d;
}
 
void destroy_dep_graph (dep_graph_t d)
{
	//TODO:
	//Free all pointers and stuff
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
	size_t iter = 0;
	size_t innerIter = 0;
	int status;
	int addstat;
	int dpos;
	if (d->execSize>0)
	{
		int pid;
		pid = fork();
		if (pid == 0){
				execute_command_r(d->exec[0]->c, 0);
		}else if (pid<0)
			error(1, 0, "Child process failed to fork");
		else if (pid>0)
		{	
			while(iter < d->depSize)
			{
				if (debugmode) printf("hello\n");
				for(/*innerIter*/; innerIter < d->dep[iter]->bef_size; innerIter++)
				{
					if(d->dep[iter]->before[innerIter] == d->exec[0])
					{
						for(dpos = innerIter; d->dep[dpos+1]!=NULL; dpos++)
						{
							d->dep[dpos] = d->dep[dpos+1];
						}
						d->dep[iter]->bef_size -= 1;
					}
				}
				if (d->dep[iter]->bef_size == 0)
				{
					if (debugmode) printf("changing stuffs\n");
					addstat = move_d_to_e(d, iter);
					if (debugmode) printf("addstat: %d\n", addstat);
				}
				iter++;
			}
			//printf("\n");
			remove_e(d, 0);
			if (d->execSize == 0)
			{
				while (waitpid(-1,&status,0)>0)
					continue;
				exit(0);
			}
			execute_dep_graph(d);
		}
	}
	
	//TODO:
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
