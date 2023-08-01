#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "greeting.h"
#include "string_trim.h"

#define BUFFERSIZE 10
#define MAXREAD 4096

struct cmd {
  char *name;
  int argc;
  char **argv;
};

struct cmd_line {
  struct cmd **cmds;
  int count;
};

void delete_comments(char *s) {
  char *p = s;
  while ((*p != '#') && (*p)) p++;
  if (*p) {
    *p = '\0';
  } else
    return;
}

void commandParser(struct cmd *cmd, char *comm) {
  // printf("\n!{%s}!\n", comm);

  int flag = 0;
  cmd->argv = malloc(1 * sizeof(char *));
  cmd->argc = 0;

  char *start, *end;
  start = comm;
  end = comm;

  while (*start != '\0') {
    while (*start == ' ') start++;
    if (*start == '\0') break;
    end = start;

    if (*end == '\"') {
      start++;
      end++;
      while (*end != '\"') {
        end++;
      }
      end--;
    } else if (*end == '\'') {
      // start++;
      end++;
      while (*end != '\'') {
        end++;
      }
      // end--;
    } else if (*end == '>') {
      if (*(end + 1) == '>') {
        end += 1;
      }
    } else if (*end == '|') {
    } else if (*end == '\\') {
      end += 2;
    } else {
      while (((*end != ' ') && (*end != '>') && (*end != '|')) && (*end)) {
        if (*end == '\\')
          end += 2;
        else
          end += 1;
      }
      end--;
    }

    char *token = malloc(sizeof(char) * (end - start + 2));
    memcpy(token, start, end - start + 1);
    token[end - start + 1] = '\0';
    // printf("token: [%s]\n", token);

    /* Remove \ signs for token. */
    // if (*token == '\"') {
    //   for (int i = 0; i < strlen(token); i++) {
    //     char *s = token + i;  // s - start
    //     // if ((*s == '\\') && ((*(s - 1) == '\\') || (*(s - 1) == '\"'))) {
    //     if ((*s == '\\') && (*(s + 1) == '\\')) {
    //       memmove(s, s + 1, strlen(token) - (s - token + 1));
    //       s[strlen(s) - 1] = '\0';
    //     }
    //   }
    // } else if (*token != '\'') {
    if (*token != '\'') {
      for (int i = 0; i < strlen(token); i++) {
        char *s = token + i;  // s - start
        // if ((*s == '\\') && ((*(s - 1) == '\\') || (*(s - 1) == '\"'))) {
        if ((*s == '\\') && (*(s + 1) != 'n')) {
          memmove(s, s + 1, strlen(token) - (s - token + 1));
          s[strlen(s) - 1] = '\0';
        }
      }
    } else {
      size_t size = strlen(token);
      memmove(token, token + 1, size - 2);
      token[strlen(token) - 2] = '\0';
    }
    /* Applicable only if token is not in '' */

    if (((end[1] == '\"') && (*(start - 1) == '\"')) ||
        ((end[1] == '\'') && (*(start - 1) == '\''))) {
      if (end[2])
        start = end + 2;
      else
        *start = '\0';
    } else
      start = end + 1;

    if (!flag) {
      cmd->name = token;
      flag = 1;
    } else {
      cmd->argv[cmd->argc] = token;
      cmd->argc += 1;

      char **new_argv = realloc(cmd->argv, sizeof(char *) * (cmd->argc + 1));
      if (new_argv == NULL) {
        exit(1);
      }
      cmd->argv = new_argv;
    }
  }

  free(comm);
}

struct cmd_line parser(char *buff) {
  struct cmd_line res;
  res.count = 0;
  res.cmds = malloc(sizeof(*res.cmds));

  char *s = buff;
  char *p = buff;
  while (1) {
    if (((*p == '|') && (*(p - 1) != '\\')) || (!*p)) {
      char *end = p - 1;
      size_t len = end - s + 1;
      char *token = calloc(len + 1, 1);
      memmove(token, s, len);

      res.cmds[res.count] = malloc(sizeof(**res.cmds));
      commandParser(res.cmds[res.count], token);
      ++res.count;

      res.cmds = realloc(res.cmds, (res.count + 1) * sizeof(*res.cmds));
      assert(res.cmds != NULL);
      s = p + 1;
      if (!*p) break;
    }
    p++;
  }

  return res;
}

void cd_command(struct cmd_line *line) {
  if (line->cmds[0]->argc > 0) {
    if (chdir(line->cmds[0]->argv[0]) == -1) {
      printf("\nCannot change directory.\n");
    }
  } else {
    if (chdir(getenv("HOME")) == -1) {
      printf("\nCannot change directory.\n");
    }
  }
}

