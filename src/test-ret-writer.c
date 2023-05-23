#include "ret-writer.h" 
#include "radar.h" 
#include <unistd.h>
#include "cody.h" 
#include "time.h" 


static ret_radar_data_t radar; 
static cody_data_t cody[6]; 
int main(int nargs, char ** args) 
{
  int noutputs = nargs > 1 ? nargs - 1 : 1; 
  const char * outdirs[noutputs]; 
  if (nargs == 1) 
  {
    outdirs[0] = "test"; 
  }
  else
  {
    for (int i = 1; i < nargs; i++) 
    {
      outdirs[i-1] = args[i]; 
    }
  }


  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now); 
  int gps_secs = now.tv_sec - 315964800 + 18; 
  int gps_weeks = gps_secs / (7*24*3600); 
  int tow = gps_secs - gps_weeks * (7*24*3600);                      
  gps_weeks -= 2048; // 2 rollovers 
  radar.gps.weeknum = gps_weeks; 
  radar.gps.tow = tow * 1000 + now.tv_nsec / 1000000; 
  radar.gps.tow_f = now.tv_nsec % 1000000; 
  printf("%ld.%09ld\n", now.tv_sec, now.tv_nsec);
  for (int i = 0; i < 6; i++) 
  {
    clock_gettime(CLOCK_REALTIME, &now); 
    printf("%ld.%09ld\n", now.tv_sec, now.tv_nsec);
    cody[i].sci.gps_subtime.big = now.tv_nsec; 
    cody[i].sci.gps_time.big = now.tv_sec - 315964800 + 18; 
    usleep(1000); 
  }

  ret_full_event_t ev = {.radar = &radar, .cody = { &cody[0], &cody[1], &cody[2], &cody[3], &cody[4], &cody[5]}}; 
  ret_writer_t *w = ret_writer_multi_init(noutputs, outdirs,1); 
  ret_writer_write_event(w,&ev); 
  ret_writer_destroy(w); 

  return 0; 
}
