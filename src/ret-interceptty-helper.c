#include <stdio.h> 
#include <sys/types.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/stat.h> 
#include <sys/file.h> 



int main(int nargs, char ** args) 
{
  const char * match = nargs < 2 ? "cc flag" : args[1]; 
  const char * fifo_name = nargs < 3 ? "/tmp/ret-fifo" : args[2]; 

  int match_len = strlen(match); 

  printf("Match string is \"%s\"\n", match); 
  printf("Trying to open %s, this might block if it's not open!", fifo_name); 
  int fd = open(fifo_name, O_WRONLY); 

  if (fd > 0) 
  {
    printf("Successfully opened %s\n", fifo_name); 
  }
  else
  {
    printf("open(%s) returned bad fd\n",fifo_name); 
    return 1; 
  }

  char * line = NULL;
  size_t line_size = 0; 

  int nmatch = 0; 
  while (getline(&line, &line_size, stdin) )
  {
    if (line[0]!='>' || line_size < 10) continue; 

    // format is 
    // > \t0xXX (C)"
    // 012 3456789
    // so we want 9th character is a match and 10th character = ) 

    if (line[9] == match[nmatch] && line[10]==')')
    {
      nmatch++; 
    }
    else
    {
      nmatch = 0; 
    }

    //great success
    if (nmatch == match_len) 
    {
      printf("Matched string!\n"); 
      write(fd,"1",1); 
    }
  }

  close(fd); 
  return 0; 

}
