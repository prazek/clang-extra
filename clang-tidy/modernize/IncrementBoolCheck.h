//===--- IncrementBoolCheck.h - clang-tidy-----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_INCREMENT_BOOL_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_INCREMENT_BOOL_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace modernize {

/// This check replaces deprecated bool incrementation with truth assignment.
///
/// Before:
///  bool x = false, y = ++x;
/// After:
///  bool x = false; y = (x = true);
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/modernize-increment-bool.html
class IncrementBoolCheck : public ClangTidyCheck {
public:
  IncrementBoolCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace modernize
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_INCREMENT_BOOL_H
