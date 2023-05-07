#define _GNU_SOURCE

#include "radar.h" 
#include <errno.h> 
#include <stdlib.h>
#include <fcntl.h> 
#include <inttypes.h> 
#include <string.h>
#include <math.h>
#include <curl/curl.h>
#include <unistd.h> 
#include <termios.h> 
#include <stdio.h> 
#include <libiberty.h> 
#include <poll.h> 
#include <sys/file.h>



static char curl_error_msg[CURL_ERROR_SIZE]; 


static int setup_serial(const char * dev, int baud) 
{
  int fd = open(dev, O_RDWR); 
  if (fd < 0) 
  {
    fprintf(stderr,"Could not open serial port %s. Probably everything will fail.\n", dev); 
    return 0; 
  }

  int locked = flock(fd, LOCK_EX | LOCK_NB); 
  if (locked < 0) 
  {

      fprintf(stderr,"Could not get exclusive access to %s. I don't like to share.\n", dev); 
      close(fd); 
      return 0; 

  }
  if (baud != 9600 && baud != 115200) 
  {
    fprintf(stderr,"Unsupported baud rate %d, setting to 9600\n", baud); 

  }
  speed_t speed = baud == 115200 ? B115200 
                  : B9600; 
             
  struct termios tty; 

  tcgetattr(fd,  &tty); 

  //clear parity bit 
  tty.c_cflag &= ~PARENB; 
  
  //one stop bit
  tty.c_cflag &= ~CSTOPB; 

  //8 bits 
  tty.c_cflag &= ~CSIZE; 
  tty.c_cflag |= CS8; 

  //no hw flow control 
  tty.c_cflag &=~CRTSCTS; 

  //turn on read/disable ctrl lines
  tty.c_cflag |= CREAD | CLOCAL; 

  //turn OFF canoncial mode
  tty.c_lflag &= ~ICANON; 

  //disable echo bits (probably already disabled?) 
  tty.c_lflag &= ~ECHO; 
  tty.c_lflag &= ~ECHOE; 
  tty.c_lflag &= ~ECHONL;

  //disable interpretation of signal chars
  tty.c_lflag &= ~ISIG; 

  //disable software flow control 
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); 

  //disable special handling of input  bytes
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|INLCR|ICRNL); 

  //disable any special output modes 
  tty.c_oflag &= ~OPOST; 
  tty.c_oflag &= ~ONLCR; 


  //make it nonblocking
  tty.c_cc[VTIME]=0; 
  tty.c_cc[VMIN]=0; 

  //baud rate  
  cfsetispeed(&tty, speed); //blazing fast! 
  cfsetospeed(&tty, speed);

  //set the serial attrs 
  if (0 != tcsetattr(fd, TCSANOW, &tty))
  {
    fprintf(stderr,"Could not configure serial port %s:(.  Got error %d: %s. I will give up  and this probably won't end well for you. \n", dev, errno, strerror(errno)); 
    close(fd); 
    return 0; 
  }

  //drain the port 
  tcflush(fd, TCIOFLUSH); 
  return fd; 
} 

int setup_int_gpio(int gpionum) 
{

    char buf[512]; 

    //export if not exported 
    sprintf(buf, "/sys/class/gpio/gpio%d", gpionum); 

    if (access(buf, F_OK))
    {
          //export the GPIO, lazy programming  
          FILE *fexport= fopen("/sys/class/gpio/export", "w"); 
          fprintf(fexport,"%d\n", gpionum); 
          fflush(fexport); 
          fclose(fexport); 
          usleep(100000); //wait to make sure it come up 
    }
 
    if (access(buf, F_OK))
    {
       fprintf(stderr,"Could not access gpio %d. Maybe I don't have permissions. Maybe this computer isn't the right kind. Maybe you forgot to reticulate the splines.\n", gpionum); 
       return 0;
    }

    //set the edge to rising 
    sprintf(buf, "/sys/class/gpio/gpio%d/edge", gpionum); 
    int edge_fd = open(buf, O_RDWR); 

    if (edge_fd <=0) 
    {
      fprintf(stderr, "Could not open %s for writing, trig gpio won't be set up. This probably means you'll wait forever, or at least until this computer turns off.\n", buf); 
      return 0; 
    }

    write(edge_fd,"rising",strlen("rising")); 
    close(edge_fd); 

    sprintf(buf,"/sys/class/gpio/gpio%d/value", gpionum); 
    int fd = open(buf,O_RDWR); 

    if (fd  <=  0) 
    {
      fprintf(stderr, "Could not open %s for writing, trig gpio won't be set up. But strange because we could open the other fd...\n", buf); 
      return fd;
    }

    return fd; 
}


