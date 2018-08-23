#include <unistd.h>
#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <queue>

#include <pcl/common/time.h>
#include <pcl/conversions.h>
#include <pcl/filters/filter.h>
#include <pcl/io/io.h>
#include <pcl/io/openni_grabber.h>
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include "pcl/common/transforms.h"

#include <v4r/common/convertImage.h>
#include <v4r/keypoints/impl/PoseIO.hpp>
#include <v4r/keypoints/impl/toString.hpp>
#include "v4r/camera_tracking_and_mapping/TSFData.h"
#include "v4r/camera_tracking_and_mapping/TSFGlobalCloudFilteringSimple.h"
#include "v4r/camera_tracking_and_mapping/TSFVisualSLAM.h"
#include "v4r/features/FeatureDetector_KD_FAST_IMGD.h"
#include "v4r/io/filesystem.h"
#include "v4r/keypoints/impl/PoseIO.hpp"
#include "v4r/keypoints/impl/invPose.hpp"
#include "v4r/reconstruction/impl/projectPointToImage.hpp"
#include "v4r/surface/mvs_texturing.h"

using namespace std;

double upscaling = 1;  // 1.25; //1.25 => 640 -> 800, 1.6 => 640 -> 1024
int batch_size_clouds = 20;

//------------------------------ helper methods -----------------------------------
void setup(int argc, char **argv);

void convertImage(const pcl::PointCloud<pcl::PointXYZRGB> &cloud, cv::Mat &image);
void convertImage(const pcl::PointCloud<pcl::PointXYZRGBNormal> &_cloud, cv::Mat &_image);
void drawCoordinateSystem(cv::Mat &im, const Eigen::Matrix4f &pose, const cv::Mat_<double> &intrinsic,
                          const cv::Mat_<double> &dist_coeffs, double size, int thickness);
void drawConfidenceBar(cv::Mat &im, const double &conf, int x_start = 50, int x_end = 200, int y = 30);
void writeKeyframes(const std::vector<v4r::TSFFrame::Ptr> &kf, const std::string &out_dir);

//----------------------------- data containers -----------------------------------
cv::Mat_<cv::Vec3b> image;
cv::Mat_<cv::Vec3b> im_draw;
pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);

std::string in_dir;
std::string out_dir_debug, out_dir_keyframes;
std::string cam_file, filenames, file_mesh, file_cloud;
std::string pattern = std::string(".*.") + std::string("pcd");

cv::Mat_<double> distCoeffs;  // = cv::Mat::zeros(4, 1, CV_64F);
cv::Mat_<double> intrinsic = cv::Mat_<double>::eye(3, 3);
cv::Mat_<double> dist_coeffs_opti = cv::Mat::zeros(4, 1, CV_64F);
cv::Mat_<double> intrinsic_opti = cv::Mat_<double>::eye(3, 3);

Eigen::Matrix4f pose;
float voxel_size = 0.002;     // 0.005;
double thr_weight = 2;        // e.g. 10    // surfel threshold for the final model
double thr_delta_angle = 80;  // e.g. 80
int poisson_depth = 8;        // 6;
int display = true;

cv::Point track_win[2];

/******************************************************************
 * MAIN
 */
