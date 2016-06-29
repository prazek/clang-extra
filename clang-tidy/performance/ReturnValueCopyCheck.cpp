//===--- ReturnValueCopyCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReturnValueCopyCheck.h"
#include "clang/AST/ASTContext.h"
#include "../utils/Matchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang::ast_matchers;
using namespace llvm;
using namespace clang::tidy::matchers;

namespace clang {
namespace tidy {
namespace performance {

namespace {

const char TemplateArgumentName[] = "templateArgument";
const char ConstructedTypeName[] = "constructedType";
const char ConstructorArgumentName[] = "argument";
const char ArgumentCanonicalTypeName[] = "argumentCanonicalType";

/// \brief Matches if the construct expression's callee's declaration matches
/// the given matcher.
///
/// Given
/// \code
///   // POD types are trivially move constructible.
///   struct Foo {
///     Foo() = default;
///     Foo(int i) { };
///   };
///
///   Foo a;
///   Foo b(1);
///
/// \endcode
/// ctorCallee(parameterCountIs(1))
///   matches "b(1)".
AST_MATCHER_P(CXXConstructExpr, ctorCallee,
              ast_matchers::internal::Matcher<CXXConstructorDecl>,
              InnerMatcher) {
  const CXXConstructorDecl *CtorDecl = Node.getConstructor();
  return (CtorDecl != nullptr &&
          InnerMatcher.matches(*CtorDecl, Finder, Builder));
}

/// \brief Matches QualType which after removing const and reference
/// matches the given matcher.
AST_MATCHER_P(QualType, ignoringRefsAndConsts,
              ast_matchers::internal::Matcher<QualType>, InnerMatcher) {
  if (Node.isNull())
    return false;

  QualType Unqualified = Node.getNonReferenceType();
  Unqualified.removeLocalConst();
  return InnerMatcher.matches(Unqualified, Finder, Builder);
}

/// \brief Matches ParmVarDecl which has default argument.
AST_MATCHER(ParmVarDecl, hasDefaultArg) { return Node.hasDefaultArg(); }

/// \brief Matches function declaration which has all parameters defaulted
/// except first.
AST_MATCHER_FUNCTION(ast_matchers::internal::Matcher<FunctionDecl>,
                     hasOneNonDefaultedParam) {
  return anyOf(parameterCountIs(1), allOf(unless(parameterCountIs(0)),
                                          hasParameter(1, hasDefaultArg())));
}

/// \brief Matches declaration of template type which matches the given
/// matcher.
AST_MATCHER_P(TemplateTypeParmDecl, hasTemplateType,
              ast_matchers::internal::Matcher<QualType>, InnerMatcher) {
  const QualType TemplateType =
      Node.getTypeForDecl()->getCanonicalTypeInternal();
  return InnerMatcher.matches(TemplateType, Finder, Builder);
}

/// \brief Matches a QualType which is CXXRecord and whose declaration has
/// a constructor that matches the given matcher.
AST_MATCHER_FUNCTION_P(ast_matchers::internal::Matcher<QualType>,
                       hasConstructor,
                       ast_matchers::internal::Matcher<CXXConstructorDecl>,
                       InnerMatcher) {
  auto CtorMatcher = cxxConstructorDecl(InnerMatcher);
  return hasDeclaration(cxxRecordDecl(
      anyOf(has(CtorMatcher), has(functionTemplateDecl(has(CtorMatcher))))));
}

/// \brief Matches a QualType whose declaration has a converting constructor
/// accepting an argument of the type from the given matcher.
AST_MATCHER_FUNCTION_P(ast_matchers::internal::Matcher<QualType>,
                       isConstructibleFromType,
                       ast_matchers::internal::Matcher<QualType>,
                       InnerMatcher) {
  // FIXME: Consider conversion operators...
  auto ConstructorMatcher = cxxConstructorDecl(
      unless(isDeleted()), hasOneNonDefaultedParam(),
      hasParameter(0, hasType(hasCanonicalType(qualType(InnerMatcher)))));

  return hasConstructor(ConstructorMatcher);
}

} // namespace

void ReturnValueCopyCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus11)
    return;

  // Matches a QualType which after ignoring refs and consts has the same type
  // as node bound to ConstructedTypeName.
  auto HasTypeSameAsConstructed = hasType(hasCanonicalType(
      ignoringRefsAndConsts(equalsBoundNode(ConstructedTypeName))));

  auto HasRvalueReferenceType =
      hasType(hasCanonicalType(qualType(rValueReferenceType())));

  auto RefOrConstVarDecl = varDecl(hasType(
      hasCanonicalType(qualType(anyOf(referenceType(), isConstQualified())))));

  // Matches a expression whose declaration has type which is const or
  // reference.
  auto IsDeclaredAsRefOrConstType =
      anyOf(hasDescendant(declRefExpr(to(RefOrConstVarDecl))),
            declRefExpr(to(RefOrConstVarDecl)));

  // Constructor parameter must not already be rreference.
  auto ParameterMatcher =
      hasType(hasCanonicalType(qualType(unless(rValueReferenceType()))));

