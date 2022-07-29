AM_YFLAGS = -d -Wno-yacc
AM_CXXFLAGS = -std=c++17

if kater
  BUILT_SOURCES = Parser.hpp
  bin_PROGRAMS = kater
  kater_SOURCES = \
	main.cpp \
	Config.cpp \
	Constraint.cpp \
	Counterexample.cpp \
	Error.cpp \
	Kater.cpp \
	KatModule.cpp \
	Lexer.lpp \
	NFA.cpp	\
	Parser.ypp \
	ParsingDriver.cpp \
	Predicate.cpp \
	Printer.cpp \
	Relation.cpp \
	RegExp.cpp \
	Saturation.cpp \
	TransLabel.cpp
endif
