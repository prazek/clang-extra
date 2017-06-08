//===--- UseClampCheck.cpp - clang-tidy------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "UseClampCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace modernize {

void UseClampCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus1z)
    return;

  auto InnerMinMatcher = callExpr(
      callee(namedDecl(hasName("::std::min"))),
      hasArgument(0, expr().bind("clamped_expr")),
      hasArgument(1, expr().bind("max_expr"))
  );

  auto InnerMaxMatcher = callExpr(
      callee(namedDecl(hasName("::std::max"))),
      hasArgument(0, expr().bind("clamped_expr")),
      hasArgument(1, expr().bind("min_expr"))
  );

  auto OuterMinMatcher = callExpr(
    callee(namedDecl(hasName("::std::min"))),
    anyOf(
      allOf(
        hasArgument(0, InnerMaxMatcher),
        hasArgument(1, expr().bind("max_expr"))
      ),
      allOf(
        hasArgument(0, expr().bind("max_expr")),
        hasArgument(1, InnerMaxMatcher)
      )
    )
  );

  auto OuterMaxMatcher = callExpr(
    callee(namedDecl(hasName("::std::max"))),
    anyOf(
      allOf(
        hasArgument(0, InnerMinMatcher),
        hasArgument(1, expr().bind("min_expr"))
      ),
      allOf(
        hasArgument(0, expr().bind("min_expr")),
        hasArgument(1, InnerMinMatcher)
      )
    )
  );

  Finder->addMatcher(OuterMinMatcher.bind("call"), this);
  Finder->addMatcher(OuterMaxMatcher.bind("call"), this);
}

std::string UseClampCheck::getExprText(const Expr *expr, const MatchFinder::MatchResult &Result) {
  bool invalidRange;
  expr->dump();
  const auto CharRange = CharSourceRange::getTokenRange(expr->getSourceRange());
  return Lexer::getSourceText(
    CharRange,
    *Result.SourceManager,
    getLangOpts(),
    &invalidRange
  );
}

void UseClampCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *Call = Result.Nodes.getNodeAs<CallExpr>("call");
  const auto *Min = Result.Nodes.getNodeAs<Expr>("min_expr");
  const auto *Max = Result.Nodes.getNodeAs<Expr>("max_expr");
  const auto *Clamped = Result.Nodes.getNodeAs<Expr>("clamped_expr");

  Call->dump();
  Min->dump();
  Max->dump();
  Clamped->dump();


  SourceRange ReplaceRange = Call->getSourceRange();

  std::string ClampText = "std::clamp(";
  ClampText += getExprText(Clamped, Result);
  ClampText += ", ";
  ClampText += getExprText(Min, Result);
  ClampText += ", ";
  ClampText += getExprText(Max, Result);
  ClampText += ")";

  diag(Call->getLocStart(), "use std::clamp instead of std::min and std::max")
    << FixItHint::CreateReplacement(ReplaceRange, ClampText);

}

} // namespace modernize
} // namespace tidy
} // namespace clang
