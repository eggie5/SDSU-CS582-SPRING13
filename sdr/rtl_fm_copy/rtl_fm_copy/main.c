

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <pthread.h>
#include <usb.h>

#include <rtl-sdr.h>

#define DEFAULT_SAMPLE_RATE		24000
#define DEFAULT_ASYNC_BUF_NUMBER	32
#define DEFAULT_BUF_LENGTH		(1 * 16384)
#define MAXIMUM_OVERSAMPLE		16
#define MAXIMUM_BUF_LENGTH		(MAXIMUM_OVERSAMPLE * DEFAULT_BUF_LENGTH)
#define AUTO_GAIN			-100

static pthread_t demod_thread;
static pthread_mutex_t data_ready;  /* locked when no fresh data available */
static pthread_mutex_t data_write;  /* locked when r/w buffer */
static int do_exit = 0;
static rtlsdr_dev_t *dev = NULL;
static int lcm_post[17] = {1,1,1,3,1,5,3,7,1,9,5,11,3,13,7,15,1};



struct fm_state
{
	int      now_r, now_j;
	int      pre_r, pre_j;
	int      prev_index;
	int      downsample;    /* min 1, max 256 */
	int      post_downsample;
	int      output_scale;
	int      squelch_level, conseq_squelch, squelch_hits, terminate_on_squelch;
	int      exit_flag;
	uint8_t  buf[MAXIMUM_BUF_LENGTH]; //raw samples from device
	uint32_t buf_len;
	int      signal[MAXIMUM_BUF_LENGTH];  /* 16 bit signed i/q pairs */
	int16_t  signal2[MAXIMUM_BUF_LENGTH]; /* signal has lowpass, signal2 has demod */
	int      signal_len;
	int      signal2_len;
	FILE     *file;
	int      edge;
	uint32_t freqs[1000];
	int      freq_len;
	int      freq_now;
	uint32_t sample_rate;
	int      output_rate;
	int      fir_enable;
	int      fir[256];  /* fir_len == downsample */
	int      fir_sum;
	int      custom_atan;
	int      deemph, deemph_a;
	int      now_lpr;
	int      prev_lpr_index;
	int      dc_block, dc_avg;
	void     (*mode_demod)(struct fm_state*);
};




static void sighandler(int signum)
{
	fprintf(stderr, "Signal caught, exiting!\n");
	do_exit = 1;
	rtlsdr_cancel_async(dev);
}


void rotate_90(unsigned char *buf, uint32_t len)
/* 90 rotation is 1+0j, 0+1j, -1+0j, 0-1j
 or [0, 1, -3, 2, -4, -5, 7, -6] */
{
	uint32_t i;
	unsigned char tmp;
	for (i=0; i<len; i+=8) {
		/* uint8_t negation = 255 - x */
		tmp = 255 - buf[i+3];
		buf[i+3] = buf[i+2];
		buf[i+2] = tmp;
        
		buf[i+4] = 255 - buf[i+4];
		buf[i+5] = 255 - buf[i+5];
        
		tmp = 255 - buf[i+6];
		buf[i+6] = buf[i+7];
		buf[i+7] = tmp;
	}
}

//this is run on the raw samples from the devices
//this is the magic IQ demodulation
void low_pass(struct fm_state *fm, unsigned char *buf, uint32_t len)
/* simple square window FIR */
{
	int i=0, i2=0;
	while (i < (int)len) {
		fm->now_r += ((int)buf[i]   - 127); //subtract 128 like matlab!
		fm->now_j += ((int)buf[i+1] - 127); //subtract 128
		i += 2;
		fm->prev_index++;
		if (fm->prev_index < fm->downsample) {
			continue;
		}
        
//        The filter removes the frequencies stemming from the negative spectrum of the real RF signal,
//        and the filter removes approximately half of the energy in the signal. In order to preserve the
//        energy in the signal, the complex signal should be multiplied by the square root of 2.
        
		fm->signal[i2]   = fm->now_r * 1.4;//fm->output_scale; //scale real component
		fm->signal[i2+1] = fm->now_j * 1.4;//fm->output_scale; //scale imag compenent
		fm->prev_index = 0;
		fm->now_r = 0;
		fm->now_j = 0;
		i2 += 2;
	}
	fm->signal_len = i2;
    
    //see the FREQ. MODULATED IQ SIGNAL
//    FILE *rfile;
//    rfile = fopen("/Users/eggie5/Development/SDSU-CS582-SPRING13/sdr/raw","w");
//    for(int j=0;j<len;j++)
//    {
//        //printf("buffer[%d]=%d\n", i,buffer[i]);
//        fprintf(rfile,"%d\n",buf[j]);
//        
//    }
//    fclose(rfile);
    
    //SEE THE FREQ MODULATED DE-IQ'S SIGNAL
//    FILE *file;
//    file = fopen("/Users/eggie5/Development/SDSU-CS582-SPRING13/sdr/low_pass","w");
//    for(int i=0;i<i2;i++)
//    {
//        //printf("buffer[%d]=%d\n", i,buffer[i]);
//        fprintf(file,"%d\n",fm->signal[i]);
//        
//    }
//    fclose(file);
}

