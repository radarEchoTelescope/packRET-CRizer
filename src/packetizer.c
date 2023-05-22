#include "cody-listener.h" 
#include "radar.h" 
#include "ret-writer.h" 
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <systemd/sd-daemon.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h> 
#include <pthread.h>
#include <systemd/sd-daemon.h> 


ret_radar_t * radar; 
cody_listener_t * cody_list; 
ret_writer_t * writer; 

#ifndef RADAR_BUF_SIZE
#define RADAR_BUF_SIZE 128 
#endif 

#define MAX_OUT_DIRS 16


//this holds the data
ret_radar_data_t radar_buf[RADAR_BUF_SIZE]; 
//this holds the read time. It also doubles as the free list (if time is  {0,0} then it's free)
struct timespec radar_buf_read_times[RADAR_BUF_SIZE]; 
volatile atomic_size_t radar_buf_free = RADAR_BUF_SIZE; 
pthread_mutex_t radar_buf_lock = PTHREAD_MUTEX_INITIALIZER;  


static void mark_free(int i) 
{
   pthread_mutex_lock(&radar_buf_lock); 
   memset(&radar_buf_read_times[i],0,sizeof(struct timespec)); 
   radar_buf_free++; 
   pthread_mutex_unlock(&radar_buf_lock); 
}

//store matches for each radar 
struct 
{
  unsigned ncodies; 
  const cody_data_t*  cody[6];
} matches [RADAR_BUF_SIZE]; 


const char * rfsoc_hostname = "192.168.97.30"; 
int interrupt_gpio =23; 

const char * ack_serial =  "/dev/serial/by-id/usb-Xilinx_JTAG+3Serial_68646-if02-port0"; 
const char * gps_serial = "/dev/ttyAMA0"; 
const char * mosquitto_host = "localhost"; 
const char * topicname = "/ret/SS%d/science_file"; 
int mosquitto_port = 1883; 
uint8_t cody_mask = 0x3f; 
uint8_t ncody; 

int noutdirs = 2; 
const char* outdirs[MAX_OUT_DIRS] = { "/data0", "/data1", 0}; 

static pthread_t the_radar_thread;;
static pthread_t the_collect_and_write_thread;  

volatile int break_flag = 0; 

double max_age = 30; // max age before giving up trying to write as a normal event
double assoc_thresh = 1e-3; // 1 ms 

void sighandler(int sig) 
{
  (void) sig; 
  break_flag = 1; 
}

double delta_time(const struct timespec * a, const struct timespec *b) 
{
  return (a->tv_sec - b->tv_sec) + 1e9*(a->tv_nsec - b->tv_nsec); 
}

struct timespec newest_radar; 
struct timespec newest_cody; 


void usage() 
{
  fprintf(stderr, "packetizer [-H RFSOC_HOSTNAME=%s] [-i rfsoc-interrupt-gpio = %d] [-a rfsoc_ack_serial = %s ]\n", rfsoc_hostname, interrupt_gpio, ack_serial);
  fprintf(stderr, "           [-g rfsoc_gps_serial = %s] [-M mosquitto host = %s] [-C CODY_MASK = 0x%hhx]\n", gps_serial, mosquitto_host, cody_mask); 
  fprintf(stderr, "           [-p mosquitto-port= %d] [-o0 outdir=%s] [-o1 outdir2 = %s (you can specify up to 16 outputs)][-t topicname = %s]\n", mosquitto_port, outdirs[0], outdirs[1],topicname); 
  fprintf(stderr, "           [-A max_age = %f]\n", max_age);
}



int parse(int nargs, char ** args) 
{
  for (int i = 1; i < nargs; i++) 
  {
    if (i == nargs -1) return 1; 

    if (!strcmp(args[i],"-H"))
    {
      rfsoc_hostname = args[++i]; 
    }
    else if (!strcmp(args[i],"-i"))
    {
      interrupt_gpio = atoi(args[++i]); 
    }
    else if (!strcmp(args[i],"-a"))
    {
      ack_serial = args[++i];
    }
    else if (!strcmp(args[i],"-g"))
    {
      gps_serial = args[++i];
    }
    else if (!strcmp(args[i],"-M"))
    {
      mosquitto_host = args[++i]; 
    }
    else if (!strcmp(args[i],"-C"))
    {
      cody_mask = strtol(args[++i],0,0); 
    }
    else if (!strcmp(args[i],"-p"))
    {
      mosquitto_port = atoi(args[++i]);
    }
    else if (args[i] == strstr(args[i],"-o"))
    {
      int ioutput = -1; 
      sscanf(args[i], "-o%d", &ioutput); 
      if (ioutput >=0 && ioutput < MAX_OUT_DIRS)
      {
        outdirs[ioutput] = args[++i]; 
        if (ioutput+1 > noutdirs) noutdirs = ioutput+1; 
        printf("Using output directory %d of %s\n", ioutput, outdirs[ioutput]); 
      }
    }
    else if (!strcmp(args[i],"-t"))
    {
      topicname = args[++i]; 
    }
    else
    {
      return 1; 
    }
  }

  return 0;
}



