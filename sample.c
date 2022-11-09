#include <gst/gst.h>
#include <glib.h>

int main_backup(int argc, char *argv[])
{
	GstElement *rkisp01, capsfilter01, *mpph264enc01, *queue01, *h264parse01, *mpegtsmux01, *filesink01, *pipline;
	GstCaps *caps;
	GstBus *bus;
	guint bus_watch_id;
	GMainLoop *loop;

	/* intializing gstreamer and others enviroment */
	gst_init(&argc, &argv);
	loop = g_main_loop_new(NULL, FALSE);

	/* create all required elements */
	pipline = gst_pipeline_new("rockchip-rk3399");
	rkisp01 = gst_element_factory_make("rkisp", "rkisp01");
	capsfilter01 = gst_element_factory_make("capsfilter", "capsfilter01");
	mpph264enc01 = gst_element_factory_make("mpph264enc", "mpph264enc01");
	queue01 = gst_element_factory_make("queue", "queue01");
	h264parse01 = gst_element_factory_make("h264parse", "h264parse01");
	mpegtsmux01 = gst_element_factory_make("mpegtsmux", "mpegtsmux");
	filesink01 = gst_element_factory_make("filesink", "filesink01");

	if (!pipeline || !rkisp01 || !capsfilter01 || !mpph264enc01 || !queue01 || !h264parse01 || !mpegtsmux01 || !filesink01)
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
	// config filesink parameters
	g_object_set(G_OBJECT(filesink01), "location", "/tmp/camear-record-pri.tx", NULL);
	gst_caps_unref(caps);

	/* we add a message handler */
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	// here, you can add your handle
	// bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
	gst_object_unref(bus);

	/* add this elements to pipline and link its */
	gst_bin_add_many(GST_BIN(pipeline), rkisp01, capsfilter01, mpph264enc01, queue01, h264parse01, mpegtsmux01, filesink01, NULL);
	gst_element_link_many(rkisp01, capsfilter01, mpph264enc01, queue01, h264parse01, mpegtsmux01, filesink01);
	// ----- here, you can add some specific handle

	/* start to running program */
	g_print("Now playing: %s\n", argv[1]);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	g_print("Running...\n");
	g_main_loop_run(loop);

	/* stop to running program */
	g_print("Returned, stopping playback\n");
	gst_element_set_state(pipeline, GST_STATE_NULL);

	g_print("Deleting pipeline\n");
	gst_object_unref(GST_OBJECT(pipeline));
	g_source_remove(bus_watch_id);
	g_main_loop_unref(loop);

	return 0;
}