void build_fir(struct fm_state *fm)
/* for now, a simple triangle
 * fancy FIRs are equally expensive, so use one */
/* point = sum(sample[i] * fir[i] * fir_len / fir_sum) */
{
	int i, len;
	len = fm->downsample;
	for(i = 0; i < (len/2); i++) {
		fm->fir[i] = i;
	}
	for(i = len-1; i >= (len/2); i--) {
		fm->fir[i] = len - i;
	}
	fm->fir_sum = 0;
	for(i = 0; i < len; i++) {
		fm->fir_sum += fm->fir[i];
	}
}



int low_pass_simple(int16_t *signal2, int len, int step)
// no wrap around, length must be multiple of step
{
	int i, i2, sum;
	for(i=0; i < len; i+=step) {
		sum = 0;
		for(i2=0; i2<step; i2++) {
			sum += (int)signal2[i + i2];
		}
		//signal2[i/step] = (int16_t)(sum / step);
		signal2[i/step] = (int16_t)(sum);
	}
	signal2[i/step + 1] = signal2[i/step];
	return len / step;
}

void low_pass_real(struct fm_state *fm)
/* simple square window FIR */
// add support for upsampling?
{
	int i=0, i2=0;
	int fast = (int)fm->sample_rate / fm->post_downsample;
	int slow = fm->output_rate;
	while (i < fm->signal2_len) {
		fm->now_lpr += fm->signal2[i];
		i++;
		fm->prev_lpr_index += slow;
		if (fm->prev_lpr_index < fast) {
			continue;
		}
		fm->signal2[i2] = (int16_t)(fm->now_lpr / (fast/slow));
		fm->prev_lpr_index -= fast;
		fm->now_lpr = 0;
		i2 += 1;
	}
	fm->signal2_len = i2;
    
//    FILE *file;
//    file = fopen("/Users/eggie5/Development/SDSU-CS582-SPRING13/sdr/fm_demod","w");
//    for(int i=0;i<i2;i++)
//    {
//        //printf("buffer[%d]=%d\n", i,buffer[i]);
//        fprintf(file,"%d\n",fm->signal2[i]);
//        
//    }
//    fclose(file);
}


void multiply(int ar, int aj, int br, int bj, int *cr, int *cj)
{
	*cr = ar*br - aj*bj;
	*cj = aj*br + ar*bj;
}

int polar_discriminant(int ar, int aj, int br, int bj)
{
	int cr, cj;
	double angle;
	multiply(ar, aj, br, -bj, &cr, &cj);
	angle = atan2((double)cj, (double)cr);
	return (int)(angle / 3.14159 * (1<<14));
}

int fast_atan2(int y, int x)
/* pre scaled for int16 */
{
	int yabs, angle;
	int pi4=(1<<12), pi34=3*(1<<12);  // note pi = 1<<14
	if (x==0 && y==0) {
		return 0;
	}
	yabs = y;
	if (yabs < 0) {
		yabs = -yabs;
	}
	if (x >= 0) {
		angle = pi4  - pi4 * (x-yabs) / (x+yabs);
	} else {
		angle = pi34 - pi4 * (x+yabs) / (yabs-x);
	}
	if (y < 0) {
		return -angle;
	}
	return angle;
}

int polar_disc_fast(int ar, int aj, int br, int bj)
{
	int cr, cj;
	multiply(ar, aj, br, -bj, &cr, &cj);
	return fast_atan2(cj, cr);
}




void fm_demod(struct fm_state *fm)
{
	int i, pcm;
	pcm = polar_discriminant(fm->signal[0], fm->signal[1],
                             fm->pre_r, fm->pre_j);
	fm->signal2[0] = (int16_t)pcm;
	for (i = 2; i < (fm->signal_len); i += 2) {
		switch (fm->custom_atan) {
            case 0:
                pcm = polar_discriminant(fm->signal[i], fm->signal[i+1],
                                         fm->signal[i-2], fm->signal[i-1]);
                break;
            case 1:
                pcm = polar_disc_fast(fm->signal[i], fm->signal[i+1],
                                      fm->signal[i-2], fm->signal[i-1]);
                break;
//            case 2:
//                pcm = polar_disc_lut(fm->signal[i], fm->signal[i+1],
//                                     fm->signal[i-2], fm->signal[i-1]);
//                break;
		}
		fm->signal2[i/2] = (int16_t)pcm;
	}
	fm->pre_r = fm->signal[fm->signal_len - 2];
	fm->pre_j = fm->signal[fm->signal_len - 1];
	fm->signal2_len = fm->signal_len/2;
}










