AC_INIT([GenMC], [0.8], [michalis@mpi-sws.org], [genmc], [https://plv.mpi-sws.org/genmc])

m4_include([m4/ax_llvm.m4])
m4_include([m4/ax_clang.m4])
m4_include([m4/ax_git_commit.m4])
m4_include([m4/ax_cxx_compile_stdcxx.m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AM_SILENT_RULES([yes])

AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([14], [noext], [mandatory])

AC_PROG_RANLIB
AM_PROG_AR

AC_LANG([C++])

AX_LLVM(,[AC_MSG_FAILURE(LLVM is required.)])
AX_CLANG(,[
  AC_MSG_WARN([genmc cannot be used without clang/clang++.])
  AC_MSG_WARN([Consider indicating the binaries clang/clang++ with switches --with-clang/--with-clangxx,])
  AC_MSG_FAILURE([Failed to detect clang/clang++.])
])
AX_GIT_COMMIT

## Check for libraries
AC_CHECK_LIB([dl], [dlopen],[],[AC_MSG_FAILURE([Could not find library libdl.])])
AC_CHECK_LIB([ffi], [ffi_call],[],[AC_MSG_FAILURE([Could not find library libffi.])])
AC_CHECK_LIB([hwloc], [hwloc_topology_init], [], [])

## Check for (proper) C++11 stdlibc++ support
AC_MSG_CHECKING([whether enum class can be used as unordered_map<> key])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <unordered_map>
]],[[
	enum class Foo {};

	std::unordered_map<Foo, int> bar;
 ]])],
        [AC_DEFINE([STDLIBCPP_SUPPORTS_ENUM_MAP_KEYS],[1],
         [Define if stdlibc++ supports enum class as keys to unordered_map<>.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check for LLVM headers
## XXX: List below is currently INCOMPLETE
AC_CHECK_HEADERS([\
  stdlib.h \
  llvm/ExecutionEngine/ExecutionEngine.h \
  llvm/ExecutionEngine/GenericValue.h \
  llvm/Pass.h \
  llvm/IR/LegacyPassManager.h \
  llvm/Support/ErrorHandling.h
],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])

## Checks for headers in alternative locations (useful for LLVM headers)
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

AC_CHECK_HEADERS_ALT([llvm/Transforms/Utils.h llvm/Transforms/Scalar.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS([ffi/ffi.h],[],[
  AC_CHECK_HEADERS([ffi.h],[],[AC_MSG_FAILURE([Could not find header ffi.h.])],[AC_INCLUDES_DEFAULT])
],[AC_INCLUDES_DEFAULT])

# Check various functions in LLVM

## Check how to create unique_ptrs<>
AC_MSG_CHECKING([how to create unique_ptrs<>])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/ADT/STLExtras.h>
]],[[
	auto up = llvm::make_unique<int>(42);
 ]])],
        [AC_DEFINE([LLVM_MAKE_UNIQUE],[llvm::make_unique],
         [How to create unique_ptrs<>.])
         AC_MSG_RESULT([llvm::make_unique<>])],
        [AC_DEFINE([LLVM_MAKE_UNIQUE],[std::make_unique],
         [How to create unique_ptrs<>.])
	AC_MSG_RESULT([std::make_unique<>])
	])

## Check whether llvm::mayBeMemoryDependent() exists (< 15)
AC_MSG_CHECKING([whether llvm::mayBeMemoryDependent() exists])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Instruction.h>
]],[[
	llvm::Instruction *i = nullptr;
	llvm::mayBeMemoryDependent(*i);
]])],
        [AC_DEFINE([LLVM_HAS_MAY_BE_MEMORY_DEPENDENT],[1],
         [Define if llvm::mayBeMemoryDependent() exists.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check whether llvm::WriteBitcodeToFile takes a pointer (>= 7)
AC_MSG_CHECKING([whether llvm::WriteBitcodeToFile takes a pointer])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <string>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Bitcode/BitcodeWriter.h>
]],[[
	std::string str;
	llvm::raw_string_ostream  stream(str);
	llvm::WriteBitcodeToFile(nullptr, stream);
]])],
        [AC_DEFINE([LLVM_WRITE_BITCODE_TO_FILE_PTR],[1],
         [Define if llvm::WriteBitcodeToFile takes a pointer.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check whether llvm::LoadInst constructor needs a type argument (>= 11)
AC_MSG_CHECKING([whether llvm::LoadInst constructor needs a type argument])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Instructions.h>
]],[[
  llvm::Value *val = nullptr;
  llvm::BasicBlock *bb = nullptr;
  llvm::LoadInst i(val, "value", bb);
]])],
        [AC_DEFINE([LLVM_LOADINST_VALUE_ONLY],[1],
         [Define if llvm::LoadInst constructor does not need a type argument.])
         AC_MSG_RESULT([no])],
        [AC_MSG_RESULT([yes])])

