

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <rtl-sdr.h>
#include <usb.h>
#include <math.h>
#include "iq.h"

int main(int argc, const char * argv[])
{
    
//    iqmod();
//    return 0;
    
    uint32_t count = rtlsdr_get_device_count();
    printf("radio count=%d\n", count);
    
    const char * radio_name = rtlsdr_get_device_name(0);
    
    
    printf("name: %s\n", radio_name);
    
    
    //get usb string
    
    
    char  manufacturer[256];
    char  product[256];
    char  serial[256];
    
    int resp=rtlsdr_get_device_usb_strings(0, manufacturer, product, serial);
    
    if(!resp)
    {
        printf("rtlsdr_get_device_usb_strings sussess\n");
        printf("manufacturer: %s\nproduct: %s\nserial: %s\n\n", manufacturer, product, serial);
    }
    
    
    int status= rtlsdr_get_index_by_serial(serial);
    printf("status=%d\n\n", status);
    
    rtlsdr_dev_t * radio_ref= NULL;
    int r = rtlsdr_open(&radio_ref, status);
    if (r < 0) {
		printf("Failed to open rtlsdr device #%d.\n", status);
		exit(1);
	}
    
    /* Set the sample rate */
    uint32_t samp_rate = 2048000; //256 KBps
	r = rtlsdr_set_sample_rate(radio_ref, samp_rate);
	if (r < 0)
    {
		fprintf(stderr, "WARNING: Failed to set sample rate.\n");
        exit(1);
    }
    
    
    uint32_t out_block_size =(16 * 16384);
    uint8_t * buffer;
    
    buffer = malloc(out_block_size * sizeof(uint8_t));
    int n_read;
    r = rtlsdr_reset_buffer(radio_ref); //have to do this to sample
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to reset buffers.\n");
    
    
    r=rtlsdr_read_sync(radio_ref, buffer, out_block_size, &n_read);
    if (r < 0) {
        printf("WARNING: sync read failed.\n");
        exit(1);
    }
    
    FILE *file;
    file = fopen("/Users/eggie5/Development/SDSU-CS582-SPRING13/sdr/rtl_fm_demod/radiosample","w+");
    
    //need to IQ demod
    for(int i=0;i<out_block_size;i++)
    {
        fprintf(file,"%c\n",buffer[i]);
    }
    fclose(file); /*done!*/
    
    printf("\nend");
    
    return 0;
}

