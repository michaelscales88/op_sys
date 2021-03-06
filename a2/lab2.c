/*
  Assignment 2
  a2.c
  Developer: Michael Scales
  Operating Systems
*/

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

static int MAX_ARGS = 50;

/*  findRedirect:
 *    Find the file in the arg. Splits the arg on the
 *    delimiter and assigns the second value from the
 *    split to the reFile.
 */
void findRedirect(char *arg, char *reFile, char *delim);

/*  tokenize:
 *    Splits a line on the "|" delimiter. Sets args to 
 *    the sequence of tokenized arguments. Sets a procCount
 *    which represents the number of arguments that were split.
 */
void tokenize(char *line, char **args, int *procCount);

/*  trim:
 *    Trim the leading and trailing whitespace from the str.
 */
void trim(char *str);

int main()
{
  int i, status, procCount;
  int left_fd[2], right_fd[2];
  bool redir_out, redir_in;
  char *line = NULL;
  char buf[256], infile[256], outfile[256];
  char **args = (char **)malloc(MAX_ARGS * sizeof(char *));
  for (i = 0; i < MAX_ARGS; i++)
    args[i] = NULL;

  size_t size;
  pid_t pid;

  while (1)
  {
    // Init
    redir_in = redir_out = false;
    procCount = 0;
    infile[0] = '\0';
    outfile[0] = '\0';

    printf("\nshhh> ");
    // Get line and process exit conitions
    if (getline(&line, &size, stdin) == EOF)
      break;
    if (strncasecmp(line, "exit", 4) == 0)
      break;

    // Get piped arguments
    strcpy(buf, line);
    tokenize(buf, args, &procCount);

    // Handle redirects
    if (*args[0] == '\0')
      continue;
    findRedirect(args[0], infile, "<");
    if (infile[0] != '\0')
      redir_in = true;
    findRedirect(args[procCount - 1], outfile, ">");
    if (outfile[0] != '\0')
      redir_out = true;

    for (i = 0; i < procCount; i++)
    {
      // If pipes exist
      if (procCount > 1)
        if (pipe(right_fd) < 0)
          exit(EXIT_FAILURE);

      pid = fork();

      // Proc area
      if (pid > 0)
      {
        if (i > 0)
        {
          close(left_fd[0]);
          close(left_fd[1]);
        }
        left_fd[0] = right_fd[0];
        left_fd[1] = right_fd[1];
      }
      else if (pid == 0)
      {
        // Pipes
        if ((procCount > 1) && (i < procCount))
        {
          if (i == 0)
          {
            // first process
            dup2(right_fd[1], 1);
            close(right_fd[0]);
            close(right_fd[1]);
          }
          else if (i == (procCount - 1))
          {
            // last process
            dup2(left_fd[0], 0);
            close(left_fd[0]);
            close(left_fd[1]);
          }
          else
          {
            // middle processes
            dup2(left_fd[0], 0);
            close(left_fd[0]);
            close(left_fd[1]);
            dup2(right_fd[1], 1);
            close(right_fd[0]);
            close(right_fd[1]);
          }
        }

        // Input redirection
        if ((i == 0) && redir_in)
        {
          int fdIn = open(infile, O_RDONLY);
          if (fdIn < 0)
            perror(outfile);
          dup2(fdIn, 0);
          close(fdIn);
        }

        // Output redirection
        if ((i == (procCount - 1)) && redir_out)
        {
          int fdOut = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, 0755);
          if (fdOut < 0)
            perror(outfile);
          dup2(fdOut, 1);
          close(fdOut);
        }

        // Get current proc arguments
        int argLength = strlen(args[i]);
        char *argCopy = (char *)malloc(argLength + 1 * sizeof(char));
        strncpy(argCopy, args[i], argLength);

        int i = 0;
        char *argv[15] = {};
        char *arg = strtok(argCopy, " ");
        do
        {
          int subArgLen = strlen(arg);
          argv[i] = (char *)malloc((subArgLen + 1) * sizeof(char));
          strncpy(argv[i++], arg, subArgLen);
        } while ((arg = strtok(NULL, " ")) && i < 15);
        argv[i] = NULL;
        free(argCopy);

        execvp(argv[0], (char *const *)argv);
        // Shouldn't get here
        perror(args[i]);
        exit(EXIT_FAILURE);
      }
      else
        fprintf(stderr, "Error forking process.\n");

      // Wait for child to finish before proceeding
      if ((waitpid(pid, &status, 0) == pid) && (status != 0))
        fprintf(stderr, "Child process failed to execute.\n");
    }
  }
  for (i = 0; i < MAX_ARGS; i++)
    if (args[i])
      free(args[i]);
  free(args);
  exit(EXIT_SUCCESS);
}

void tokenize(char *line, char **args, int *procCount)
{
  int i = 0,
      size;
  char *cmd = strtok(line, "|");

  while (cmd != NULL && i < MAX_ARGS)
  {
    trim(cmd);
    size = strlen(cmd);
    args[i] = (char *)malloc(size + 1 * sizeof(char));
    strncpy(args[i++], cmd, size);
    cmd = strtok(NULL, "|");
  }

  *procCount = i;
  // Ignore stale args
  args[i] = NULL;
}

void findRedirect(char *arg, char *reFile, char *delim)
{
  char *match = strtok(arg, delim);
  if (match != NULL)
  {
    trim(match);
    strcpy(arg, match);
  }
  else
    return;

  match = strtok(NULL, " ");
  if (match != NULL)
  {
    trim(match);
    strcpy(reFile, match);
  }
}

void trim(char *str)
{
  if (!str)
    return;

  char *ptr = str;
  int len = strlen(ptr);

  while (len - 1 > 0 && isspace(ptr[len - 1]))
    ptr[--len] = 0;

  while (*ptr && isspace(*ptr))
    ++ptr, --len;

  memmove(str, ptr, len + 1);
}