int main(int argc, char *argv[]) {
  int sleep = 0;
  // char filename[PATH_MAX];
  bool loop = false;
  Eigen::Matrix4f inv_pose;
  double time0, time1, mean_time = 0;
  int cnt_time = 0;

  setup(argc, argv);

  intrinsic(0, 0) = intrinsic(1, 1) = 525;
  intrinsic(0, 2) = 320, intrinsic(1, 2) = 240;

  if (cam_file.size() > 0) {
    cv::FileStorage fs(cam_file, cv::FileStorage::READ);
    fs["camera_matrix"] >> intrinsic;
    fs["distortion_coefficients"] >> distCoeffs;
  }

  cv::namedWindow("image", CV_WINDOW_AUTOSIZE);

  std::vector<std::pair<Eigen::Matrix4f, int>> all_poses;  //<pose, kf_index>

  pcl::PointCloud<pcl::PointXYZRGBNormal> filt_cloud;
  Eigen::Matrix4f filt_pose;
  double timestamp;
  bool have_pose;

  // configure camera tracking, temporal smothing and mapping
  v4r::TSFVisualSLAM tsf;
  tsf.setCameraParameter(intrinsic, upscaling);

  v4r::TSFVisualSLAM::Parameter param;
  param.map_param.refine_plk = false;
  param.map_param.detect_loops = true;
  param.map_param.ba.depth_error_scale = 100;
  param.map_param.ba.px_error_scale = 1;
  param.filt_param.batch_size_clouds = batch_size_clouds;
  param.diff_cam_distance_map = 0.01;  // 0.2;
  param.diff_delta_angle_map = 1;      // 3;
  param.filt_param.type =
      0;  // 3;        // 0...ori. col., 1..col mean, 2..bilin., 3..bilin col and depth with cut off thr
  param.map_param.nb_tracked_frames = 20;
  param.map_param.ba.optimize_delta_cloud_rgb_pose_global = true;
  param.map_param.ba.optimize_delta_cloud_rgb_pose = false;
  param.map_param.ba.optimize_focal_length = false;     // true;
  param.map_param.ba.optimize_principal_point = false;  // true;
  param.map_param.ba.optimize_radial_k1 = false;
  param.map_param.ba.optimize_radial_k2 = false;
  param.map_param.ba.optimize_radial_k3 = false;
  param.map_param.ba.optimize_tangential_p1 = false;
  param.map_param.ba.optimize_tangential_p2 = false;
  tsf.setParameter(param);

  v4r::FeatureDetector::Ptr detector(new v4r::FeatureDetector_KD_FAST_IMGD());
  tsf.setDetectors(detector, detector);

  std::vector<std::string> cloud_files;
  cloud_files = v4r::io::getFilesInDirectory(in_dir, pattern, false);

  std::sort(cloud_files.begin(), cloud_files.end());

  double conf_ransac_iter = 1;
  double conf_tracked_points = 1;
  double ts_last = 0;

  pcl::PCDReader pcd;
  Eigen::Vector4f origin;
  Eigen::Quaternionf orientation;
  int version;

  // start camera tracking
  for (int i = 0; i < (int)cloud_files.size() || loop; i++) {
    cout << "---------------- FRAME #" << i << " -----------------------" << endl;
    cout << in_dir + std::string("/") + cloud_files[i] << endl;

    pcl::PCLPointCloud2::Ptr cloud2;
    cloud2.reset(new pcl::PCLPointCloud2);
    if (pcd.read(in_dir + std::string(cloud_files[i]), *cloud2, origin, orientation, version) < 0)
      continue;
    pcl::fromPCLPointCloud2(*cloud2, *cloud);
    //        cout<<"cloud: "<<cloud->width<<"x"<<cloud->height<<endl;

    //    if (pcl::io::loadPCDFile(in_dir + std::string(cloud_files[i]), *cloud) == -1)
    //      continue;

    std::size_t found = cloud_files[i].find_last_of('.');
    std::string ts_txt = cloud_files[i].substr(0, found);
    double ts = std::atof(ts_txt.c_str());
    cout << "ts=" << ts << endl;

    convertImage(*cloud, image);
    image.copyTo(im_draw);

    tsf.setDebugImage(im_draw);

    // track
    {
      pcl::ScopeTime t("overall time");
      time0 = t.getTime();
      have_pose = tsf.track(*cloud, ts, pose, conf_ransac_iter, conf_tracked_points);
      time1 = t.getTime();
    }  //-- overall time --

    // ---- END batch filtering ---

    cout << "conf (ransac, tracked points): " << conf_ransac_iter << ", " << conf_tracked_points << endl;
    if (!have_pose)
      cout << "Lost pose!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;

    all_poses.push_back(std::make_pair(pose, -1));
    v4r::invPose(pose, inv_pose);

    // get filtered frame
    tsf.getFilteredCloudNormals(filt_cloud, filt_pose, timestamp);

    // debug save
    if (out_dir_debug.size() != 0 && timestamp != ts_last && filt_cloud.points.size() > 0) {
      if ((int)timestamp >= 0 && (int)timestamp < (int)cloud_files.size()) {
        pcl::io::savePCDFileBinaryCompressed(
            out_dir_debug + std::string("/") + cloud_files[(int)timestamp] + std::string("-filt.pcd"), filt_cloud);
        convertImage(filt_cloud, image);
        cv::imwrite(out_dir_debug + std::string("/") + cloud_files[(int)timestamp] + std::string("-filt.jpg"), image);
        ts_last = timestamp;
      }
    }

    mean_time += (time1 - time0);
    cnt_time++;
    cout << "mean=" << mean_time / double(cnt_time) << "ms (" << 1000. / (mean_time / double(cnt_time)) << "fps)"
         << endl;
    cout << "timestamp (c/f): " << i << "/" << timestamp << endl;

    // debug out draw
    int key = 0;
    if (display) {
      drawConfidenceBar(im_draw, conf_ransac_iter, 50, 200, 30);
      drawConfidenceBar(im_draw, conf_tracked_points, 50, 200, 50);
      cv::imshow("image", im_draw);
      key = cv::waitKey(sleep);
      if (conf_ransac_iter < 0.2)
        cv::waitKey(0);
    } else
      usleep(50000);

    if (((char)key) == 27)
      break;
    if (((char)key) == 'r')
      sleep = 100;
    if (((char)key) == 's')
      sleep = 0;
  }

  // optimize map
  tsf.stop();
  tsf.optimizeMap();

  // tsf.getMap();
  tsf.getCameraParameter(intrinsic_opti, dist_coeffs_opti);

  // store keyframes
  if (out_dir_keyframes.size() != 0) {
    writeKeyframes(tsf.getMap(), out_dir_keyframes);
    cv::FileStorage fs(out_dir_keyframes + std::string("/intrinsics.yml"), cv::FileStorage::WRITE);
    fs << "camera_matrix" << intrinsic_opti;
    fs << "distortion_coefficients" << dist_coeffs_opti;
  }

  // create model in global coordinates
  cout << "Create pointcloud model..." << endl;
  pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr glob_cloud(new pcl::PointCloud<pcl::PointXYZRGBNormal>());
  pcl::PolygonMesh mesh;
  v4r::TSFGlobalCloudFilteringSimple gfilt;
  v4r::TSFGlobalCloudFilteringSimple::Parameter filt_param;
  filt_param.filter_largest_cluster = false;
  filt_param.voxel_size = voxel_size;
  filt_param.thr_weight = thr_weight;
  filt_param.thr_delta_angle = thr_delta_angle;
  filt_param.poisson_depth = poisson_depth;
  gfilt.setParameter(filt_param);

  gfilt.setCameraParameter(intrinsic_opti, dist_coeffs_opti);
  gfilt.getGlobalCloudFiltered(tsf.getMap(), *glob_cloud);

  if (file_cloud.size() > 0)
    pcl::io::savePCDFileBinaryCompressed(file_cloud, *glob_cloud);

  cout << "Create mesh..." << endl;
  gfilt.getMesh(glob_cloud, mesh);

  auto intr = v4r::Intrinsics::fromCameraMatrixAndResolution(intrinsic_opti, cloud->width, cloud->height);
  auto map_frames = tsf.getMap();
  pcl::PointCloud<pcl::PointXYZRGB> im_cloud;
  std::vector<cv::Mat> images(map_frames.size());
  v4r::surface::MVSTexturing::PoseVector poses(map_frames.size());
  for (size_t i = 0; i < map_frames.size(); ++i) {
    v4r::TSFData::convert(map_frames[i]->sf_cloud, im_cloud);
    v4r::convertImage(im_cloud, images[i]);
    poses[i] = map_frames[i]->delta_cloud_rgb_pose * map_frames[i]->pose;
  }

  cout << "Texture mapping..." << endl;
  v4r::surface::MVSTexturing tex;
  tex.mapTextureToMesh(mesh, intr, images, poses, "log/textured_mesh");

  // store resulting files
  if (file_mesh.size() > 0)
    pcl::io::savePLYFile(file_mesh, mesh);

  cout << "Finished!" << endl;

  return 0;
}

