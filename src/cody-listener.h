#ifndef _CODY_LISTENER_H
#define _CODY_LISTENER_H

#include "cody.h"

typedef struct cody_listener cody_listener_t;

/** Initialize a cody listener listening to packets from the given mqtt_host on the given port.
 * Topic format string is something with a %d in it (only one % is allowed and it must be %d), and cody_mask is a mask of cody stations (1-indexed) to subscribe to. 
 * To subscribe to all 6 nominal stations, pass a mask of 0x3f (or... '?' if you want to be cute).  
 * Nbuf is how many CODY messages to make room for per station. 
 * emerquency_queue, if not null, is a directory where messages that would not fit in our receive buffer are written... 
 * ideally it's not used. 
 * 
 * */ 
cody_listener_t * cody_listener_init(const char * mqtt_host, 
                                     int mqtt_port, 
                                     const char * topic_format_string, 
                                     uint8_t cody_mask,
                                     int Nbuf, const char * emergency_queue ); 



int cody_listener_nrdy(cody_listener_t * l, uint8_t cody); 
cody_data_t * cody_listener_get(cody_listener_t *l, uint8_t cody, uint32_t i); 
void cody_listener_release(cody_listener_t *l, uint8_t cody, uint32_t i); 
void  cody_listener_finish(cody_listener_t *l); 


#endif
