/* First, we include standard headers.
 * Generally speaking, a LITMUS^RT real-time task can perform any
 * system call, etc., but no real-time guarantees can be made if a
 * system call blocks. To be on the safe side, only use I/O for debugging
 * purposes and from non-real-time sections.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Second, we include the LITMUS^RT user space library header.
 * This header, part of liblitmus, provides the user space API of
 * LITMUS^RT.
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "/usr/include/SDL/SDL.h"
#include </usr/include/SDL/SDL_thread.h>
#include "/home/aditya/litmus/liblitmus/include/litmus.h"

/* Next, we define period and execution cost to be constant. 
 * These are only constants for convenience in this example, they can be
 * determined at run time, e.g., from command line parameters.
 *
 * These are in milliseconds.
 */
#define PERIOD            100
#define RELATIVE_DEADLINE 100
#define EXEC_COST         50

/* Catch errors.
 */
#define CALL( exp ) do { \
		int ret; \
		ret = exp; \
		if (ret != 0) \
			fprintf(stderr, "%s failed: %m\n", #exp);\
		else \
			fprintf(stderr, "%s ok.\n", #exp); \
	} while (0)


/* Declare the periodically invoked job. 
 * Returns 1 -> task should exit.
 *         0 -> task should continue.
 */
int job(void);

AVFormatContext *pVideoCtx = NULL;
	  int             i, vFrame;
	  AVCodecContext  *pCodCtx = NULL;
	  AVCodec         *ptrCod = NULL;
	  AVFrame         *framePtr = NULL; 
	  AVPacket        pkg;
	  int             frameEnd;
  	  //float           aspect_ratio;

	  AVDictionary    *optionsDict = NULL;
	  struct SwsContext *swsCtx = NULL;

	  SDL_Overlay     *texture = NULL;
	  SDL_Surface     *disp = NULL;
	  SDL_Rect        rect;
	  SDL_Event       event;

	  AVPicture pict;


/* typically, main() does a couple of things: 
 * 	1) parse command line parameters, etc.
 *	2) Setup work environment.
 *	3) Setup real-time parameters.
 *	4) Transition to real-time mode.
 *	5) Invoke periodic or sporadic jobs.
 *	6) Transition to background mode.
 *	7) Clean up and exit.
 *
 * The following main() function provides the basic skeleton of a single-threaded
 * LITMUS^RT real-time task. In a real program, all the return values should be 
 * checked for errors.
 */
int main(int argc, char** argv)
{
	int do_exit;
	struct rt_task param;

	/* Setup task parameters */
	init_rt_task_param(&param);
	param.exec_cost = EXEC_COST;
	param.period = PERIOD;
	param.relative_deadline = RELATIVE_DEADLINE;

	/* What to do in the case of budget overruns? */
	param.budget_policy = NO_ENFORCEMENT;

	/* The task class parameter is ignored by most plugins. */
	param.cls = RT_CLASS_SOFT;

	/* The priority parameter is only used by fixed-priority plugins. */
	param.priority = LITMUS_LOWEST_PRIORITY;

	 av_register_all();
	 
	  // Execute SDL
	  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
	    fprintf(stderr, "SDL Failed - %s\n", SDL_GetError());
	    exit(1);
	  }else{
	  	fprintf(stderr, "%s\n", "Executing SDL..."); 
	  }

	  // Open video available in the path
	  if(avformat_open_input(&pVideoCtx, "/home/aditya/Videos/sample1.mp4", NULL, NULL)!=0){
	    return -1; // Couldn't open file
	  }else{
	  	fprintf(stderr, "%s\n", "Playing Video, Enjoy!!!"); 
	  }
	  // Collect video information
	  if(avformat_find_stream_info(pVideoCtx, NULL)<0)
	    return -1; // Couldn't find 
	  	
	  // Find the first video frame
	  vFrame=-1;
	  for(i=0; i<pVideoCtx->nb_streams; i++)
	    if(pVideoCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
	      vFrame=i;
	      break;
	    }
	  if(vFrame==-1)
	    return -1; // No Video fame
	  
	  // Get a pointer to the codec context for the video 
	  pCodCtx=pVideoCtx->streams[vFrame]->codec;
	  
	  // Find video frame decoder
	  ptrCod=avcodec_find_decoder(pCodCtx->codec_id);
	  if(ptrCod==NULL) {
	    fprintf(stderr, "Codec crashed!\n");
	    return -1; // No Codec
	  }
	  
	  // Open codec
	  if(avcodec_open2(pCodCtx, ptrCod, &optionsDict)<0)
	    return -1; // Could not open codec
	  
	  // Allocate video frame
	  framePtr=avcodec_alloc_frame();

	  // Make a disp to put our video
	#ifndef __DARWIN__
		disp = SDL_SetVideoMode(pCodCtx->width, pCodCtx->height, 0, 0);
	#else
		disp = SDL_SetVideoMode(pCodCtx->width, pCodCtx->height, 24, 0);
	#endif
	  if(!disp) {
	    fprintf(stderr, "SDL: Error while setting video mode - exiting\n");
	    exit(1);
	  }
	  
	  // Allocate a place to put our YUV image on that disp
	  texture = SDL_CreateYUVOverlay(pCodCtx->width,
					 pCodCtx->height,
					 SDL_YV12_OVERLAY,
					 disp);
	  
	  swsCtx =
	    sws_getContext
	    (
		pCodCtx->width,
		pCodCtx->height,
		pCodCtx->pix_fmt,
		pCodCtx->width,
		pCodCtx->height,
		PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	    ); 	

	/* The task is in background mode upon startup. */

	CALL( init_litmus() );

	CALL( set_rt_task_param(gettid(), &param) );

	fprintf(stderr, "%s\n", "RT Task Set");

	/*****
	 * 4) Transition to real-time mode.
	 */
	CALL( task_mode(LITMUS_RT_TASK) );

	/* The task is now executing as a real-time task if the call didn't fail. 
	 */

	fprintf(stderr,"%s\n","some informatoin");
	/*****
	 * 5) Invoke real-time jobs.
	 */
	do {
		/* Wait until the next job is released. */
		//sleep_next_period();
		/* Invoke job. */
		do_exit = job();		
		// printf("yes%d",do_exit);
	} while (!do_exit);


	
	/*****
	 * 6) Transition to background mode.
	 */
	CALL( task_mode(BACKGROUND_TASK) );

	

	/***** 
	 * 7) Clean up, maybe print results and stats, and exit.
	 */
	// Free the packet that was allocated by av_read_frame
	    av_free_packet(&pkg);
	    SDL_PollEvent(&event);
	    switch(event.type) {
	    case SDL_QUIT:
	      SDL_Quit();
	      exit(0);
	      break;
	    default:
	      break;
	}
	return 0;
	}



