#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#include <stdio.h>

void greeting() {
  // "orsh-0.1$ "
  printf(ANSI_COLOR_RED "o" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_GREEN "r" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_YELLOW "s" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_BLUE "h" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_MAGENTA "-" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_CYAN "0" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_CYAN "." ANSI_COLOR_RESET);
  printf(ANSI_COLOR_CYAN "1" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_RED "$ " ANSI_COLOR_RESET);
}