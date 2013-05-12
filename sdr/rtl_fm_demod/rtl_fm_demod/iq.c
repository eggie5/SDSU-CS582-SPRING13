
#include <stdio.h>
#include <math.h>

const int NUMSAMPLES= 512;
#define PI 3.14

int iqmod()
{
    short idata [NUMSAMPLES];
    short qdata [NUMSAMPLES];
    int numsamples = NUMSAMPLES;
    for(int index=0; index<numsamples; index++)
    {
        idata[index]=32768 * sin((2*PI*index)/numsamples);
        qdata[index]=32768 * cos((2*PI*index)/numsamples);
    }
    
    //interleave i & q (little endian)
    char iqbuffer[NUMSAMPLES*4];
    for (int index=0; index<NUMSAMPLES; index++)
    {
        short ivalue = idata[index];
        short qvalue = qdata[index];
        iqbuffer[index*4] = (ivalue >> 8) & 0xFF; //divide 256
        iqbuffer[index*4+1] = ivalue & 0xFF;
        iqbuffer[index*4+2] = (qvalue >> 8) & 0xFF;
        iqbuffer[index*4+3] = qvalue & 0xFF;
    }

    
    char *ofile = "/Users/eggie5/Development/SDSU-CS582-SPRING13/sdr/rtl_fm_demod/iqdata";
    FILE *outfile = fopen(ofile, "w");
    if (outfile==NULL) perror ("Error opening file to write");
    
    for(int index=0; index<(numsamples*4); index++)
    {
        fprintf(outfile, "%d\n", iqbuffer[index]);
    }
    fclose(outfile);
    
    return 0;
}