#define _GNU_SOURCE
#include "radar.h" 
#include <stdio.h> 
#include <signal.h> 
#include <time.h>
#include <string.h> 
#include <stdlib.h> 

static ret_radar_data_t d;

const char * hostname = "192.168.97.30"; 
int interrupt_gpio = 23;
const char * ack_serial =  "/dev/serial/by-id/usb-Xilinx_JTAG+3Serial_68646-if02-port0"; 
const char * gps_serial = "/dev/ttyAMA0"; 
int N = -1; 

int noutput_dirs = 0;
const char * output_dir[16] = {0};
char * output_name[16] = {0}; 

volatile int break_flag = 0; 

void sighandler(int sig) 
{
  (void) sig; 
  break_flag = 1; 
}


void usage() 
{
  fprintf(stderr, "radar-get [-h HOSTNAME=%s] [-i interrupt-gpio = %d] [-a ack_serial = %s ] [-g gps_serial = %s] [-N number = %d] [-o output_dir =(none)] [-o additional_output_dir=(none)] \n",
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

  int nevents = 0; 

  printf("{\n  \"events\" = [ "); 
  while(1) 
  {
    if (N > 0 && nevents > N) break; 
    if (!ret_radar_next_event(radar, &d)) 
    {
      printf("  {\n"); 
      printf("    \"i\"=%d\n", nevents); 
      printf("    \"radar\" :"); 
      ret_radar_rfsoc_dump(stdout,&d.rfsoc, 6); 
      printf("    , \"gps_tm\" :\n"); 
      ret_radar_gps_tm_dump(stdout,&d.gps, 6); 
      printf("   }\n"); 
      nevents++; 
      if (noutput_dirs > 0) 
      {
        struct timespec ts; 
        ret_radar_fill_time(&d.gps, &ts); 
        struct tm * gmt = gmtime(&ts.tv_sec); 

        for (int ioutput = 0; ioutput < noutput_dirs; ioutput++) 
        {
          if (!output_name[ioutput])
          {
            asprintf(&output_name[ioutput],"%s/%d-%02d-%2d.%02d%02d%02d.%09ld.RAD", output_dir[ioutput],
                gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec, ts.tv_nsec);
          }
          else
          {
            sprintf(output_name[ioutput],"%s/%d-%02d-%2d.%02d%02d%02d.%09ld.RAD", output_dir[ioutput],
                gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec, ts.tv_nsec);
          }

          FILE * f = fopen(output_name[ioutput],"w"); 
          fwrite(&d, sizeof(d), 1, f); 
          fclose(f); 
        }
      }
    }
    if (break_flag) break; 
  }

  printf("  ]\n}\n"); 
  ret_radar_close(radar); 
  return 0; 
}


