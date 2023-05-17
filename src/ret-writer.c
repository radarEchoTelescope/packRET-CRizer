#include "ret-writer.h"
#include <libtar.h> 
#include "cody.h"
#include "radar.h" 
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> 
#include "tarbuf.h" 



struct ret_writer 
{
  char * buf; 
  char * basedir; 
};


ret_writer_t * ret_writer_init(const char * output_directory) 
{
  ret_writer_t * w = malloc(sizeof(ret_writer_t)); 
  w->basedir = strdup(output_directory); 
  w->buf = calloc(strlen(w->basedir) + 128,1); 
  return w; 
}

static int mkdir_if_needed(char * path)
{
  struct stat st; 

  // check if the path is a file, if it is, then find the dirname 
  if ( stat(path,&st) || ( (st.st_mode & S_IFMT) != S_IFDIR))//not there, try to make it
  {

     //check if there's a trailing slash, and remove if it ther is 
   
      int len = strlen(path); 
   
      char * trailing_slash = 0;
      if (path[len-1] == '/')
      {
        trailing_slash = &path[len-1];
        *trailing_slash = 0;
      }

     //do we hae any intermediate path elements
     char * slash = strrchr(path,'/'); 
     
     if (slash) 
     {
       *slash = 0; 
       if (mkdir_if_needed(path)) 
       {
         *slash = '/';
         return -1; 
       }
       *slash = '/'; 
     }

     if (trailing_slash) *trailing_slash = '/'; //restore
     return mkdir(path,0755);
  }

  return 0; 
}


int ret_writer_write_event(ret_writer_t * w, const ret_full_event_t * ev)
{

  if (!w) return -1; 

  //make sure not everythign is NULL 
  struct timespec ts_radar; 
  struct timespec ts_cody[6]; 
  struct timespec ts_tar; 

  if (ev->radar)
  {
    ret_radar_fill_time(&ev->radar->gps, &ts_radar); 
    ts_tar = ts_radar; 
  }
  int ncody = 0; 
  for (int i = 0; i < 6; i++) 
  {
    if (ev->cody[i])
    {
      cody_data_fill_time(ev->cody[i], &ts_cody[i]); 
      if (!ncody && !ev->radar) //use first cody for timestamp
      {
        ts_tar = ts_cody[i]; 
      }
        ncody++; 
    }
  }

  if (!ncody && !ev->radar) return 0; 

  struct tm tm_time; 
  gmtime_r(&ts_tar.tv_sec, &tm_time); 

  //create directories is not alread there
  //

  sprintf(w->buf,"%s/%d/%02d/%02d/%02d", w->basedir, tm_time.tm_year + 1900, 
      tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour); 
  mkdir_if_needed(w->buf); 
 
  int nw = 0;
  //tarname 
  sprintf(w->buf,"%s/%d/%02d/%02d/%02d/RET.%04d%02d%02d.%02d%02d%02d.%09ld.tar", 
      w->basedir, 
      tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour, 
      tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, 
      tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ts_tar.tv_nsec); 

  TAR * tar; 

  int ret = tar_open(&tar, w->buf, NULL, O_WRONLY, 0444,0); 

  if (ret) return -1; 

  if (ev->radar) 
  {
    gmtime_r(&ts_radar.tv_sec, &tm_time); 
    sprintf(w->buf,"RET.%04d%02d%02d.%02d%02d%02d.%09ld.RDR", 
        tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, 
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ts_tar.tv_nsec); 
    nw += tar_buf_write(tar, sizeof(*(ev->radar)), ev->radar, w->buf); 
  }

  for (int i = 0; i< 6; i++) 
  {
    if (ev->cody[i]) 
    {
      gmtime_r(&ts_cody[i].tv_sec, &tm_time); 
      sprintf(w->buf,"RET.%04d%02d%02d.%02d%02d%02d.%09ld.CDY%d", 
          tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, 
          tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ts_tar.tv_nsec, i+1); 
      nw += tar_buf_write(tar, sizeof(*ev->cody[i]), ev->cody[i], w->buf); 
    }
  }


  tar_append_eof(tar); 
  tar_close(tar); 
  return nw; 
}