static void optimal_settings(struct fm_state *fm, int freq, int hopping)
{
	int r, capture_freq, capture_rate;
	fm->downsample = (1000000 / fm->sample_rate) + 1;
	fm->freq_now = freq;
	capture_rate = fm->downsample * fm->sample_rate;
	capture_freq = fm->freqs[freq] + capture_rate/4;
	capture_freq += fm->edge * fm->sample_rate / 2;
	fm->output_scale = 1;
    
	/* Set the frequency */
	r = rtlsdr_set_center_freq(dev, (uint32_t)capture_freq);
	if (hopping) {
		return;}
	fprintf(stderr, "Oversampling input by: %ix.\n", fm->downsample);
	fprintf(stderr, "Oversampling output by: %ix.\n", fm->post_downsample);
	fprintf(stderr, "Buffer size: %0.2fms\n",
            1000 * 0.5 * lcm_post[fm->post_downsample] * (float)DEFAULT_BUF_LENGTH / (float)capture_rate);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to set center freq.\n");}
	else {
		fprintf(stderr, "Tuned to %u Hz.\n", capture_freq);}
    
	/* Set the sample rate */
	fprintf(stderr, "Sampling at %u Hz.\n", capture_rate);
	if (fm->output_rate > 0) {
		fprintf(stderr, "Output at %u Hz.\n", fm->output_rate);
	} else {
		fprintf(stderr, "Output at %u Hz.\n", fm->sample_rate/fm->post_downsample);}
	r = rtlsdr_set_sample_rate(dev, (uint32_t)capture_rate);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to set sample rate.\n");}
    
}

void full_demod(struct fm_state *fm)
{
	 
	rotate_90(fm->buf, fm->buf_len);
    
    low_pass(fm, fm->buf, fm->buf_len);
 
	pthread_mutex_unlock(&data_write);
	fm->mode_demod(fm);

    fm->signal2_len = low_pass_simple(fm->signal2, fm->signal2_len, fm->post_downsample);

    low_pass_real(fm); // some freaky stuff w/o this filter
    
	/* ignore under runs for now */
	fwrite(fm->signal2, 2, fm->signal2_len, fm->file);
}

static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx)
{
	struct fm_state *fm2 = ctx;
	if (do_exit) {
		return;}
	if (!ctx) {
		return;}
	pthread_mutex_lock(&data_write);
    //copy the samples from device to the
    //fm->signal2
    
	memcpy(fm2->buf, buf, len);
	fm2->buf_len = len;
	pthread_mutex_unlock(&data_ready);
	/* single threaded uses 25% less CPU? */
	/* full_demod(fm2); */
}

static void *demod_thread_fn(void *arg)
{
	struct fm_state *fm2 = arg;
	while (!do_exit) {
		pthread_mutex_lock(&data_ready);
		full_demod(fm2);
		if (fm2->exit_flag)
        {
			do_exit = 1;
			rtlsdr_cancel_async(dev);
        }
	}
	return 0;
}

double atofs(char* f)
/* standard suffixes */
{
	char* chop;
    double suff = 1.0;
	chop = malloc((strlen(f)+1)*sizeof(char));
	strncpy(chop, f, strlen(f)-1);
	switch (f[strlen(f)-1]) {
		case 'G':
			suff *= 1e3;
		case 'M':
			suff *= 1e3;
		case 'k':
			suff *= 1e3;
        suff *= atof(chop);}
	free(chop);
	if (suff != 1.0) {
		return suff;}
	return atof(f);
}



void fm_init(struct fm_state *fm)
{
	fm->freqs[0] = 100000000;
	fm->sample_rate = DEFAULT_SAMPLE_RATE;
	fm->squelch_level = 0;
	fm->conseq_squelch = 20;
	fm->terminate_on_squelch = 0;
	fm->squelch_hits = 0;
	fm->freq_len = 0;
	fm->edge = 0;
	fm->fir_enable = 0;
	fm->prev_index = 0;
	fm->post_downsample = 1;  // once this works, default = 4
	fm->custom_atan = 0;
	fm->deemph = 0;
	fm->output_rate = -1;  // flag for disabled
	fm->mode_demod = &fm_demod;
	fm->pre_j = fm->pre_r = fm->now_r = fm->now_j = 0;
	fm->prev_lpr_index = 0;
	fm->deemph_a = 0;
	fm->now_lpr = 0;
	fm->dc_block = 0;
	fm->dc_avg = 0;
}

