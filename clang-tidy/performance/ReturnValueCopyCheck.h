//===--- ReturnValueCopyCheck.h - clang-tidy-----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_RETURN_VALUE_COPY_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_RETURN_VALUE_COPY_H

#include "../ClangTidy.h"
#include "../utils/IncludeInserter.h"

namespace clang {
namespace tidy {
namespace performance {

/// Adds `std::move` in returns statements where returned object is copied and
/// adding `std::move` can make it being moved.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/performance-return-value-copy.html
class ReturnValueCopyCheck : public ClangTidyCheck {
public:
  ReturnValueCopyCheck(StringRef Name, ClangTidyContext *Context);

  void registerPPCallbacks(CompilerInstance &Compiler) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  std::unique_ptr<utils::IncludeInserter> Inserter;
  const utils::IncludeSorter::IncludeStyle IncludeStyle;
};

} // namespace performance
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_RETURN_VALUE_COPY_H
