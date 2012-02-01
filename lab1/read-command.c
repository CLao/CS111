// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

void command_debug (command_t s)
{
	char **wrdptr;
	printf ("Type: %i\n",s->type);
	printf ("Status: %i\n",s->status);
	printf ("Input: %p\n",s->input);
	printf ("Output: %p\n",s->output);
	switch (s->type) {
		case SIMPLE_COMMAND:
			wrdptr = s->u.word;
			if (wrdptr == NULL)
			{error (15, 0, "LOL");}
			do
			{
				printf (" %s", *wrdptr);
			}
			while (*++wrdptr);
			break;
		default:
			break;
	}
}

enum char_type
  {
	WORD_CHAR,			// ASCII letters, digits, or !%+-./:@^_
	SPECIAL_TOKEN,		// ; | && || ( ) < > '\n'
	SPACE,				// isspace()
	UNSUPPORTED_TOKEN,	// All else.
  };

int returnType(int c)
{
	if (isalnum(c) || c == '!' || c == '%' || c == '+'
		|| c == '-' || c == '.' || c == '/' || c == ':' || c == '@' 
		|| c == '^' || c == '_' || c == '#') 
		return WORD_CHAR;
	
	if (c == ';' || c == '|' || c == '&' || c == '(' || c == ')' || c == '<' 
		|| c == '>' || c == '\n')
		return SPECIAL_TOKEN;
	
	if (c != '\n' && isspace (c)) return SPACE;
	
	if (c == EOF) return EOF;
	
	return UNSUPPORTED_TOKEN;
}

