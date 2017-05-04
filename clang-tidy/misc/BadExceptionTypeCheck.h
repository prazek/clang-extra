//===--- BadExceptionTypeCheck.h - clang-tidy--------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_BAD_EXCEPTION_TYPE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_BAD_EXCEPTION_TYPE_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace misc {

/// \brief checks for locations that throw an object of inappropriate type.
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-bad-exception-type.html
class BadExceptionTypeCheck : public ClangTidyCheck {
public:
  BadExceptionTypeCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_BAD_EXCEPTION_TYPE_H
