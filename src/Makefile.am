## Make autotools quit complaining about explicit rules
AUTOMAKE_OPTIONS = -Wno-override

noinst_LIBRARIES = libgenmc.a
libgenmc_a_SOURCES = \
  Config.cpp Config.hpp \
  DeclareAssumePass.cpp DeclareAssumePass.hpp \
  DeclareEndLoopPass.cpp DeclareEndLoopPass.hpp \
  DefineLibcFunsPass.cpp DefineLibcFunsPass.hpp \
  DriverFactory.cpp DriverFactory.hpp \
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
  GenMCDriver.cpp GenMCDriver.hpp \
  RC11WeakRADriver.cpp RC11WeakRADriver.hpp \
  RC11WBDriver.cpp RC11WBDriver.hpp \
  RC11MODriver.cpp RC11MODriver.hpp \
  SpinAssumePass.cpp SpinAssumePass.hpp \
  VSet.hpp VSet.tcc \
  View.hpp View.cpp

CXX = clang++
AM_CXXFLAGS=-std=c++1z -Wno-unknown-warning-option
bin_PROGRAMS = genmc
genmc_SOURCES = main.cpp
genmc_LDADD   = libgenmc.a -lclangCodeGen -lclangFrontend -lclangFrontendTool -lclangDriver -lclangSerialization -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangARCMigrate -lclangRewriteFrontend -lclangEdit -lclangAST -lclangLex -lclangBasic
