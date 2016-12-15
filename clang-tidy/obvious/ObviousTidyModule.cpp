//===------- ObviousTidyModule.cpp - clang-tidy ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"

#include "InvalidRangeCheck.h"
using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace obvious {

class ObviousModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<InvalidRangeCheck>("obvious-invalid-range");
  }
};

// Register the ObviousModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<ObviousModule> X("obvious-module",
                                                     "Add obvious checks.");

} // namespace obvious

// This anchor is used to force the linker to link in the generated object file
// and thus register the ObviousModule.
volatile int ObviousModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
