#the project file for tmake

TEMPLATE  =  app
CONFIG    =  qt warn_on debug
LIBS      =  -lao -ldl
HEADERS   =  testwindow.h graph.h data.h audio.h arrays.h
SOURCES   =  main.cpp testwindow.cpp graph.cpp data.cpp audio.cpp
TARGET    =  mask