void destroy_command_stream (command_stream_t s);
command_t readScript (command_stream_t s);
void cDFS (command_stream_t s);

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
	// Allocate memory and set initial position.
	command_stream_t returnStream = checked_malloc (sizeof (struct command_stream));
	returnStream->position = 0;
	returnStream->tokens = checked_malloc (sizeof (char*));
	returnStream->sizeTokens = sizeof (char*);
	returnStream->done = 0;
	returnStream->script = NULL;
	returnStream->cLen = 0;
	returnStream->cSize = sizeof (command_t*);
	returnStream->cArray = NULL;
	int currentByte;
	size_t tokenSize;
	unsigned lines;
	lines = 0;
	char prevChar;
	char prevToken;
	prevChar = '\0';
	prevToken = '\0';
	int adjacentAnd;
	int adjacentOr;
	adjacentAnd = 0;
	adjacentOr = 0;
	int shouldAdd;
	shouldAdd = 1;
	int ump;
	ump = 0;
	
	
	// Read the stream into the token array.
	for (currentByte = get_next_byte (get_next_byte_argument);
			currentByte != EOF; )
	{
		shouldAdd = 1;
		char* token;
		
		// Remove leading whitespace.
		while (returnType (currentByte) == SPACE)
		{
			adjacentAnd = 0;
			adjacentOr = 0;
			prevChar = currentByte;
			currentByte = get_next_byte (get_next_byte_argument);
		}
		
		
		// If current byte is a #, throw away the entire line up to \n
		if (currentByte == '#')
		{
			adjacentAnd = 0;
			adjacentOr = 0;
			while (currentByte != '\n' && currentByte != EOF)
			{
				prevChar = currentByte;
				prevToken = '#';
				currentByte = get_next_byte (get_next_byte_argument);
			}
		}
			
		// Allocate space for a token if there is a token to take.
		if (currentByte != EOF)
		{	
			tokenSize = (sizeof (char));
			token = checked_malloc (tokenSize * 2);
			token[0] = '\0';
			token[1] = '\0';
		
		
			// Check for a special token.
			if (returnType (currentByte) == SPECIAL_TOKEN)
			{
				token[0] = currentByte;
				if (currentByte == '(')
					ump++;
				if (currentByte == ')')
					ump--;
				if (currentByte == '\n')
				{
					adjacentAnd = 0;
					adjacentOr = 0;
					if (prevChar == '<' || prevChar == '>')
					{
						free (token);
						//isFreed = 1;
						destroy_command_stream (returnStream);
						error (1, 0, "%u: Invalid newline after \'%c\'.", lines, prevChar);
					}
					if (returnType (prevChar) == WORD_CHAR || prevChar == ')')// || prevToken == '#')
					{ token[0] = ';'; }
					else {shouldAdd = 0;}
				}
				
				if (currentByte == '<' || currentByte == '>')
				{
					if (prevChar == '<' || prevChar == '>' || prevChar == '\0')
					{
						free (token);
						//isFreed = 1;
						destroy_command_stream (returnStream);
						error (1, 0, "%u: Unsupported token \"%c%c\".", lines, prevChar, currentByte);
					}
				}
				
				if (currentByte == '|')
				{
					adjacentAnd = 0;
					if (prevToken == ';' || prevToken == '\n' || prevToken == '\0')
					{
						free (token);
						//isFreed=1;
						destroy_command_stream (returnStream);
						error (1, 0, "%u: Invalid '|' after \'%c\'.", lines, prevChar);
					}
				}
				
				if (currentByte == '&')
				{
					adjacentOr = 0;
					if (prevToken == ';' || prevToken == '\n' || prevToken == '\0')
					{
						free (token);
						//isFreed=1;
						destroy_command_stream (returnStream);
						error (1, 0, "%u: Invalid '&' after \'%c\'.", lines, prevChar);
					}
				}

				if (currentByte == ';')
				{
					adjacentAnd = 0;
					adjacentOr = 0;
					if (prevToken == ';' || prevToken == '\n' || prevToken == '\0')
					{
						free (token);
						//isFreed=1;
						destroy_command_stream (returnStream);
						error (1, 0, "%u: Invalid ';' after \'%c\'.", lines, prevChar);
					}
				}
				
				prevChar = currentByte;
				prevToken = prevChar;
				currentByte = get_next_byte (get_next_byte_argument);
					
				if (prevChar == '&')
				{
					if (currentByte != '&' && adjacentAnd > 1)
					{
						free (token);
						//isFreed=1;
						destroy_command_stream (returnStream);
						error (1, 0, "%u: Invalid token \'&\'.", lines);
					}
					else adjacentAnd++;
				}
				
				if (prevChar == '|')
				{
					if (currentByte != '|' && adjacentOr > 1)
					{
						free (token);
						//isFreed = 1;
						destroy_command_stream (returnStream);
						error (1, 0, "%u: Invalid token \'|\'.", lines);
					}
					else adjacentOr++;
				}
		
			}
			
			else if (returnType (currentByte) == UNSUPPORTED_TOKEN)
			{
				free (token);
				//isFreed = 1;
				destroy_command_stream (returnStream);
				error (1, 0, "%u: Unsupported token \'%c\'.", lines, currentByte);
				return NULL;
			}
			
			// If current byte is not a special token, it's a word
			else 
			{
				adjacentAnd = 0;
				adjacentOr = 0;
				// Inner loop reads a single token word.
				while (returnType (currentByte) == WORD_CHAR)
				{
					// Reallocate the token string if more space is needed.
					if (tokenSize - (sizeof (char) * (strlen (token) + 1)) < sizeof (char))
					{
						token = checked_grow_alloc (token, &tokenSize);
					}
			
					// Add the current char to the token.
					size_t lastPosition = strlen (token);
					token[lastPosition] = currentByte;
					token[++lastPosition] = '\0';
	
					// Advance the stream.
					prevChar = currentByte;
					prevToken = prevChar;
					currentByte = get_next_byte (get_next_byte_argument);
				}
			}
		
			// Reallocate space for the token array if necessary.
			if (returnStream->sizeTokens < ((returnStream->numTokens + 1) * sizeof (char*)))
			{
				returnStream->tokens = //checked_realloc (returnStream->tokens, (returnStream->sizeTokens) += sizeof (char*));
				checked_grow_alloc (returnStream->tokens, &(returnStream->sizeTokens));
			}
		
			// Add token to token array.
			if (shouldAdd)
			{
				returnStream->tokens[returnStream->numTokens] = token;
				returnStream->numTokens++;
			}
			else if (token != NULL){ free (token); }
			
		}
	}
	if (ump != 0)
		{ error (1, 0, "0: Unmatched parentheses!");}
	returnStream->script = readScript (returnStream);
	returnStream->cArray = checked_malloc (returnStream->cSize);
	cDFS (returnStream);
	//error (9, 0, "Length of command array = %zu", returnStream->cLen);
	returnStream->position = 0;
	return returnStream;
}

