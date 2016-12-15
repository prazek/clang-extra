//===--- InvalidRangeCheck.h - clang-tidy------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OBVIOUS_INVALID_RANGE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OBVIOUS_INVALID_RANGE_H

#include "../ClangTidy.h"
#include <string>
#include <vector>

namespace clang {
namespace tidy {
namespace obvious {

/// Find invalid call to algorithm with obviously invalid range of iterators.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/obvious-invalid-range.html
class InvalidRangeCheck : public ClangTidyCheck {
public:
  InvalidRangeCheck(StringRef Name, ClangTidyContext *Context);
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;

private:
  std::vector<std::string> AlgorithmNames;
};

} // namespace obvious
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OBVIOUS_INVALID_RANGE_H
