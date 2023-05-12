#include "tarbuf.h" 
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

const char * out = "test.tar"; 


int main(int nargs, char ** args) 
{
  if (nargs> 1) out = args[1]; 

  TAR * t; 
  int topen =tar_open(&t, out, NULL, O_WRONLY | O_CREAT, 0644, 0); 
  printf("tar_open(%s) returned %d\n", out, topen); 

  const char * test1 = "this is the first test\n"; 
  const char * test2 = "this is the second test\n"; 

  tar_buf_write(t, strlen(test1), test1, "test1.txt"); 
  tar_buf_write(t, strlen(test2), test2, "test2.txt") ; 

  tar_append_eof(t); 
  return 0; 

}



