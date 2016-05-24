//===--- BoolToIntegerConversionCheck.h - clang-tidy-------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_BOOL_TO_INTEGER_CONVERSION_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_BOOL_TO_INTEGER_CONVERSION_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace bugprone {

/// Finds implicit casts of bool literal to integer types like int a = true,
/// and replaces it with integer literals like int a = 1
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/modernize-bool-to-integer-conversion.html
class BoolToIntegerConversionCheck : public ClangTidyCheck {
public:
  BoolToIntegerConversionCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  void changeFunctionReturnType(const FunctionDecl &FD, const Expr &Return);
};

} // namespace bugprone
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_BOOL_TO_INTEGER_CONVERSION_H
