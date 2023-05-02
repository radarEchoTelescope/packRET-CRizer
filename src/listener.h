#ifndef PACKRETCRIZER_LISTENER_H
#define PACKRETCRIZER_LISTENER_H

//dumb listener that either listens to an mqtt packet or a directory
// knows nothing about the payload 



typedef int listener_handle_t; 
typedef int mqtt_connection_t; 
typedef struct listener_msg 
{
  size_t size; 
  struct timespec rcv_time; 
  const void * data; 
} listener_msg_t; 

typedef int (*listener_callback_t) ( listener_handle_t h, int navail); 



mqtt_connection_t mqtt_connect( const char * server, const char * port); 

listener_handle_t listener_mqtt_open(const char * server, int port, const char * topic, size_t bufsiz); 
listener_handle_t listener_dir_open(const char * dir, const char * filter, int recursive); 

// add a callback when at least min_avail are available. Setitng this to 0 removes the callback. 
// only one callback per listener.. 
int listener_set_callback(listener_handle_t h,  listener_callback_t callback, unsigned min_avail); 

listener_msg_t  listener_peek(listener_handle_t h); 
int listener_pop(listener_handle_t h); 
int listener_navail(listener_handle_t h); 
int listener_remove(listener_handle_t h); 







#endif
