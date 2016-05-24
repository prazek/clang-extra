//===------- BugProneTidyModule.cpp - clang-tidy --------------------------===//
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
#include "BoolToIntegerConversionCheck.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace bugprone {

class BugProneModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<BoolToIntegerConversionCheck>(
        "bugprone-bool-to-integer-conversion");
  }
};

// Register the BoostModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<BugProneModule> X("bugprone-module",
                                                      "Add bugprone checks.");

} // namespace bugprone

// This anchor is used to force the linker to link in the generated object file
// and thus register the BoostModule.
volatile int BugProneModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
