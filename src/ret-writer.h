#ifndef _RET_WRITER_H
#define _RET_WRITER_H


typedef struct ret_writer ret_writer_t; 

typedef struct cody_data cody_data_t; 
typedef struct ret_radar_data ret_radar_data_t;  

// a full event, this is what is written or read... 
typedef struct ret_full_event
{
  ret_radar_data_t * radar; //radar data, can be NULL
  cody_data_t * cody[6];  //cody data, can be NULL 
} ret_full_event_t; ; 



//Initialize writer to output director
ret_writer_t * ret_writer_init(const char * output_directory); 

// write radar and cody data. It's ok if some of them are NULL, but if all are NULL nothing will happen 
int ret_writer_write_event(ret_writer_t * w, const ret_full_event_t *ev); 
void ret_writer_destroy(ret_writer_t * w); 

// reads a tar file into a ret_full_event_t
// if any of the pointers within the event are NULL, this will allocate them. Otherwise will reuse them. 
// User will eventually be responsible for freeing... 
int ret_read_full_event(const char * tarfile, ret_full_event_t * ev); 

//frees pointers within the event and sets them to NULL. Does NOT free the event itself (Because we don't know where it was allocated) 
int ret_free_full_event(ret_full_event_t * ev); 



#endif