void destroy_command_stream (command_stream_t s)
{
	size_t index;
	index = 0;
	char* freeToken;
	freeToken = NULL;

	// NULL check.
	if (s == NULL)
	{
		error (1, 0, "Error: Attempted to free NULL pointer.");
	}
	
	// Free each individual token in the stream.
	
	while (index < s->numTokens)
	{
		freeToken = s->tokens[index];
		free (freeToken);
		index++;
	}
	
	// Free the array of temp commands.
	free (s->tokens);
	
	// Free the sequence command array.
	if (s->cArray != NULL)
		{ free (s->cArray); }
	
	// Free the entire stream.
	free (s);
	

	
	return;
}

// Reads the next token of a command stream only if it matches a given token.
int accept (command_stream_t s, char* token)
{
	if (s->position >= s->numTokens)
	{
		return -1;
	}
	
	char* currentToken = s->tokens[s->position];
	if (strcmp (currentToken, token) == 0)
	{
		s->position++;
		return 1;
	}
	
	return 0;
}

// Returns 1 if the expected token matches, and 0 if not. -1 if error.
int expect (command_stream_t s, char* token)
{
	if (s->position >= s->numTokens)
	{
		return -1;
	}
	
	char* currentToken = s->tokens[s->position];

	if (currentToken == NULL)
	{
		return -1;
	}
	
	if (strcmp (currentToken, token) == 0)
	{
		destroy_command_stream (s);
		return 0;
	}
	
	s->position++;
	
	return 1;
}

char* peek (command_stream_t s)
{
	if (s->position >= s->numTokens)
	{
		return NULL;
	}
	
	char* nextToken;
	nextToken = s->tokens[s->position];

	return nextToken;
}

char* getWord (command_stream_t s)
{
	char* nextToken;
	nextToken = peek (s);
	char* token;
	token = checked_malloc ((strlen (nextToken) + 1) * sizeof (char));
	strcpy (token, nextToken);
	s->position++;
	return nextToken;
}

char* backup (command_stream_t s)
{
	if (s->position != 0)
		s->position--;
		
	char* nextToken = s->tokens[s->position];

	return nextToken;
}

int specialToken (char* t)
{
	if (t == NULL)
	{
		return -1;
	}
	if (strcmp (t, ";") && strcmp (t, "|") && strcmp (t, "&") && strcmp (t, "(")
		&& strcmp (t, ")") && strcmp (t, "<") && strcmp (t, ">") && strcmp (t, "\n"))
	{
		return 0;
	}
	return 1;
}

// Attempts to return an array representing a simple command
char** getCommand (command_stream_t s)
{
	char** array;
	array = NULL;
	size_t arraySize, arrayLen;
	
	if (!specialToken (peek (s)))
	{
		arraySize = sizeof (char*);
		arrayLen = 1;
		array = checked_malloc (arraySize);

	
		while (!specialToken (peek (s)))
		{
			char* word;
			word = getWord (s);
		
			if ( (arrayLen + 1) * sizeof (char*) > arraySize)
			{
				array = checked_grow_alloc (array, &arraySize);
			}
		
			array[arrayLen - 1] = word;
			arrayLen++;
		}
	} //endif
	
	return array;
}

// Given the position in the stream, find the line number of that position.
unsigned findLineNumber (command_stream_t s, unsigned pos)
{
	unsigned line;
	line = 1;
	
	unsigned index;
	for (index = 0; index < pos; index++)
	{
		if (strcmp (s->tokens[index], "\n") == 0)
		{
			line++;
		}
	}
	
	return line;
}

