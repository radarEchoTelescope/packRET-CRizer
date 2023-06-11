#include <stdio.h> 
#include <sys/types.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/stat.h> 
#include <sys/file.h> 
#include <time.h> 
#include <signal.h> 

volatile int got_sigpipe = 0; 

void sigpipe_handler(int signum) 
{
  (void)signum; 

  got_sigpipe = 1; 

}


int main(int nargs, char ** args) 
{
  const char * match = nargs < 2 ? "crc32" : args[1]; 
  const char * fifo_name = nargs < 3 ? "/tmp/ret-fifo" : args[2]; 

  int match_len = strlen(match); 
  signal(SIGPIPE, sigpipe_handler); 

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
    while (getline(&line, &line_size, stdin) && !got_sigpipe )
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
        struct timespec ts; 
        clock_gettime(CLOCK_REALTIME,&ts); 
        if (1!=write(fd,"1",1))
        {
          fprintf(stderr,"Matched string, but failed to write, is there nothing listening? Will try to reopen\n"); 
          close(fd); 
          break; 
        }
        else
        {
          struct tm * t = gmtime(&ts.tv_sec); 
          printf("Matched string and sent trigger to fifo at %02d:%02d:%02d.%09luZ\n", t->tm_hour, t->tm_min, t->tm_sec, ts.tv_nsec); 
          fflush(stdout); 
        }
      }
    }
    if (got_sigpipe) 
    {
      fprintf(stderr,"Got sigpipe?\n"); 
      got_sigpipe = 0; 
    }
  }

  close(fd); 
  return 0; 

}