## Check whether llvm::Align has been introduced (>= 11)
AC_MSG_CHECKING([whether llvm::Align has been introduced])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/Alignment.h>
]],[[
  llvm::Align a;
]])],
        [AC_DEFINE([LLVM_HAS_ALIGN],[1],
         [Define if llvm::Align exists.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check whether llvm::InlineFunction needs dereference (>= 11)
AC_MSG_CHECKING([whether llvm::InlineFunction needs dereference])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/Cloning.h>
]],[[
  llvm::CallInst *ci = nullptr;
  llvm::InlineFunctionInfo ifi;
  llvm::InlineFunction(*ci, ifi);
]])],
        [AC_DEFINE([LLVM_INLINE_FUNCTION_NEEDS_DEREF],[1],
         [Define if the argument to llvm::InlineFunction should be dereferenced.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check whether VectorTyID exists (< 11)
AC_MSG_CHECKING([for llvm::Type::VectorTyID])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Type.h>
]],[[
  bool exists = (llvm::Type::VectorTyID == 42);
]])],
        [AC_DEFINE([LLVM_VECTOR_TYPEID_CASES],[case llvm::Type::VectorTyID:],
         [Case labels depending on whether VectorTyID exists.])
         AC_MSG_RESULT([llvm::Type::VectorTyId])],
        [AC_DEFINE([LLVM_VECTOR_TYPEID_CASES],
         [case llvm::Type::FixedVectorTyID: case llvm::Type::ScalableVectorTyID:],
         [Case labels depending on whether VectorTyID exists.])
         AC_MSG_RESULT([{llvm::Type::FixedVectorTyID, llvm::Type::ScalableVectorTyID}])])

## Check whether Instruction::isLifetimeStartOrEnd() exists (LLVM > 12)
AC_MSG_CHECKING([for Instruction::isLifetimeStartOrEnd])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Instruction.h>
]],[[
  llvm::Instruction *i = nullptr;
  auto res = i->isLifetimeStartOrEnd();
]])],
        [AC_DEFINE([LLVM_HAS_IS_LIFETIME_START_OR_END],[1],
         [Define if the function Instruction::isLifetimeStartOrEnd() exists.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check whether Instruction::isDroppable() exists (LLVM > 12)
AC_MSG_CHECKING([for Instruction::isDroppable])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Instruction.h>
]],[[
  llvm::Instruction *i = nullptr;
  auto res = i->isDroppable()
]])],
        [AC_DEFINE([LLVM_HAS_IS_DROPPABLE],[1],
         [Define if the function Instruction::isDroppable() exists.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])


## Check whether the function llvm::onlyUsedByLifetimeMarkersOrDroppableInsts() exists (LLVM >= 12)
AC_MSG_CHECKING([for llvm::onlyUsedByLifetimeMarkersOrDroppableInsts])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Analysis/ValueTracking.h>
]],[[
  int result = llvm::onlyUsedByLifetimeMarkersOrDroppableInsts(nullptr);
]])],
        [AC_DEFINE([LLVM_HAS_ONLYUSEDBYLIFETIMEMARKERSORDROPPABLEINSTS],[1],
         [Define if the function llvm::onlyUsedByLifetimeMarkersOrDroppableInsts exists.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check whether the class llvm::TerminatorInst exists (LLVM < 8)
AC_MSG_CHECKING([for llvm::TerminatorInst])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Instructions.h>
]],[[
  llvm::TerminatorInst *I = nullptr;
]])],
        [AC_DEFINE([LLVM_HAS_TERMINATORINST],[1],
         [Define if the class llvm::TerminatorInst exists.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check whether CallSite exists (< 11)
AC_MSG_CHECKING([for llvm::CallSite])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/CallSite.h>
]],[[
  llvm::CallSite *CS = nullptr;
]])],
        [AC_DEFINE([LLVM_HAS_CALLSITE],[1],
         [Define if llvm::CallSite exists.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check if llvm::Module::getOrInsertFunction returns a Constant (LLVM >= 9)
AC_MSG_CHECKING([whether llvm::Module::getOrInsertFunction returns a Constant])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Module.h>
]],[[
  llvm::Module *M = nullptr;
  llvm::Constant *F = M->getOrInsertFunction("foo", nullptr, {});
]])],
        [AC_DEFINE([LLVM_GETORINSERTFUNCTION_RET_FUNCTION],[1],
         [Define if llvm::Module::getOrInsertFunction returns a Constant *.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check if llvm::sys::fs::OpenFlags::F_None exists
AC_MSG_CHECKING([for llvm::sys::fs::OpenFlags::F_None])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/FileSystem.h>
]],[[
  llvm::sys::fs::OpenFlags f = llvm::sys::fs::F_None;
]])],
           [AC_DEFINE([HAVE_LLVM_SYS_FS_OPENFLAGS_FNONE],[1],
            [Define if the type llvm::sys::fs::OpenFlags::F_None exists.])
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

## Check whether llvm::PointerType::getElementType() exists (LLVM <= 14)
AC_MSG_CHECKING([whether PointerType::getElementType() exists])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/DerivedTypes.h>
]],[[
  llvm::PointerType *t = nullptr;
  t->getElementType();
]])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([LLVM_HAS_POINTER_GET_ELEMENT_TYPE],[1],
         [Define if llvm::PointerType::getElementType() exists.])],
        [AC_MSG_RESULT([no])])