void quotesStatus(char *str, int *singleQuotesClosed, int *doubleQuotesClosed) {
  // printf("got: %s\n", str);
  char *s = str;
  // size_t singleQuotesCount = 0;
  // size_t doubleQuotesCount = 0;

  while (*s) {
    /* If the function starts for the first time */
    if (*singleQuotesClosed == 1 && *doubleQuotesClosed == 1) {
      if ((*s == '\"') && (*singleQuotesClosed)) {
        *doubleQuotesClosed = 0;
      } else if ((*s == '\'') && (*doubleQuotesClosed)) {
        *singleQuotesClosed = 0;
      }
    } else {
      if ((*s == '\"') && (*singleQuotesClosed)) {
        if ((*(s - 1) == '\\')) {
          if ((*(s - 2) == '\\')) {
            *doubleQuotesClosed = 1;
          }
        } else {
          *doubleQuotesClosed = 1;
        }
      } else if ((*s == '\'') && (*doubleQuotesClosed)) {
        *singleQuotesClosed = 1;
      }
    }

    s++;
  }
}

void freeAll(char *buff, struct cmd_line *line, char **cmd) {
  free(cmd);
  free(buff);
  for (int k = 0; k < line->count; k++) {
    free(line->cmds[k]->name);
    for (int m = 0; m < line->cmds[k]->argc; m++) {
      free(line->cmds[k]->argv[m]);
    }
    free(line->cmds[k]->argv);
    free(line->cmds[k]);
  }
  free(line->cmds);
}