/******************************** SOME HELPER METHODS **********************************/

/**
 * setup
 */
void setup(int argc, char **argv) {
  cv::FileStorage fs;
  int c;
  while (1) {
    c = getopt(argc, argv, "i:p:a:o:m:c:d:k:h");
    if (c == -1)
      break;
    switch (c) {
      case 'i':
        in_dir = optarg;
        break;
      case 'p':
        pattern = optarg;
        break;
      case 'a':
        cam_file = optarg;
        break;
      case 'm':
        file_mesh = optarg;
        break;
      case 'c':
        file_cloud = optarg;
        break;
      case 'o':
        out_dir_debug = optarg;
        break;
      case 'd':
        display = std::atoi(optarg);
        break;
      case 'k':
        out_dir_keyframes = optarg;
        break;

      case 'h':
        printf(
            "%s [-f filenames] [-s start_idx] [-e end_idx] [-a cam_file.yml] [-h]\n"
            "   -i input directory\n"
            "   -p input file pattern (\".*.pcd\")\n"
            "   -a camera calibration files (opencv format)\n"
            "   -m mesh file name\n"
            "   -o output directory\n"
            "   -k output director for keyframes (inkl. poses)\n"
            "   -c point cloud file name\n"
            "   -d display results [0/1]\n"
            "   -h help\n",
            argv[0]);
        exit(1);

        break;
    }
  }
}

