#include "tarbuf.h" 
#include <unistd.h> 
#include <time.h>
#include <sys/stat.h>
#include <string.h>


int tar_buf_write(TAR *t, size_t len, const void * buf, const char * name) 
{
                                                //
  struct stat st = 
  {
    .st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH,
    .st_uid = getuid(), 
    .st_gid = getgid(),
    .st_size = len,
    .st_mtime = time(0)
  }; 

  return tar_buf_write_with_stat(t,buf,name,&st); 
}


int tar_buf_write_with_stat(TAR * t,  const void * buf, const char * name, const struct stat * st)
{

  //set header block 

  memset(&(t->th_buf), 0, sizeof (struct tar_header)); //clear it
  th_set_from_stat(t,(struct stat*)st); 
  th_set_path(t,name); 
  th_write(t); 

  size_t nleft = st->st_size;
  size_t nwritten = 0;
  for (; nleft >= T_BLOCKSIZE; nleft-= T_BLOCKSIZE)
  {
    if (T_BLOCKSIZE == tar_block_write(t, buf + nwritten))
    {
      nwritten += T_BLOCKSIZE; 
    }
    else
    {
      return -1; 
    }
  }

  if (nleft) 
  {
    char block[T_BLOCKSIZE]; 
    memcpy(block, buf + nwritten, nleft); 
    memset(block+nleft, 0, T_BLOCKSIZE - nleft); 
    if (T_BLOCKSIZE == tar_block_write(t,block))
    {
      nwritten += nleft; 
    }
    else
    {
      return -1; 
    }
  }
  return nwritten; 
}

int tar_buf_read(TAR *t, size_t destlen, void * buf)
{
  size_t tsize = th_get_size(t); 
  size_t nleft = destlen < tsize ? destlen : tsize;
  size_t nread = 0;


  for (; nleft >= T_BLOCKSIZE; nleft-= T_BLOCKSIZE)
  {
    if (T_BLOCKSIZE == tar_block_read(t, buf+ nread))
    {
      nread += T_BLOCKSIZE; 
    }
    else
    {
      return -1; 
    }
  }

  if (nleft) 
  {
    char block[T_BLOCKSIZE];
    if (T_BLOCKSIZE != tar_block_read(t, block))
    {
      return -1; 
    }
    memcpy(buf + nread, block, nleft); 
    nread += nleft; 
  }

  return nread;
}
