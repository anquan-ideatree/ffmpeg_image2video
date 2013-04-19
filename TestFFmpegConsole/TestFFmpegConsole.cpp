// TestFFmpegConsole.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
extern "C"
{
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>

}

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

static	void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);


static void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	const char *szFilename = "d:\\1234";
	int y;

	// Open file
	fopen_s(&pFile, szFilename, "wt");
	if (pFile == NULL)
		return;

	// Write header
	//printf("P6\n%d %d\n255\n", width, height);
	printf("bbbbbbbbbb\n");
	// Write pixel data
	for (y = 0; y < height; y++)
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], sizeof(char),
				width * 3, pFile);

	// Close file
	fclose(pFile);
}

int _tmain(int argc, _TCHAR* argv[])
{
	AVFormatContext *pFormatCtx = NULL;
	int i, videoStream;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame;
	AVFrame *pFrameRGB;
	AVPacket packet;
	int frameFinished = NULL;
	int numBytes;
	uint8_t *buffer;
	struct SwsContext *pSwsCtx;
	// Register all formats and codecs
	av_register_all();
	avcodec_register_all();
	const char *filename = "d:\\1.jpg";
	// Open video file
	if (avformat_open_input(&pFormatCtx, filename, NULL,NULL) != 0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if (av_find_stream_info(pFormatCtx) < 0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, filename, 0);

	// Find the first video stream
	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1)
		return -1; // Didn't find a video stream

	// Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		return -1; // Could not open codec

	// Allocate video frame
	pFrame = avcodec_alloc_frame();

	// Allocate an AVFrame structure
	pFrameRGB = avcodec_alloc_frame();
	if (pFrameRGB == NULL)
		return -1;

	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
			pCodecCtx->height);
	buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) pFrameRGB, buffer, PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);

	// Read frames and save first five frames to disk
	i = 0;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {

			// Allocate video frame
			pFrame = avcodec_alloc_frame();
			int w = pCodecCtx->width;
			int h = pCodecCtx->height;
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			pSwsCtx = sws_getContext(w, h, pCodecCtx->pix_fmt, w, h,
					PIX_FMT_RGB565, SWS_POINT, NULL, NULL, NULL);
			// Did we get a video frame?

			if (frameFinished) {
				// Convert the image from its native format to RGB
				sws_scale(pSwsCtx, pFrame->data, pFrame->linesize, 0,
						pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
				// Save the frame to disk

				++i;
				printf("%d\n", i);
				SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	// Free the RGB image
	av_free(buffer);
	av_free(pFrameRGB);

	// Free the YUV frame
	av_free(pFrame);

	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	av_close_input_file(pFormatCtx);

	return 0;
}
