//===--- UseBoolLiteralsCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "UseBoolLiteralsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void UseBoolLiteralsCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      implicitCastExpr(has(integerLiteral().bind("literal")),
                       hasImplicitDestinationType(qualType(booleanType()))),
      this);
}

/// Checks, if integer \p Literal is preprocessor dependent
static bool isPreprocessorIndependent(const IntegerLiteral *Literal,
                                      const MatchFinder::MatchResult &Result) {
  const LangOptions &Opts = Result.Context->getLangOpts();
  std::string LiteralSource = Lexer::getSourceText(
      CharSourceRange::getTokenRange(Literal->getSourceRange()),
      *Result.SourceManager, Opts);

  return LiteralSource.size() >= 1 && isDigit(LiteralSource.front());
}

void UseBoolLiteralsCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Literal = Result.Nodes.getNodeAs<IntegerLiteral>("literal");

  if (isPreprocessorIndependent(Literal, Result)) {
    bool LiteralBooleanValue = Literal->getValue().getBoolValue();
    diag(Literal->getLocation(), "implicitly converting integer literal to "
                                 "bool, use bool literal instead")
        << FixItHint::CreateReplacement(Literal->getSourceRange(),
                                        LiteralBooleanValue ? "true" : "false");
  } else {
    diag(Literal->getLocation(),
         "implicitly converting integer literal to bool");
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
