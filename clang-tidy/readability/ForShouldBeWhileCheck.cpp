//===--- ForShouldBeWhileCheck.cpp - clang-tidy----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ForShouldBeWhileCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace readability {

void ForShouldBeWhileCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(forStmt(allOf(unless(hasIncrement(anything())), unless(hasLoopInit(anything())))).bind("for"), this);
}

void ForShouldBeWhileCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *For = Result.Nodes.getNodeAs<ForStmt>("for");

  const LangOptions &Opts = getLangOpts();

  const auto *CondExpr = For->getCond();

  std::string CondText;

  if (CondExpr) {
    const auto CondRange = CharSourceRange::getTokenRange(CondExpr->getSourceRange());

    bool invalidRange;
    if (CondRange.isValid()) {
      CondText = Lexer::getSourceText(
        CondRange,
        *Result.SourceManager,
        Opts,
        &invalidRange
      );
    }
  } else {
    CondText = "true";
  }



  std::string ReplacementText = "while (";
  ReplacementText += CondText;

  ReplacementText += ")";

  auto Hint = FixItHint::CreateReplacement(SourceRange(For->getForLoc(), For->getRParenLoc()),
                                      ReplacementText);

  diag(For->getForLoc(), "this for loop should be a while loop") << Hint;
}

} // namespace readability
} // namespace tidy
} // namespace clang