struct ret_curl_target
{
  ret_radar_data_t * data; 
  size_t nwritten; 
}; 

static size_t my_curl_write_cb(char *ptr, size_t sz, size_t nm, void *userdata) 
{
  struct ret_curl_target * t = (struct ret_curl_target *) userdata; 

  size_t len_after = t->nwritten + sz*nm; 
  if (len_after > sizeof(ret_radar_data_t))
  {
    fprintf(stderr,"WARNING: we have %zu more bytes than we expect from tftp? Someone screwed up, probably one of us two \n", len_after - sizeof(ret_radar_data_t));
    len_after = sizeof(ret_radar_data_t); 
  }
  size_t nwrite = len_after - t->nwritten;
  memcpy( (uint8_t*) t->data+t->nwritten, ptr, nwrite); 
  t->nwritten += nwrite; 
  return  nwrite; 
}


static CURL * setup_curl_handle(const char *hostname)
{
  CURL * curl = curl_easy_init(); 

  if (!curl) 
  {
    fprintf(stderr,"Could not init curl handle. Are you a B-field?\n"); 
    return NULL; 
  }

  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_msg); 
  //set up our callback 
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_cb); 
  char * url = NULL; 
  asprintf(&url,"tftp://%s/RADAR", hostname); 

  //set our tranfser
  curl_easy_setopt(curl, CURLOPT_URL, url); 
  free(url); 

  // I bet our TFTP server doesn't support anything 
  curl_easy_setopt(curl, CURLOPT_TFTP_NO_OPTIONS, 1L);


  return curl; 
} 


struct ret_radar
{
  CURL * tftp_handle; 
  int int_fd; 
  struct pollfd interrupt_fdset; 
  int ack_fd; 
  int gps_fd; 
  struct timespec timeout; 
};



ret_radar_t *ret_radar_open(const char * hostname, int interrupt_gpio,
                             const char* ack_serial, const char * gps_serial) 
{
  //first try to open everything 

  CURL * curl = setup_curl_handle(hostname); 
  if (!curl) return NULL; 

  int gpio_fd = setup_int_gpio(interrupt_gpio); 
  if (gpio_fd <=0) return NULL; 

  int ack_fd = setup_serial(ack_serial, 115200); 
  if (ack_fd <=0)
  {
    fprintf(stderr,"Running without ack? Is that what you want?\n"); 
  }

  int gps_fd = setup_serial(gps_serial, 115200); 
  if (gps_fd <=0) return NULL; 

  // we got everything open 
  ret_radar_t * ret = calloc(1,sizeof(*ret)); 

  if (!ret) 
  {
    fprintf(stderr,"ret handle allocation failed. Best of luck to you in your future endeavors... \n"); 
    return NULL; 
  }

  ret->tftp_handle = curl; 
  ret->int_fd = gpio_fd; 
  ret->ack_fd = ack_fd; 
  ret->gps_fd = gps_fd; 
  ret->interrupt_fdset.fd = gpio_fd; 
  ret->interrupt_fdset.events = POLLPRI; 

  return ret; 
}


void ret_radar_set_timeout(ret_radar_t * h, double t) 
{
  if (!h) return; 

  if ( (t > (1 << 31)) || t <=0 ) //infinite, or effectively so 
  {                               
    h->timeout.tv_sec = 0;
    h->timeout.tv_nsec = 0;
    return ; 
  }

  h->timeout.tv_sec = (int) t; 
  h->timeout.tv_nsec = (t - floor(t)) * 1e9; 
}



static int do_poll(ret_radar_t * h) 
{
  if (!h) return -1; 
  char val; 
  lseek(h->int_fd, 0, SEEK_SET); 
  read(h->int_fd, &val,1); 
  if (val == '1' ) return 1; 

  // is there a race condition here? if so we should probably poll in smaller increments... 
  ppoll(&h->interrupt_fdset, 1, &h->timeout, 0); 

  lseek(h->int_fd,0,SEEK_SET); 
  read(h->int_fd, &val,1); 
  return val=='1'; 
}


int ret_radar_data_check_crc(const ret_radar_data_t * d) 
{

    uint8_t * cdata = (uint8_t*) d;
    uint32_t crc = xcrc32( cdata + sizeof(crc), sizeof(ret_radar_data_t) - sizeof(crc),0xffffffff); 
    if (crc == d->crc32)
    {
      return 0; 
    }
    fprintf(stderr,"crc check failed, %u vs %u\n", crc, d->crc32); 
    return 1; 
}

