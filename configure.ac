AC_INIT([rcmc], [0.1], [kostis@it.uu.se])

m4_include([m4/ax_llvm.m4])
m4_include([m4/ax_clang.m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CXX
AC_PROG_RANLIB
AM_PROG_AR

AC_LANG([C++])

AX_LLVM(,[AC_MSG_FAILURE(LLVM is required.)])
AX_CLANG(,[
  AC_MSG_WARN([rcmc cannot be used without clang/clang++.])
  AC_MSG_WARN([Consider indicating the binaries clang/clang++ with switches --with-clang/--with-clangxx,])
  AC_MSG_FAILURE([Failed to detect clang/clang++.])
])

## XXX: List below is currently INCOMPLETE
AC_CHECK_HEADERS([\
  stdlib.h \
  llvm/ExecutionEngine/ExecutionEngine.h \
  llvm/ExecutionEngine/GenericValue.h \
  llvm/Pass.h \
  llvm/Support/ErrorHandling.h
],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])

## Checks for header files in alternative locations
AC_DEFUN([AC_CHECK_HEADERS_ALT],
[
  ac_check_headers_alt_ok="no"
  AC_CHECK_HEADERS([$1],[ac_check_headers_alt_ok="yes"],[],[$4])
  if test "x$ac_check_headers_alt_ok" = "xyes"; then
    $2
    :
  else
    $3
    :
  fi
])

AC_CHECK_HEADERS_ALT([llvm/DataLayout.h llvm/IR/DataLayout.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/DerivedTypes.h llvm/IR/DerivedTypes.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Function.h llvm/IR/Function.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/InstVisitor.h llvm/IR/InstVisitor.h llvm/Support/InstVisitor.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Module.h llvm/IR/Module.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/LLVMContext.h llvm/IR/LLVMContext.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/PassManager.h llvm/IR/PassManager.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])

AC_CHECK_HEADERS([ffi/ffi.h],[],[
  AC_CHECK_HEADERS([ffi.h],[],[AC_MSG_FAILURE([Could not find header ffi.h.])],[AC_INCLUDES_DEFAULT])
],[AC_INCLUDES_DEFAULT])


# Check various functions in LLVM

## Check which method in llvm::Module returns the data layout string
AC_MSG_CHECKING([for llvm::Module::getDataLayoutStr])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#elif defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#endif
#if defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#elif defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#endif
]],[[
          llvm::Module *M = 0;
          std::string s = M->getDataLayoutStr();
        ]])],
        [AC_DEFINE([LLVM_MODULE_GET_DATA_LAYOUT_STRING],[getDataLayoutStr],
            [The name of the method in llvm::Module which returns the data layout string.])
         AC_MSG_RESULT([getDataLayoutStr])
        ],
        [AC_DEFINE([LLVM_MODULE_GET_DATA_LAYOUT_STRING],[getDataLayout],
            [The name of the method in llvm::Module which returns the data layout string.])
         AC_MSG_RESULT([getDataLayout])
        ])

## Check the signature of llvm::Module::materializeAllPermanently
AC_MSG_CHECKING([for llvm::Module::MaterializeAllPermanently])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#elif defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#endif
#if defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#elif defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#endif
]],[[
  llvm::Module *M = 0;
  std::error_code ec = M->materializeAllPermanently();
]])],
           [AC_DEFINE([LLVM_MODULE_MATERIALIZE_ALL_PERMANENTLY_ERRORCODE_BOOL],[1],
            [Define if Module::materializeAllPermanently accepts zero arguments and returns an error_code.])
            AC_MSG_RESULT([(bool) -> error_code])
           ],
           [
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#elif defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#endif
]],[[
  llvm::Module *M = 0;
  std::string *s = 0;
  bool b = M->MaterializeAllPermanently(s);
]])],
           [AC_DEFINE([LLVM_MODULE_MATERIALIZE_ALL_PERMANENTLY_BOOL_STRPTR],[1],
            [Define if Module::MaterializeAllPermanenty has signature std::string* -> bool])
            AC_MSG_RESULT([string* -> bool])],
           [AC_MSG_RESULT([use materializeAll instead])
	    ## Check the signature of llvm::Module::materializeAll
	    AC_MSG_CHECKING([the signature of llvm::Module::materializeAll])
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/Error.h>
#if defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#elif defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#endif
]],[[
  llvm::Module *M = 0;
  llvm::Error Err = M->materializeAll();
]])],
           [AC_DEFINE([LLVM_MODULE_MATERIALIZE_LLVM_ALL_ERROR],[1],
            [Define if Module::materializeAll has signature () -> llvm::Error])
            AC_MSG_RESULT([() -> llvm::Error])],
           [AC_MSG_RESULT([() -> llvm::error_code])])
])])

