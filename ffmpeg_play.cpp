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
	//�ͷű��ζ�ȡ��֡�ڴ�
	av_free_packet(packet);

	cvReleaseImage( &pCVFrame ); //�ͷ�ͼ��
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

	//��pFrameRGB֡���Ϸ�����ڴ�;
	out_bufferRGB=new uint8_t[avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height)];
	avpicture_fill((AVPicture *)pFrameRGB, out_bufferRGB, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

	//YUV to RGB
	sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
	memcpy(pCVFrame->imageData,out_bufferRGB,pCodecCtx->width*pCodecCtx->height*24/8);
	pCVFrame->widthStep=pCodecCtx->width*3; //4096
	pCVFrame->origin=0;

	cvShowImage("decode",pCVFrame);//��ʾ  
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
	//ffmpegע�Ḵ�������������ȵĺ���av_register_all()��
	//�ú��������л���ffmpeg��Ӧ�ó����м������ǵ�һ�������õġ�ֻ�е����˸ú���������ʹ�ø��������������ȡ�
	//����ע�������е��ļ���ʽ�ͱ�������Ŀ⣬�������ǽ����Զ���ʹ���ڱ��򿪵ĺ��ʸ�ʽ���ļ��ϡ�ע����ֻ��Ҫ���� av_register_all()һ�Σ����������������main()�����������������ϲ����Ҳ����ֻע���ض��ĸ�ʽ�ͱ������������ͨ����û�б�Ҫ��������
	av_register_all();

	//��Ҫ����������Ƶ
	avformat_network_init();

	pFormatCtx = avformat_alloc_context();

	//����Ƶ�ļ�,ͨ������filepath������ļ��������������ȡ�ļ���ͷ�����Ұ���Ϣ���浽���Ǹ���AVFormatContext�ṹ���С�
	//���2����������ָ��������ļ���ʽ�������С�͸�ʽ���������������������Ϊ��NULL����0��libavformat���Զ������Щ������
	if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0)
	{
		printf("�޷����ļ�\n");
		return;
	}

	//�����ļ�������Ϣ,avformat_open_input����ֻ�Ǽ�����ļ���ͷ��������Ҫ������ļ��е�������Ϣ
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
	//�����ļ��ĸ��������ҵ���һ����Ƶ��������¼�����ı�����Ϣ
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

	//�ڿ��������֧�ָø�ʽ�Ľ�����
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
	{
		printf("Codec not found.\n");
		return;
	}

	//�򿪽�����
	if(avcodec_open(pCodecCtx, pCodec)<0)
	{
		printf("Could not open codec.\n");
		return;
	}
}

void ffmpegDeCode :: prepare()
{
	//����һ��ָ֡�룬ָ�������ԭʼ֡
	pFrame=avcodec_alloc_frame();

	y_size = pCodecCtx->width * pCodecCtx->height;

	//����֡�ڴ�
	packet=(AVPacket *)malloc(sizeof(AVPacket));
	av_new_packet(packet, y_size);

	//���һ����Ϣ-----------------------------
	printf("�ļ���Ϣ-----------------------------------------\n");
	av_dump_format(pFormatCtx,0,filepath,0);//av_dump_formatֻ�Ǹ����Ժ���������ļ���������Ƶ���Ļ�����Ϣ�ˣ�֡�ʡ��ֱ��ʡ���Ƶ�����ȵ�
	printf("-------------------------------------------------\n");

}

void ffmpegDeCode :: decodeOne()
{
	if(packet->stream_index==videoindex)
	{
		//����һ��֡
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if(ret < 0)
		{
			printf("�������\n");
			return;
		}
		if(got_picture)
		{
			printf("֡��: %d \n",nFrame);
			nFrame++;

			//���ݱ�����Ϣ������Ⱦ��ʽ
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
		printf("���һ֡ %d\n", nFrame);  
		nFrame++;  

		//���ݱ�����Ϣ������Ⱦ��ʽ
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

	//av_read_frame ��ȡһ��֡�������֡��Xbreak��
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

