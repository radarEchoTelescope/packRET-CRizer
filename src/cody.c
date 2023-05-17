#include "cody.h"

int cody_data_fill_time(const cody_data_t * d, struct timespec *t)
{
  if (!d || !t) return -1; 
  t->tv_nsec = d->sci.gps_subtime.big;
  t->tv_sec = d->sci.gps_time.big + 315964800-18; // UTC to GPS epoch correction and number of leap seconds after 1980 
  return 0; 
}


int cody_data_dump(FILE *f, const cody_data_t * d, int indent) 
{
  int nwr = 0; 

#define dump_u16(x) nwr += fprintf(f,"%*s  \"" #x "\": %hu,\n", indent, " ", d->hdr.x) 

  dump_u16(packet_len); 
  dump_u16(station_event); 
  dump_u16(event_len); 
  dump_u16(t3_ev_count); 
  dump_u16(station_id); 
  dump_u16(header_len); 

#undef dump_u16
#define dump_u16(x) nwr += fprintf(f,"%*s  \"" #x "\": %hu,\n", indent, " ", d->sci.x) 
#define dump_bw(x) nwr += fprintf(f,"%*s  \"" #x "\": %u,\n", indent, " ", d->sci.x.big) 
#define dump_arr(x,format,N) fprintf(f,"%*s  \"" #x "\" : [", indent, " "); for (int i = 0; i < N; i++) nwr += fprintf(f,format "%s", d->sci.x[i], i < N-1 ? "," :"")  ; fprintf(f,"],\n"); 


  dump_bw(gps_time);
  dump_bw(gps_subtime);
  dump_u16(trigger_flag);
  dump_u16(trigger_pos);
  dump_u16(sampling_freq);
  dump_u16(ch_mask);
  dump_u16(ch_mask);
  dump_u16(trace_len);
  dump_u16(event_version);
  dump_u16(event_status);
  dump_u16(pps_delay);
  dump_bw(gps_x0_slipping_avg); 
  dump_bw(gps_lat_marcs); 
  dump_bw(gps_lon_marcs); 
  dump_bw(gps_height_cm); 
  dump_arr(threshold, "%hu", 2);
  dump_arr(header, "%hu", 3);
  dump_u16(ew_first_sample); 
  dump_u16(ew_scaler); 
  dump_u16(ew_reset_baseline); 
  dump_u16(ns_first_sample); 
  dump_u16(ns_scaler); 
  dump_u16(ns_reset_baseline); 
  dump_u16(post_trigger); 
  dump_u16(first_column); 
  dump_u16(mode); 
  dump_u16(crate_power); 
  dump_u16(crate_temperature); 
  dump_arr(ew_samples, "%hu", 2560);
  dump_arr(ns_samples, "%hu", 2560);
  return nwr; 
}

