#include "main.h"

enum
{
    S_NULL = 0,
    S_OK = 1,

    S_INITIALIZING = 2,
    S_DEINITIALIZING = 3,
    S_READING = 4,
    S_WRITING = 5,
};

enum
{
    A_INIT = 10,
    A_DEINIT = 11,
    A_READ = 12,
    A_WRITE = 13,
};

int main()
{
    Handler *handler;
    handler = CreateHandler();
    Init(handler);
}

int StateStep(Handler *handler, int action, int location)
{

    if (handler->state == S_NULL || handler->state == S_OK)
    {
        // if the status is static state, handle it.
        switch (handler->state)
        {
        case S_NULL:
            if (action == A_INIT)
            {
                handler->state = S_DEINITIALIZING;
                return 0;
            }
            return -1;
        case S_OK:
            if (action == A_DEINIT)
            {
                handler->state = S_DEINITIALIZING;
                return 0;
            }
            else if (action == A_READ)
            {
                handler->state = S_READING;
                return 0;
            }
            else if (action == A_WRITE)
            {
                handler->state = S_WRITING;
                return 0;
            }
            return -1;
        default:
            // the branch is never reachable
            return -1;
        }
    }
    else
    {
        // if the status is dynamic state, handle it.
        switch (handler->state)
        {
        case S_INITIALIZING:
            if (action == A_INIT)
            {
                if (location == 1)
                {
                    handler->state = S_OK;
                    return 0;
                }
            }
            return -1;

        case S_DEINITIALIZING:
            if (action == A_DEINIT)
            {
                if (location == 1)
                {
                    handler->state = S_NULL;
                    return 0;
                }
            }
            return -1;
        case S_READING:
            if (action == A_READ)
            {
                if (location == 1)
                {
                    handler->state = S_OK;
                    return 0;
                }
            }
            return -1;
        case S_WRITING:
            if (action == A_WRITE)
            {
                if (location == 1)
                {
                    handler->state = S_OK;
                    return 0;
                }
            }
            return -1;
        default:
            // the branch should never reach
            break;
        }
    }
}

/* The appsink has received a buffer */
GstFlowReturn new_sample_cb(GstElement *sink, Handler *handler)
{
    GstSample *sample;

    /* Retrieve the buffer */
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (sample)
    {
        /* The only thing we do in this example is print a * to indicate a received buffer */
        g_print("*");
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}

/* This function is called when an error message is posted on the bus */
static void error_cb(GstBus *bus, GstMessage *msg, Handler *handler)
{
    GError *err;
    gchar *debug_info;

    /* Print error details on the screen */
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);

    g_main_loop_quit(handler->loop);
}

Handler *CreateHandler()
{
    Handler *handler;
    handler = (Handler *)malloc(sizeof(Handler));
    handler->state = S_NULL;
    handler->Pipeline = NULL;
    handler->loop = NULL;

    return handler;
}

