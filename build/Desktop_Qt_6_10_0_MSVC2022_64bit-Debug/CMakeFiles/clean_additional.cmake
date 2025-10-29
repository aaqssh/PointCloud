# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\RollingBallSim_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\RollingBallSim_autogen.dir\\ParseCache.txt"
  "RollingBallSim_autogen"
  )
endif()
