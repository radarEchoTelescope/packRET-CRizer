#define _GNU_SOURCE
#include "radar.h" 
#include <stdio.h> 
#include <signal.h> 
#include <time.h>
#include <string.h> 
#include "ret-writer.h" 
#include <stdlib.h> 


static ret_radar_data_t d;

const char * hostname = "192.168.97.30"; 
int interrupt_gpio = 23;
const char * ack_serial =  "/dev/serial/by-id/usb-Xilinx_JTAG+3Serial_68646-if02-port0"; 
const char * gps_serial = "/dev/ttyAMA0"; 
int N = -1; 
int compress = 0;
int verbose = 0; 
int quiet = 0; 

int noutput_dirs = 0;
const char * output_dir[16] = {0};

volatile int break_flag = 0; 

void sighandler(int sig) 
{
  (void) sig; 
  break_flag = 1; 
}


void usage() 
{
  fprintf(stderr, "radar-get [-h HOSTNAME=%s] [-i interrupt-gpio = %d] [-a ack_serial = %s ] [-g gps_serial = %s] [-N number = %d] [-o output_dir =(none)] [-o additional_output_dir=(none)] [-z] [-V] [-q]\n",
      hostname, interrupt_gpio, ack_serial, gps_serial, N); 
  exit(1); 
}

int main(int nargs, char ** args)
{

  for (int i = 1; i < nargs; i++) 
  {

    if (!strcmp(args[i],"-z"))
    {
      compress = 1; 
      continue; 
    }

    if (!strcmp(args[i],"-V"))
    {
      verbose = 1; 
      continue; 
    }

    if (!strcmp(args[i],"-q"))
    {
      quiet = 1; 
      continue; 
    }




    if (i == nargs -1) usage(); 

    if (!strcmp(args[i],"-h"))
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
    else if (!strcmp(args[i],"-N"))
    {
      N = atoi(args[++i]);
    }
    else if (!strcmp(args[i],"-o"))
    {
      if (noutput_dirs < 16) 
      {
        output_dir[noutput_dirs++] = args[++i]; 
      }
      else
      {
        fprintf(stderr,"Maximum of 16 output dirs!\n"); 
        usage(); 
      }
    }
    else
    {
      usage(); 
    }
  }


  signal(SIGINT, sighandler); 

  ret_radar_t * radar = ret_radar_open(hostname, interrupt_gpio, ack_serial, gps_serial);
  ret_radar_set_verbose(radar,verbose); 
  ret_radar_set_timeout(radar,1); 

  int nevents = 0; 

  ret_writer_t * w = 0; 

  if (noutput_dirs > 0) 
  {
     w = ret_writer_multi_init(noutput_dirs, output_dir, compress);  
  }

  if (!quiet) 
    printf("{\n  \"events\" = [ "); 
  while(1) 
  {
    if (N > 0 && nevents > N) break; 
    memset(&d,0,sizeof(d)); 
    if (!ret_radar_next_event(radar, &d)) 
    {
      if (!quiet) 
      {
        printf("  {\n"); 
        printf("    \"i\":%d,\n", nevents); 
        printf("    \"radar\" :"); 
        ret_radar_rfsoc_dump(stdout,&d.rfsoc, 6); 
        printf("    , \"gps_tm\" :\n"); 
        ret_radar_gps_tm_dump(stdout,&d.gps, 6); 
        printf("   }\n"); 
      }
      else
      {
        printf(" Event %d, crc: %u\n", nevents, d.rfsoc.crc32); 
      }
      fflush(stdout); 

      nevents++; 
      if (noutput_dirs > 0) 
      {
        ret_writer_write_radar(w, &d); 
      }
      if (break_flag) break; 
    }
    if (break_flag) break; 
  }

   if (!quiet) printf("  ]\n}\n"); 
  ret_radar_close(radar); 
  return 0; 
}