// radar thread grabs radar events, puts them in a the radar buffer
void * radar_thread(void* ignored)  
{

  (void) ignored; 

  while(!break_flag) 
  {

    static ret_radar_data_t data;
    if (!ret_radar_next_event(radar, &data))
    {
      
      while(1) 
      {

        if (!radar_buf_free)
        {
          usleep(100e3); 
          continue; 
        }
        
        int parallel_universe = 0; 

        //we are trying to add to the queue, so we need to make sure nothing is removed at the same time,
        //so we lock it
        pthread_mutex_lock(&radar_buf_lock); 
        for (size_t i = 0; i < RADAR_BUF_SIZE; i++) 
        {
          if (radar_buf_read_times[i].tv_sec == 0 && radar_buf_read_times[i].tv_nsec == 0)
          {
            ret_radar_fill_time(&data.gps, &radar_buf_read_times[i]); 
            if (delta_time(&radar_buf_read_times[i], &newest_radar) > 0)
            {
              newest_radar = radar_buf_read_times[i];
            }
            else
            {
              parallel_universe++; 
            }

            memcpy(&radar_buf[i], &data, sizeof(data)); 
            radar_buf_free--; 
          }
        }
        pthread_mutex_unlock(&radar_buf_lock); 

        if (parallel_universe) 
           fprintf(stderr, "WARNING: Radar events might be going back in time?\n"); 
      }
    }
  }
  return NULL; 
}


void * collect_and_write_thread(void* ignored) 
{

  (void) ignored; 

  while (!break_flag) 
  {
    // check if any rfsoc events are ready, if so start a match... 
    // with the same index as the buffer
    
    if (radar_buf_free < RADAR_BUF_SIZE)
    {
      //retrieve the ones that are filled.
      static uint8_t filled_entries[RADAR_BUF_SIZE] = {0}; 
      struct timespec newest_radar_copy;
      pthread_mutex_lock(&radar_buf_lock); 
      for (size_t i = 0; i < RADAR_BUF_SIZE; i++)
      {
        filled_entries[i] = !!radar_buf_read_times[i].tv_sec; 
      }
      newest_radar_copy = newest_radar; 
      pthread_mutex_unlock(&radar_buf_lock); 

      //now go through the codies in our mask 
      //now loop over again

      for (int i = 0; i < RADAR_BUF_SIZE; i++) 
      {
        if (!filled_entries[i]) continue; 

        for (int icody = 0; icody<6; icody++) 
        {
          if ( (cody_mask & (1 << icody)) == 0) continue; 

          //no match yet for this cody 
          if ( !matches[i].cody[icody])
          {

            int nrdy = cody_listener_nrdy(cody_list, icody+1); //cody here is 1-indexed
            if (!nrdy) continue;                                                                

            
            for (int irdy = 0; irdy < nrdy; irdy++) 
            {

              const cody_data_t * cody = cody_listener_get(cody_list, icody+1, irdy); 

              struct timespec ts; 
              cody_data_fill_time(cody,&ts); 

              if (delta_time(&ts, &newest_cody) > 0) 
              {
                newest_cody = ts; 
              }


              if (fabs(delta_time(&radar_buf_read_times[i], &ts)) < assoc_thresh)
              {
                
                 matches[i].cody[icody] = cody; 
                 matches[i].ncodies++; 
                 break; 
              }

              double age = delta_time(&newest_cody, &ts);
              // check if we have passed max age on this cody 
              if (age > max_age)
              {
                fprintf(stderr,"Found unmatched event that is %f seconds older than the newest CODY event. Writing it out to stag folder.\n", age); 
                ret_writer_write_stag_cody(writer, cody, icody+1); 
                cody_listener_release(cody_list,cody); 
              }
            }
          }
        }

        //see if we have a complete match, or if we need to max_age...  
        if (matches[i].ncodies == ncody || delta_time(&newest_radar_copy,&radar_buf_read_times[i]) > max_age)
        {
          ret_full_event_t full = {
            .radar = &radar_buf[i], 
             .cody = { 
               matches[i].cody[0],
               matches[i].cody[1], 
               matches[i].cody[2], 
               matches[i].cody[3], 
               matches[i].cody[4], 
               matches[i].cody[5]
             }
          };

          ret_writer_write_event(writer, &full); 

          //now we can release everythig related to this event
          mark_free(i); 
          for (int icody = 0; icody < 6; icody++) 
          {
            if (matches[i].cody[icody])
            {
              cody_listener_release(cody_list,matches[i].cody[icody]); 
              matches[i].cody[icody] = NULL; 
            }
          }
        }
      }
    }
    else
    {
      usleep(500); 
    }
  }


  return NULL; 
}




int main(int nargs, char ** args) 
{

  if (parse(nargs, args))
  {
    usage(); 
    return 1; 
  }

  ncody = __builtin_popcount(cody_mask); 

  signal(SIGINT, sighandler); 

  radar = ret_radar_open(rfsoc_hostname, interrupt_gpio, ack_serial, gps_serial);

  if (!radar) 
  {
    fprintf(stderr, "Could not open rfsoc at %s\n", rfsoc_hostname); 
    return 1; 
  }

  cody_list = cody_listener_init(mosquitto_host, mosquitto_port, topicname, cody_mask, 64,0); 
  if (!cody_list) 
  {
    fprintf(stderr, "Could not open cody listener at %s:%d\n", mosquitto_host, mosquitto_port); 
    return 1; 
  }

  writer = ret_writer_multi_init(noutdirs, outdirs); 
  if (!writer) 
  {
    fprintf(stderr,"Could int initialize writer\n"); 
    return 1; 
  }



  pthread_create(&the_radar_thread, NULL, radar_thread, NULL); 
  pthread_create(&the_collect_and_write_thread, NULL, collect_and_write_thread, NULL); 
  
  while (!break_flag)
  {
    //kick the watchdog, if we're runnign with one. 
    sd_notify(0,"WATCHDOG=1"); 
    sleep(5); 
  }


  pthread_join(the_radar_thread, 0);
  ret_radar_close(radar); 
  cody_listener_finish(cody_list); 
  pthread_join(the_collect_and_write_thread, 0);
  ret_writer_destroy(writer); 

  return 0; 
}



