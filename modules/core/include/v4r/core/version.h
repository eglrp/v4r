#ifndef __V4R_VERSION_HPP__
#define __V4R_VERSION_HPP__

#define V4R_VERSION_MAJOR 2
#define V4R_VERSION_MINOR 0
#define V4R_VERSION_REVISION 3
#define V4R_VERSION_STATUS ""

#define V4RAUX_STR_EXP(__A) #__A
#define V4RAUX_STR(__A) V4RAUX_STR_EXP(__A)

#define V4RAUX_STRW_EXP(__A) L#__A
#define V4RAUX_STRW(__A) V4RAUX_STRW_EXP(__A)

#define V4R_VERSION             \
  V4RAUX_STR(V4R_VERSION_MAJOR) \
  "." V4RAUX_STR(V4R_VERSION_MINOR) "." V4RAUX_STR(V4R_VERSION_REVISION) V4R_VERSION_STATUS

#endif
