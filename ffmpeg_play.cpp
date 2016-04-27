//ffmpeg simple player
//
//
#include "stdafx.h"


#pragma comment(lib, "avcodec.lib   ")
#pragma comment(lib, "avformat.lib  ")
#pragma comment(lib, "avutil.lib    ")
#pragma comment(lib, "avdevice.lib  ")
#pragma comment(lib, "avfilter.lib  ")
#pragma comment(lib, "postproc.lib  ")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib   ")
#pragma comment(lib, "libmysql.lib  ")
#pragma comment(lib, "opencv_highgui300d.lib")
#pragma comment(lib, "opencv_core300d.lib   ")


class ffmpegDeCode
{
public:
	ffmpegDeCode(char * file = NULL, char * window = NULL);
	~ffmpegDeCode();

	void decodeOne();
	void lastFrame();
	int  readOne();


private:
	char * WindowName;
	AVFrame	*pFrame;
	AVFormatContext	*pFormatCtx  ;
	int	i; 
	int videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec   ;
	char *filepath;
	IplImage* pCVFrame; 
	int nFrame;
	uint8_t *out_bufferRGB;
	int ret, got_picture;
	SwsContext *img_convert_ctx;
	int y_size;
	AVPacket *packet;

	void init();
	void openDecde();
	void prepare();
	void get(AVCodecContext	*pCodecCtx, SwsContext *img_convert_ctx,AVFrame	*pFrame);

};

ffmpegDeCode :: ~ffmpegDeCode()
{
	//释放本次读取的帧内存
	av_free_packet(packet);

	cvReleaseImage( &pCVFrame ); //释放图像
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
}

ffmpegDeCode :: ffmpegDeCode(char * file, char * window)
{
	pFrame = NULL/**pFrameRGB = NULL*/;
	pFormatCtx  = NULL;
	i=0;
	videoindex=0;
	pCodecCtx   = NULL;
	pCodec      = NULL;
	pCVFrame = NULL; 
	nFrame = 0;
	out_bufferRGB = NULL;
	ret = 0;
	got_picture = 0;
	img_convert_ctx = NULL;
	y_size = 0;
	packet = NULL;

	if (NULL == file)
	{
		//char filepath[]="nwn.mp4";
		//filepath =  "qiantai.asf";
		filepath =  "test.mp4";
	}
	else
	{
		filepath = file;
	}

	if (NULL == window)
	{
		WindowName = "decode";
	}
	else
	{
		WindowName = window;
	}

	init();
	openDecde();
	prepare();

	return;
}

void ffmpegDeCode :: get(AVCodecContext	* pCodecCtx, SwsContext * img_convert_ctx, AVFrame * pFrame)
{
	IplImage * pCVFrame = NULL;
	if (NULL == pCVFrame)
	{
		pCVFrame=cvCreateImage(cvSize(pCodecCtx->width, pCodecCtx->height),8,3); 
	}

	AVFrame	*pFrameRGB = NULL;
	uint8_t *out_bufferRGB = NULL;
	pFrameRGB = avcodec_alloc_frame();

	//给pFrameRGB帧加上分配的内存;
	out_bufferRGB=new uint8_t[avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height)];
	avpicture_fill((AVPicture *)pFrameRGB, out_bufferRGB, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

	//YUV to RGB
	sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
	memcpy(pCVFrame->imageData,out_bufferRGB,pCodecCtx->width*pCodecCtx->height*24/8);
	pCVFrame->widthStep=pCodecCtx->width*3; //4096
	pCVFrame->origin=0;

	cvShowImage("decode",pCVFrame);//显示  
	cvReleaseImage(&pCVFrame);
	delete[] out_bufferRGB;
	av_free(pFrameRGB);

	cvWaitKey(20);
}

int ffmpegDeCode :: readOne()
{
	int result = 0;
	result = av_read_frame(pFormatCtx, packet);
	return result;
}