## Check if the value F_None in llvm::sys::fs::OpenFlags exists
AC_MSG_CHECKING([for llvm::sys::fs::OpenFlags (F_None)])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/FileSystem.h>
]],[[
  llvm::sys::fs::OpenFlags f = llvm::sys::fs::F_None;
  return f;
]])],
           [AC_DEFINE([HAVE_LLVM_SYS_FS_OPENFLAGS],[1],
            [Define if the type llvm::sys::fs::OpenFlags exists.])
            AC_MSG_RESULT([yes])],
           [AC_MSG_RESULT([no])])

## Check the type of the error reporting argument for llvm::raw_fd_ostream
AC_MSG_CHECKING([for error reporting in llvm::raw_fd_ostream])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
]],[[
  std::string errs;
#ifdef HAVE_LLVM_SYS_FS_OPENFLAGS
  llvm::raw_fd_ostream("foo",errs,llvm::sys::fs::F_None);
#else
  llvm::raw_fd_ostream("foo",errs,0);
#endif
]])],
        [AC_DEFINE([LLVM_RAW_FD_OSTREAM_ERR_STR],[1],
         [Define if llvm::raw_fd_ostream reports errors through a string reference.])
         AC_MSG_RESULT([string])],
        [AC_MSG_RESULT([error_code])])

## Check whether an ExecutionEngine takes a Module pointer or a Module unique_ptr
AC_MSG_CHECKING([for unique_ptr<Module> in ExecutionEngine])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/ExecutionEngine/ExecutionEngine.h>
]],[[
  class EE : public llvm::ExecutionEngine {
  public:
    EE(llvm::Module *M) : llvm::ExecutionEngine(std::unique_ptr<llvm::Module>(M)) {};
  };
]])],
        [AC_DEFINE([LLVM_EXECUTIONENGINE_MODULE_UNIQUE_PTR],[1],
         [Define if llvm::ExecutionEngine takes a unique_ptr to its Module.])
         AC_MSG_RESULT([yes])],
       	[AC_MSG_RESULT([no])])

## Check if Mutexes use the method "lock" or "acquire"
AC_MSG_CHECKING([for llvm::sys::Mutex::lock])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/Mutex.h>
]],[[
  llvm::sys::Mutex m;
  m.lock();
]])],
        [AC_DEFINE([LLVM_SYS_MUTEX_LOCK_FN],[lock],
         [The name of the locking method of llvm::sys::mutex.])
         AC_DEFINE([LLVM_SYS_MUTEX_UNLOCK_FN],[unlock],
         [The name of the unlocking method of llvm::sys::mutex.])
         AC_MSG_RESULT([lock])],
        [AC_DEFINE([LLVM_SYS_MUTEX_LOCK_FN],[acquire],
         [The name of the locking method of llvm::sys::mutex.])
         AC_DEFINE([LLVM_SYS_MUTEX_UNLOCK_FN],[release],
         [The name of the unlocking method of llvm::sys::mutex.])
         AC_MSG_RESULT([acquire])])

## Check the signature of llvm::ParseIR
AC_MSG_CHECKING([for llvm::ParseIR])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#if defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#elif defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#endif
]],[[
  llvm::SMDiagnostic err;
  llvm::LLVMContext ctx;
  llvm::ParseIR((llvm::MemoryBuffer*)0,
                err,
                ctx);
]])],
        [AC_DEFINE([LLVM_PARSE_IR_MEMBUF_PTR],[1],
         [Define if llvm::ParseIR takes a MemoryBuffer pointer as argument.])
         AC_MSG_RESULT([takes MemoryBuffer*])],
        [AC_MSG_RESULT([takes MemoryBufferRef])])

## Check the return type of llvm::MemoryBuffer::getMemBuffer
AC_MSG_CHECKING([for return type of llvm::MemoryBuffer::getMemBuffer])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/MemoryBuffer.h>
]],[[
  llvm::MemoryBuffer *b =
    llvm::MemoryBuffer::getMemBuffer("");
]])],
        [AC_DEFINE([LLVM_GETMEMBUFFER_RET_PTR],[1],
         [Define if llvm::MemoryBuffer::getMemBuffer returns a pointer.])
         AC_MSG_RESULT([llvm::MemoryBuffer*])],
        [AC_MSG_RESULT([std::unique_ptr<llvm::MemoryBuffer>])])

## Check the type of list_type
AC_MSG_CHECKING([for the type of list_type])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Job.h>
]],[[
   std::unique_ptr<clang::driver::Compilation> C;
   const clang::driver::JobList &Jobs = C->getJobs();
   const clang::driver::Command &Cmd =
     *llvm::cast<clang::driver::Command>(*Jobs.begin());
 ]])],
        [AC_DEFINE([CLANG_LIST_TYPE_JOB_PTR],[1],
         [Define if list_type is a SmallVector of Job*.])
         AC_MSG_RESULT([Job*])],
        [AC_MSG_RESULT([std::unique_ptr<Command>])])

