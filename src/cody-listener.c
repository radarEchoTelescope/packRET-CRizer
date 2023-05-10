
#define _GNU_SOURCE
#include "cody-listener.h" 
#include "mosquitto.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h> 
#include <dirent.h>
#include <unistd.h> 
#include <sys/types.h>
#include <fcntl.h> 


#define NCODY 6

struct cody_data_wrapper
{
  int msgid; 
  struct timespec receive_time; 
  cody_data_t cody_data; 
};

struct cody_listener
{
  struct mosquitto * mosq; 
  char * topicbuf[NCODY]; 
  struct cody_data_wrapper * buf[NCODY]; 
  int buf_rdy[NCODY]; 
  pthread_mutex_t buf_mutex[NCODY]; 
  uint64_t * buf_freelist[NCODY];
  DIR * emergency; 
  int emergency_dirfd; 
  int bufsize; 
}; 

void msg_callback(struct mosquitto * mosq, void * unsafe_listener, const struct mosquitto_message *msg)
{
  struct timespec now; 
  (void) mosq; 
  clock_gettime(CLOCK_REALTIME, &now); 
  cody_listener_t * l = (cody_listener_t*) unsafe_listener; 

  //figure out which cody it is. since we don't know the string at compile time, faster to do a strcmp 

  int icody = -1; 
  for (int i = 0; i < 6; i++) 
  {
    if(!strcmp(msg->topic,l->topicbuf[i]))
    {
      icody = i;
    }
  }

  if (icody < 0)
  {
    fprintf(stderr,"How did we get topic %s\n?", msg->topic); 
    return; 
  }

  if (msg->payloadlen != sizeof(cody_science_t))
  {
    fprintf(stderr,"Unexpected payload length %d (expected %zu) in msgid %d\n", msg->payloadlen, sizeof(cody_science_t), msg->mid); 
    return; 
  }

  cody_data_t * cody = (cody_data_t*) msg->payload; 

  //verify that the time is non-zero 
  if (!cody->sci.gps_time.big)
  {
    fprintf(stderr,"Unexpected 0 gps_time in cody packet, discarding (cody=%d, msgid=%d)\n", icody+1, msg->mid); 
    return; 
  }

  pthread_mutex_lock(&l->buf_mutex[icody]); 

  //check if our buffer is full and write to emergency queue if it is 
  if (l->buf_rdy[icody] == l->bufsize) 
  {
    fprintf(stderr,"WARNING, queue for CODY %d is full. %s\n", icody+1, l->emergency? "Writing to emergency queue" : "Traces will be lost. Consider increasing buffer size or adding emergency queue\n"); 
    if (l->emergency)
    {
      char queue_name[128]; 
      struct timespec t; 
      cody_data_fill_time(cody, &t); 
      sprintf(queue_name,"CODY%d_%jd.%09d.dat\n", icody+1, t.tv_sec, (int) t.tv_nsec); 
      int efd = openat(l->emergency_dirfd, queue_name, O_CREAT | O_RDWR, S_IRUSR | S_IRGRP | S_IROTH); 

      if (!efd) 
      {
        fprintf(stderr, "Failed to open %s in emergency queue dir\n", queue_name); 
      }
      else
      {
         size_t nwr = 0;
         while (nwr < sizeof(cody_data_t))
         {
           int this_nwr =  write(efd, cody + nwr, sizeof(cody_data_t) - nwr);
           if (this_nwr < 0) 
           {
             fprintf(stderr,"uhoh, write failure (err %d) writing to %s\n", this_nwr, queue_name); 
             break; 
           }
           nwr += this_nwr; 
         }

         close(efd); 
      }
    }
  }
  else
  {
    int freelist_size = (l->bufsize+63)/64; 
    //let's find a free spot 
    for (int i = 0; i < freelist_size; i++) 
    {
      // 1 means free, so all zeros means not free. 
      if (l->buf_freelist[icody][i])
      {
        // if we are in the last entry, some bits might not ve valid. But that's ok,       
        // because if we got this far, these are the only openings left
        // if they were all full, we'd fail the bufsize check 
        
        int local_idx = __builtin_ffsll(l->buf_freelist[icody][i])-1; 
        //clear the bit, marking it as used. 
        l->buf_freelist[icody][i] &= ~(1 << local_idx); 
        int idx = i * 64 + local_idx; 

        //fill in our index 
        l->buf[icody][idx].msgid = msg->mid; 
        l->buf[icody][idx].receive_time = now; 
        memcpy(&l->buf[icody][idx].cody_data, cody, sizeof(cody_data_t)); 
      }
    }

    l->buf_rdy[icody]++; 
  }

  pthread_mutex_unlock(&l->buf_mutex[icody]); 
}


