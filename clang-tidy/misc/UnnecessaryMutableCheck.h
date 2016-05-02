//===--- UnnecessaryMutableCheck.h - clang-tidy------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_UNNECESSARY_MUTABLE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_UNNECESSARY_MUTABLE_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace misc {

/// Searches for and removes unnecessary 'mutable' keywords (that is, for
/// fields that cannot be possibly changed in const methods).
///
/// Example:
///
/// class MyClass {
///   mutable int field;  // will change into "int field;"
/// };
///
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-unnecessary-mutable.html
class UnnecessaryMutableCheck : public ClangTidyCheck {
public:
  UnnecessaryMutableCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_UNNECESSARY_MUTABLE_H
