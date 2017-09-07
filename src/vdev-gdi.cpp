// ����ͷ�ļ�
#include "vdev.h"

extern "C" {
#include "libavformat/avformat.h"
}

// �ڲ���������
#define DEF_VDEV_BUF_NUM  3

// �ڲ����Ͷ���
typedef struct
{
    // common members
    VDEV_COMMON_MEMBERS

    HDC      hdcsrc;
    HDC      hdcdst;
    HBITMAP *hbitmaps;
    BYTE   **pbmpbufs;
} VDEVGDICTXT;

// �ڲ�����ʵ��
static void* video_render_thread_proc(void *param)
{
    VDEVGDICTXT *c = (VDEVGDICTXT*)param;

    while (1) {
        sem_wait(&c->semr);
        if (c->status & VDEV_CLOSE) break;

        if (c->refresh_flag) {
            c->refresh_flag = 0;
            vdev_refresh_background(c);
        }

        int64_t apts = c->apts;
        int64_t vpts = c->vpts = c->ppts[c->head];
#if CLEAR_VDEV_WHEN_COMPLETED
        if (vpts != -1 && !(c->status & VDEV_COMPLETED)) {
#else
        if (vpts != -1) {
#endif
            SelectObject(c->hdcsrc, c->hbitmaps[c->head]);
            BitBlt(c->hdcdst, c->x, c->y, c->w, c->h, c->hdcsrc, 0, 0, SRCCOPY);
        }

        av_log(NULL, AV_LOG_DEBUG, "vpts: %lld\n", vpts);
        if (++c->head == c->bufnum) c->head = 0;
        sem_post(&c->semw);

        if (!(c->status & (VDEV_PAUSE|VDEV_COMPLETED))) {
            // send play progress event
            vdev_player_event(c, PLAY_PROGRESS, c->vpts > c->apts ? c->vpts : c->apts);

            //++ play completed ++//
            if (c->completed_apts != c->apts || c->completed_vpts != c->vpts) {
                c->completed_apts = c->apts;
                c->completed_vpts = c->vpts;
                c->completed_counter = 0;
            }
            else if (++c->completed_counter == 50) {
                av_log(NULL, AV_LOG_INFO, "play completed !\n");
                c->status |= VDEV_COMPLETED;
                vdev_player_event(c, PLAY_COMPLETED, 0);

#if CLEAR_VDEV_WHEN_COMPLETED
                InvalidateRect((HWND)c->hwnd, NULL, TRUE);
#endif
            }
            //-- play completed --//

            //++ frame rate & av sync control ++//
            DWORD   tickcur  = GetTickCount();
            int     tickdiff = tickcur - c->ticklast;
            int64_t avdiff   = apts - vpts - c->tickavdiff;
            c->ticklast = tickcur;
            if (tickdiff - c->tickframe >  2) c->ticksleep--;
            if (tickdiff - c->tickframe < -2) c->ticksleep++;
            if (apts != -1 && vpts != -1) {
                if (avdiff > 5) c->ticksleep-=2;
                if (avdiff <-5) c->ticksleep+=2;
            }
            if (c->ticksleep < 0) c->ticksleep = 0;
            if (c->ticksleep > 0) Sleep(c->ticksleep);
            av_log(NULL, AV_LOG_INFO, "gdi d: %3lld, s: %d\n", avdiff, c->ticksleep);
            //-- frame rate & av sync control --//
        }
        else Sleep(c->tickframe);
    }

    return NULL;
}

// �ӿں���ʵ��
void* vdev_gdi_create(void *surface, int bufnum, int w, int h, int frate)
{
    VDEVGDICTXT *ctxt = (VDEVGDICTXT*)calloc(1, sizeof(VDEVGDICTXT));
    if (!ctxt) {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate gdi vdev context !\n");
        exit(0);
    }

    // init vdev context
    bufnum          = bufnum ? bufnum : DEF_VDEV_BUF_NUM;
    ctxt->hwnd      = surface;
    ctxt->bufnum    = bufnum;
    ctxt->pixfmt    = AV_PIX_FMT_RGB32;
    ctxt->w         = w;
    ctxt->h         = h;
    ctxt->sw        = w;
    ctxt->sh        = h;
    ctxt->tickframe = 1000 / frate;
    ctxt->ticksleep = ctxt->tickframe;
    ctxt->apts      = -1;
    ctxt->vpts      = -1;

    // alloc buffer & semaphore
    ctxt->ppts     = (int64_t*)calloc(bufnum, sizeof(int64_t));
    ctxt->hbitmaps = (HBITMAP*)calloc(bufnum, sizeof(HBITMAP));
    ctxt->pbmpbufs = (BYTE**  )calloc(bufnum, sizeof(BYTE*  ));

    // create semaphore
    sem_init(&ctxt->semr, 0, 0     );
    sem_init(&ctxt->semw, 0, bufnum);

    ctxt->hdcdst = GetDC((HWND)ctxt->hwnd);
    ctxt->hdcsrc = CreateCompatibleDC(ctxt->hdcdst);
    if (!ctxt->ppts || !ctxt->hbitmaps || !ctxt->pbmpbufs || !ctxt->semr || !ctxt->semw || !ctxt->hdcdst || !ctxt->hdcsrc) {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate resources for vdev-gdi !\n");
        exit(0);
    }

    // create video rendering thread
    pthread_create(&ctxt->thread, NULL, video_render_thread_proc, ctxt);
    return ctxt;
}

void vdev_gdi_destroy(void *ctxt)
{
    int i;
    VDEVGDICTXT *c = (VDEVGDICTXT*)ctxt;

    // make visual effect & rendering thread safely exit
    c->status = VDEV_CLOSE;
    sem_post(&c->semr);
    pthread_join(c->thread, NULL);

    //++ for video
    DeleteDC (c->hdcsrc);
    ReleaseDC((HWND)c->hwnd, c->hdcdst);
    for (i=0; i<c->bufnum; i++) {
        if (c->hbitmaps[i]) {
            DeleteObject(c->hbitmaps[i]);
        }
    }
    //-- for video

    // close semaphore
    sem_destroy(&c->semr);
    sem_destroy(&c->semw);

#if CLEAR_VDEV_WHEN_DESTROYED
    // clear window to background
    InvalidateRect((HWND)c->hwnd, NULL, TRUE);
#endif

    // free memory
    free(c->ppts    );
    free(c->hbitmaps);
    free(c->pbmpbufs);
    free(c);
}

void vdev_gdi_request(void *ctxt, uint8_t *buffer[8], int linesize[8])
{
    VDEVGDICTXT *c = (VDEVGDICTXT*)ctxt;

    sem_wait(&c->semw);

    BITMAP bitmap;
    int bmpw = 0;
    int bmph = 0;
    if (c->hbitmaps[c->tail]) {
        GetObject(c->hbitmaps[c->tail], sizeof(BITMAP), &bitmap);
        bmpw = bitmap.bmWidth ;
        bmph = bitmap.bmHeight;
    }

    if (bmpw != c->w || bmph != c->h) {
        if (c->hbitmaps[c->tail]) {
            DeleteObject(c->hbitmaps[c->tail]);
        }

        BITMAPINFO bmpinfo = {0};
        bmpinfo.bmiHeader.biSize        =  sizeof(BITMAPINFOHEADER);
        bmpinfo.bmiHeader.biWidth       =  c->w;
        bmpinfo.bmiHeader.biHeight      = -c->h;
        bmpinfo.bmiHeader.biPlanes      =  1;
        bmpinfo.bmiHeader.biBitCount    =  32;
        bmpinfo.bmiHeader.biCompression =  BI_RGB;
        c->hbitmaps[c->tail] = CreateDIBSection(c->hdcsrc, &bmpinfo, DIB_RGB_COLORS,
                                        (void**)&c->pbmpbufs[c->tail], NULL, 0);
        GetObject(c->hbitmaps[c->tail], sizeof(BITMAP), &bitmap);
    }

    if (buffer  ) buffer[0]   = c->pbmpbufs[c->tail];
    if (linesize) linesize[0] = bitmap.bmWidthBytes ;
}

void vdev_gdi_post(void *ctxt, int64_t pts)
{
    VDEVGDICTXT *c = (VDEVGDICTXT*)ctxt;
    c->ppts[c->tail] = pts;
    if (++c->tail == c->bufnum) c->tail = 0;
    sem_post(&c->semr);
}

void vdev_gdi_setrect(void *ctxt, int x, int y, int w, int h)
{
    VDEV_COMMON_CTXT *c = (VDEV_COMMON_CTXT*)ctxt;
    c->x  = x; c->y  = y;
    c->w  = w; c->h  = h;
    c->sw = w; c->sh = h;
    c->refresh_flag  = 1;
}
