## Make autotools quit complaining about explicit rules
AUTOMAKE_OPTIONS = -Wno-override

noinst_LIBRARIES = librcmc.a
librcmc_a_SOURCES = \
  Config.cpp Config.hpp \
  DeclareAssumePass.cpp DeclareAssumePass.hpp \
  DeclareEndLoopPass.cpp DeclareEndLoopPass.hpp \
  DefineLibcFunsPass.cpp DefineLibcFunsPass.hpp \
  Error.cpp Error.hpp \
  Event.cpp Event.hpp \
  EventLabel.cpp EventLabel.hpp \
  Execution.cpp \
  ExecutionGraph.cpp ExecutionGraph.hpp \
  ExternalFunctions.cpp \
  Interpreter.cpp Interpreter.h \
  IntrinsicLoweringPass.cpp IntrinsicLoweringPass.hpp \
  Library.cpp Library.hpp \
  LLVMModule.cpp LLVMModule.hpp \
  LoopUnrollPass.cpp LoopUnrollPass.hpp \
  ModOrder.cpp ModOrder.hpp \
  Parser.cpp Parser.hpp \
  RCMCDriver.cpp RCMCDriver.hpp \
  RevisitSet.cpp RevisitSet.hpp \
  SpinAssumePass.cpp SpinAssumePass.hpp \
  Thread.cpp Thread.hpp \
  vecset.h vecset.tcc \
  View.hpp View.cpp

CXX = clang++
AM_CXXFLAGS=-std=c++1z -Wno-unknown-warning-option
bin_PROGRAMS = rcmc
rcmc_SOURCES = main.cpp
rcmc_LDADD   = librcmc.a -lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization -lclangCodeGen -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangARCMigrate -lclangRewriteFrontend -lclangEdit -lclangAST -lclangLex -lclangBasic