  // Constructor argument must
  // * have different type than constructed type
  // * not be r value reference type
  // * not be declared as const or as reference to other variable
  // * not be temporary object
  // * not be cast from temporary object
  auto ArgumentMatcher =
      unless(anyOf(HasTypeSameAsConstructed, HasRvalueReferenceType,
                   IsDeclaredAsRefOrConstType, cxxTemporaryObjectExpr(),
                   hasDescendant(cxxTemporaryObjectExpr())));

  // Matches a QualType which is constructible from argumentCanonicalType type
  // * by r reference or
  // * by value, and argumentCanonicalType is move constructible
  auto IsMoveOrCopyConstructibleFromParam = isConstructibleFromType(anyOf(
      allOf(rValueReferenceType(),
            ignoringRefsAndConsts(equalsBoundNode(ArgumentCanonicalTypeName))),
      allOf(equalsBoundNode(ArgumentCanonicalTypeName),
            hasConstructor(isMoveConstructor()))));

  // Matches a constructor declaration which is template and can be used
  // to move-construct from any type.
  auto MoveConstructorFromAnyType = cxxConstructorDecl(
      hasParent(functionTemplateDecl(has(templateTypeParmDecl(
          hasTemplateType(qualType().bind(TemplateArgumentName)))))),
      hasOneNonDefaultedParam(),
      hasParameter(
          0, hasType(hasCanonicalType(allOf(
                 rValueReferenceType(),
                 ignoringRefsAndConsts(equalsBoundNode(TemplateArgumentName)))))));

  // Matches a QualType that has template move constructor from any type
  // (like boost::any).
  auto IsCopyConstructibleFromParamViaTemplate =
      hasConstructor(MoveConstructorFromAnyType);

  // Matches construct expr that
  // * has one argument
  // * that argument satisfies ArgumentMatcher
  // * argument is not the result of move constructor
  // * parameter of constructor satisfies ParameterMatcher
  // * constructed type is move constructible from argument
  // ** or is value constructible from argument and argument is movable
  //    constructible
  // ** constructed type has template constructor that can take by rref
  //    (like boost::any)
  auto ConstructExprMatcher =
      cxxConstructExpr(
          hasType(qualType().bind(ConstructedTypeName)), argumentCountIs(1),
          unless(has(ignoringParenImpCasts(
              cxxConstructExpr(ctorCallee(isMoveConstructor()))))),
          hasArgument(0, hasType(hasCanonicalType(ignoringRefsAndConsts(
                             qualType().bind(ArgumentCanonicalTypeName))))),
          ctorCallee(hasParameter(0, ParameterMatcher)),
          hasArgument(0, ArgumentMatcher),
          hasArgument(0, expr().bind(ConstructorArgumentName)),
          hasType(qualType(anyOf(IsMoveOrCopyConstructibleFromParam,
                                 IsCopyConstructibleFromParamViaTemplate))))
          .bind("construct");

  auto IsCopyOrMoveConstructor =
      anyOf(isCopyConstructor(), isMoveConstructor());

  Finder->addMatcher(
      returnStmt(has(ignoringImplicit(cxxConstructExpr(
                     ctorCallee(IsCopyOrMoveConstructor),
                     has(ignoringParenImpCasts(ConstructExprMatcher))))))
          .bind("return"),
      this);
}

ReturnValueCopyCheck::ReturnValueCopyCheck(StringRef Name,
                                           ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      IncludeStyle(utils::IncludeSorter::parseIncludeStyle(
          Options.get("IncludeStyle", "llvm"))) {}

void ReturnValueCopyCheck::registerPPCallbacks(CompilerInstance &Compiler) {
  // Only register the preprocessor callbacks for C++; the functionality
  // currently does not provide any benefit to other languages, despite being
  // benign.
  if (getLangOpts().CPlusPlus11) {
    Inserter.reset(new utils::IncludeInserter(
        Compiler.getSourceManager(), Compiler.getLangOpts(), IncludeStyle));
    Compiler.getPreprocessor().addPPCallbacks(Inserter->CreatePPCallbacks());
  }
}

void ReturnValueCopyCheck::check(const MatchFinder::MatchResult &Result) {
  const LangOptions &Opts = Result.Context->getLangOpts();

  const auto *Argument = Result.Nodes.getNodeAs<Expr>(ConstructorArgumentName);
  assert(Argument != nullptr);

  const auto *Type = Result.Nodes.getNodeAs<QualType>(ConstructedTypeName);
  assert(Type != nullptr);
  assert(!Type->isNull());

  std::string ReplacementText = Lexer::getSourceText(
      CharSourceRange::getTokenRange(Argument->getSourceRange()),
      *Result.SourceManager, Opts);

  ReplacementText =
      Type->getAsString(getLangOpts()) + "(std::move(" + ReplacementText + "))";

  auto Diag = diag(Argument->getExprLoc(),
                   "returned object is not moved; consider wrapping it with "
                   "std::move or changing return type to avoid the copy");
  Diag << FixItHint::CreateReplacement(Argument->getSourceRange(),
                                       ReplacementText);

  if (auto IncludeFixit = Inserter->CreateIncludeInsertion(
          Result.SourceManager->getFileID(Argument->getLocStart()), "utility",
          /*IsAngled=*/true)) {
    Diag << *IncludeFixit;
  }
}

} // namespace performance
} // namespace tidy
} // namespace clang
