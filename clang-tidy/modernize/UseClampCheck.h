//===--- UseClampCheck.h - clang-tidy----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_USE_CLAMP_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_USE_CLAMP_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace modernize {

/// Replace std::min(std::max(x, lo), hi) idiom with std::clamp(x, lo, hi)
class UseClampCheck : public ClangTidyCheck {
public:
  UseClampCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
private:
  std::string getExprText(const Expr *expr, const ast_matchers::MatchFinder::MatchResult &Result);
};

} // namespace modernize
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_USE_CLAMP_H
