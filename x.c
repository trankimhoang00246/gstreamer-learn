#include <gst/gst.h>

int main(int argc, char *argv[])
{
    // Khởi tạo GStreamer
    gst_init(&argc, &argv);

    // Tạo đường ống
    GstElement *pipeline, *filesrc, *qtdemux, *h264parse, *avdec_h264, *videoconvert, *autovideosink;
    pipeline = gst_pipeline_new("my-pipeline");
    filesrc = gst_element_factory_make("filesrc", "file-source");
    qtdemux = gst_element_factory_make("qtdemux", "qtdemux");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    avdec_h264 = gst_element_factory_make("avdec_h264", "avdec_h264");
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    autovideosink = gst_element_factory_make("autovideosink", "autovideosink");

    // Kiểm tra xem các phần tử đã tạo thành công hay không
    if (!pipeline || !filesrc || !qtdemux || !h264parse || !avdec_h264 || !videoconvert || !autovideosink)
    {
        g_printerr("One or more elements could not be created. Exiting.\n");
        return -1;
    }

    // Thiết lập địa chỉ tệp nguồn
    g_object_set(G_OBJECT(filesrc), "location", "/home/hoang/Public/PC-HOANG/ThucTap/OTS/AI/GSTBuffer-Snapshot/TID.mp4", NULL);

    // Thêm các phần tử vào đường ống
    gst_bin_add_many(GST_BIN(pipeline), filesrc, qtdemux, h264parse, avdec_h264, videoconvert, autovideosink, NULL);

    // Liên kết các phần tử trong đường ống
    if (!gst_element_link(filesrc, qtdemux))
    {
        g_printerr("Elements could not be linked. Exiting.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    else if (!gst_element_link(qtdemux, h264parse))
    {
        g_printerr("Elements could not be linked. Exiting. 1\n");
        gst_object_unref(pipeline);
        return -1;
    }
    else if (!gst_element_link(h264parse, avdec_h264))
    {
        g_printerr("Elements could not be linked. Exiting. 2\n");
        gst_object_unref(pipeline);
        return -1;
    }
    else if (!gst_element_link(avdec_h264, videoconvert))
    {
        g_printerr("Elements could not be linked. Exiting. 3\n");
        gst_object_unref(pipeline);
        return -1;
    }
    else if (!gst_element_link(videoconvert, autovideosink))
    {
        g_printerr("Elements could not be linked. Exiting. 4\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Đặt trạng thái của đường ống thành PLAYING
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Tạo Main Loop để duy trì chạy ứng dụng
    GMainLoop *loop;
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Dọn dẹp tài nguyên khi kết thúc
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return 0;
}