## Check how to call getAnalysis to get LoopInfo
AC_MSG_CHECKING([how to get LoopInfo analysis])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Analysis/LoopPass.h>
]],[[
  llvm::LPPassManager *LPM = 0;
  LPM->getAnalysis<llvm::LoopInfo>();
]])],
        [AC_MSG_RESULT([llvm::LoopInfo])
         AC_DEFINE([LLVM_GET_ANALYSIS_LOOP_INFO],[1],
         [Define if LPPassManager::getAnalysis<LoopInfo> can be called.])],
        [AC_MSG_RESULT([llvm::LoopInfoWrapperPass])])

## Check if llvm::cl::getRegisteredOptions takes an argument
AC_MSG_CHECKING([if llvm::cl::getRegisteredOptions takes an argument.])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/CommandLine.h>
]],[[
  llvm::StringMap<llvm::cl::Option*> opts;
  llvm::cl::getRegisteredOptions(opts);
]])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([LLVM_CL_GETREGISTEREDOPTIONS_TAKES_ARGUMENT],[1],
         [Define if llvm::cl::getRegisteredOptions takes an argument.])],
        [AC_MSG_RESULT([no])])

## Check the parameter type for ExecutionEngine::runFunction
AC_MSG_CHECKING([for parameter type for ExecutionEngine::runFunction])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
]],[[
  class EE : public llvm::ExecutionEngine{
  public:
    virtual llvm::GenericValue runFunction(llvm::Function*, const std::vector<llvm::GenericValue>&) override {
      return llvm::GenericValue();
    };
  };
]])],
        [AC_MSG_RESULT([std::vector<llvm::GenericValue>])
         AC_DEFINE([LLVM_EXECUTION_ENGINE_RUN_FUNCTION_VECTOR],[1],
         [Define if ExecutionEngine::runFunction takes a std::vector<llvm::GenericValue> argument.])],
        [AC_MSG_RESULT([llvm::ArrayRef<llvm::GenericValue>])])

## Check if llvm::PassManager requires a template argument
AC_MSG_CHECKING([if llvm::PassManager requires a template argument])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_PASSMANAGER_H)
#include <llvm/PassManager.h>
#elif defined(HAVE_LLVM_IR_PASSMANAGER_H)
#include <llvm/IR/PassManager.h>
#endif
]],[[
  llvm::PassManager PM;
]])],
        [AC_MSG_RESULT([no])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([LLVM_PASSMANAGER_TEMPLATE],[1],
         [Define if llvm::PassManager requires a template argument.])
         AC_CHECK_HEADERS([llvm/IR/LegacyPassManager.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])])

## Check if the datalayout of an execution engine is a pointer
AC_MSG_CHECKING([whether DataLayout of ExecutionEngine is a pointer])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/ExecutionEngine/ExecutionEngine.h>
]],[[
  llvm::ExecutionEngine *E = 0;
  const llvm::DataLayout *DL = E->getDataLayout();
]])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([LLVM_EXECUTIONENGINE_DATALAYOUT_PTR],[1],
   [Define if the DataLayout field of ExecutionEngine is a pointer.])],
  [AC_MSG_RESULT([no])])

## Check if llvm::LoopInfo::markAsRemoved exists
AC_MSG_CHECKING([for llvm::LoopInfo::markAsRemoved])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Analysis/LoopPass.h>
]],[[
  llvm::Loop * L = 0;
  llvm::LoopInfo *LI = 0;
  LI->markAsRemoved(L);
]])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([HAVE_LLVM_LOOPINFO_MARK_AS_REMOVED],[1],
   [Define if llvm::LoopInfo::markAsRemoved exists])],
  [AC_MSG_RESULT([no])])

## Check if initializeIPA exists
AC_MSG_CHECKING([for llvm::initializeIPA])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_ANALYSIS_VERIFIER_H)
#include <llvm/Analysis/Verifier.h>
#elif defined(HAVE_LLVM_IR_VERIFIER_H)
#include <llvm/IR/Verifier.h>
#endif
#if defined(HAVE_LLVM_PASSMANAGER_H)
#include <llvm/PassManager.h>
#elif defined(HAVE_LLVM_IR_PASSMANAGER_H)
#include <llvm/IR/PassManager.h>
#endif
#if defined(HAVE_LLVM_IR_LEGACYPASSMANAGER_H) && defined(LLVM_PASSMANAGER_TEMPLATE)
#include <llvm/IR/LegacyPassManager.h>
#endif
]],[[
  void (*f)(llvm::PassRegistry&) = llvm::initializeIPA;
]])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([HAVE_LLVM_INITIALIZE_IPA],[1],
   [Define if llvm::initializeIPA exists])],
  [AC_MSG_RESULT([no])])


AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
