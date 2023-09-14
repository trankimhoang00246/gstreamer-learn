#include <gst/gst.h>

int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    GstElement *pipeline, *filesrc, *decodebin, *videoconvert, *videoscale, *autovideosink;

    pipeline = gst_pipeline_new("mypipeline");
    filesrc = gst_element_factory_make("filesrc", "file-source");
    decodebin = gst_element_factory_make("decodebin", "decodebin");
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    videoscale = gst_element_factory_make("videoscale", "videoscale");
    autovideosink = gst_element_factory_make("autovideosink", "autovideosink");

    if (!pipeline || !filesrc || !decodebin || !videoconvert || !videoscale || !autovideosink)
    {
        g_printerr("One or more elements could not be created. Exiting.\n");
        return -1;
    }

    g_object_set(G_OBJECT(filesrc), "location", "/home/hoang/Public/PC-HOANG/ThucTap/OTS/AI/GSTBuffer-Snapshot/TID.mp4", NULL);

    gst_bin_add_many(GST_BIN(pipeline), filesrc, decodebin, videoconvert, videoscale, autovideosink, NULL);

    if (!gst_element_link(filesrc, decodebin) ||
        !gst_element_link(decodebin, videoconvert) ||
        !gst_element_link(videoconvert, videoscale) ||
        !gst_element_link(videoscale, autovideosink))
    {
        g_printerr("Elements could not be linked. Exiting.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    gst_element_set_state(pipeline, GST_STATE_NULL);

    gst_object_unref(bus);
    gst_object_unref(pipeline);

    return 0;
}

// gst-launch-1.0 filesrc location="/home/hoang/Public/PC-HOANG/ThucTap/OTS/AI/GSTBuffer-Snapshot/TID.mp4" ! decodebin ! videoconvert ! videoscale ! autovideosink