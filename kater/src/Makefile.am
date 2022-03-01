AM_YFLAGS = -d
AM_CXXFLAGS = -std=c++14

if kater
  BUILT_SOURCES = Parser.hpp
  bin_PROGRAMS = kater
  kater_SOURCES = main.cpp Driver.cpp Lexer.lpp Parser.ypp NFA.cpp RegExp.cpp TransLabel.cpp
endif
