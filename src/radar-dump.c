#include "radar.h" 
#include <stdio.h> 


static ret_radar_data_t data; 
int main(int nargs, char ** args) 
{

  if (nargs <2) return 1; 
  FILE * f = fopen(args[1],"r"); 

  fread(&data, sizeof(data),1,f); 
  printf("{\n  \"radar\"=\n"); 
  ret_radar_rfsoc_dump(f, &data.rfsoc,4); 
  printf("\n  \"gps\"=\n");
  ret_radar_gps_tm_dump(f, &data.gps,4); 
  printf("\n}\n"); 

  fclose(f); 
  return 0;

}
