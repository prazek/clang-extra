//===--- IncrementBoolCheck.cpp - clang-tidy-------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IncrementBoolCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <clang/Lex/Lexer.h>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace modernize {

static const char ExprName[] = "expr";
static const char IncrText[] = "++";
static const char ParentName[] = "parent";
static const char ParenExprName[] = "parenExpr";
static const char UnaryOpName[] = "unaryOp";

void IncrementBoolCheck::registerMatchers(MatchFinder *Finder) {
  const auto ExprMatcher =
      allOf(has(expr(hasType(booleanType())).bind(ExprName)),
            hasOperatorName(IncrText));

  const auto ParentMatcher = hasParent(compoundStmt().bind(ParentName));
  const auto ParMatcher = hasDescendant(parenExpr().bind(ParenExprName));

  // We want to match the operator, no matter if it is a whole statement or not;
  // no matter if it is a parenthesized expression or not; we just might add
  // additional bindings.
  Finder->addMatcher(unaryOperator(ExprMatcher,
                                   eachOf(ParentMatcher, unless(ParentMatcher)),
                                   eachOf(ParMatcher, unless(ParMatcher)),
                                   unless(isInTemplateInstantiation()))
                         .bind(UnaryOpName),
                     this);
}

void IncrementBoolCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<UnaryOperator>(UnaryOpName);
  const auto *MatchedExpr = Result.Nodes.getNodeAs<Expr>(ExprName);

  // Don't fix if expression type is dependent on template initialization
  if (MatchedExpr->isTypeDependent())
    return;

  if (MatchedDecl->getLocStart().isMacroId())
    return;

  SourceManager &SM = *Result.SourceManager;

  bool Invalid = false;
  StringRef SubExpr = Lexer::getSourceText(
      CharSourceRange::getTokenRange(MatchedExpr->getLocStart(),
                                     MatchedExpr->getLocEnd()),
      SM, LangOptions(), &Invalid);
  assert(!Invalid);


  bool IsLastOperation = Result.Nodes.getNodeAs<CompoundStmt>(ParentName);

  auto Diag = diag(MatchedDecl->getLocStart(),
                   "bool incrementation is deprecated, use assignment");

  // Postfix operations are probably impossible to change automatically.
  if (MatchedDecl->isPostfix() && !IsLastOperation)
    return;

  bool UseInnerParentheses = Result.Nodes.getNodeAs<ParenExpr>(ParenExprName);
  bool UseOuterParentheses = !IsLastOperation;

  StringRef Replacement = SubExpr;
  if (UseInnerParentheses)
    Replacement = llvm::Twine("(" + Replacement + ")").str();

  Replacement = llvm::Twine(Replacement + " = true").str();
  if (UseOuterParentheses)
    Replacement = llvm::Twine("(" + Replacement + ")").str();

  Diag << FixItHint::CreateReplacement(
      CharSourceRange::getTokenRange(MatchedDecl->getLocStart(),
                                     MatchedDecl->getLocEnd()),
      Replacement);
}

} // namespace modernize
} // namespace tidy
} // namespace clang
