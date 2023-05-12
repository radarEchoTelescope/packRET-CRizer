#ifndef _TAR_BUF_H
#define _TAR_BUF_H

#include <libtar.h> 

/** 
 *  Helpers for writing tar files with internal files from memory buffers 
 *  The TAR file must already have been initialized... 
 *
 */ 

// writes memory at buf of length len  to file with path name. Current uid/guid is used, as well as current mtime. 
int tar_buf_write(TAR *t, size_t len, const void * buf, const char * name); 
// writes memory at buf of length len to file with path name, with specified stat. The length is given by st->st_size
int tar_buf_write_with_stat(TAR *t, const void * buf, const char * name, const struct stat * st); 

//reads whatever the current entry is in the tar file to buf. 
// note that this might be truncated if testlen is smaller than size. If destlen is larger than size, that memory is not touched. 
int tar_buf_read(TAR *t, size_t destlen, void * buf); 


#endif



