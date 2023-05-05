#include "radar.h" 
#include <stdio.h> 
#include <signal.h> 
#include <string.h> 
#include <stdlib.h> 

static ret_radar_data_t d;
static ret_radar_gps_tm_t tm;

const char * hostname = "192.168.98.40"; 
int interrupt_gpio = 23;
const char * ack_serial =  "/dev/serial/by-id/usb-Xilinx_JTAG+3Serial_68646-if02-port0"; 
const char * gps_serial = "/dev/ttyAMA0"; 
int N = -1; 

volatile int break_flag = 0; 

void sighandler(int sig) 
{
  (void) sig; 
  break_flag = 1; 
}


void usage() 
{
  fprintf(stderr, "radar-get [-h HOSTNAME=%s] [-i interrupt-gpio = %d] [-a ack_serial = %s ] [-g gps_serial = %s] [-N number = %d]\n",
      hostname, interrupt_gpio, ack_serial, gps_serial, N); 
  exit(1); 
}

int main(int nargs, char ** args)
{

  for (int i = 1; i < nargs; i++) 
  {
    if (i == nargs -1) usage(); 

    if (!strcmp(args[i],"-h"))
    {
      hostname = args[i++]; 
    }

    if (!strcmp(args[i],"-i"))
    {
      interrupt_gpio = atoi(args[i++]); 
    }

    if (!strcmp(args[i],"-a"))
    {
      ack_serial = args[i++];
    }

    if (!strcmp(args[i],"-g"))
    {
      gps_serial = args[i++];
    }
    if (!strcmp(args[i],"-N"))
    {
      N = atoi(args[i++]);
    }
  }


  signal(SIGINT, sighandler); 

  ret_radar_t * radar = ret_radar_open(hostname, interrupt_gpio, ack_serial, gps_serial);

  int nevents = 0; 

  printf("{\n  \"events\" = [ "); 
  while(1) 
  {
    if (N > 0 && N > nevents) break; 
    if (!ret_radar_next_event(radar, &tm, &d)) 
    {
      printf("  {\n"); 
      printf("    \"i\"=%d\n", nevents); 
      printf("    \"radar\" :"); 
      ret_radar_dump(stdout,&d, 6); 
      printf("    , \"gps_tm\" :\n"); 
      ret_radar_gps_tm_dump(stdout,&tm, 6); 
      printf("   }\n"); 
      nevents++; 
    }
    if (break_flag) break; 
  }

  printf("  ]\n}\n"); 
  ret_radar_close(radar); 
  return 0; 
}