unsigned findLastSeq (command_stream_t s, unsigned sInit, unsigned sLimit, int *errorC)
{
	char* token;
	unsigned index;
	index = sLimit;
	while (index > sInit)
	{
		token = s->tokens[index];
		if (strcmp (token, ")") == 0)
		{
			unsigned curPos;
			curPos = index;
			unsigned ump;
			ump = 1;
			while (index >= sInit && ump != 0)
			{
				index--;
				token = s->tokens[index];
				if (strcmp (token, "(") == 0)
				{ump--;}
				if (strcmp (token, ")") == 0)
				{ump++;}
			}
			if (index > sInit)
			{
				index--;
			}
			if (ump != 0)
			{
				*errorC = 4;
				return curPos;
			}
		}
		if (strcmp (token, "(") == 0)
		{
			*errorC = -1;
			return index;
		}
		
		if (strcmp (token, "\n") == 0)
		{
			if (index != sInit)
			{
				token = s->tokens[index - 1];
				if ((strcmp (token, "<") == 0) || (strcmp (token, ">") == 0))
				{
					*errorC = 1;
					return index;
				}
				
				else if (specialToken (token))
				{
					index = findLastSeq (s, sInit, index - 1, errorC);
					return index;
				}
			}
			
			if (index != (sLimit))
			{
				token = s->tokens[index + 1];
				if ((strcmp (token, "(") != 0) && (strcmp (token, ")") != 0)
					&& strcmp (token, "\n") != 0 && specialToken (token))
				{
					//printf ("%u", specialToken (token));
					*errorC = 1;
					return index;
				}
			}
			
			return index;
		}
		if (strcmp (token, ";") == 0)
		{
			return index;
		}
		
		index--;
	} // End while.
	
	*errorC = -1;
	return 0;
}

unsigned findLastAndOr (command_stream_t s, unsigned sInit, unsigned sLimit, int *errorC)
{
	char* token;
	unsigned index;
	index = sLimit;
	while (index > sInit)
	{
		token = s->tokens[index];
		
		if (strcmp (token, ")") == 0)
		{
			unsigned curPos;
			curPos = index;
			unsigned ump;
			ump = 1;
			while (index >= sInit && ump != 0)
			{
				index--;
				token = s->tokens[index];
				if (strcmp (token, "(") == 0)
				{ump--;}
				if (strcmp (token, ")") == 0)
				{ump++;}
			}
			if (index > sInit)
			{
				index--;
			}
			if (ump != 0)
			{
				*errorC = 4;
				return curPos;
			}
		}
		if (strcmp (token, "(") == 0)
		{
			*errorC = -1;
			return index;
		}
		
		if (strcmp (token, "|") == 0)
		{
			if (index != 0)
			{
				token = s->tokens[index - 1];
				if ((strcmp (token, "|") == 0))
				{
					*errorC = -2;
					return index;
				}
			}
		}
		
		if (strcmp (token, "&") == 0)
		{
			if (index != 0)
			{
				token = s->tokens[index - 1];
				if ((strcmp (token, "&") == 0))
				{
					*errorC = -3;
					return index;
				}
			}
		}
		index--;
	}// End while.
	
	*errorC = -1;
	return 0;
}

unsigned findLastPipe (command_stream_t s, unsigned sInit, unsigned sLimit, int *errorC)
{
	char* token;
	unsigned index;
	index = sLimit;
	while (index > sInit)
	{
		token = s->tokens[index];
		
		if (strcmp (token, ")") == 0)
		{
			unsigned curPos;
			curPos = index;
			unsigned ump;
			ump = 1;
			while (index >= sInit && ump != 0)
			{
				index--;
				token = s->tokens[index];
				if (strcmp (token, "(") == 0)
				{ump--;}
				if (strcmp (token, ")") == 0)
				{ump++;}
			}
			if (index > sInit)
			{
				index--;
			}
			if (ump != 0)
			{
				*errorC = 4;
				return curPos;
			}
		}
		if (strcmp (token, "(") == 0)
		{
			*errorC = -1;
			return index;
		}
		
		if (strcmp (token, "|") == 0)
		{
			return index;
		}
		
		index--;
	} // End while.
	
	*errorC = -1;
	return 0;
}

