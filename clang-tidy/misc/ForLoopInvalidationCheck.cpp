//===--- ForLoopInvalidationCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ForLoopInvalidationCheck.h"
#include "../utils/ExprSequence.h"
#include "../utils/OptionsUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Analysis/CFG.h"
#include "clang/Analysis/Analyses/ReachableCode.h"

using namespace clang::ast_matchers;
using namespace clang::tidy::utils;
using namespace clang::ast_type_traits;

namespace clang {
namespace tidy {
namespace misc {

namespace {

const char CalledObject[] = "calledObject";
const char MethodDeclaration[] = "methodDeclaration";
const char MethodCall[] = "methodCall";
const char ForStatement[] = "forRange";
const char FunctionDeclaration[] = "functionDecl";
const char DefaultAllowedTypes[] =
    "::std::list; ::std::forward_list";

bool HaveEqualNames(const CXXMethodDecl *First, const CXXMethodDecl *Second) {
  return First->getQualifiedNameAsString() ==
         Second->getQualifiedNameAsString();
}

bool HaveEqualParametersList(const CXXMethodDecl *First,
                             const CXXMethodDecl *Second) {
  if (First->param_size() != Second->param_size())
    return false;

  for (size_t i = 0; i < First->param_size(); i++)
    if (First->parameters()[i]->getType() != Second->parameters()[i]->getType())
      return false;

  return true;
};

/// \brief Returns true if given method have alternative which is const.
bool HaveEquivalentConstMethod(const CXXMethodDecl *Method) {
  const auto *Class = Method->getParent();
  for (const auto *M : Class->methods()) {
    if (M != Method && M->isConst() && HaveEqualNames(Method, M) &&
        HaveEqualParametersList(Method, M))
      return true;
  }
  return false;
};

} // namespace

ForLoopInvalidationCheck::ForLoopInvalidationCheck(StringRef Name,
                                                   ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      AllowedTypes(utils::options::parseStringList(
          Options.get("AllowedTypes", DefaultAllowedTypes))) {}

void ForLoopInvalidationCheck::registerMatchers(MatchFinder *Finder) {
  auto AllowedTypesMatcher = hasAnyName(
      SmallVector<StringRef, 5>(AllowedTypes.begin(), AllowedTypes.end()));
  auto CalledObjectRef =
      declRefExpr(to(varDecl(hasType(qualType(hasDeclaration(
                                 namedDecl(unless(AllowedTypesMatcher))))))
                         .bind(CalledObject)));
  auto MethodMatcher = cxxMethodDecl(unless(isConst())).bind(MethodDeclaration);

  Finder->addMatcher(
      callExpr(anyOf(cxxMemberCallExpr(on(CalledObjectRef)),
                     cxxOperatorCallExpr(hasArgument(0, CalledObjectRef))),
               callee(MethodMatcher),
               hasAncestor(cxxForRangeStmt(hasRangeInit(
                   declRefExpr(to(varDecl(equalsBoundNode(CalledObject))))))
               .bind(ForStatement)),
               hasAncestor(functionDecl().bind(FunctionDeclaration)))
          .bind(MethodCall),
      this);
}

void ForLoopInvalidationCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MethodDecl =
      Result.Nodes.getNodeAs<CXXMethodDecl>(MethodDeclaration);
  const auto *MatchedExpression = Result.Nodes.getNodeAs<CallExpr>(MethodCall);
  const auto *MatchedForStatement = Result.Nodes.getNodeAs<CXXForRangeStmt>(ForStatement);
  const auto *MatchedFunction = Result.Nodes.getNodeAs<FunctionDecl>(FunctionDeclaration);

  if (HaveEquivalentConstMethod(MethodDecl))
    return;

  CFG::BuildOptions Options;

  Stmt* ForStmt = const_cast<CXXForRangeStmt*>(MatchedForStatement);

  // Construct a CFG and ExprSequence so that it is possible to find possible
  // sequences of operations.
  const std::unique_ptr<CFG> TheCFG =
      CFG::buildCFG(nullptr, ForStmt, Result.Context, Options);
  const std::unique_ptr<StmtToBlockMap> BlockMap(
      new StmtToBlockMap(TheCFG.get(), Result.Context));

  auto *CallCFGBlock = BlockMap->blockContainingStmt(MatchedExpression);
  auto *IncCFGBlock = BlockMap->blockContainingStmt(MatchedForStatement->getInc());

  llvm::BitVector Reachable(TheCFG->getNumBlockIDs());
  reachable_code::ScanReachableFromBlock(CallCFGBlock, Reachable);

  if (Reachable[IncCFGBlock->getBlockID()]) {
      diag(MatchedExpression->getLocStart(),
           "this call may lead to iterator invalidation");
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
