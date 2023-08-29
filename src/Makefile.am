## Make autotools quit complaining about explicit rules
AUTOMAKE_OPTIONS = -Wno-override

noinst_LIBRARIES = libgenmc.a
libgenmc_a_SOURCES = \
  ASize.cpp ASize.hpp \
  BisimilarityCheckerPass.cpp BisimilarityCheckerPass.hpp \
  Bitmask.hpp \
  CallInfoCollectionPass.cpp CallInfoCollectionPass.hpp \
  CodeCondenserPass.cpp CodeCondenserPass.hpp \
  Calculator.hpp \
  CallInstWrapper.hpp \
  ConfirmationAnnotationPass.cpp ConfirmationAnnotationPass.hpp \
  Config.cpp Config.hpp \
  DeclareInternalsPass.cpp DeclareInternalsPass.hpp \
  DefineLibcFunsPass.cpp DefineLibcFunsPass.hpp \
  DepExecutionGraph.cpp DepExecutionGraph.hpp \
  DepInfo.cpp DepInfo.hpp \
  DepTracker.hpp \
  DepView.cpp DepView.hpp \
  DriverFactory.hpp \
  EliminateAnnotationsPass.cpp EliminateAnnotationsPass.hpp \
  EliminateCASPHIsPass.cpp EliminateCASPHIsPass.hpp \
  EliminateCastsPass.cpp EliminateCastsPass.hpp \
  EliminateRedundantInstPass.cpp EliminateRedundantInstPass.hpp \
  EliminateUnusedCodePass.cpp EliminateUnusedCodePass.hpp \
  Error.hpp \
  EscapeCheckerPass.cpp EscapeCheckerPass.hpp \
  Event.cpp Event.hpp \
  EventAttr.hpp \
  EventLabel.cpp EventLabel.hpp \
  Execution.cpp \
  ExecutionGraph.cpp ExecutionGraph.hpp \
  ExternalFunctions.cpp \
  FunctionInlinerPass.cpp FunctionInlinerPass.hpp \
  GenMCDriver.cpp GenMCDriver.hpp \
  GraphIterators.hpp \
  IMMDriver.cpp IMMDriver.hpp \
  InstAnnotator.cpp InstAnnotator.hpp \
  Interpreter.cpp Interpreter.h \
  InterpreterEnumAPI.cpp InterpreterEnumAPI.hpp \
  IntrinsicLoweringPass.cpp IntrinsicLoweringPass.hpp \
  LabelVisitor.hpp \
  LoadAnnotationPass.cpp LoadAnnotationPass.hpp \
  LocalSimplifyCFGPass.cpp LocalSimplifyCFGPass.hpp \
  Logger.hpp \
  LLVMModule.cpp LLVMModule.hpp \
  LLVMUtils.cpp LLVMUtils.hpp \
  LoopUnrollPass.cpp LoopUnrollPass.hpp \
  LoopJumpThreadingPass.cpp LoopJumpThreadingPass.hpp \
  Matrix2D.hpp Matrix2D.tcc \
  MaximalIterator.hpp \
  MDataCollectionPass.hpp MDataCollectionPass.cpp \
  MemAccess.hpp \
  ModuleID.hpp \
  ModuleInfo.cpp ModuleInfo.hpp \
  NameInfo.cpp NameInfo.hpp \
  Parser.cpp Parser.hpp \
  PromoteMemIntrinsicPass.cpp PromoteMemIntrinsicPass.hpp \
  PropagateAssumesPass.cpp PropagateAssumesPass.hpp \
  RC11Driver.cpp RC11Driver.hpp \
  Revisit.cpp Revisit.hpp \
  RevisitSet.cpp RevisitSet.hpp \
  SAddr.cpp SAddr.hpp \
  SAddrAllocator.hpp \
  SCDriver.cpp SCDriver.hpp \
  SExpr.tcc SExpr.hpp \
  SExprVisitor.tcc SExprVisitor.hpp \
  Stamp.cpp Stamp.hpp \
  SVal.cpp SVal.hpp \
  SpinAssumePass.cpp SpinAssumePass.hpp \
  WBIterator.hpp \
  ThreadInfo.hpp \
  ThreadPinner.cpp ThreadPinner.hpp \
  ThreadPool.cpp ThreadPool.hpp \
  TSODriver.cpp TSODriver.hpp \
  WorkSet.hpp WorkSet.cpp \
  value_ptr.hpp \
  Verbosity.hpp Verbosity.cpp \
  VectorClock.hpp VectorClock.cpp \
  VerificationError.hpp VerificationError.cpp \
  View.hpp View.cpp \
  VSet.hpp VSet.tcc


AM_CXXFLAGS = -DINCLUDE_DIR=\"$(pkgincludedir)/$(pkg)\" -DSRC_INCLUDE_DIR=\"$(abs_top_builddir)/include\"
bin_PROGRAMS = genmc
genmc_SOURCES = main.cpp
genmc_LDADD   = libgenmc.a -lpthread
