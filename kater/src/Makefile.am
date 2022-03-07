AM_YFLAGS = -d
AM_CXXFLAGS = -std=c++17

if kater
  BUILT_SOURCES = Parser.hpp
  bin_PROGRAMS = kater
  kater_SOURCES = \
	main.cpp \
	Config.cpp \
	Driver.cpp \
	Lexer.lpp \
	NFA.cpp	\
	Parser.ypp \
	RegExp.cpp \
	TransLabel.cpp
endif