int job(void) 
{
if(av_read_frame(pVideoCtx, &pkg)>=0) {
  //	fprintf(stderr,"%s\n","FR");
    // Is this a packet from the video stream?
    if(pkg.stream_index==vFrame) {
      // Decode video frame
      avcodec_decode_video2(pCodCtx, framePtr, &frameEnd, 
			   &pkg);
      
      // Did we get a video frame?
      if(frameEnd) {
	SDL_LockYUVOverlay(texture);

	
	pict.data[0] = texture->pixels[0];
	pict.data[1] = texture->pixels[2];
	pict.data[2] = texture->pixels[1];

	pict.linesize[0] = texture->pitches[0];
	pict.linesize[1] = texture->pitches[2];
	pict.linesize[2] = texture->pitches[1];

	// Convert the image into YUV format that SDL uses
    sws_scale
    (
        swsCtx, 
        (uint8_t const * const *)framePtr->data, 
        framePtr->linesize, 
        0,
        pCodCtx->height,
        pict.data,
        pict.linesize
    );
	
	SDL_UnlockYUVOverlay(texture);
	
	rect.x = 0;
	rect.y = 0;
	rect.w = (pCodCtx->width);
	rect.h = (pCodCtx->height);
	SDL_DisplayYUVOverlay(texture, &rect);
      
      }
    }
	//return 0;	
	}else{
		return 1;
	}



	return 0;
}
