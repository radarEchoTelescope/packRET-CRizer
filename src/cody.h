#ifndef _CODY_H
#define _CODY_H

#include <stdint.h> 
#include <stdio.h> 
#include <time.h> 

#ifdef __cplusplus
extern "C" { 
#endif 

typedef struct cody_header
{
  uint16_t packet_len; 
  uint16_t station_event; 
  uint16_t event_len; 
  uint16_t t3_ev_countt; 
  uint16_t station_id; 
  uint16_t header_len; 

} cody_header_t;


typedef union cody_bigword
{
  struct 
  {
    uint16_t lsw; 
    uint16_t msw; 
  } normal; 
  uint32_t big;
} cody_bigword_t;


typedef struct 
__attribute__ ((packed,aligned((2))))
cody_science
{
  cody_bigword_t gps_time;
  cody_bigword_t gps_subtime;
  uint16_t trigger_flag; 
  uint16_t trigger_pos; 
  uint16_t sampling_freq; 
  uint16_t ch_mask;
  uint16_t adc_res; 
  uint16_t trace_len; 
  uint16_t event_version; 
  uint16_t event_status; 
  uint16_t pps_delay; 
  cody_bigword_t gps_x0_slipping_avg;
  cody_bigword_t gps_lat_marcs;
  cody_bigword_t gps_lon_marcs; 
  cody_bigword_t gps_height_cm; 
  uint16_t threshold[2]; 
  uint16_t header[3]; 
  uint16_t ew_first_sample;
  uint16_t ew_scaler; 
  uint16_t ew_reset_baseline; 
  uint16_t ns_first_sample;
  uint16_t ns_scaler; 
  uint16_t ns_first_baseline; 
  uint16_t post_trigger; 
  uint16_t first_column; 
  uint16_t mode; 
  uint16_t crate_power; 
  uint16_t crate_temperature; 
  uint16_t t1_counter; 
  uint16_t ew_samples[2560]; 
  uint16_t ns_samples[2560]; 
} cody_science_t; 

typedef struct
__attribute__ ((packed,aligned((2))))
cody_data
{
   uint16_t len; // not documented, but seems to be here 
   cody_header_t hdr; 
   cody_science_t sci;  
} cody_data_t; 


int cody_data_fill_time(const cody_data_t * d, struct timespec *t);
int cody_data_dump(FILE *f, const cody_data_t * d, int indent); 

#ifdef __cplusplus
}
#endif
#endif
