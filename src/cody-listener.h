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



//Return the number of ready events at this moment for the specified cody. Note that this is 1-indexed. 
int cody_listener_nrdy(cody_listener_t * l, uint8_t cody); 
//Grab  the data corresponding to the ith available event for this cody. Note that what the ith event is can change between calls
//even without you having done anything, so don't make multiple calla ssuming you always get the same thing. Note that this is 1 indexed.
const cody_data_t * cody_listener_get(cody_listener_t *l, uint8_t cody, uint32_t i); 

/** Release data associated with this pointer, allowing it to be reused later */ 
int cody_listener_release(cody_listener_t *l, const cody_data_t * data);

//wait for any of the codies to be ready. Returns 0 if none are after timeout, otherwise the 1-indexed cody. Timeout of 0 means no timeout; 
int cody_listener_wait(cody_listener_t * l, float timeout); 

void  cody_listener_finish(cody_listener_t *l); 


#endif
