#pragma once

/*
  This function trims only the beginning of a null-terminated string.
  Also, it deletes all \n chars in the end.
*/
void string_trim_beginning(char *s);

char *string_trim_inplace(char *s);
char *string_trim(char *s);