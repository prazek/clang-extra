//===--- BoolToIntegerConversionCheck.cpp - clang-tidy---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "BoolToIntegerConversionCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace bugprone {

AST_MATCHER(FieldDecl, isOneBitBitField) {
  return Node.isBitField() &&
         Node.getBitWidthValue(Finder->getASTContext()) == 1;
}

void BoolToIntegerConversionCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus)
    return;

  auto convertingToOneBitBitfieldInInitializer = hasAncestor(
      cxxConstructorDecl(hasAnyConstructorInitializer(cxxCtorInitializer(
          forField(isOneBitBitField()),
          withInitializer(implicitCastExpr(equalsBoundNode("cast")))))));

  auto convertingToOneBitBitfieldByOperator =
      hasParent(binaryOperator(hasLHS(memberExpr(
          hasDeclaration(fieldDecl(isOneBitBitField()).bind("bitfield"))))));

  auto boolOperand = expr(ignoringImpCasts(hasType(booleanType())));
  auto comparingWithBool =
      hasParent(binaryOperator(hasLHS(boolOperand), hasRHS(boolOperand)));

  auto isInReturnStmt = hasParent(returnStmt());
  auto forTheSameFunction = forFunction(equalsBoundNode("function"));
  auto returnNotBool = returnStmt(
      forTheSameFunction,
      unless(has(ignoringParenImpCasts(expr(hasType(booleanType()))))));
  auto allReturnsBool = hasBody(unless(hasDescendant(returnNotBool)));

  auto soughtCast =
      implicitCastExpr(has(cxxBoolLiteral().bind("bool_literal"))).bind("cast");

  Finder->addMatcher(
      stmt(allOf(soughtCast,
                 unless(anyOf(
                     convertingToOneBitBitfieldByOperator,
                     convertingToOneBitBitfieldInInitializer, comparingWithBool,
                     allOf(isInReturnStmt,
                           forFunction(functionDecl().bind("function")),
                           forFunction(allReturnsBool)))))),
      this);

  Finder->addMatcher(
      decl(functionDecl().bind("function"),
           functionDecl(allReturnsBool,
                        hasDescendant(returnStmt(has(expr().bind("return")),
                                                 forTheSameFunction)),
                        unless(returns(booleanType())))),
      this);
}

void BoolToIntegerConversionCheck::check(
    const MatchFinder::MatchResult &Result) {

  if (const auto *Function = Result.Nodes.getNodeAs<FunctionDecl>("function")) {
    Function->dumpColor();
    const auto *Return = Result.Nodes.getNodeAs<Expr>("return");
    return changeFunctionReturnType(*Function, *Return);
  }

  const auto *BoolLiteral =
      Result.Nodes.getNodeAs<CXXBoolLiteralExpr>("bool_literal");
  const auto *Cast = Result.Nodes.getNodeAs<ImplicitCastExpr>("cast");
  const auto Type = Cast->getType().getLocalUnqualifiedType();
  auto Location = BoolLiteral->getLocation();

  auto Diag = diag(Location, "implicitly converting bool literal to %0; use "
                             "integer literal instead")
              << Type;

  if (!Location.isMacroID())
    Diag << FixItHint::CreateReplacement(BoolLiteral->getSourceRange(),
                                         BoolLiteral->getValue() ? "1" : "0");
}

void BoolToIntegerConversionCheck::changeFunctionReturnType(
    const FunctionDecl &FD, const Expr &Return) {

  diag(FD.getLocation(), "function has return type %0 but returns only bools")
      << FD.getReturnType();

  auto Diag = diag(Return.getExprLoc(), "converting bool to %0 here",
                   DiagnosticIDs::Level::Note)
              << FD.getReturnType();

  if (auto *MD = dyn_cast<CXXMethodDecl>(&FD)) {
    // We skip functions that are virtual, because we would have to be sure that
    // all other overriden functions can be changed
    if (MD->isVirtual())
      return;
  }

  // Abort if there is redeclaration in macro
  for (const auto &Redecl : FD.redecls())
    if (Redecl->getLocation().isMacroID())
      return;

  // Change type for all declarations.
  for (const auto &Redecl : FD.redecls())
    Diag << FixItHint::CreateReplacement(Redecl->getReturnTypeSourceRange(),
                                         "bool");
}

} // namespace bugprone
} // namespace tidy
} // namespace clang
