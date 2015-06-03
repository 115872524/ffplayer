#ifndef _BMP_QUEUE_H_
#define _BMP_QUEUE_H_

// ����ͷ�ļ�
#include <inttypes.h>
#include <semaphore.h>

// android
#include <android_runtime/AndroidRuntime.h>
#include <system/window.h>

// namespace android
using namespace android;

// ��������
#define DEF_BMP_QUEUE_SIZE   3

typedef struct {
    long      head;
    long      tail;
    long      size;
    sem_t     semr;
    sem_t     semw;
    int64_t  *ppts;
    sp<ANativeWindow> win;
} BMPQUEUE;

// ��������
bool bmpqueue_create (BMPQUEUE *pbq, sp<ANativeWindow> win, int w, int h);
void bmpqueue_destroy(BMPQUEUE *pbq);
bool bmpqueue_isempty(BMPQUEUE *pbq);

//++ ���������ӿں������ڿ��п�д bitmap �Ĺ��� ++//
// bmpqueue_write_request ȡ�õ�ǰ��д�Ŀ��� bitmap
// bmpqueue_write_release �ͷŵ�ǰ��д�Ŀ��� bitmap
// bmpqueue_write_post ���д�����
// ����ʹ�÷�����
// bmpqueue_write_request ���᷵�� bitmap
// ��� bitmap ������
// ������ɹ�����ִ��
//     bmpqueue_write_post
// ������ʧ�ܣ���ִ��
//     bmpqueue_write_release
void bmpqueue_write_request(BMPQUEUE *pbq, int64_t **ppts, uint8_t **pbuf, int *stride);
void bmpqueue_write_release(BMPQUEUE *pbq);
void bmpqueue_write_done   (BMPQUEUE *pbq);
//-- ���������ӿں������ڿ��п�д bitmap �Ĺ��� --//

//++ ���������ӿں������ڿ��пɶ� bitmap �Ĺ��� ++//
void bmpqueue_read_request(BMPQUEUE *pbq, int64_t **ppts, ANativeWindowBuffer **buf);
void bmpqueue_read_release(BMPQUEUE *pbq);
void bmpqueue_read_done   (BMPQUEUE *pbq);
//-- ���������ӿں������ڿ��пɶ� bitmap �Ĺ��� --//


#endif




