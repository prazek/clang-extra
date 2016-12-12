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

/// Visitor which tracks all modifications of containers that might
/// invalidate many references/iterators/pointers.
class ContainerTrackingVisitor
    : public clang::RecursiveASTVisitor<ContainerTrackingVisitor> {
public:
  ContainerTrackingVisitor(const Expr *RefExpr, const Expr *ContainerExpr) {
    // TODO write constructor
  }
};

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-invalidated-iterators.html
class InvalidatedIteratorsCheck : public ClangTidyCheck {
public:
  InvalidatedIteratorsCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_INVALIDATED_ITERATORS_H
