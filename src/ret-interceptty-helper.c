#include <stdio.h> 
#include <sys/types.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/stat.h> 
#include <sys/file.h> 
#include <time.h> 



int main(int nargs, char ** args) 
{
  const char * match = nargs < 2 ? "cc flag" : args[1]; 
  const char * fifo_name = nargs < 3 ? "/tmp/ret-fifo" : args[2]; 

  int match_len = strlen(match); 

  printf("Match string is \"%s\"\n", match); 

  int fd = 0;
  while(1) 
  {
    printf("Trying to open %s, this might block if it's not open!\n", fifo_name); 
    fd = open(fifo_name, O_WRONLY); 

    if (fd > 0) 
    {
      printf("Successfully opened %s\n", fifo_name); 
    }
    else
    {
      printf("open(%s) returned bad fd. Sleeping and then retrying...\n",fifo_name); 
      sleep(1); 
      continue; 
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
        if (1!=write(fd,"1",1))
        {
          fprintf(stderr,"Failed to write, is there nothing listening? Will try to reopen\n"); 
          close(fd); 
          break; 
        }
      }
    }
  }

  close(fd); 
  return 0; 

}
