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

static const auto LEXICAL_CAST_NAME = "boost::lexical_cast";

void UseAutoCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus11)
    return;

  Finder->addMatcher(
    declStmt(has(varDecl(hasInitializer(callExpr(callee(
      functionDecl(hasName(LEXICAL_CAST_NAME))))),
                         unless(hasType(autoType())))))
      .bind("lexical_cast"),
    this);

}

void UseAutoCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<DeclStmt>("lexical_cast");
  const auto *FirstDecl = dyn_cast<VarDecl>(*MatchedDecl->decl_begin());
  auto TypeRange = FirstDecl->getTypeSourceInfo()->getTypeLoc()
    .getSourceRange();

  diag(TypeRange.getBegin(), "redundant type for lexical_calst; use auto "
    "instead ") << FixItHint::CreateReplacement(TypeRange, "auto");
}

} // namespace boost
} // namespace tidy
} // namespace clang
