
#include "listener.h" 
#include <mosquitto.h> 
#include <sys/inotify.h> 
#include <pthread.h>


struct 
{
  enum
  {
    LISTENER_MQTT, 
    LISTENER_INOTIFY
  } type; 

  union 
  {
    struct inotify
    {



    };

    struct mqtt
    {



    }
  } setup; 
  listener_callback_t callback;
  int callback_min_avail; 
} listener; 



size_t nlisteners_used; 
size_t nlisteners_alloc; 
struct listener *listeners[256];



//thread handles inotify listens 
static void * inotify_thread(void *) 
{



}

//thread handles mqtt 
static void * inotify 


listener_handle_t listener_mqtt_open(const char * server, int port, const char * topic, size_t bufsiz)
{




}



