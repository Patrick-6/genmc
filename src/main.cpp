/*
 * RCMC -- Model Checking for C11 programs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Author: Michalis Kokologiannakis <mixaskok@gmail.com>
 */

#include "config.h"
#include "Config.hpp"
#include "RCMCDriver.hpp"
#include "Error.hpp"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <memory>

#include <cstdlib>
#include <ctime>
#include <fstream>

using namespace clang;
using namespace clang::driver;

std::string GetExecutablePath(const char *Argv0)
{
	/*
	 * This just needs to be some symbol in the binary; C++ doesn't
	 * allow taking the address of ::main however.
	 */
	void *MainAddr = (void*) (intptr_t) GetExecutablePath;
	return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}

int main(int argc, char **argv)
{
	clock_t start = clock();
	Config *conf = new Config(argc, argv);
	conf->getConfigOptions();
	if (conf->inputFromBitcodeFile) {
		RCMCDriver *driver = new RCMCDriver(conf, start);
		driver->parseRun();
		delete conf;
		delete driver;
		/* TODO: Check globalContext.destroy() and llvm::shutdown() */
		return 0;
	}

	void *MainAddr = (void*) (intptr_t) GetExecutablePath;
	std::string Path = CLANGPATH;
	IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
	TextDiagnosticPrinter *DiagClient =
		new TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);

	IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
	DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);

	// Use ELF on windows for now.
	std::string TripleStr = llvm::sys::getProcessTriple();
	llvm::Triple T(TripleStr);
	if (T.isOSBinFormatCOFF())
		T.setObjectFormat(llvm::Triple::ELF);

	Driver TheDriver(Path, T.str(), Diags);
	TheDriver.setTitle("clang interpreter");
	TheDriver.setCheckInputsExist(false);

	SmallVector<const char *, 16> Args;//(argv, argv + argc);
	Args.push_back("-fsyntax-only");
	Args.push_back("-g"); /* Compile with -g to get debugging mdata */
	for (auto &f : conf->cflags)
		Args.push_back(f.c_str());
	Args.push_back(conf->inputFile.c_str());
	std::unique_ptr<Compilation> C(TheDriver.BuildCompilation(Args));
	if (!C)
		return 0;

	const driver::JobList &Jobs = C->getJobs();
#ifdef CLANG_LIST_TYPE_JOB_PTR
	const driver::Command &Cmd = *cast<driver::Command>(*Jobs.begin());
#else
	const driver::Command &Cmd = cast<driver::Command>(*Jobs.begin());
#endif
	const driver::ArgStringList &CCArgs = Cmd.getArguments();
	CompilerInvocation *CI = new CompilerInvocation;
	CompilerInvocation::CreateFromArgs(*CI,
					   const_cast<const char **>(CCArgs.data()),
					   const_cast<const char **>(CCArgs.data()) +
					   CCArgs.size(),
					   Diags);

	// Show the invocation, with -v.
	if (CI->getHeaderSearchOpts().Verbose) {
		llvm::errs() << "clang invocation:\n";
		Jobs.Print(llvm::errs(), "\n", true);
		llvm::errs() << "\n";
	}

	CompilerInstance Clang;
	Clang.setInvocation(std::move(CI));//std::move(CI.get()));

	// Create the compilers actual diagnostics engine.
	Clang.createDiagnostics();
	if (!Clang.hasDiagnostics())
		return 1;

	// Infer the builtin include path if unspecified.
	if (Clang.getHeaderSearchOpts().UseBuiltinIncludes &&
	    Clang.getHeaderSearchOpts().ResourceDir.empty())
		Clang.getHeaderSearchOpts().ResourceDir =
			CompilerInvocation::GetResourcesPath(argv[0], MainAddr);

	// Create and execute the frontend to generate an LLVM bitcode module.
	std::unique_ptr<CodeGenAction> Act(new EmitLLVMOnlyAction());
	if (!Clang.ExecuteAction(*Act))
		return 1;

#ifdef LLVM_EXECUTIONENGINE_MODULE_UNIQUE_PTR
	RCMCDriver *driver = new RCMCDriver(conf, Act->takeModule(), start);
#else
	RCMCDriver *driver =
		new RCMCDriver(conf, std::unique_ptr<llvm::Module>(Act->takeModule()), start);
#endif

	driver->run();

	delete conf;
	delete driver;
	/* TODO: Check globalContext.destroy() and llvm::shutdown() */

	return 0;
}
