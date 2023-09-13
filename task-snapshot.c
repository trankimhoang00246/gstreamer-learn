#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>

// Biến đánh dấu sự kiện Ctrl + t
gboolean ctrl_t_pressed = FALSE;

// Hàm xử lý sự kiện từ bàn phím
gboolean key_event_handler(GIOChannel *source, GIOCondition condition, gpointer data)
{
    gchar c;
    GIOStatus status;

    status = g_io_channel_read_chars(source, &c, 1, NULL, NULL);

    if (status == G_IO_STATUS_NORMAL)
    {
        if (c == '\x14')
        { // Ctrl + t
            ctrl_t_pressed = TRUE;
        }
    }

    return TRUE;
}

static GstPadProbeReturn
cb_have_data(GstPad *pad,
             GstPadProbeInfo *info,
             gpointer user_data)
{
    GstBuffer *buffer, *copied_buffer;
    buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    copied_buffer = gst_buffer_copy(buffer);
    if (copied_buffer == NULL)
    {
        g_print("Failed to create a copy of the buffer.\n");
        return GST_PAD_PROBE_OK;
    }

    if (ctrl_t_pressed && buffer)
    {
        ctrl_t_pressed = FALSE;

        time_t rawtime;
        struct tm *timeinfo;
        char timestamp[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        // Định tên dạng thời gian và là duy nhất
        strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", timeinfo);
        gchar filename[100]; // Dùng để lưu tên tệp
        g_snprintf(filename, sizeof(filename), "snapshot_%s.jpg", timestamp);

        //----------------------------------------------------------------------------------------------
        GstElement *my_pipeline, *appsrc, *jpegenc, *filesink;

        my_pipeline = gst_pipeline_new("my-pipeline");
        appsrc = gst_element_factory_make("appsrc", "app-source");
        jpegenc = gst_element_factory_make("jpegenc", "jpegenc");
        filesink = gst_element_factory_make("filesink", "filesink");

        if (!my_pipeline || !appsrc || !jpegenc || !filesink)
        {
            g_printerr("One or more elements could not be created. Exiting.\n");
            return FALSE;
        }

        g_object_set(G_OBJECT(filesink), "location", filename, NULL);
        gst_bin_add_many(GST_BIN(my_pipeline), appsrc, jpegenc, filesink, NULL);
        if (!gst_element_link_many(appsrc, jpegenc, filesink, NULL))
        {
            g_printerr("Elements could not be linked. Exiting.\n");
            gst_object_unref(my_pipeline);
            return FALSE;
        }

        gst_element_set_state(my_pipeline, GST_STATE_PLAYING);

        gboolean ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), copied_buffer);

        if (ret == GST_FLOW_OK)
        {
            g_print("Pushed buffer successfully.\n");
        }
        else
        {
            g_print("Pushed buffer with an error: %s\n", gst_flow_get_name(ret));
        }

        gst_element_set_state(my_pipeline, GST_STATE_NULL);
        gst_object_unref(my_pipeline);
        gst_buffer_unref(copied_buffer);
    }

    return GST_PAD_PROBE_OK;
}

gint main(gint argc,
          gchar *argv[])
{
    GMainLoop *loop;
    GstElement *pipeline, *src, *sink, *filter, *csp;
    GstCaps *filtercaps;
    GstPad *pad;

    /* init GStreamer */
    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    // Sử dụng GIOChannel để theo dõi sự kiện từ bàn phím
    GIOChannel *io_stdin = g_io_channel_unix_new(fileno(stdin));
    g_io_add_watch(io_stdin, G_IO_IN, key_event_handler, NULL);

    /* build */
    pipeline = gst_pipeline_new("pipeline");
    src = gst_element_factory_make("videotestsrc", "src");
    if (src == NULL)
        g_error("Could not create 'videotestsrc' element");

    filter = gst_element_factory_make("capsfilter", "filter");
    g_assert(filter != NULL); /* should always exist */

    csp = gst_element_factory_make("videoconvert", "csp");
    if (csp == NULL)
        g_error("Could not create 'videoconvert' element");

    sink = gst_element_factory_make("xvimagesink", "sink");
    if (sink == NULL)
    {
        sink = gst_element_factory_make("ximagesink", "sink");
        if (sink == NULL)
            g_error("Could not create neither 'xvimagesink' nor 'ximagesink' element");
    }

    gst_bin_add_many(GST_BIN(pipeline), src, filter, csp, sink, NULL);
    gst_element_link_many(src, filter, csp, sink, NULL);
    filtercaps = gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, "RGB16",
                                     "width", G_TYPE_INT, 384,
                                     "height", G_TYPE_INT, 288,
                                     "framerate", GST_TYPE_FRACTION, 25, 1,
                                     NULL);
    g_object_set(G_OBJECT(filter), "caps", filtercaps, NULL);
    gst_caps_unref(filtercaps);

    pad = gst_element_get_static_pad(sink, "sink");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER,
                      (GstPadProbeCallback)cb_have_data, NULL, NULL);
    gst_object_unref(pad);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    if (gst_element_get_state(pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE)
    {
        g_error("Failed to go into PLAYING state");
    }

    g_print("Running ...\n");
    g_main_loop_run(loop);

    /* exit */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}