void parseIO (command_stream_t s, command_t c, unsigned sInit, unsigned sLimit, unsigned *lineNum, int *errorC)
{
	unsigned pos;
	char* token;
	c->input = NULL;
	c->output = NULL;
	pos = sInit + 1;
	while (pos < sLimit)
	{
		token = s->tokens[pos];
		if (strcmp (token, "<") == 0)
		{
			if (specialToken (s->tokens[pos + 1]))
			{
				*errorC = 5;
				return;
			}
			else
			{
				c->input = strdup(s->tokens[pos + 1]);
				if ((pos+3)<=sLimit && strcmp (s->tokens[pos + 2], ">") == 0)
				{
					if (specialToken (s->tokens[pos + 3]))
					{
						*errorC = 5;
						return;
					}
					else
					{
						c->output = strdup(s->tokens[pos + 3]);
						return;
					}
				} //endif
			} // endelse
		} // endif
		if (strcmp (token, ">") == 0)
		{
			if (specialToken (s->tokens[pos + 1]))
			{
				*errorC = 5;
				return;
			}
			else
			{
				c->output = strdup(s->tokens[pos + 1]);
				return;
			}
		}
		pos++;
	}
}

command_t parse (command_stream_t s, unsigned sInit, unsigned sLimit, unsigned *lineNum, int *errorC)
{
	// Initialize command to return.
	command_t c, c1, c2;
	c = checked_malloc (sizeof (struct command));
	c->type = -1;
	c->status = -1;
	c->input = 0;
	c->output = 0;
	
	unsigned pos;
	pos = sLimit;
	
	// Error case
	if (sInit > sLimit)
	{
		*errorC = 2;
		free(c);
		return NULL;
	}
	
	// Dispose of trailing newlines/semicolons
	while (sLimit > sInit && (strcmp (s->tokens[sLimit], "\n") == 0 
			|| strcmp (s->tokens[sLimit], ";") == 0))
	{
		sLimit--;
	}
	

	// Eat stream until last sequence command and divide stream at that point.
	pos = findLastSeq (s, sInit, sLimit, errorC);
	if (*errorC != -1)
	{
		c->type = SEQUENCE_COMMAND;

		if (*errorC != 0)
		{
			*lineNum = findLineNumber (s, pos);
			free (c);
			return NULL;
		}
		// Recursively parse both halves
		c1 = parse (s, sInit, pos - 1, lineNum, errorC);
		if (*errorC != 0)
		{
			free (c);
			return NULL;
		}
		c2 = parse (s, pos + 1, sLimit, lineNum, errorC);
		if (*errorC != 0)
		{
			free (c);
			return NULL;
		}
		c->u.command[0] = c1;
		c->u.command[1] = c2;
		
		return c;
	}

	*errorC = 0;
	// If no sequence tokens, eat stream until the last and/or and divide stream at that point.
	pos = findLastAndOr (s, sInit, sLimit, errorC);
	if (*errorC != -1)
	{
		if (*errorC == -2)
		{
			*errorC = 0;
			c->type = OR_COMMAND;
		}
		if (*errorC == -3)
		{
			*errorC = 0;
			c->type = AND_COMMAND;
		}
		if (*errorC != 0)
		{
			*lineNum = findLineNumber (s, pos);
			free (c);
			return NULL;
		}
		// Recursively parse both halves
		c1 = parse (s, sInit, pos - 2, lineNum, errorC);
		if (*errorC != 0)
		{
			free (c);
			return NULL;
		}
		c2 = parse (s, pos + 1, sLimit, lineNum, errorC);
		if (*errorC != 0)
		{
			free (c);
			return NULL;
		}
		c->u.command[0] = c1;
		c->u.command[1] = c2;
		
		return c;
	}
	
	*errorC = 0;
	// If no and/or tokens, eat stream until last pipe and divide stream at that point.
	pos = findLastPipe (s, sInit, sLimit, errorC);
	if (*errorC != -1)
	{
		c->type = PIPE_COMMAND;
		if (*errorC != 0)
		{
			*lineNum = findLineNumber (s, pos);
			free (c);
			return NULL;
		}
		// Recursively parse both halves
		c1 = parse (s, sInit, pos - 1, lineNum, errorC);
		if (*errorC != 0)
		{
			free (c);
			return NULL;
		}
		c2 = parse (s, pos + 1, sLimit, lineNum, errorC);
		if (*errorC != 0)
		{
			free (c);
			return NULL;
		}
		c->u.command[0] = c1;
		c->u.command[1] = c2;
		
		return c;
	}
	
	*errorC = 0;
	// Resolve subshell command
	pos = sInit;
	s->position = sInit;
	if (accept (s, "("))
	{
		c->type = SUBSHELL_COMMAND;
		c->u.subshell_command = parse (s, sInit + 1, sLimit - 1, lineNum, errorC);
		return c;
		if (*errorC != 0)
		{
			free (c);
			return NULL;
		}
	}

	// Resolve simple command
	c->type = SIMPLE_COMMAND;
	c->u.word = getCommand (s);
	if (c->u.word == NULL)
	{
		free(c);
		return NULL;
	}
		// I/O redirections
	parseIO (s, c, sInit, sLimit, lineNum, errorC);
	if (*errorC != 0)
	{
		free (c);
		*lineNum = findLineNumber (s, pos);
		return NULL;
	}
	//print_command (c);
	return c;
}