void convertImage(const pcl::PointCloud<pcl::PointXYZRGB> &_cloud, cv::Mat &_image) {
  _image = cv::Mat_<cv::Vec3b>(_cloud.height, _cloud.width);

  for (unsigned v = 0; v < _cloud.height; v++) {
    for (unsigned u = 0; u < _cloud.width; u++) {
      cv::Vec3b &cv_pt = _image.at<cv::Vec3b>(v, u);
      const pcl::PointXYZRGB &pt = _cloud(u, v);

      cv_pt[2] = pt.r;
      cv_pt[1] = pt.g;
      cv_pt[0] = pt.b;
    }
  }
}

void convertImage(const pcl::PointCloud<pcl::PointXYZRGBNormal> &_cloud, cv::Mat &_image) {
  _image = cv::Mat_<cv::Vec3b>(_cloud.height, _cloud.width);

  for (unsigned v = 0; v < _cloud.height; v++) {
    for (unsigned u = 0; u < _cloud.width; u++) {
      cv::Vec3b &cv_pt = _image.at<cv::Vec3b>(v, u);
      const pcl::PointXYZRGBNormal &pt = _cloud(u, v);

      cv_pt[2] = pt.r;
      cv_pt[1] = pt.g;
      cv_pt[0] = pt.b;
    }
  }
}

void drawCoordinateSystem(cv::Mat &im, const Eigen::Matrix4f &_pose, const cv::Mat_<double> &_intrinsic,
                          const cv::Mat_<double> &dist_coeffs, double size, int thickness) {
  Eigen::Matrix3f R = _pose.topLeftCorner<3, 3>();
  Eigen::Vector3f t = _pose.block<3, 1>(0, 3);

  Eigen::Vector3f pt0 = R * Eigen::Vector3f(0, 0, 0) + t;
  Eigen::Vector3f pt_x = R * Eigen::Vector3f(size, 0, 0) + t;
  Eigen::Vector3f pt_y = R * Eigen::Vector3f(0, size, 0) + t;
  Eigen::Vector3f pt_z = R * Eigen::Vector3f(0, 0, size) + t;

  cv::Point2f im_pt0, im_pt_x, im_pt_y, im_pt_z;

  if (!dist_coeffs.empty()) {
    v4r::projectPointToImage(&pt0[0], &_intrinsic(0), &dist_coeffs(0), &im_pt0.x);
    v4r::projectPointToImage(&pt_x[0], &_intrinsic(0), &dist_coeffs(0), &im_pt_x.x);
    v4r::projectPointToImage(&pt_y[0], &_intrinsic(0), &dist_coeffs(0), &im_pt_y.x);
    v4r::projectPointToImage(&pt_z[0], &_intrinsic(0), &dist_coeffs(0), &im_pt_z.x);
  } else {
    v4r::projectPointToImage(&pt0[0], &_intrinsic(0), &im_pt0.x);
    v4r::projectPointToImage(&pt_x[0], &_intrinsic(0), &im_pt_x.x);
    v4r::projectPointToImage(&pt_y[0], &_intrinsic(0), &im_pt_y.x);
    v4r::projectPointToImage(&pt_z[0], &_intrinsic(0), &im_pt_z.x);
  }

  cv::line(im, im_pt0, im_pt_x, CV_RGB(255, 0, 0), thickness);
  cv::line(im, im_pt0, im_pt_y, CV_RGB(0, 255, 0), thickness);
  cv::line(im, im_pt0, im_pt_z, CV_RGB(0, 0, 255), thickness);
}

