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
    GstBuffer *buffer, *inputBuffer;
    buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    inputBuffer = gst_buffer_copy(buffer);

    if (inputBuffer == NULL)
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
        GstMapInfo map;
        if (gst_buffer_map(inputBuffer, &map, GST_MAP_READ))
        {
            // Lưu dữ liệu từ map.data thành tệp JPEG
            FILE *jpeg_file = fopen(filename, "wb");
            if (jpeg_file)
            {
                fwrite(map.data, 1, map.size, jpeg_file);
                fclose(jpeg_file);
            }
            gst_buffer_unmap(inputBuffer, &map);
        }
    }

    return GST_PAD_PROBE_OK;
}

gint main(gint argc,
          gchar *argv[])
{
    GMainLoop *loop;
    GstElement *pipeline, *src, *sink, *csp;
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

    csp = gst_element_factory_make("videoconvert", "csp");
    if (csp == NULL)
        g_error("Could not create 'videoconvert' element");

    sink = gst_element_factory_make("autovideosink", "sink");
    if (sink == NULL)
    {
        sink = gst_element_factory_make("ximagesink", "sink");
        if (sink == NULL)
            g_error("Could not create neither 'xvimagesink' nor 'ximagesink' element");
    }

    GstElement *jpegenc = gst_element_factory_make("jpegenc", "jpegenc");
    GstElement *jpegdec = gst_element_factory_make("jpegdec", "jpegdec");

    gst_bin_add_many(GST_BIN(pipeline), src, jpegenc, jpegdec, csp, sink, NULL);
    gst_element_link_many(src, jpegenc, jpegdec, csp, sink, NULL);

    pad = gst_element_get_static_pad(jpegdec, "sink");
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

// gcc -o task-snapshot task-snapshot.c $(pkg-config --cflags --libs gstreamer-1.0 gstreamer-app-1.0)
//./task-snapshot
