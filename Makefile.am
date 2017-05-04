## Make autotools quit complaining about explicit rules
AUTOMAKE_OPTIONS = -Wno-override

noinst_LIBRARIES = librcmc.a
librcmc_a_SOURCES = \
  Config.cpp \
  DeclareAssumePass.cpp DeclareAssumePass.hpp \
  Driver.cpp Driver.hpp \
  Error.cpp Error.hpp \
  Event.cpp Event.hpp \
  Execution.cpp \
  ExecutionGraph.cpp ExecutionGraph.hpp \
  ExternalFunctions.cpp \
  Interpreter.cpp Interpreter.h \
  LLVMModule.cpp LLVMModule.hpp \
  LoopUnrollPass.cpp LoopUnrollPass.hpp \
  Parser.cpp Parser.hpp \
  SpinAssumePass.cpp SpinAssumePass.hpp \
  Thread.cpp Thread.hpp \
  vecset.h vecset.tcc

CXX = clang++
AM_CXXFLAGS=`llvm-config --cxxflags` -std=c++14 -Wno-unknown-warning-option 
AM_LDFLAGS =`llvm-config --ldflags --libs --system-libs`

bin_PROGRAMS = rcmc
rcmc_SOURCES = main.cpp
rcmc_LDADD   = librcmc.a
rcmc_LDFLAGS =`llvm-config --ldflags --libs --system-libs`

#%.o: %.cpp %.hpp
#	$(GXX) -c $< -Wno-unknown-warning-option $(GXX_FLAGS)