int Init(Handler *handler)
{
    // create required elements by handler
    GstElement *rkisp01, *capsfilter01, *mpph264enc01, *queue01, *h264parse01, *mpegtsmux01, *appsink01;
    GstCaps *caps;
    GstBus *bus;
    guint bus_watch_id;
    int flag; // the flag determine the branch, branch 1: exit. branch 2: initialize handler

    flag = StateStep(handler, A_INIT, 0);
    if (flag == -1)
    {
        return -1;
    }

    /* intializing gstreamer and others enviroment */
    gst_init(NULL, NULL);
    handler->loop = g_main_loop_new(NULL, FALSE);

    /* create all required elements */
    handler->Pipeline = gst_pipeline_new("rockchip-rk3399");
    rkisp01 = gst_element_factory_make("rkisp", "rkisp01");
    capsfilter01 = gst_element_factory_make("capsfilter", "capsfilter01");
    mpph264enc01 = gst_element_factory_make("mpph264enc", "mpph264enc01");
    queue01 = gst_element_factory_make("queue", "queue01");
    h264parse01 = gst_element_factory_make("h264parse", "h264parse01");
    mpegtsmux01 = gst_element_factory_make("mpegtsmux", "mpegtsmux");
    appsink01 = gst_element_factory_make("appsink", "appsink01");

    if (!handler->Pipeline || !rkisp01 || !capsfilter01 || !mpph264enc01 || !queue01 || !h264parse01 || !mpegtsmux01 || !appsink01)
    {
        g_printerr("One element could not be created. Exiting.\n");
        return -1;
    }

    /* configure this elements */
    // config rkisp source parameters
    g_object_set(G_OBJECT(rkisp01), "num-buffers", 512, NULL);
    g_object_set(G_OBJECT(rkisp01), "device", "/dev/video0", NULL);
    g_object_set(G_OBJECT(rkisp01), "io-mode", 1, NULL);
    // create capsfilter parameters
    caps = gst_caps_new_simple("video/x-raw",
                               "format", G_TYPE_STRING, "NV12",
                               "width", G_TYPE_INT, 1280,
                               "height", G_TYPE_INT, 720,
                               "framerate", GST_TYPE_FRACTION, 30, 1,
                               NULL);
    g_object_set(G_OBJECT(capsfilter01), "caps", caps, NULL);
    // config appsink parameters
    g_object_set(appsink01, "emit-signals", TRUE, "caps", caps, NULL);
    g_signal_connect(appsink01, "new-sample", G_CALLBACK(new_sample_cb), handler);
    gst_caps_unref(caps);

    /* we add a message handler */
    bus = gst_pipeline_get_bus(GST_PIPELINE(handler->Pipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, handler);
    gst_object_unref(bus);

    /* add this elements to Pipeline and link its */
    gst_bin_add_many(GST_BIN(handler->Pipeline), rkisp01, capsfilter01, mpph264enc01, queue01, h264parse01, mpegtsmux01, appsink01, NULL);
    gst_element_link_many(rkisp01, capsfilter01, mpph264enc01, queue01, h264parse01, mpegtsmux01, appsink01, NULL);
    // ----- here, you can add some specific handle

    flag = StateStep(handler, A_INIT, 1);
    if (flag == -1)
    {
        return -1;
    }

    return 0;
}

int DeInit(Handler *handler)
{
    int flag;

    flag = StateStep(handler, A_DEINIT, 0);
    if (flag == -1)
    {
        return -1;
    }

    /* stop to running program */
    g_print("Returned, stopping playback\n");
    gst_element_set_state(handler->Pipeline, GST_STATE_NULL);

    g_print("Deleting pipeline\n");
    gst_object_unref(GST_OBJECT(handler->Pipeline));
    g_main_loop_unref(handler->loop);

    flag = StateStep(handler, A_DEINIT, 1);
    if (flag == -1)
    {
        return -1;
    }

    return 0;
}

int ReadData(Handler *handler)
{
    int flag;

    flag = StateStep(handler, A_READ, 0);
    if (flag == -1)
    {
        return -1;
    }

    // start to playing the handler
    g_print("Now playing\n");
    gst_element_set_state(handler->Pipeline, GST_STATE_PLAYING);

    // to do something through main loop
    g_print("Running...\n");
    g_main_loop_run(handler->loop);

    // stop to playing the handler
    g_print("Returned, stopping playback\n");
    gst_element_set_state(handler->Pipeline, GST_STATE_PAUSED);

    g_print("Running...\n");
    g_main_loop_quit(handler->loop);

    flag = StateStep(handler, A_READ, 1);
    if (flag == -1)
    {
        return -1;
    }

    return 0;
}

int WriteData(Handler *handler)
{
    int flag;

    flag = StateStep(handler, A_WRITE, 0);
    if (flag == -1)
    {
        return -1;
    }

    // ----- here, you can add your code to do.

    flag = StateStep(handler, A_WRITE, 1);
    if (flag == -1)
    {
        return -1;
    }

    return 0;
}

void GetStatus()
{
}

void SetStatus()
{
}
