project(intercharts)

HLCM_Library(${PROJECT_NAME} INTERFACE
  SOURCES chart.cpp bar_chart.cpp
  PUBLIC_HEADERS chart.h bar_chart.h set.h
)