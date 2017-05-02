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

#include "LLVMModule.hpp"
#include "DeclareAssumePass.hpp"
#include "SpinAssumePass.hpp"
#include "LoopUnrollPass.hpp"
#include "Error.hpp"
#include <llvm/PassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

/* TODO: Move explanation comments to *.hpp files. */
namespace LLVMModule {
/* Global variable to handle the LLVM context */
	llvm::LLVMContext *globalContext = nullptr;
	
/* Returns the LLVM context */
	llvm::LLVMContext &getLLVMContext(void)
	{
		if (!globalContext)
			globalContext = new llvm::LLVMContext();
		return *globalContext;
	}
	
/* 
 * Destroys the LLVM context. This function should be called explicitly
 * when we are done managing the LLVM data.
 */
	void destroyLLVMContext(void)
	{
		delete globalContext;
	}
	
/* Returns the LLVM module corresponding to the source code stored in src. */
	llvm::Module *getLLVMModule(std::string &src)
	{
		llvm::Module *m;
		llvm::MemoryBuffer *buf;
		llvm::SMDiagnostic err;
		
		buf = llvm::MemoryBuffer::getMemBuffer(src, "", false);
		return llvm::ParseIR(buf, err, getLLVMContext());
	}

	bool transformLLVMModule(llvm::Module &mod, Config *conf)
	{
		llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
		llvm::PassManager PM;
		bool modified;
		
		llvm::initializeCore(Registry);
		llvm::initializeScalarOpts(Registry);
		llvm::initializeObjCARCOpts(Registry);
		llvm::initializeVectorization(Registry);
		llvm::initializeIPO(Registry);
		llvm::initializeAnalysis(Registry);
		llvm::initializeIPA(Registry);
		llvm::initializeTransformUtils(Registry);
		llvm::initializeInstCombine(Registry);
		llvm::initializeInstrumentation(Registry);
		llvm::initializeTarget(Registry);

		PM.add(new DeclareAssumePass());
		if (conf->spinAssume){
			PM.add(new SpinAssumePass());
		} 
		if (conf->unroll >= 0){
			PM.add(new LoopUnrollPass(conf->unroll));
		}
		// PM.add(new AddLibPass());
		modified = PM.run(mod);
		assert(!llvm::verifyModule(mod));
		return modified;
	}

	void printLLVMModule(llvm::Module &mod, std::string &out)
	{
		llvm::PassManager PM;
		std::string err;
		llvm::raw_ostream *os = new llvm::raw_fd_ostream(out.c_str(), err,
								 llvm::sys::fs::F_None);

		/* TODO: Do we need an exception? If yes, properly handle it */
		if (err.size()) {
			delete os;
			WARN("Failed to write transformed module to file "
			     + out + ": " + err);
			return;
		}
		PM.add(llvm::createPrintModulePass(*os));
		PM.run(mod);
		return;
	}

}
