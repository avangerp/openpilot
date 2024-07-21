#pragma once

#include <memory>
#include <utility>

#include "system/camerad/cameras/camera_common.h"
#include "system/camerad/cameras/camera_util.h"
#include "system/camerad/sensors/sensor.h"
#include "common/params.h"
#include "common/util.h"

#define FRAME_BUF_COUNT 4

struct CameraConfig {
  int camera_num;
  VisionStreamType stream_type;
  float focal_len;  // millimeters
  bool enabled;
};

const CameraConfig WIDE_ROAD_CAMERA_CONFIG = {
  .camera_num = 0,
  .stream_type = VISION_STREAM_WIDE_ROAD,
  .focal_len = 1.71,
  .enabled = !getenv("DISABLE_WIDE_ROAD"),
};

const CameraConfig ROAD_CAMERA_CONFIG = {
  .camera_num = 1,
  .stream_type = VISION_STREAM_ROAD,
  .focal_len = 8.0,
  .enabled = !getenv("DISABLE_ROAD"),
};

const CameraConfig DRIVER_CAMERA_CONFIG = {
  .camera_num = 2,
  .stream_type = VISION_STREAM_DRIVER,
  .focal_len = 1.71,
  .enabled = !getenv("DISABLE_DRIVER"),
};

class CameraState {
public:
  MultiCameraState *multi_cam_state;
  std::unique_ptr<const SensorInfo> ci;
  bool enabled;
  VisionStreamType stream_type;
  float focal_len;

  std::mutex exp_lock;

  int exposure_time;
  bool dc_gain_enabled;
  int dc_gain_weight;
  int gain_idx;
  float analog_gain_frac;

  float cur_ev[3];
  float best_ev_score;
  int new_exp_g;
  int new_exp_t;

  Rect ae_xywh;
  float measured_grey_fraction;
  float target_grey_fraction;

  unique_fd sensor_fd;
  unique_fd csiphy_fd;

  int camera_num;
  float fl_pix;

  CameraState(MultiCameraState *multi_camera_state, const CameraConfig &config);
  void handle_camera_event(void *evdat);
  void update_exposure_score(float desired_ev, int exp_t, int exp_g_idx, float exp_gain);
  void set_camera_exposure(float grey_frac);

  void sensors_start();

  void camera_open();
  void set_exposure_rect();
  void sensor_set_parameters();
  void camera_map_bufs();
  void camera_init(VisionIpcServer *v, cl_device_id device_id, cl_context ctx);
  void camera_close();

  int32_t session_handle;
  int32_t sensor_dev_handle;
  int32_t isp_dev_handle;
  int32_t csiphy_dev_handle;

  int32_t link_handle;

  int buf0_handle;
  int buf_handle[FRAME_BUF_COUNT];
  int sync_objs[FRAME_BUF_COUNT];
  int request_ids[FRAME_BUF_COUNT];
  int request_id_last;
  int frame_id_last;
  int idx_offset;
  bool skipped;

  CameraBuf buf;
  MemoryManager mm;

  void config_isp(int io_mem_handle, int fence, int request_id, int buf0_mem_handle, int buf0_offset);
  void enqueue_req_multi(int start, int n, bool dp);
  void enqueue_buffer(int i, bool dp);
  int clear_req_queue();

  int sensors_init();
  void sensors_poke(int request_id);
  void sensors_i2c(const struct i2c_random_wr_payload* dat, int len, int op_code, bool data_word);

private:
  bool openSensor();
  void configISP();
  void configCSIPHY();
  void linkDevices();

  // for debugging
  Params params;
};

class MultiCameraState {
public:
  MultiCameraState();

  unique_fd video0_fd;
  unique_fd cam_sync_fd;
  unique_fd isp_fd;
  int device_iommu;
  int cdm_iommu;

  CameraState road_cam;
  CameraState wide_road_cam;
  CameraState driver_cam;

  PubMaster *pm;
};
