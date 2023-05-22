#define _GNU_SOURCE
#include <stdio.h> 
#include <signal.h> 
#include <time.h>
#include "cody-listener.h" 
#include <string.h>


volatile int stop = 0; 

static void sighandler(int signum) 
{
  if (signum == SIGINT || signum == SIGTERM)
  stop =1; 
}


//todo, be less dumb 
char * output_name = NULL; 
char * output_name2 = NULL; 



int main(int nargs, char ** args) 
{

  const char * out_dir = nargs < 3 ? NULL : args[2]; 
  const char * out_dir2 = nargs < 4 ? NULL : args[3]; 


  const char * topic_string = nargs < 2 ? "test_topic/cody%d" : args[1]; 

  signal(SIGINT, sighandler); 

  cody_listener_t * l = cody_listener_init("localhost", 1883, topic_string, 0x3f, 64, 0); 

  if (!l) 
  {
    fprintf(stderr,"Could not initialize listener\n"); 
    return 1; 
  }
  while(!stop) 
  {
    cody_listener_wait(l,1); 
    for (int icody = 1; icody <=6; icody++) 
    {
      while(cody_listener_nrdy(l,icody) > 0)
      {
         const cody_data_t* cody = cody_listener_get(l,icody,0); 
         printf("--------------FROM CODY%d----------------\n", icody); 
         struct timespec ts; 
         cody_data_fill_time(cody,&ts); 
         struct tm * gmt = gmtime(&ts.tv_sec); 
         printf("%d-%02d-%02d-%02d:%02d:%02d.%09ld\n", gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec, ts.tv_nsec); 
         if (out_dir) 
         {
           if (!output_name)
           {
             asprintf(&output_name,"%s/%d-%02d-%2d.%02d%02d%02d.%09ld.CDY%d", out_dir,
                 gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec, ts.tv_nsec, icody);
           }
           else
           {
             sprintf(output_name,"%s/%d-%02d-%2d.%02d%02d%02d.%09ld.CDY%d", out_dir,
                 gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec, ts.tv_nsec, icody);
           }
 
         }
         if (out_dir2) 
         {
           if (!output_name2)
           {
             asprintf(&output_name2,"%s/%d-%02d-%2d.%02d%02d%02d.%09ld.CDY%d", out_dir2,
                 gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec, ts.tv_nsec, icody);
           }
           else
           {
             sprintf(output_name2,"%s/%d-%02d-%2d.%02d%02d%02d.%09ld.CDY%d", out_dir2,
                 gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec, ts.tv_nsec, icody);
           }
         }



      //   cody_data_dump(stdout,cody,0); 
         cody_listener_release(l,cody); 
      }
    }
  }
  cody_listener_finish(l); 
  return 0; 

}