void execute(char *buff, int *code) {
  /* Parse buff by | pipe sign .*/
  struct cmd_line line = parser(buff);
  int **fd;
  int *child_pids;

  if (line.count > 1) {
    fd = malloc(line.count * sizeof(int *));
    for (int i = 0; i < line.count; i++) {
      fd[i] = calloc(2, sizeof(int));
      pipe(fd[i]);
    }

    child_pids = calloc(line.count, sizeof(int));  // free in every if
  }

  for (int n = 0; n < line.count; n++) {
    char **cmd = malloc((line.cmds[n]->argc + 2) * sizeof(char *));
    cmd[0] = line.cmds[n]->name;
    for (int i = 1; i < line.cmds[n]->argc + 1; i++) {
      cmd[i] = line.cmds[n]->argv[i - 1];
    }
    cmd[line.cmds[n]->argc + 1] = (char *)0;
    // printf("{%s}\n", cmd[0]);
    char *secondLast = line.cmds[n]->argv[line.cmds[n]->argc - 2];  // ???

    if (line.cmds[n]->name == NULL) {
      freeAll(buff, &line, cmd);
      exit(0);
    }  // means comment

    if (!strcmp(line.cmds[n]->name, "cd")) {
      cd_command(&line);
      free(cmd);
    } else if ((!strcmp(line.cmds[n]->name, "exit")) && (line.count == 1)) {
      int exitCode = 0;
      if (line.cmds[n]->argc == 1) exitCode = atoi(cmd[1]);
      freeAll(buff, &line, cmd);
      exit(exitCode);
    } else if ((line.count == 1) && (line.cmds[n]->argc >= 2) &&
               (!strcmp(secondLast, ">") || !strcmp(secondLast, ">>"))) {
      pid_t child_pid = fork();
      if (child_pid == 0) {
        char *file_name = line.cmds[n]->argv[line.cmds[n]->argc - 1];
        int file;

        if (!strcmp(secondLast, ">"))
          file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        else
          file = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0777);

        dup2(file, STDOUT_FILENO);
        close(file);
        cmd[line.cmds[n]->argc] = (char *)0;
        cmd[line.cmds[n]->argc - 1] = (char *)0;
        execvp(line.cmds[n]->name, cmd);
      }
      free(cmd);
    } else {
      if (line.count > 1) {
        if ((line.cmds[n]->argc >= 2) &&
            (!strcmp(secondLast, ">") || !strcmp(secondLast, ">>"))) {
          pid_t child_pid = fork();
          if (child_pid == 0) {
            char *file_name = line.cmds[n]->argv[line.cmds[n]->argc - 1];
            int file;

            if (!strcmp(secondLast, ">"))
              file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
            else
              file = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0777);

            dup2(file, STDOUT_FILENO);
            close(file);
            cmd[line.cmds[n]->argc] = (char *)0;
            cmd[line.cmds[n]->argc - 1] = (char *)0;

            dup2(fd[n - 1][0], STDIN_FILENO);
            close(fd[n - 1][0]);
            close(fd[n - 1][1]);

            int status_code = execvp(line.cmds[n]->name, cmd);
            if (status_code == -1) {
              // printf("Process did not terminate correctly\n");
              freeAll(buff, &line, cmd);
              exit(1);
            }
          }
          child_pids[n] = child_pid;

          if (n > 0) {
            close(fd[n - 1][0]);
            close(fd[n - 1][1]);
          }
          // free(cmd);
        } else {
          int exitCode;
          pid_t child_pid = fork();
          if (child_pid == 0) {
            if ((line.cmds[n]->argc >= 2) &&
                (!strcmp(secondLast, ">") || !strcmp(secondLast, ">>"))) {
              pid_t child_pid = fork();
              if (child_pid == 0) {
                char *file_name = line.cmds[n]->argv[line.cmds[n]->argc - 1];
                int file;

                if (!strcmp(secondLast, ">"))
                  file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                else
                  file = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0777);

                dup2(file, STDOUT_FILENO);
                close(file);
                cmd[line.cmds[n]->argc] = (char *)0;
                cmd[line.cmds[n]->argc - 1] = (char *)0;

                dup2(fd[n - 1][0], STDIN_FILENO);
                close(fd[n - 1][0]);
                close(fd[n - 1][1]);

                int status_code = execvp(line.cmds[n]->name, cmd);
                if (status_code == -1) {
                  // printf("Process did not terminate correctly\n");
                  freeAll(buff, &line, cmd);
                  exit(1);
                }
              }
              child_pids[n] = child_pid;

              if (n > 0) {
                close(fd[n - 1][0]);
                close(fd[n - 1][1]);
              }
            }

            if (n == 0) {
              dup2(fd[n][1], STDOUT_FILENO);
              close(fd[n][0]);
              close(fd[n][1]);
            } else if (n == line.count - 1) {
              dup2(fd[n - 1][0], STDIN_FILENO);
              close(fd[n - 1][0]);
              close(fd[n - 1][1]);
            } else {
              dup2(fd[n - 1][0], STDIN_FILENO);
              close(fd[n - 1][0]);
              close(fd[n - 1][1]);

              dup2(fd[n][1], STDOUT_FILENO);
              close(fd[n][0]);
              close(fd[n][1]);
            }
            // //////////////////////////////
            int status_code = 0;
            if (!strcmp(line.cmds[n]->name, "exit")) {
              int exitCode = 0;
              if (line.cmds[n]->argc == 1) exitCode = atoi(cmd[1]);
              // printf("%d", exitCode);
              freeAll(buff, &line, cmd);
              for (int i = 0; i < line.count; i++) {
                free(fd[i]);
              }
              free(fd);
              free(child_pids);
              exit(exitCode);
            } else {
              status_code = execvp(line.cmds[n]->name, cmd);
            }

            /////////////////////////////////
            if (status_code == -1) {
              int exitCode = 0;
              if (!strcmp(line.cmds[n]->name, "exit")) exitCode = atoi(cmd[1]);
              *code = exitCode;
              printf("Process did not terminate correctly\n");
              printf("%s", strerror(errno));
              // printf("%d", n);
              freeAll(buff, &line, cmd);
              for (int i = 0; i < line.count; i++) {
                free(fd[i]);
              }
              free(fd);
              free(child_pids);
              // exit(1);

              exit(1);
            }
          }
          // int returnStatus;
          // waitpid(child_pid, &returnStatus, 0);
          // *code = WEXITSTATUS(returnStatus);

          child_pids[n] = child_pid;

          if (n >= 1) {
            close(fd[n - 1][0]);
            close(fd[n - 1][1]);
          }
        }
      } else {
        pid_t child_pid = fork();
        if (child_pid == 0) {
          int status_code = execvp(line.cmds[n]->name, cmd);
          if (status_code == -1) {
            // printf("Process did not terminate correctly\n");
            freeAll(buff, &line, cmd);
            exit(1);
          }
        }
        int returnStatus;
        waitpid(child_pid, &returnStatus, 0);
        *code = WEXITSTATUS(returnStatus);
      }
      free(cmd);
    }
  }

  if (line.count > 1) {
    for (int k = 0; k < line.count; k++) {
      int returnStatus;
      waitpid(child_pids[k], &returnStatus, 0);
      *code = WEXITSTATUS(returnStatus);
    }

    for (int i = 0; i < line.count; i++) {
      free(fd[i]);
    }
    free(fd);
    free(child_pids);
  }

  for (int k = 0; k < line.count; k++) {
    free(line.cmds[k]->name);
    for (int m = 0; m < line.cmds[k]->argc; m++) {
      free(line.cmds[k]->argv[m]);
    }
    free(line.cmds[k]->argv);
    free(line.cmds[k]);
  }
  free(line.cmds);
  free(buff);
}

