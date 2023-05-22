# packRET-CRizer
data packetizer for RET-CR

Requirements:  
libmosquitto-dev[el], libcurl-dev[el], libtar-dev[el], binutils (or libiberty if packaged separately)), libsystemd-dev[el]

# Useful Programs:

## radar-get:  rfsoc test program

This listens for GPIO triggers from the rfsoc and then grabs it via TFTP. Data is dumped to stdout as json, but can also be output as binary by passing one or more -o flags (up to 16 output locations are supported)


## test-cody-listener:  test cody MQTT client 

This listens for cody events over MQTT and optionally writes them out. First argument is the topic to listen to (defaults to /ret/SS%d/science_file) ). 
Second argument (optional) is directory to write output to. Third argument (optional) is an additional driectory to write to. No writing is done if fewer than 2 arguments are given. Only supports writing to 2 locations... 


## packetizer:  The real DAQ program (under development, not teested yet!) 

This listens for GPIO triggers form the rfsoc and also listens for CODY events via MQTT, then tries to put them together. 


# Less  useful programs 

test-ret-writer: ret-writer test program
This just makes sure the writer sort of wroks

test-tarwbuf: tarbuf test program
This just tests writing to tar files 



# Coming soon 

Javascript for plotting json output using jsroot,  plus example html... 
