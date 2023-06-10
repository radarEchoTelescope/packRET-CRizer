#include "ret-writer.h"
//#include "cody.h"
#include "radar.h" 
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> 
#ifdef WITH_LIBTAR
#include "tarbuf.h" 
#include <libtar.h> 
#endif
#include <zlib.h> 
#include <sys/stat.h>



struct ret_writer 
{
  int noutputs; 
  struct 
  {
    char * buf; 
    char * basedir; 
  } * outputs; 

  int compress; 
};


ret_writer_t * ret_writer_multi_init(int ndirs, const char * const *  output_directory, int compress) 
{
  ret_writer_t * w = malloc(sizeof(ret_writer_t)); 

  w->noutputs = ndirs; 
  w->outputs = malloc(ndirs * sizeof(*w->outputs)); 

  for (int i = 0; i < ndirs; i++) 
  {
    w->outputs[i].basedir = strdup(output_directory[i]); 
    w->outputs[i].buf = calloc(strlen(w->outputs[i].basedir) + 256,1); 
  }
  w->compress = compress;
  return w; 
}

ret_writer_t * ret_writer_init(const char * output_directory, int compress) 
{
  return ret_writer_multi_init(1,&output_directory, compress); 
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

static int do_write(const char * where, size_t howbig,  const void * what, int compress) 
{

  if (!compress) 
  {
    FILE * f = fopen(where,"w"); 
    if (!f) return -1; 
    
    int nw = fwrite(what, howbig,1,f); 
    fclose(f); 
    return nw;
  }

  gzFile gz = gzopen(where,"w"); 
  if (!gz) return -1; 
  gzsetparams(gz, 3, Z_FILTERED); 
  int nw = gzwrite(gz, what,howbig); 
  gzclose(gz); 
  return nw; 
}

/*
int ret_writer_write_cody(ret_writer_t *w, const cody_data_t* cody, int icody)
{
  if (!cody) return -1; 

  struct timespec ts; 
  cody_data_fill_time(cody,&ts); 
  struct tm tm_time; 
  gmtime_r(&ts.tv_sec, &tm_time); 


  int ret = INT32_MIN; 
  for (int i = 0; i < w->noutputs; i++) 
  {
    sprintf(w->outputs[i].buf,"%s/CDY%d/%d/%02d/%02d/%02d", w->outputs[i].basedir, icody,
    tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour); 
    mkdir_if_needed(w->outputs[i].buf); 

    sprintf(w->outputs[i].buf,"%s/CDY%d/%d/%02d/%02d/%02d/RET.%04d%02d%02d.%02d%02d%02d.%09ld.CDY%d", 
      w->outputs[i].basedir,icody, 
      tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour,
      tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, 
      tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ts.tv_nsec, icody); 

    int nw = do_write(w->outputs[i].buf,sizeof(*cody), cody, w->compress); 
    if (nw > ret) ret = nw; 
  }

  return ret > 0 ? ret*((int)sizeof(*cody)) : (int) ret; 
}
*/

int ret_writer_write_radar(ret_writer_t * w, const ret_radar_data_t * rad) 
{

  if (!rad) return -1; 

  struct timespec ts; 
  ret_radar_fill_time(&rad->gps,&ts); 
  struct tm tm_time; 
  gmtime_r(&ts.tv_sec, &tm_time); 

  int ret = INT32_MIN; 
  for (int i = 0; i < w->noutputs; i++) 
  {
    sprintf(w->outputs[i].buf,"%s/RDR/%d/%02d/%02d/%02d", w->outputs[i].basedir,
        tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour); 
    mkdir_if_needed(w->outputs[i].buf); 
 
    sprintf(w->outputs[i].buf,"%s/RDR/%d/%02d/%02d/%02d/RET.%04d%02d%02d.%02d%02d%02d.%09ld.RDR",
      w->outputs[i].basedir,
      tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour,
      tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, 
      tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ts.tv_nsec); 

    int nw = do_write(w->outputs[i].buf,sizeof(*rad), rad, w->compress);
    if (nw > ret) ret = nw; 
  }
  return ret > 0 ? ret*((int)sizeof(*rad)) : ret; 
}


/*
//todo implement tar compression 
int ret_writer_write_full_event(ret_writer_t * w, const ret_full_event_t * ev)
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

  int ret = INT32_MIN; 

  for (int o = 0; o < w->noutputs; o++) 
  {

    sprintf(w->outputs[o].buf,"%s/%d/%02d/%02d/%02d", w->outputs[o].basedir, tm_time.tm_year + 1900, 
        tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour); 
    mkdir_if_needed(w->outputs[o].buf); 
   
    int nw = 0;
    //tarname 
    sprintf(w->outputs[o].buf,"%s/%d/%02d/%02d/%02d/RET.%04d%02d%02d.%02d%02d%02d.%09ld.tar", 
        w->outputs[o].basedir, 
        tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour, 
        tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, 
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ts_tar.tv_nsec); 

    TAR * tar; 

    int ret = tar_open(&tar, w->outputs[o].buf, NULL, O_WRONLY | O_CREAT, 0644,0); 

    if (ret) 
    {
      if (ret < 0) ret = -1; 
    }

    if (ev->radar) 
    {
      gmtime_r(&ts_radar.tv_sec, &tm_time); 
      sprintf(w->outputs[o].buf,"RET.%04d%02d%02d.%02d%02d%02d.%09ld.RDR", 
          tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, 
          tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ts_radar.tv_nsec); 
      nw += tar_buf_write(tar, sizeof(*(ev->radar)), ev->radar, w->outputs[o].buf); 
    }

    for (int i = 0; i< 6; i++) 
    {
      if (ev->cody[i]) 
      {
        gmtime_r(&ts_cody[i].tv_sec, &tm_time); 
        sprintf(w->outputs[o].buf,"RET.%04d%02d%02d.%02d%02d%02d.%09ld.CDY%d", 
            tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, 
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ts_cody[i].tv_nsec, i+1); 
        nw += tar_buf_write(tar, sizeof(*ev->cody[i]), ev->cody[i], w->outputs[o].buf); 
      }
    }


    tar_append_eof(tar); 
    tar_close(tar); 
    if (nw > ret) ret = nw; 
  }
  return ret; 
}
*/

void ret_writer_destroy(ret_writer_t *w) 
{
  if (w) 
  {
    for (int i = 0; i < w->noutputs; i++) 
    {
      free(w->outputs[i].buf); 
      free(w->outputs[i].basedir); 
    }
    free(w); 
  }
}
