v4r_add_module(
  DESCRIPTION "Application Libraries"
  REQUIRED v4r_core v4r_common v4r_features v4r_recognition v4r_keypoints v4r_io v4r_registration pcl pcl_1_8 opencv
  OPTIONAL caffe v4r_rendering v4r_change_detection
)
