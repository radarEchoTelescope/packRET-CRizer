
#include "radar.h" 
#include <stdio.h> 
#include <time.h> 


const char * serial = "/dev/ttyAMA1"; 

int main(int nargs, char ** args)
{

  if (nargs > 1) serial = args[1]; 


  ret_radar_hk_t * hk = ret_radar_hk_open(serial); 
  if (!hk) 
  {
    printf("Could not open %s\n", serial); 
    return 1; 
  }

  ret_radar_hk_data_t data; 

  ret_radar_hk_fill(hk,&data); 
  time_t now = time(0); 
  struct tm * gmt = gmtime(&now); 
  printf(":::At %s", asctime(gmt)); 
  printf("Board Temperature: %f\n", data.board_temp); 
  printf("Air Temperature: %f\n", data.air_temp); 
  printf("Vin: %f\n", data.vin); 
  

  ret_radar_hk_close(hk); 

  return 0; 



}
