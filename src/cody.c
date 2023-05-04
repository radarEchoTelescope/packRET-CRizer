#include "cody.h"

int cody_data_fill_time(const cody_data_t * d, struct timespec *t)
{
  t->nsecs = d->gps_subtime.big;
  t->secs = d->gps_time.big + 315964800-18; // UTC to GPS epoch correction and number of leap seconds after 1980 
}

#define dump_u16(x) nwr += fprintf(f,"%*s  \"" #x "\": %hu,\n", indent, " ", d->x) 

int cody_data_dump(FILE *f, const cody_data_t * d, int indent) 
{
  //TODO  implement this... 
  return -1; 
}

