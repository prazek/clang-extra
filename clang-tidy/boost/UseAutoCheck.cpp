//===--- UseAutoCheck.cpp - clang-tidy-------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "UseAutoCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace boost {

void UseAutoCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus11)
    return;

  Finder->addMatcher(
    varDecl(has(ignoringImplicit(callExpr(callee(functionDecl(
      hasAnyName("llvm::dyn_cast", "llvm::dyn_cast_or_null", "llvm::cast",
                 "llvm::cast_or_null")))))),
            unless(anyOf(hasType(autoType()),
                         hasType(pointerType(pointee(autoType()))),
                         hasType(referenceType(pointee(autoType()))))))
                       .bind("cast"), this);
}

void UseAutoCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>("cast");
  auto TypeRange = MatchedDecl->getTypeSourceInfo()->getTypeLoc().getSourceRange();

  diag(TypeRange.getBegin(), "redundant type; use auto instead")
    << FixItHint::CreateReplacement(TypeRange, "auto *");
}

} // namespace boost
} // namespace tidy
} // namespace clang