void ffmpegDeCode :: init()
{
	//ffmpeg注册复用器，编码器等的函数av_register_all()。
	//该函数在所有基于ffmpeg的应用程序中几乎都是第一个被调用的。只有调用了该函数，才能使用复用器，编码器等。
	//这里注册了所有的文件格式和编解码器的库，所以它们将被自动的使用在被打开的合适格式的文件上。注意你只需要调用 av_register_all()一次，因此我们在主函数main()中来调用它。如果你喜欢，也可以只注册特定的格式和编解码器，但是通常你没有必要这样做。
	av_register_all();

	//需要播放网络视频
	avformat_network_init();

	pFormatCtx = avformat_alloc_context();

	//打开视频文件,通过参数filepath来获得文件名。这个函数读取文件的头部并且把信息保存到我们给的AVFormatContext结构体中。
	//最后2个参数用来指定特殊的文件格式，缓冲大小和格式参数，但如果把它们设置为空NULL或者0，libavformat将自动检测这些参数。
	if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0)
	{
		printf("无法打开文件\n");
		return;
	}

	//查找文件的流信息,avformat_open_input函数只是检测了文件的头部，接着要检查在文件中的流的信息
	if(av_find_stream_info(pFormatCtx)<0)
	{
		printf("Couldn't find stream information.\n");
		return;
	}

	cvNamedWindow(WindowName);  
	return;
}

void ffmpegDeCode :: openDecde()
{
	//遍历文件的各个流，找到第一个视频流，并记录该流的编码信息
	videoindex=-1;
	int gg=pFormatCtx->nb_streams;
	//cout<<gg<<endl;
	for(i=0; i<pFormatCtx->nb_streams; i++) 
	{
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
			break;
		}
	}
	if(videoindex==-1)
	{
		printf("Didn't find a video stream.\n");
		return;
	}
	pCodecCtx=pFormatCtx->streams[videoindex]->codec;

	//在库里面查找支持该格式的解码器
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
	{
		printf("Codec not found.\n");
		return;
	}

	//打开解码器
	if(avcodec_open(pCodecCtx, pCodec)<0)
	{
		printf("Could not open codec.\n");
		return;
	}
}

void ffmpegDeCode :: prepare()
{
	//分配一个帧指针，指向解码后的原始帧
	pFrame=avcodec_alloc_frame();

	y_size = pCodecCtx->width * pCodecCtx->height;

	//分配帧内存
	packet=(AVPacket *)malloc(sizeof(AVPacket));
	av_new_packet(packet, y_size);

	//输出一下信息-----------------------------
	printf("文件信息-----------------------------------------\n");
	av_dump_format(pFormatCtx,0,filepath,0);//av_dump_format只是个调试函数，输出文件的音、视频流的基本信息了，帧率、分辨率、音频采样等等
	printf("-------------------------------------------------\n");

}

void ffmpegDeCode :: decodeOne()
{
	if(packet->stream_index==videoindex)
	{
		//解码一个帧
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if(ret < 0)
		{
			printf("解码错误\n");
			return;
		}
		if(got_picture)
		{
			printf("帧号: %d \n",nFrame);
			nFrame++;

			//根据编码信息设置渲染格式
			img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL); 

			//----------------------opencv
			if (NULL == pCVFrame)
			{
				pCVFrame=cvCreateImage(cvSize(pCodecCtx->width, pCodecCtx->height),8,3); 
			}

			if(img_convert_ctx != NULL)  
			{  
				get(pCodecCtx, img_convert_ctx,
					pFrame);
			}  

		}
	}
	av_free_packet(packet);
}

void ffmpegDeCode :: lastFrame()
{
	ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
	if(got_picture) 
	{  
		printf("最后一帧 %d\n", nFrame);  
		nFrame++;  

		//根据编码信息设置渲染格式
		img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL); 

		if(img_convert_ctx != NULL)  
		{  
			get(pCodecCtx, img_convert_ctx,pFrame);

		}  
	}  

}

int _tmain(int argc, _TCHAR* argv[])
{
	//char * WindowName = "decode";
	ffmpegDeCode deCode;

	//av_read_frame 读取一个帧（到最后帧则Xbreak）
	while(deCode.readOne()>=0)
	{
		deCode.decodeOne();
	}

	/* some codecs,such as MPEG, transmit the I and P frame with a 
    latency of one frame. You must do thefollowing to have a 
    chance to get the last frame of the video */  
	deCode.lastFrame();

	return 0;
}

