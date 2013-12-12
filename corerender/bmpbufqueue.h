#ifndef _BMPBUF_QUEUE_H_
#define _BMPBUF_QUEUE_H_

// ����ͷ�ļ�
#include <windows.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// ��������
#define DEF_BMPBUF_QUEUE_SIZE   32

typedef struct {
    long      head;
    long      tail;
    long      size;
    long      curnum;
    HANDLE    semr;
    HANDLE    semw;
    int64_t  *ppts;
    HBITMAP  *hbitmaps;
    BYTE    **pbmpbufs;
} BMPBUFQUEUE;

// ��������
BOOL bmpbufqueue_create (BMPBUFQUEUE *pbq, HDC hdc, int w, int h, int cdepth);
void bmpbufqueue_destroy(BMPBUFQUEUE *pbq);
void bmpbufqueue_flush  (BMPBUFQUEUE *pbq);
BOOL bmpbufqueue_isempty(BMPBUFQUEUE *pbq);

//++ ���������ӿں������ڿ��п�д bitmap �Ĺ��� ++//
// bmpbufqueue_write_request ȡ�õ�ǰ��д�Ŀ��� bitmap
// bmpbufqueue_write_release �ͷŵ�ǰ��д�Ŀ��� bitmap
// bmpbufqueue_write_post ���д�����
// ����ʹ�÷�����
// bmpbufqueue_write_request ���᷵�� bitmap
// ��� bitmap ������
// ������ɹ�����ִ��
//     bmpbufqueue_write_post
// ������ʧ�ܣ���ִ��
//     bmpbufqueue_write_release
void bmpbufqueue_write_request(BMPBUFQUEUE *pbq, int64_t **ppts, BYTE **pbuf, int *stride);
void bmpbufqueue_write_release(BMPBUFQUEUE *pbq);
void bmpbufqueue_write_done   (BMPBUFQUEUE *pbq);
//-- ���������ӿں������ڿ��п�д bitmap �Ĺ��� --//

//++ ���������ӿں������ڿ��пɶ� bitmap �Ĺ��� ++//
void bmpbufqueue_read_request(BMPBUFQUEUE *pbq, int64_t **ppts, HBITMAP *hbitmap);
void bmpbufqueue_read_release(BMPBUFQUEUE *pbq);
void bmpbufqueue_read_done   (BMPBUFQUEUE *pbq);
//-- ���������ӿں������ڿ��пɶ� bitmap �Ĺ��� --//

#ifdef __cplusplus
}
#endif

#endif




