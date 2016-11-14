//===--- UseAutoCheck.h - clang-tidy-----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LLVM_USE_AUTO_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LLVM_USE_AUTO_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace llvm {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/llvm-use-auto.html
class UseAutoCheck : public ClangTidyCheck {
public:
  UseAutoCheck(StringRef Name, ClangTidyContext *Context);
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  std::vector<std::string> Functions;
};

} // namespace llvm
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LLVM_USE_AUTO_H
