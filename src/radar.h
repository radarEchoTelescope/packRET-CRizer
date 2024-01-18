#ifndef _RET_RADAR_H
#define _RET_RADAR_H

#include <stdint.h>
#include <stdio.h>
#include <time.h> 

typedef struct ret_radar ret_radar_t; 
typedef struct ret_radar_hk ret_radar_hk_t; 

typedef struct ret_radar_gps_tm 
{
  uint16_t count; 
  uint16_t weeknum; 
  uint32_t tow; 
  uint32_t tow_f; 
  uint32_t acc;  // if this is 0xffffff this means it's the CPU time 
} ret_radar_gps_tm_t; 



#ifndef u32 
#define u32 uint32_t
#endif

#ifndef XTime
#define XTime uint64_t
#endif

#ifndef u64
#define u64 uint64_t 
#endif

#ifndef s16 
#define s16 int16_t
#endif

typedef struct ret_radar_rfsoc_data {
  u32 crc32;
  u32 index;
  u32 struct_version;
  u64 cpu_time;
  u32 trigger_info;
  u64 l0_station_counter[12];
  u32 l1_station_counter_second[6];
  u32 l1_station_counter_subsecond[6];
  u32 l0_rate_monitor[12];
  u32 l1_rate_monitor[6];
  u32 adc_read_index;
  u32 rf_threshold;
  u32 n_surface_stations;
  u32 rf_read_window;
  u32 carrier_cancel_flag;
  u32 tx_atten;
  u32 cc_atten[4];
  float tx_phase;
  float tx_amplitude;
  float cc_amplitude;
  float cc_delay[4];
  u32 tx_mode;
  float tx_freq;
  u32 priority;
  s16 adc_0_data[16384];
  s16 adc_1_data[16384];
  s16 adc_2_data[16384];
  s16 adc_3_data[16384];
}ret_radar_rfsoc_data_t;


typedef struct ret_radar_data
{
  ret_radar_rfsoc_data_t rfsoc; 
  ret_radar_gps_tm_t gps; 
} ret_radar_data_t; 

typedef struct ret_radar_hk_data
{
  float board_temp; 
  float air_temp; 
  float vin; 
} ret_radar_hk_data_t; 

// open a handle 
//   hostname is the hostname to connect to ,
//   interrupt_gpio is the gpio number for the interrupt, if you pass 0 or a negative number, a fifo will be used with the name /tmp/ret-fifo[-N] where N is the absolute value of the number if non-zero
//   ack_serial is the serial device to send acks too 
//   gps_serial_or_file is the serial device or file to get GPS time marks from

ret_radar_t *ret_radar_open(const char * hostname, int interrupt_gpio, const char* ack_serial, const char * gps_serial_or_file); 

// open a handle for reading housekeeping (this would generally be asynchronous from events) 
ret_radar_hk_t * ret_radar_hk_open(const char * hk_serial);

// set the read timeout (in seconds). 0 for none (default)
void ret_radar_set_timeout(ret_radar_t * h, double timeout) ;
void ret_radar_set_verbose(ret_radar_t *h, int verbose); 

// poll for the next event, filling it 
// returns 0 if successful, -1 if timeout exceeded or another error. 
int ret_radar_next_event(ret_radar_t * h, ret_radar_data_t * d); 

// check the CRC of the data. Returns 0 if correct. 
int ret_radar_data_check_crc(const ret_radar_rfsoc_data_t * d); 

//dumps data as json 
int ret_radar_rfsoc_dump(FILE *f , const ret_radar_rfsoc_data_t * data, int indent); 
int ret_radar_gps_tm_dump(FILE *, const ret_radar_gps_tm_t * data, int indent); 
int ret_radar_fill_time(const ret_radar_gps_tm_t *g, struct timespec *t); 

int ret_radar_hk_fill(ret_radar_hk_t  * h, ret_radar_hk_data_t * hk); 

//close handle
void ret_radar_close(ret_radar_t *h ); 
void ret_radar_hk_close(ret_radar_hk_t *h ); 



#endif