cody_listener_t * cody_listener_init (const char * mqtt_host, int port, const char * topicstr, 
    uint8_t mask, int Nbuf, const char * emergency_queue) 
{
  if (!mask || mask > 0x3f)
  {
    fprintf(stderr,"mask 0x%x is invalid; should only contain first 6 bits and shouldn't be empty\n", mask); 
    return NULL; 
  }

  //check to make sure topicstr is valid
  char * pct = strchr(topicstr,'%'); 
  if (!pct || (pct[1] !='d') || (strrchr(topicstr,'%')!=pct) || strchr(topicstr,'#') || strchr(topicstr,'?'))
  {
    fprintf(stderr,"topicstr must have exactly one %%d and no other %% in it. Also no # or ?. Given string of \"%s\" fails.\n", topicstr); 
  }

  static int listener_counter = 0; 
  char listener_name[16]; 
  sprintf(listener_name,"cody_%d", listener_counter++); 
 
  cody_listener_t * l = malloc(sizeof(struct cody_listener)); 
  memset(l,0,sizeof(cody_listener_t)); 

  struct mosquitto * mosq = mosquitto_new(listener_name, 0, l); 

  if (mosq == NULL) 
    goto fail; 

  int notok = mosquitto_connect(mosq, mqtt_host,port,10);

  if(notok) 
    goto fail; 

  l->mosq = mosq; 
  l->bufsize = Nbuf; 
  if (emergency_queue) 
  {

    l->emergency = opendir(emergency_queue); 
    if (!l->emergency) 
    {
      fprintf(stderr,"WARNING: emergency_queue at %s  was specified, but could not open?\n", emergency_queue); 
    }
    else
    {
      l->emergency_dirfd = dirfd(l->emergency); 
    }
  }


  char * subs[6] = {0}; 
  int nsubs = 0; 
  for (int i = 0; i < 6; i++) 
  {
    if (mask & (1 << i))
    {
      asprintf(&l->topicbuf[i], topicstr, i+1); 
      subs[nsubs++] = l->topicbuf[i];
    }
  }

  notok = mosquitto_subscribe_multiple(mosq, 0, nsubs, subs,2,0,0); 

  if (notok)
    goto fail; 

  mosquitto_message_callback_set(mosq, msg_callback); 
 
  for (int i = 0; i < nsubs; i++) 
  {
    free(subs[i]); 
  }


  for (int i = 0; i < 6; i++) 
  {
    if (!l->topicbuf[i]) continue; //skipped cody 

    pthread_mutex_init(&l->buf_mutex[i], PTHREAD_MUTEX_DEFAULT); 
    l->buf[i] = malloc(sizeof(struct cody_data_wrapper) * Nbuf); 
    int freelist_size = (Nbuf+63)/64; 
    l->buf_freelist[i] = malloc(sizeof(uint64_t) * freelist_size); 
    if (!l->buf[i] || !l->buf_freelist[i]) 
      goto fail; 

    //initialize freelist  (we will mark extra as free if not a multiple of 64, but that's ok).
    memset(l->buf_freelist[i], 0xff, freelist_size * sizeof(uint64_t)); 
  }

 return l; 

fail: 
  if (mosq)
  {
    mosquitto_destroy(mosq); 
    l->mosq = 0; 
  }

  
  cody_listener_finish(l); 


 return NULL; 


}

void cody_listener_finish(cody_listener_t * l)
{
  if (!l) return; 

  if (l->mosq)
    mosquitto_destroy(l->mosq);

  for (int i = 0; i < 6; i++) 
  {
    if (l->buf[i])
    {
      free(l->buf[i]); 
      pthread_mutex_destroy(&l->buf_mutex[i]);
    }
    if (l->topicbuf[i]) free(l->topicbuf[i]); 
  }

  if (l->emergency)
    closedir(l->emergency); 

  free(l); 
}

int cody_listener_nrdy(cody_listener_t * l, uint8_t cody)
{

  if (!l || cody <1 || cody > 6) return -1; 
  //should I take the mutex? Probably fine if I don't. 
  return l->buf_rdy[cody-1]; 
}


static int get_ith_filled_entry(uint8_t cody, cody_listener_t * l, int entry, int free) 
{
  int freelist_size = (l->bufsize+63)/64; 
  int sofar = 0; 

  for (int i = 0; i < freelist_size; i++) 
  {
    int nfilled = 64 - __builtin_popcount(l->buf_freelist[cody-1][i]); 

    if (sofar + nfilled > entry)
    {
      //we are in this block
      
      for (int j = 0; j < 64; j++) 
      {
        if ( (l->buf_freelist[cody-1][i] & (1 << j)) == 0)
        {
          if (sofar == entry)
          {
            if (free) 
            {
              l->buf_freelist[cody-1][i] |= 1 << j;
            }
            return i * 64 + j; 
          }
          sofar++; 
        }
      }
    }
  }
  return -1; 
}

cody_data_t * cody_listener_get(cody_listener_t * l, uint8_t cody, uint32_t i) 
{
  if (!l || cody < 1 || cody > 6 || !l->buf[cody-1]) return NULL; 

  pthread_mutex_lock(&l->buf_mutex[cody-1]); 
  int idx = get_ith_filled_entry(cody,l,i,0); 
  pthread_mutex_unlock(&l->buf_mutex[cody-1]); 
  return &l->buf[cody-1][idx].cody_data; 
}

void cody_listener_release(cody_listener_t * l, uint8_t cody, uint32_t i) 
{
  if (!l || cody < 1 || cody > 6 || !l->buf[cody-1]) return; 

  pthread_mutex_lock(&l->buf_mutex[cody-1]); 
  get_ith_filled_entry(cody,l,i,1); 
  l->buf_rdy[cody-1]--; 
  pthread_mutex_unlock(&l->buf_mutex[cody-1]); 
}