int ret_radar_next_event(ret_radar_t * h, ret_radar_gps_tm_t * tm, ret_radar_data_t * d) 
{
  if (!h) return -1; 

  struct ret_curl_target t; 
  t.data= d; 

  curl_easy_setopt(h->tftp_handle, CURLOPT_WRITEDATA,&t); 
  // first we wait on the interrupt, potentially with a timeout
  if (!do_poll(h))
  {
    return -1; 
  }

  // tell the GPS we want the timestamps
  write(h->gps_fd,"G",1); 


  int notok = 1;
  while(1) 
  {
    t.nwritten = 0; 
    //initialize the transfer
    //this is blocking. Maybe use non-blocking later so can read serial and gps timestamp at same time? 
    notok = curl_easy_perform(h->tftp_handle); 


    if (notok || ret_radar_data_check_crc(d))
    {
      if (notok) fprintf(stderr,"curl returned %d (%s), retrying\n", notok, curl_error_msg); 
      if (h->ack_fd >0) write(h->ack_fd,"?",1); 
    }
    else
    {
      break; 
    }

  }  


  //read the GPS, TODO add error checking 
  read(h->gps_fd, tm, sizeof(*tm)); 
  if (h->ack_fd >0) write(h->ack_fd,"!",1); 

  return 0; 
} 


void ret_radar_close(ret_radar_t * h) 
{
  if (!h) return ; 

  curl_easy_cleanup(h->tftp_handle); 
  close(h->int_fd); 
  close(h->ack_fd); 
  close(h->gps_fd); 
  free(h); 
}

#define dump_u32(x) nwr += fprintf(f,"%*s  \"" #x "\": %u,\n", indent, " ", data->x) 
#define dump_u64(x) nwr += fprintf(f,"%*s  \"" #x "\": %"PRIu64",\n", indent, " ", data->x) 
#define dump_u16(x) nwr += fprintf(f,"%*s  \"" #x "\": %hu,\n", indent, " ", data->x) 
#define dump_h32(x) nwr += fprintf(f,"%*s  \"" #x "\": 0x%x,\n", indent, " ", data->x) 
#define dump_float(x) nwr += fprintf(f,"%*s  \"" #x "\": %f,\n", indent, " ", data->x) 
#define dump_arr(x,format,N) fprintf(f,"%*s  \"" #x "\" : [", indent, " "); for (int i = 0; i < N; i++) nwr += fprintf(f,format "%s", data->x[i], i < N-1 ? "," :"")  ; fprintf(f,"],\n"); 


int ret_radar_dump(FILE *f, const ret_radar_data_t *data, int indent) 
{

  int nwr = 0;
  nwr += fprintf(f,"%*s{\n", indent, " "); 
  dump_u32(crc32); 
  dump_u32(index); 
  dump_u32(struct_version); 
  dump_u64(cpu_time); 
  dump_u32(surface_trigger_info); 
  dump_arr(station_counter, "%"PRIu64, 6); 
  dump_arr(l0_rate_monitor, "%u", 6); 
  dump_u32(adc_read_index);
  dump_u32(dac_write_index);
  dump_u32(rf_threshold);
  dump_u32(n_surface_stations);
  dump_u32(rf_read_window);
  dump_u32(carrier_cancel_flag);
  dump_u32(tx_atten);
  dump_u32(cc_atten);
  dump_arr(tx_phase,"%f",4); 
  dump_arr(tx_amplitude,"%f",4); 
  dump_float(cc_amplitude); 
  dump_u32(tx_mode); 
  dump_u32(tx_freq); 
  dump_u32(priority); 
  dump_arr(adc_0_data,"%hd",16384);
  dump_arr(adc_1_data,"%hd",16384);
  dump_arr(adc_2_data,"%hd",16384);
  dump_arr(adc_3_data,"%hd",16384);
  nwr += fprintf(f,"%*s}\n", indent, " "); 
  return nwr;
}

int ret_radar_gps_tm_dump(FILE * f, const ret_radar_gps_tm_t * data, int indent) 
{
  int nwr = 0; 
  //TODO: convert to useful units 
  nwr += fprintf(f,"%*s{\n", indent, " "); 
  dump_u16(count); 
  dump_u16(weeknum); 
  dump_u32(tow); 
  dump_u32(tow_f); 
  dump_u32(acc); 
  nwr += fprintf(f,"%*s}\n", indent, " "); 
  return nwr; 
}

#undef dump_u32
#undef dump_h32
#undef dump_arr
