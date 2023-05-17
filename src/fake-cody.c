#include "cody.h" 
#include <mosquitto.h> 
#include <time.h>
#include <signal.h> 
#include <unistd.h> 


volatile int quit = 0;
void sighandler(int signum)
{
  (void) signum; 
  quit =1; 
}


const char * topics[6] = 
{
"test_topic/cody1", 
"test_topic/cody2", 
"test_topic/cody3", 
"test_topic/cody4", 
"test_topic/cody5", 
"test_topic/cody6" 
}; 


static cody_data_t cody; 

int main(int nargs, char ** args) 
{

  if (nargs > 1) 
  {
    FILE * fcody = fopen(args[1],"r"); 
    fread(&cody, 1, sizeof(cody), fcody); 
    fclose(fcody); 
  }

  struct mosquitto * mosq = mosquitto_new("fake-cody",0,0); 

  if (mosquitto_connect(mosq,"localhost",1883,10))
  {
    fprintf(stderr,"Could not connect to mqtt server\n"); 
    return 1; 
  }

  mosquitto_loop_start(mosq); 
  signal(SIGINT,sighandler); 
  signal(SIGTERM,sighandler); 

  while (!quit) 
  {
    sleep(1); 
    struct timespec now;
    for (int i = 0; i <6; i++) 
    {
      clock_gettime(CLOCK_REALTIME, &now); 
      cody.sci.gps_subtime.big = now.tv_nsec; 
      cody.sci.gps_time.big = now.tv_sec - 315964800 + 18; 
      mosquitto_publish(mosq,NULL, topics[i], sizeof(cody), &cody, 2, 1); 
      clock_gettime(CLOCK_REALTIME, &now); 
      cody.sci.gps_subtime.big = now.tv_nsec; 
      cody.sci.gps_time.big = now.tv_sec - 315964800 + 18; 
      mosquitto_publish(mosq,NULL, topics[i], sizeof(cody), &cody, 2, 1); 
    }
  }

  mosquitto_disconnect(mosq); 
  mosquitto_loop_stop(mosq,0); 
  mosquitto_destroy(mosq); 

  return 0; 
}
