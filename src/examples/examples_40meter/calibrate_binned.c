/* GRASP: Copyright 1997,1998  Bruce Allen */
#include "grasp.h"
#define NPOINT 8192
#define NBIN 500

int main(int argc,char **argv) {
	void graphout(int,float,float);
	void realft(float*,unsigned long,int);
	float data[NPOINT],average[NPOINT/2],response[NPOINT+4];
	float spec,decaytime;
	float srate,tstart=0,freq,tlock=0.0;
	FILE *fpifo,*fplock,*fpss;
	int i,j,code,npoint,remain,ir,ii,reset=0;
	short datas[NPOINT];
	double mod,logfbot,logftop,dlogf,logf,f0,f1,binspec,binf;
	int nbin,pass=0;

	/* number of points to sample and fft (power of 2) */
	npoint=NPOINT;

	/* open the IFO output file, lock file, and swept-sine file */
	fpifo=grasp_open("GRASP_DATAPATH","channel.0","r");
	fplock=grasp_open("GRASP_DATAPATH","channel.10","r");
	fpss=grasp_open("GRASP_DATAPATH","swept-sine.ascii","r");

	/* set the decay time (sec) */
	decaytime=30.0;

	/* get data */
	while ((code=get_data(fpifo,fplock,&tstart,npoint,datas,&remain,&srate,0))) {
 		/* put data into floats */
		for (i=0;i<npoint;i++) data[i]=datas[i];

		/* get the normalization */
		if (!pass++)
			normalize_gw(fpss,npoint,srate,response);

    		/* Reset if just locked */
      		if (code==1) {
			reset=0;
			tlock=tstart;
			avg_spec(data,average,npoint,&reset,srate,decaytime,2,1);
		} else {

			/* track average power spectrum, with Welch windowing */
			avg_spec(data,average,npoint,&reset,srate,decaytime,2,1);

			/* set number of bins between 10 Hz and Nyquist */
			f1=0.5*srate;
			f0=10.0;
			dlogf=(log(f1)-log(f0))/NBIN;

			/* set top and bottom log frequencies, initial binned quantities */
			logfbot=log(f0);
			logftop=logfbot+dlogf;
			nbin=0;
			binspec=binf=0;

			/* loop over all frequencies except DC (j=0) & Nyquist (j=npoint/2) */
			for (j=1;j<npoint/2;j++) {
				/* subscripts of real, imaginary parts */
				ii=(ir=j+j)+1;

				/* frequency of the point */
				freq=srate*j/npoint;

				/* determine power spectrum in (meters/rHz) */
				mod=response[ir]*response[ir]+response[ii]*response[ii];
				spec=sqrt(average[j]*mod);

				/* are we within desired log freq interval? */
				logf=log(freq);
				if (logf<=logfbot) {
					/* not in the range that we plan to plot */
					continue;
				}
				else if (logf>logftop) {
					if (nbin)
						/* one or more bins are filled */
						printf("%e\t%e\n",binf/nbin,binspec/nbin);
					else
						/* nothing in the bin, just output the one point */
						printf("%e\t%e\n",freq,spec);

					/* initialize binned quantities */
					nbin=0;
					binspec=binf=0;
					logfbot=logf;
					logftop=logf+dlogf;
				}
				/* bin the different quantities */
				binspec+=spec;
				binf+=freq;
				nbin+=1;
			}
			/* print out useful things for xmgr program ... */
			graphout(0,tstart,tlock);
		}
	}
	return 0;
 }


void graphout(int last,float time,float tlock) {
   static int count=0;
   printf("&\n");                            /* end of set marker             */
   /* first time we draw the plot */
   if (count++==0) {
      printf("@doublebuffer true\n");       /* keeps display from flashing    */
      printf("@focus off\n");               /* turn off the focus markers     */
      printf("@s0 color 2\n");              /* FFT is red                     */
      printf("@g0 type logxy\n");           /* set graph type to log-log      */
      printf("@autoscale \n");              /* autoscale FFT                  */
      printf("@world xmin %e\n",10.0);      /* set min x                      */
      printf("@world xmax %e\n",5000.0);    /* set max x                      */
      printf("@world ymin %e\n",1.e-19);    /* set min y                      */
      printf("@world ymax %e\n",1.e-9);     /* set max y                      */
      printf("@yaxis tick minor on\n");     /* turn on tick marks             */
      printf("@yaxis tick major on\n");     /* turn on tick marks             */
      printf("@yaxis tick minor 2\n");      /* turn on tick marks             */
      printf("@yaxis tick major 1\n");      /* turn on tick marks             */
      printf("@redraw \n");                 /* redraw graph                   */
      printf("@xaxis label \"f (Hz)\"\n");  /* FFT horizontal axis label      */
      printf("@yaxis label \"meters/rHz\"\n");  /* FFT vertical axis label    */
      printf("@title \"Calibrated IFO Spectrum\"\n");/* set title             */
      /* set subtitle     */
      printf("@subtitle \"%.2f sec since last lock. t = %.2f sec.\"\n",time-tlock,time);
      if (!last) printf("@kill s0\n");      /* kill graph; ready to read agai */
   }
   else {
      /* other times we redraw the plot */
      /* set subtitle     */
      printf("@subtitle \"%.2f sec since last lock. t = %.2f sec.\"\n",time-tlock,time);
      printf("@s0 color 2\n");              /* FFT is red                      */
      printf("@g0 type logxy\n");           /* set graph type to log-log       */
      printf("@world xmin %e\n",10.0);      /* set min x                       */
      printf("@world xmax %e\n",5000.0);    /* set max x                       */
      printf("@world ymin %e\n",1.e-19);    /* set min y                       */
      printf("@world ymax %e\n",1.e-9);     /* set max y                       */
      printf("@yaxis tick minor on\n");     /* turn on tick marks              */
      printf("@yaxis tick major on\n");     /* turn on tick marks              */
      printf("@yaxis tick minor 2\n");      /* turn on tick marks              */
      printf("@yaxis tick major 1\n");      /* turn on tick marks              */
      printf("@redraw\n");                  /* redraw the graph                */
      if (!last) printf("@kill s0\n");      /* kill graph, ready to read again */
   }
   return;
}
