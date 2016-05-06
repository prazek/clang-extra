//===--- ReturningTypeCheck.cpp - clang-tidy-------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReturningTypeCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include <iostream>
#include <utility>

using namespace clang::ast_matchers;
using namespace llvm;

namespace clang {
namespace tidy {
namespace performance {

// TODO: Where should I add hasCannonicalType?

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

/// \brief Matches if the matched type after removing const and referencec
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
                     haveOneActiveArgument) {
  return anyOf(parameterCountIs(1),
               allOf(unless(parameterCountIs(0)),
                     hasParameter(2, hasDefaultArgument())));
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
  // How is working binding in matcher? Is it global or matcher-local?
  // (If it is global, that should be rewritten!)
  const char TemplateArgument[] = "templateArgument";
  return cxxConstructorDecl(
      hasParent(functionTemplateDecl(has(templateTypeParmDecl(
          hasTemplateType(qualType().bind(TemplateArgument)))))),
      haveOneActiveArgument(),
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
      cxxRecordDecl(hasDescendant(cxxConstructorDecl(InnerMatcher))));
}

/// \brief
AST_MATCHER_FUNCTION_P(ast_matchers::internal::Matcher<QualType>,
                       hasConstructorFromType,
                       ast_matchers::internal::Matcher<QualType>,
                       InnerMatcher) {
  auto ConstructorMatcher = cxxConstructorDecl(
      unless(isDeleted()), haveOneActiveArgument(),
      hasParameter(
          0, hasType(qualType(hasCanonicalType(qualType(InnerMatcher))))));

  return hasConstructor(ConstructorMatcher);
}

} // namespace

void ReturningTypeCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus11)
    return;

  // Has it type same as "constructedType" ignoring refs and consts?
  auto HasTypeSameAsConstructed = hasType(qualType(hasCanonicalType(
      ignoringRefsAndConsts(equalsBoundNode("constructedType")))));

  auto HasRvalueReferenceType = hasType(qualType(rValueReferenceType()));

  auto RefOrConstVarDecl =
      varDecl(hasType(qualType(anyOf(referenceType(), isConstQualified()))));

  // Does this expression have declaration with is reference or const?
  auto IsDeclaredAsRefOrConstType =
      anyOf(hasDescendant(declRefExpr(to(RefOrConstVarDecl))),
            declRefExpr(to(RefOrConstVarDecl)));

  auto ConstructorParameterMatcher =
      hasType(qualType(unless(rValueReferenceType())));

  auto ConstructorArgumentMatcher =
      unless(anyOf(HasTypeSameAsConstructed, HasRvalueReferenceType,
                   IsDeclaredAsRefOrConstType, cxxTemporaryObjectExpr(),
                   hasDescendant(cxxTemporaryObjectExpr())));

  // Is this QualType constructible from argumentCannonicalType type
  // * by r reference or
  // * by value, and argumentCannonicalType is move constructible?
  auto IsMoveOrCopyConstructibleFromParam = hasConstructorFromType(anyOf(
      allOf(rValueReferenceType(),
            ignoringRefsAndConsts(equalsBoundNode("argumentCanonicalType"))),
      allOf(equalsBoundNode("argumentCanonicalType"),
            hasConstructor(isMoveConstructor()))));

  // Does this type has template constructor which is move constructor
  // from any type (like boost::any?)
  auto IsPotentiallyCopyConstructibleFromParam =
      hasConstructor(moveConstructorFromAnyType());

  auto ConstructExprMatcher =
      cxxConstructExpr(
          hasType(qualType().bind("constructedType")), argumentCountIs(1),
          hasArgument(0,
                      hasType(qualType(hasCanonicalType(ignoringRefsAndConsts(
                          qualType().bind("argumentCanonicalType")))))),
          ctorCallee(hasParameter(0, ConstructorParameterMatcher)),
          hasArgument(0, ConstructorArgumentMatcher),
          hasArgument(0, expr().bind("argument")),
          hasType(qualType(anyOf(IsMoveOrCopyConstructibleFromParam,
                                 IsPotentiallyCopyConstructibleFromParam))))
          .bind("construct");

  auto IsCopyOrMoveConstructor =
      anyOf(isCopyConstructor(), isMoveConstructor());

  Finder->addMatcher(
      returnStmt(has(ignoringParenImpCasts(cxxConstructExpr(
                     ctorCallee(IsCopyOrMoveConstructor),
                     has(ignoringParenImpCasts(ConstructExprMatcher))))))
          .bind("return"),
      this);
}

ReturningTypeCheck::ReturningTypeCheck(StringRef Name,
                                       ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      IncludeStyle(utils::IncludeSorter::parseIncludeStyle(
          Options.get("IncludeStyle", "llvm"))) {}

void ReturningTypeCheck::registerPPCallbacks(CompilerInstance &Compiler) {
  // Only register the preprocessor callbacks for C++; the functionality
  // currently does not provide any benefit to other languages, despite being
  // benign.
  if (getLangOpts().CPlusPlus11) {
    Inserter.reset(new utils::IncludeInserter(
        Compiler.getSourceManager(), Compiler.getLangOpts(), IncludeStyle));
    Compiler.getPreprocessor().addPPCallbacks(Inserter->CreatePPCallbacks());
  }
}

void ReturningTypeCheck::check(const MatchFinder::MatchResult &Result) {
  const LangOptions &Opts = Result.Context->getLangOpts();
  
  const auto *Argument = Result.Nodes.getNodeAs<Expr>("argument");
  assert(Argument != nullptr);

  const auto *Type = Result.Nodes.getNodeAs<QualType>("constructedType");
  assert(Type != nullptr);

  // TODO: Is it doing the same check as above or not?
  assert(!Type->isNull());

  std::string ReplacementText = Lexer::getSourceText(
      CharSourceRange::getTokenRange(Argument->getSourceRange()),
      *Result.SourceManager, Opts);

  ReplacementText =
      Type->getAsString(getLangOpts()) + "(std::move(" + ReplacementText + "))";

  auto Diag = diag(Argument->getExprLoc(),
                   "expression could be wrapped with std::move");
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