command_t readScript (command_stream_t s)
{	
	// Initialze variables.
	command_t c;
	int errorC;
	unsigned lineNum;
	c = NULL;
	errorC = 0;
	lineNum = 1;

	c = parse (s, 0, s->numTokens - 1, &lineNum, &errorC);
	s->done = 1;
//	if (c == NULL && errorC == 0)
//	{
//		destroy_command_stream(s);
//		error (1, 0, "Parse error; returned NULL");
//	}
	
	if (errorC != 0)
	{
		free(c);
		destroy_command_stream(s);
	}

	switch (errorC)
	{
		case 0:
			break;
		case 1:
			error (errorC, 0, "%u: Expected token error.", lineNum);
			break;
		case 2:
			error (errorC, 0, "%u: Stream access error.", lineNum);
			break;
		case 3:
			error (errorC, 0, "%u: Error, invalid token around newline.", lineNum);
			break;
		case 4:
			error (errorC, 0, "%u: Error, unmatched parenthesis.", lineNum);
			break;
		case 5:
			error (errorC, 0, "%u: Error, invalid I/O indirection", lineNum);
			break;
		case 6:
			error (errorC, 0, "%u: Invalid sequence command separator.", lineNum);
			break;
		default:
			error (1, 0, "%u: Unkown error", lineNum);
			break;
	}
	return c;
}

// Format the command list into an array by Depth First Search
void cDFSr (command_t c, command_stream_t s)
{
	if (c == NULL) { return; }
	switch (c->type)
	{
		case AND_COMMAND:
		case OR_COMMAND:
		case PIPE_COMMAND:
		case SIMPLE_COMMAND:
		case SUBSHELL_COMMAND:
		
			if ((s->cLen + 1) * sizeof (command_t) > s->cSize)
				s->cArray = checked_grow_alloc (s->cArray, &(s->cSize));
			s->cArray[s->cLen] = c;
			s->cLen++;
			break;
			
		case SEQUENCE_COMMAND:
			
			cDFSr (c->u.command[0], s);
			cDFSr (c->u.command[1], s);
			
			break;
			
		default:
			return;
	}
}

void cDFS (command_stream_t s)
{
	cDFSr (s->script, s);
}

command_t
read_command_stream (command_stream_t s)
{
	//command_debug (s->cArray[0]);
	//error (9, 0, "Size of command array is %p", s->cArray);
	if (s->cLen == 0)
	{ return NULL; }
	
	if (s->position >= s->cLen)
	{ return NULL; } 
	
	s->position++;
	return s->cArray[s->position - 1];
}
