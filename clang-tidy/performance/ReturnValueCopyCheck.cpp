//===--- ReturnValueCopyCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReturnValueCopyCheck.h"
#include "../utils/Matchers.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
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

/// \brief Matches ParmVarDecls which have default arguments.
AST_MATCHER(ParmVarDecl, hasDefaultArgument) { return Node.hasDefaultArg(); }

/// \brief Matches function declarations which have all arguments defaulted
/// except first.
AST_MATCHER_FUNCTION(ast_matchers::internal::Matcher<FunctionDecl>,
                     hasOneActiveArgument) {
  return anyOf(parameterCountIs(1),
               allOf(unless(parameterCountIs(0)),
                     hasParameter(1, hasDefaultArgument())));
}

/// \brief Matches declarations of template type which matches the given
/// matcher.
AST_MATCHER_P(TemplateTypeParmDecl, hasTemplateType,
              ast_matchers::internal::Matcher<QualType>, InnerMatcher) {
  const QualType TemplateType =
      Node.getTypeForDecl()->getCanonicalTypeInternal();
  return InnerMatcher.matches(TemplateType, Finder, Builder);
}

/// \brief Matches constructor declarations which are templates and can be used
/// to move-construct from any type.
AST_MATCHER_FUNCTION(ast_matchers::internal::Matcher<CXXConstructorDecl>,
                     moveConstructorFromAnyType) {
  // Potentially danger: this matcher binds a name, with probably
  // mean that you cant use it twice in your check!
  const char TemplateArgument[] = "templateArgument";
  return cxxConstructorDecl(
      hasParent(functionTemplateDecl(has(templateTypeParmDecl(
          hasTemplateType(qualType().bind(TemplateArgument)))))),
      hasOneActiveArgument(),
      hasParameter(
          0, hasType(hasCanonicalType(allOf(
                 rValueReferenceType(),
                 ignoringRefsAndConsts(equalsBoundNode(TemplateArgument)))))));
}

/// \brief Matches to qual types which are cxx records and has constructors that
/// matches the given matcher.
AST_MATCHER_FUNCTION_P(ast_matchers::internal::Matcher<QualType>,
                       hasConstructor,
                       ast_matchers::internal::Matcher<CXXConstructorDecl>,
                       InnerMatcher) {
  return hasDeclaration(
      cxxRecordDecl(hasMethod(cxxConstructorDecl(InnerMatcher))));
}

/// \brief Matches to qual types which has constructors from type that matches
/// the given matcher.
AST_MATCHER_FUNCTION_P(ast_matchers::internal::Matcher<QualType>,
                       isConstructibleFromType,
                       ast_matchers::internal::Matcher<QualType>,
                       InnerMatcher) {
  // FIXME: Consider conversion operators...
  auto ConstructorMatcher = cxxConstructorDecl(
      unless(isDeleted()), hasOneActiveArgument(),
      hasParameter(0, hasType(hasCanonicalType(qualType(InnerMatcher)))));

  return hasConstructor(ConstructorMatcher);
}

} // namespace

void ReturnValueCopyCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus11)
    return;

  // Matches to type with after ignoring refs and consts is the same as
  // "constructedType".
  auto HasTypeSameAsConstructed = hasType(hasCanonicalType(
      ignoringRefsAndConsts(equalsBoundNode("constructedType"))));

  auto HasRvalueReferenceType =
      hasType(hasCanonicalType(qualType(rValueReferenceType())));

  auto RefOrConstVarDecl = varDecl(hasType(
      hasCanonicalType(qualType(anyOf(referenceType(), isConstQualified())))));

  // Matches to expression expression that have declaration with is reference or
  // const.
  auto IsDeclaredAsRefOrConstType =
      anyOf(hasDescendant(declRefExpr(to(RefOrConstVarDecl))),
            declRefExpr(to(RefOrConstVarDecl)));

  // Constructor parameter must not already be rreference
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

  // Matches to type constructible from argumentCanonicalType type
  // * by r reference or
  // * by value, and argumentCanonicalType is move constructible
  auto IsMoveOrCopyConstructibleFromParam = isConstructibleFromType(anyOf(
      allOf(rValueReferenceType(),
            ignoringRefsAndConsts(equalsBoundNode("argumentCanonicalType"))),
      allOf(equalsBoundNode("argumentCanonicalType"),
            hasConstructor(isMoveConstructor()))));

  // Matches to type that has template constructor which is
  // move constructor from any type (like boost::any).
  auto IsCopyConstructibleFromParamViaTemplate =
      hasConstructor(moveConstructorFromAnyType());

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
          hasType(qualType().bind("constructedType")), argumentCountIs(1),
          unless(has(ignoringParenImpCasts(
              cxxConstructExpr(ctorCallee(isMoveConstructor()))))),
          hasArgument(0, hasType(hasCanonicalType(ignoringRefsAndConsts(
                             qualType().bind("argumentCanonicalType"))))),
          ctorCallee(hasParameter(0, ParameterMatcher)),
          hasArgument(0, ArgumentMatcher),
          hasArgument(0, expr().bind("argument")),
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

  const auto *Argument = Result.Nodes.getNodeAs<Expr>("argument");
  assert(Argument != nullptr);

  const auto *Type = Result.Nodes.getNodeAs<QualType>("constructedType");
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