int main(void) {
  int code = 0;
  while (1) {
    greeting();

    char **buffs = calloc(1, sizeof(char *));
    int i = 0;
    int singleQuotesClosed = 1;
    int doubleQuotesClosed = 1;

    /*
    The input is read until no |, \ in the end. (\\ and \| are ok)
    AND no open quotation marks.
    */
    do {
      // unsigned long int len;
      // buffs[i] = NULL;
      // int x = getline(&buffs[i], &len, stdin);
      // if (x == -1) {  // eof detected
      //   for (int k = 0; k < i + 1; k++) free(buffs[k]);
      //   free(buffs);
      //   exit(code);
      // }

      int length = 0;
      buffs[i] = NULL;
      while (1) {
        int ch = getchar();
        // if (ch == '\n') break;
        if (ch == EOF) {
          for (int k = 0; k < i + 1; k++) free(buffs[k]);
          free(buffs);
          exit(code);
        }

        buffs[i] = realloc(buffs[i], length + 2);
        if (buffs[i] == NULL) {
          perror("Failed when allocated memory.");
          exit(127);
        }
        buffs[i][length] = (char)ch;
        buffs[i][length + 1] = '\0';
        ++length;
        if (buffs[i][length - 1] == '\n') break;
      }

      // if (strlen(buffs[i]) == MAXREAD) {
      //   unsigned long int len;
      //   int t = 0;
      //   char *left = NULL;
      //   getline(&left, &len, stdin);
      //   printf("[%d]", strlen(left));
      //   buffs[i] = realloc(buffs[i], MAXREAD + sizeof(left));
      //   for (int k = MAXREAD; k < MAXREAD + sizeof(left); k++)
      //     buffs[i][k] = '\0';
      //   strcat(buffs[i], left);
      //   free(left);
      // }
      // char *buf;
      // char *final = NULL;
      // int t = 1;
      // do {
      //   unsigned long int len;
      //   buf = NULL;
      //   int x = getline(&buf, &len, stdin);
      //   final = realloc(final, sizeof(buf));
      //   for (int k = (t - 1) * 4096; k < t * sizeof(buf); k++) final[k] =
      //   '\0'; strcat(final, buf); if (x == -1) {  // eof detected
      //     for (int k = 0; k < i + 1; k++) free(buffs[k]);
      //     free(buffs);
      //     free(final);
      //     free(buf);
      //     exit(code);
      //   }
      // } while (sizeof(buf) == 4096);

      // buffs[i] = final;
      // free(buf);

      quotesStatus(buffs[i], &singleQuotesClosed, &doubleQuotesClosed);
      buffs = realloc(buffs, sizeof(char *) * (i + 2));
      i++;
    } while (((buffs[i - 1][strlen(buffs[i - 1]) - 2] == '\\') &&
              ((buffs[i - 1][strlen(buffs[i - 1]) - 3] != '\\'))) ||
             ((buffs[i - 1][strlen(buffs[i - 1]) - 2] == '|') &&
              (buffs[i - 1][strlen(buffs[i - 1]) - 3] != '\\')) ||
             (!singleQuotesClosed || !doubleQuotesClosed));
    /* Concatenate all pieces in a single buff. */
    char *buff;
    if (i == 1) {
      string_trim_inplace(buffs[0]);
      buff = calloc(strlen(buffs[0]) + 1, 1);
      strcat(buff, buffs[0]);
    } else {
      for (int k = 0; k < i; k++) {
        if (k == 0) {
          string_trim_beginning(buffs[k]);
          char *lastChar = &buffs[k][strlen(buffs[k]) - 1];
          if (*lastChar == '\\') *lastChar = '\0';
          buff = calloc(strlen(buffs[0]) + 1, 1);
          strcat(buff, buffs[k]);
        } else if (k == i - 1) {
          char *lastChar = &buffs[k][strlen(buffs[k]) - 1];
          *lastChar = '\0';
          buff = realloc(buff, strlen(buff) + strlen(buffs[k]) + 2);
          strcat(buff, buffs[k]);
        } else {
          char *secondLast = &buffs[k][strlen(buffs[k]) - 2];
          if (*secondLast == '\\')
            *secondLast = '\0';
          else if (*secondLast == '|')
            secondLast[1] = '\0';
          buff = realloc(buff, strlen(buff) + strlen(buffs[k]) + 2);
          strcat(buff, buffs[k]);
        }
      }
    }
    for (int k = 0; k < i; k++) {
      free(buffs[k]);
    }
    free(buffs);

    /* Additional makeup for buffer. */
    if ((*buff == '\n') || (!*buff)) {
      free(buff);
      continue;
    }
    string_trim_inplace(buff);
    delete_comments(buff);

    if (!*buff) {
      free(buff);
      continue;
    }  // defense against comments like: #kqjewlkq

    // printf("buff: [%s]\n", buff);

    /* Execute the whole line, pipe by pipe. */
    execute(buff, &code);
  }
}
