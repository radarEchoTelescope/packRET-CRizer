#include "cody-listener.h" 
#include "radar.h" 
#include "ret-writer.h" 
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <systemd/sd-daemon.h> 


struct ret_radar_t * radar; 
struct cody_listener_t * cody_list; 
struct ret_writer_t * writer; 

size_t radar_buf_sz; 
ret_radar_data_t * radar_buf; 

const char * rfsoc_hostname = "192.168.97.30"; 
int interrupt_gpio =23; 

const char * ack_serial =  "/dev/serial/by-id/usb-Xilinx_JTAG+3Serial_68646-if02-port0"; 
const char * gps_serial = "/dev/ttyAMA0"; 
const char * mosquitto_host = "localhost" 
const char * topicname = "/ret/SS%d/science_file"
int mosquitto_port = 1883; 
uint8_t cody_mask = 0x3f; 

const char* outdir = "/data"; 

static pthread_t the_radar_thread;;
static pthread_t the_cody_thread;  
static pthread_t the_collect_and_write_thread;  

volatile int break_flag = 0; 

int max_age = 30; // max age before giving up trying to write 
double assoc_thresh = 1e-3; // 1 ms 

void sighandler(int sig) 
{
  (void) sig; 
  break_flag = 1; 
}



void usage() 
{
  fprintf(stderr, "packetizer [-H RFSOC_HOSTNAME=%s] [-i rfsoc-interrupt-gpio = %d] [-a rfsoc_ack_serial = %s ]\n", hostname, interrupt_gpio, ack_serial);
  fprintf(stderr, "           [-g rfsoc_gps_serial = %s] [-M mosquitto host = localhost] [-C CODY_MASK = 0x%x]\n", gps_serial, cody_mask); 
  fprintf(stderr,"            [-p mosquitto-port= %d] [-o outdir=%s] [-t topicname = %s]\n", mosquitto_port, outdir,topicname); 
}



int parse(int nargs, char ** args) 
{
  for (int i = 1; i < nargs; i++) 
  {
    if (i == nargs -1) return 1; 

    if (!strcmp(args[i],"-H"))
    {
      hostname = args[++i]; 
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
      cody_mask = strtod(args[++i],0,0); 
    }
    else if (!strcmp(args[i],"-p"))
    {
      mosquitto_port = atoi(args[++i])
    }
    else if (!strcmp(args[i],"-o"))
    {
      outdir = args[++i]; 
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



// radar thread grabs radar events, puts them in a the radar bufber
void * radar_thread(void*)  
{


}

void * cody_thread(void*) 
{


}

void * collect_and_write_thread(void*) 
{



}




int main(int nargs, char ** args) 
{

  if (parse(nargs, args))
  {
    usage(); 
    return 1; 

  signal(SIGINT, sighandler); 

  radar = ret_radar_open(rfsoc_hostname, interrupt_gpio, ack_serial, gps_serial);

  if (!radar) 
  {
    fprintf(stderr, "Could not open rfsoc at %s\n", rfsoc_hostname); 
    return 1; 
  }

  cody_list = cody_listener_init(mosquitto_host, mosquitto_port, topic_string, cody_mask, 64,0); 
  if (!cody_list) 
  {
    fprintf(stderr, "Could not open cody listener at %s:%d\n", mosquitt_host, mosquitto_port); 
    return 1; 
  }

  writer = ret_writer_init(outdir); 
  if (!writer) 
  {
    fprintf(stderr,"Could int initialize writer with out dir %s\n", outdir); 
    return 1; 
  }

  pthread_create(&the_radar_thread, NULL, radar_thread, NULL); 
  pthread_create(&the_cody_thread, NULL, cody_thread, NULL); 
  pthread_create(&the_collect_and_write_thread, NULL, collect_and_write_thread, NULL); 
  
  while (!break_flag)
  {
    //kick the watchdog, if we're runnign with one. 
    sd_notify(0,"WATCHDOG=1"); 
    sleep(5); 
  }


  pthread_join(the_radar_thread, 0);
  ret_radar_close(radar); 
  pthread_join(the_cody_thread, 0);
  cody_listener_finish(cody_list); 
  pthread_join(the_collect_and_write_thread, 0);
  ret_writer_destroy(writer); 

  return 0; 
}



