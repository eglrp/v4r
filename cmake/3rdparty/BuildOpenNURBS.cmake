v4r_build_external_project("OpenNURBS"
  BUNDLED
  CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DPCL_DIR=${PCL_CONFIG_PATH}"
)