/**
 * drawConfidenceBar
 */
void drawConfidenceBar(cv::Mat &im, const double &conf, int x_start, int x_end, int y) {
  int bar_start = x_start, bar_end = x_end;
  int diff = bar_end - bar_start;
  int draw_end = diff * conf;
  double col_scale = (diff > 0 ? 255. / (double)diff : 255.);
  cv::Point2f pt1(0, y);
  cv::Point2f pt2(0, y);
  cv::Vec3b col(0, 0, 0);

  if (draw_end <= 0)
    draw_end = 1;

  for (int i = 0; i < draw_end; i++) {
    col = cv::Vec3b(255 - (i * col_scale), i * col_scale, 0);
    pt1.x = bar_start + i;
    pt2.x = bar_start + i + 1;
    cv::line(im, pt1, pt2, CV_RGB(col[0], col[1], col[2]), 8);
  }
}

/**
 * @brief writeKeyframes
 * @param kf
 * @param out_dir
 */
void writeKeyframes(const std::vector<v4r::TSFFrame::Ptr> &kf, const std::string &out_dir) {
  //  std::string cloud_names = out_dir + "/cloud_%08d.pcd";
  //  std::string image_names = out_dir + "/image_%08d.jpg";
  //  std::string pose_names_cloud = out_dir + "/pose_%08d.txt";
  //  std::string pose_names_im = out_dir + "/pose_image_%08d.txt";

  // char filename[PATH_MAX];
  pcl::PointCloud<pcl::PointXYZRGB> cloud;
  cv::Mat image;
  Eigen::Matrix4f inv_pose;

  for (unsigned i = 0; i < kf.size(); i++) {
    v4r::TSFData::convert(kf[i]->sf_cloud, cloud);
    v4r::TSFData::convert(kf[i]->sf_cloud, image);

    // store cloud
    //    snprintf(filename, PATH_MAX, cloud_names.c_str(), i);
    pcl::io::savePCDFileBinaryCompressed(out_dir + v4r::toString(kf[i]->timestamp, 6) + std::string(".pcd"), cloud);

    // store image
    //    snprintf(filename, PATH_MAX, image_names.c_str(), i);
    cv::imwrite(out_dir + v4r::toString(kf[i]->timestamp, 6) + std::string(".jpg"), image);

    // store poses
    v4r::invPose(kf[i]->pose, inv_pose);
    //    snprintf(filename, PATH_MAX, pose_names_cloud.c_str(), i);
    v4r::writePose(out_dir + v4r::toString(kf[i]->timestamp, 6) + std::string("-pose.txt"), std::string(), inv_pose);

    v4r::invPose(kf[i]->delta_cloud_rgb_pose * kf[i]->pose, inv_pose);
    //    snprintf(filename, PATH_MAX, pose_names_im.c_str(), i);
    v4r::writePose(out_dir + v4r::toString(kf[i]->timestamp, 6) + std::string("-pose_im.txt"), std::string(), inv_pose);
  }
}
