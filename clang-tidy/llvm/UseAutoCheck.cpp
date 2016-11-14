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
#include "../utils/OptionsUtils.h"
#include <iostream>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace llvm {

static const auto DefaultFunctions =
    "::llvm::dyn_cast; ::llvm::dyn_cast_or_null; ::llvm::cast; ::llvm::cast_or_null; ::std::make_shared; ::std::make_unique";


bool EqualForAuto(QualType lhs, QualType rhs) {
  if (lhs.getUnqualifiedType() == rhs.getUnqualifiedType()) {
    return true;
  }
  else if (lhs.getTypePtr()->isPointerType() && rhs.getTypePtr()->isPointerType()) {
    return lhs.getTypePtr()->getPointeeType().getUnqualifiedType() ==
           rhs.getTypePtr()->getPointeeType().getUnqualifiedType();
  }
  else if (lhs.getTypePtr()->isReferenceType() && rhs.getTypePtr()->isReferenceType()) {
    return lhs.getTypePtr()->getPointeeType().getUnqualifiedType() ==
           rhs.getTypePtr()->getPointeeType().getUnqualifiedType();
  }
  else if (rhs.getTypePtr()->isReferenceType()) {
    return lhs.getUnqualifiedType() ==
           rhs.getTypePtr()->getPointeeType().getUnqualifiedType();
  }
  else {
    return false;
  }
}

UseAutoCheck::UseAutoCheck(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      Functions(utils::options::parseStringList(Options.get(
          "Functions", DefaultFunctions))) {}

void UseAutoCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus11)
    return;

  SmallVector<StringRef, 8> FunctionNames(Functions.begin(), Functions.end());

  auto callInteresting = callExpr(callee(functionDecl(hasAnyName(FunctionNames)))).bind("call_expr");
  auto hasAutoType = anyOf(hasType(autoType()),
                           hasType(pointerType(pointee(autoType()))),
                           hasType(referenceType(pointee(autoType()))));

  Finder->addMatcher(
    declStmt(hasSingleDecl(varDecl(hasInitializer(ignoringImplicit(anyOf(callInteresting,
                                                  cxxConstructExpr(hasArgument(0, callInteresting))))),
            unless(hasAutoType))
                       .bind("cast"))), this);
}

void UseAutoCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>("cast");
  const auto *MatchedCall = Result.Nodes.getNodeAs<CallExpr>("call_expr");
  auto TypeRange = MatchedDecl->getTypeSourceInfo()->getTypeLoc().getSourceRange();

  auto ResultType = MatchedCall->getCallReturnType(*Result.Context).getCanonicalType();
  auto DeclType = MatchedDecl->getType().getCanonicalType();

  if (!EqualForAuto(DeclType, ResultType))
    return;

  const bool IsPointer = MatchedDecl->getType().getTypePtr()->isPointerType();
  const bool IsReference = MatchedDecl->getType().getTypePtr()->isReferenceType();

  auto Diag = diag(TypeRange.getBegin(), "redundant type; use auto instead");

  if (IsPointer) {
      Diag << FixItHint::CreateReplacement(TypeRange, "auto *");
  }
  else if (IsReference) {
      Diag << FixItHint::CreateReplacement(TypeRange, "auto &");
  }
  else {
    Diag << FixItHint::CreateReplacement(TypeRange, "auto");
  }
}

} // namespace llvm
} // namespace tidy
} // namespace clang
