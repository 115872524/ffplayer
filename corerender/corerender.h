#ifndef _CORERENDER_H_
#define _CORERENDER_H_

// ����ͷ�ļ�
#include <windows.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

// ���Ͷ���
typedef struct
{
    int16_t *data;
    int32_t  size;
} AUDIOBUF;

// ��������
HANDLE renderopen(HWND hwnd, AVRational frate, int pixfmt, int w, int h,
                  int64_t chan_layout, AVSampleFormat format, int rate);

void renderclose     (HANDLE hrender);
void renderaudiowrite(HANDLE hrender, AVFrame *audio);
void rendervideowrite(HANDLE hrender, AVFrame *video);
void rendersetrect   (HANDLE hrender, int x, int y, int w, int h);
void renderstart     (HANDLE hrender);
void renderpause     (HANDLE hrender);
void renderflush     (HANDLE hrender);

#endif
















