#ifndef __FFPLAYER_SNAPSHOT_H__
#define __FFPLAYER_SNAPSHOT_H__

#ifdef __cplusplus
extern "C" {
#endif

// ����ͷ�ļ�
#include "libavutil/frame.h"

// ��������
void* snapshot_init(void);
int   snapshot_take(void *ctxt, char *file, AVFrame *video);
void  snapshot_free(void *ctxt);

#ifdef __cplusplus
}
#endif

#endif


