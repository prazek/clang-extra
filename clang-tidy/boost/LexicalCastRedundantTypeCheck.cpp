//===--- LexicalCastRedundantTypeCheck.cpp - clang-tidy--------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "LexicalCastRedundantTypeCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace boost {

static const std::string LEXICAL_CAST_NAME = "boost::lexical_cast";
void LexicalCastRedundantTypeCheck::registerMatchers(MatchFinder *Finder) {

  if (!getLangOpts().CPlusPlus11)
    return;

  Finder->addMatcher(
    declStmt(has(varDecl(hasInitializer(callExpr(callee(
      functionDecl(hasName(LEXICAL_CAST_NAME))))),
                         unless(hasType(autoType())))))
      .bind("same_type"),
    this);

  Finder->addMatcher(
    declStmt(has(varDecl(hasInitializer(implicitCastExpr(has(callExpr(
      callee(functionDecl(hasName(LEXICAL_CAST_NAME))))))))))
      .bind("different_type"),
    this);
}

void LexicalCastRedundantTypeCheck::apply(const DeclStmt *D,
                                          const std::string &Warning,
                                          bool DoFixit) {
  const auto *Matched = dyn_cast<VarDecl>(*D->decl_begin());
  if (!Matched)
    return; // TOO remove?

  SourceRange Range(
    Matched->getTypeSourceInfo()->getTypeLoc().getSourceRange());

  auto Diag = diag(Range.getBegin(), Warning);
  if (DoFixit)
    Diag << FixItHint::CreateReplacement(Range, "auto");
}


void LexicalCastRedundantTypeCheck::check(
  const MatchFinder::MatchResult &Result) {

  const auto *MatchedSameType = Result.Nodes.getNodeAs<DeclStmt>("same_type");
  const auto *MatchedDifferentType =
    Result.Nodes.getNodeAs<DeclStmt>("different_type");

  if (MatchedSameType) {
    apply(MatchedSameType, "redundant type for lexical_cast; use auto "
      "instead", true);
  }
  if (MatchedDifferentType) {
    apply(MatchedDifferentType,
          "different types for lexical_cast. Use auto instead", false);
  }
}

} // namespace boost
} // namespace tidy
} // namespace clang