## Check if llvm::InsertPreheadersForLoop() takes MemorySSAUpdater
AC_MSG_CHECKING([if llvm::InsertPreheaderForLoop takes MemorySSAUpdater])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Transforms/Utils/LoopUtils.h>
]],[[
  llvm::InsertPreheaderForLoop(nullptr, nullptr, nullptr, nullptr, false);
]])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([LLVM_INSERT_PREHEADER_FOR_LOOP_NEEDS_UPDATER],[1],
         [Define if llvm::InsertPreheaderForLoop needs MemorySSAUpdater.])],
        [AC_MSG_RESULT([no])])

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

## Check if we can get the address space of a global value (LLVM >= 8)
AC_MSG_CHECKING([if GlobalValue has getAddressSpace()])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/GlobalValue.h>
]],[[
          llvm::GlobalValue *gv = nullptr;
          unsigned int spc = gv->getAddressSpace();
        ]])],
        [AC_DEFINE([LLVM_GLOBALVALUE_HAS_GET_ADDRESS_SPACE],[1],
            [Define if we can get the address space from a GlobalValue.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check whether llvm::df_iterator_default_set exists
AC_MSG_CHECKING([for llvm::df_iterator_default_set])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/ADT/DepthFirstIterator.h>
]],[[
  llvm::df_iterator_default_set<int *> Reachable;
]])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([LLVM_HAVE_DF_ITERATOR_DEFAULT_SET],[1],
   [Define if llvm::df_iterator_default_set exists])],
  [AC_MSG_RESULT([no])])

## Check whether llvm::EliminateUnreachableBlocks exists
AC_MSG_CHECKING([for llvm::EliminateUnreachableBlocks])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
]],[[
  llvm::Function *F = nullptr;
  llvm::EliminateUnreachableBlocks(*F);
]])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([LLVM_HAVE_ELIMINATE_UNREACHABLE_BLOCKS],[1],
   [Define if llvm::EliminateUnreachableBlocks exists])],
  [AC_MSG_RESULT([no])])

## Check if llvm::handleOperandChange needs a Use argument
AC_MSG_CHECKING([whether llvm::handleOperandChange needs a Use argument])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Value.h>
#include <llvm/IR/Constant.h>
]],[[
  llvm::Constant *C = nullptr;
  C->handleOperandChange(nullptr, nullptr, nullptr);
]])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([LLVM_HANDLE_OPERAND_CHANGE_NEEDS_USE],[1],
   [Define if llvm::handleOperandChange needs a Use argument])],
  [AC_MSG_RESULT([no])])


AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