int main(int argc, char **argv)
{

	struct sigaction sigact;
	struct fm_state fm;
 
	int r, wb_mode = 0;
	int i, gain = AUTO_GAIN; // tenths of a dB
	uint8_t *buffer;
	uint32_t dev_index = 0;
	int device_count;
	int ppm_error = 0;
	char vendor[256], product[256], serial[256];
	fm_init(&fm);
	pthread_mutex_init(&data_ready, NULL);
	pthread_mutex_init(&data_write, NULL);
    
	
    fm.freqs[fm.freq_len] = (uint32_t)atofs("89.5M");
    fm.freq_len++;


    wb_mode = 1;
    fm.mode_demod = &fm_demod;
    fm.sample_rate = 170000;
    fm.output_rate = 32000;
    fm.custom_atan = 1;
    fm.post_downsample = 4;
    fm.deemph = 1;
    fm.squelch_level = 0;

    

	/* quadruple sample_rate to limit to Δθ to ±π/2 */
	fm.sample_rate *= fm.post_downsample;
    
	if (fm.freq_len == 0) {
		fprintf(stderr, "Please specify a frequency.\n");
		exit(1);
	}
    
	if (fm.freq_len > 1) {
		fm.terminate_on_squelch = 0;
	}
    
	
 
 
    
	buffer = malloc(lcm_post[fm.post_downsample] * DEFAULT_BUF_LENGTH * sizeof(uint8_t));
    
	device_count = rtlsdr_get_device_count();
	if (!device_count) {
		fprintf(stderr, "No supported devices found.\n");
		exit(1);
	}
    
	fprintf(stderr, "Found %d device(s):\n", device_count);
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		fprintf(stderr, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);
	}
	fprintf(stderr, "\n");
    
	fprintf(stderr, "Using device %d: %s\n",
            dev_index, rtlsdr_get_device_name(dev_index));
    
	r = rtlsdr_open(&dev, dev_index);
	if (r < 0) {
		fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);
		exit(1);
	}

	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);
 
    
	/* WBFM is special */
	if (wb_mode) {
		fm.freqs[0] += 16000;
	}
    
	if (fm.deemph) {
		fm.deemph_a = (int)round(1.0/((1.0-exp(-1.0/(fm.output_rate * 75e-6)))));
	}
    
	optimal_settings(&fm, 0, 0);
	build_fir(&fm);
    
	/* Set the tuner gain */
	if (gain == AUTO_GAIN) {
		r = rtlsdr_set_tuner_gain_mode(dev, 0);
	} else {
		r = rtlsdr_set_tuner_gain_mode(dev, 1);
		r = rtlsdr_set_tuner_gain(dev, gain);
	}
	if (r != 0) {
		fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
	} else if (gain == AUTO_GAIN) {
		fprintf(stderr, "Tuner gain set to automatic.\n");
	} else {
		fprintf(stderr, "Tuner gain set to %0.2f dB.\n", gain/10.0);
	}
	r = rtlsdr_set_freq_correction(dev, ppm_error);
    
 
    fprintf(stdout, "Outputting to stdout.\n");
    fm.file = stdout;

 
    
    
	/* Reset endpoint before we start reading from it (mandatory) */
	r = rtlsdr_reset_buffer(dev);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to reset buffers.\n");}
    
    //create the demod thread
	pthread_create(&demod_thread, NULL, demod_thread_fn, (void *)(&fm));
	rtlsdr_read_async(dev, rtlsdr_callback, (void *)(&fm),
                      DEFAULT_ASYNC_BUF_NUMBER,
                      lcm_post[fm.post_downsample] * DEFAULT_BUF_LENGTH);
    
    
    //close and cleanup
	if (do_exit) {
		fprintf(stderr, "\nUser cancel, exiting...\n");}
	else {
		fprintf(stderr, "\nLibrary error %d, exiting...\n", r);}
	rtlsdr_cancel_async(dev);
	pthread_mutex_destroy(&data_ready);
	pthread_mutex_destroy(&data_write);
    
	if (fm.file != stdout) {
		fclose(fm.file);}
    
	rtlsdr_close(dev);
	free (buffer);
	return r >= 0 ? r : -r;
}
