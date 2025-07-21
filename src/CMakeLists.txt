set(LIB_SOURCES
  ADT/DepView.cpp
  ADT/VectorClock.cpp
  ADT/View.cpp
  Config/Config.cpp
  Config/Verbosity.cpp
  ExecutionGraph/DepExecutionGraph.cpp
  ExecutionGraph/DepInfo.cpp
  ExecutionGraph/Event.cpp
  ExecutionGraph/EventLabel.cpp
  ExecutionGraph/ExecutionGraph.cpp
  ExecutionGraph/GraphUtils.cpp
  ExecutionGraph/Stamp.cpp
  ExecutionGraph/Consistency/BoundDecider.cpp
  ExecutionGraph/Consistency/ConsistencyChecker.cpp
  ExecutionGraph/Consistency/ContextBoundDecider.cpp
  ExecutionGraph/Consistency/IMMChecker.cpp
  ExecutionGraph/Consistency/RAChecker.cpp
  ExecutionGraph/Consistency/RC11Checker.cpp
  ExecutionGraph/Consistency/RoundBoundDecider.cpp
  ExecutionGraph/Consistency/SCChecker.cpp
  ExecutionGraph/Consistency/SymmetryChecker.cpp
  ExecutionGraph/Consistency/TSOChecker.cpp
  Runtime/Execution.cpp
  Runtime/ExternalFunctions.cpp
  Runtime/Interpreter.cpp
  Runtime/InterpreterEnumAPI.cpp
  Static/LLVMModule.cpp
  Static/LLVMUtils.cpp
  Static/ModuleInfo.cpp
  Static/Transforms/BisimilarityCheckerPass.cpp
  Static/Transforms/CallInfoCollectionPass.cpp
  Static/Transforms/CodeCondenserPass.cpp
  Static/Transforms/ConfirmationAnnotationPass.cpp
  Static/Transforms/DeclareInternalsPass.cpp
  Static/Transforms/DefineLibcFunsPass.cpp
  Static/Transforms/EliminateAnnotationsPass.cpp
  Static/Transforms/EliminateCASPHIsPass.cpp
  Static/Transforms/EliminateCastsPass.cpp
  Static/Transforms/EliminateRedundantInstPass.cpp
  Static/Transforms/EliminateUnusedCodePass.cpp
  Static/Transforms/EscapeCheckerPass.cpp
  Static/Transforms/FunctionInlinerPass.cpp
  Static/Transforms/InstAnnotator.cpp
  Static/Transforms/IntrinsicLoweringPass.cpp
  Static/Transforms/LoadAnnotationPass.cpp
  Static/Transforms/LocalSimplifyCFGPass.cpp
  Static/Transforms/LoopJumpThreadingPass.cpp
  Static/Transforms/LoopUnrollPass.cpp
  Static/Transforms/MDataCollectionPass.cpp
  Static/Transforms/MMDetectorPass.cpp
  Static/Transforms/PromoteMemIntrinsicPass.cpp
  Static/Transforms/PropagateAssumesPass.cpp
  Static/Transforms/RustPrepPass.cpp
  Static/Transforms/SpinAssumePass.cpp
  Static/Transforms/BarrierResultCheckerPass.cpp
  Support/ASize.cpp
  Support/MemOrdering.cpp
  Support/NameInfo.cpp
  Support/Parser.cpp
  Support/SAddr.cpp
  Support/SAddrAllocator.cpp
  Support/SVal.cpp
  Support/ThreadPinner.cpp
  Support/ThreadPool.cpp
  Verification/ChoiceMap.cpp
  Verification/GenMCDriver.cpp
  Verification/WorkList.cpp
  Verification/VerificationError.cpp
  Verification/Revisit.cpp
  Verification/Relinche/LinearizabilityChecker.cpp
  Verification/Relinche/Linearization.cpp
  Verification/Relinche/Observation.cpp
  Verification/Relinche/Specification.cpp
  Verification/Scheduler.cpp
)
set(SOURCES
  main.cpp
)

### GenMC library:
if (BUILD_MODEL_CHECKER)
  find_package(Threads REQUIRED)
  find_library(HWLOC hwloc)

  add_library(genmc_lib ${GENMC_LIB_TYPE} ${LIB_SOURCES})
  if (INSTALL_MODEL_CHECKER)
    install(TARGETS genmc_lib ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
    # FIXME(cmake): make this more precise (don't export everything?)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.def"
            PATTERN "CMakeFiles" EXCLUDE
            PATTERN "*.cpp" EXCLUDE)
  endif()

  target_compile_features(genmc_lib PRIVATE cxx_std_23)
  target_link_libraries(genmc_lib PUBLIC ffi dl Threads::Threads LLVM)
  if(HWLOC)
    target_link_libraries(genmc_lib PRIVATE hwloc)
  else()
    message(STATUS "hwloc not found. Thread pinning disabled.")
  endif()
  target_link_libraries(genmc_lib PRIVATE genmc_config_includes)

  # Includes and definitions, which will be inherited by dependencies: 
  target_compile_definitions(genmc_lib PUBLIC ${LLVM_DEFINITIONS})
  target_include_directories(genmc_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LLVM_INCLUDE_DIRS})
endif()


### GenMC executable:
if (BUILD_LLI)
  add_executable(${PROJECT_NAME} ${SOURCES})
  if (INSTALL_LLI)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME})
  endif()

  target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
  target_link_libraries(${PROJECT_NAME} PRIVATE genmc_config_includes)
  target_link_libraries(${PROJECT_NAME} PRIVATE genmc_lib)
endif()
