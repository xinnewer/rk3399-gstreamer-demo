#ifndef __MAIN_H__
#define __MAIN_H__

#include <gst/gst.h>
#include <glib.h>

typedef struct
{
    GstElement *Pipeline;

    GMainLoop *loop;

    int state;

} Handler;

int Init(Handler *handler);
int DeInit(Handler *handler);
int ReadData(Handler *handler);
int WriteData(Handler *handler);
void GetStatus();
void SetStatus();
Handler *CreateHandler();

#endif