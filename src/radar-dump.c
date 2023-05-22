#include "radar.h" 
#include <stdio.h> 
#include <zlib.h> 


static ret_radar_data_t data; 
int main(int nargs, char ** args) 
{


  printf("{\n  \"events\": [\n"); 

  for (int i = 1; i<nargs; i++) 
  {
    gzFile f = gzopen(args[i],"r"); 
    gzread(f, &data, sizeof(data)); 
    printf("  {\n     \"radar\":\n"); 
    ret_radar_rfsoc_dump(stdout, &data.rfsoc,6); 
    printf(",\n     \"gps\":\n");
    ret_radar_gps_tm_dump(stdout, &data.gps,6); 
    printf("\n  },\n"); 

    gzclose(f); 
  }
  printf("  ]\n}\n"); 
  return 0;

}
