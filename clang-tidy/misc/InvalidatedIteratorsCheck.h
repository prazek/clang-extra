//===--- InvalidatedIteratorsCheck.h - clang-tidy----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_INVALIDATED_ITERATORS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_INVALIDATED_ITERATORS_H

#include "../ClangTidy.h"
#include "clang/AST/RecursiveASTVisitor.h"

namespace clang {
namespace tidy {
namespace misc {

/// This check warns about the possible use of the invalidated container
/// iterators, pointers and/or references.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-invalidated-iterators.html
class InvalidatedIteratorsCheck : public ClangTidyCheck {
public:
  InvalidatedIteratorsCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  using ExprMatcherType = ast_matchers::internal::BindableMatcher<Stmt>;

  // Returns the clang-tidy matcher binding to the possibly invalidating uses
  // of the container (either invoking a dangerous member call or calling
  // another function with a non-const reference.)
  ExprMatcherType getModifyingMatcher(const VarDecl *VectorDecl);

  // Checks if given Func can invalidate the container at its given argument
  // (ArgId). For convienience, number of Func's arguemnts is also passed.
  bool canFuncInvalidate(const FunctionDecl *Func, unsigned ArgId,
                         unsigned NumCallArgs, ASTContext *Context);

  // Checks if given Call can invalidate the container provided by Match.
  bool canCallInvalidate(const CallExpr *Call, ast_matchers::BoundNodes Match,
                         ASTContext *Context);

  // A cache which remembers the results of canFuncInvalidate calls. It
  // is necessary in order to prevent canFuncInvalidate from looping or
  // having exponential time behavior.
  std::map<std::tuple<const FunctionDecl *, unsigned, unsigned>, bool>
      CanFuncInvalidateMemo;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_INVALIDATED_ITERATORS_H
