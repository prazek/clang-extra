//===--- ForLoopIteratorInvalidationCheck.cpp - clang-tidy-----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ForLoopIteratorInvalidationCheck.h"
#include "../utils/ExprSequence.h"
#include "../utils/OptionsUtils.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Analysis/Analyses/ReachableCode.h"
#include "clang/Analysis/CFG.h"

using namespace clang::ast_matchers;
using namespace clang::tidy::utils;

namespace clang {
namespace tidy {
namespace misc {

namespace {

const char CalledObject[] = "calledObject";
const char MethodDeclaration[] = "methodDeclaration";
const char MethodCall[] = "methodCall";
const char ForStatement[] = "forRange";
const char FunctionDeclaration[] = "functionDecl";
const char DefaultSafeTypes[] = "::std::list; ::std::forward_list";

/// \brief Checks if two methods (or overloaded operators) methods of the same
/// class and have the same names.
bool haveEqualNames(const CXXMethodDecl *First, const CXXMethodDecl *Second) {
  return First->getQualifiedNameAsString() ==
         Second->getQualifiedNameAsString();
}

/// \brief Checks if two functions have same parameter lists with regard to
/// types.
bool haveSameParameters(const CXXMethodDecl *First,
                        const CXXMethodDecl *Second) {
  if (First->param_size() != Second->param_size())
    return false;

  for (size_t i = 0; i < First->param_size(); i++)
    if (First->parameters()[i]->getType() != Second->parameters()[i]->getType())
      return false;

  return true;
};

/// \brief Checks if class of given non-const method have method with the same
/// name and parameters but const.
bool hasEquivalentConstSubstitute(const CXXMethodDecl *Method) {
  const auto *Class = Method->getParent();
  for (const auto *M : Class->methods()) {
    if (M != Method && M->isConst() && haveEqualNames(Method, M) &&
        haveSameParameters(Method, M))
      return true;
  }
  return false;
};

} // namespace

ForLoopIteratorInvalidationCheck::ForLoopIteratorInvalidationCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      SafeTypes(utils::options::parseStringList(
          Options.get("SafeTypes", DefaultSafeTypes))) {}

void ForLoopIteratorInvalidationCheck::registerMatchers(MatchFinder *Finder) {
  auto SafeTypesMatcher = hasDeclaration(namedDecl(unless(hasAnyName(
      SmallVector<StringRef, 5>(SafeTypes.begin(), SafeTypes.end())))));
  auto CalledObjectRef = declRefExpr(to(
      varDecl(hasType(qualType(anyOf(
                  SafeTypesMatcher, referenceType(pointee(SafeTypesMatcher))))))
          .bind(CalledObject)));
  auto MethodMatcher = cxxMethodDecl(unless(isConst())).bind(MethodDeclaration);

  Finder->addMatcher(
      callExpr(
          anyOf(cxxMemberCallExpr(on(CalledObjectRef)),
                cxxOperatorCallExpr(hasArgument(0, CalledObjectRef))),
          callee(MethodMatcher),
          hasAncestor(cxxForRangeStmt(hasRangeInit(declRefExpr(to(varDecl(
                                          equalsBoundNode(CalledObject))))))
                          .bind(ForStatement)),
          hasAncestor(functionDecl().bind(FunctionDeclaration)))
          .bind(MethodCall),
      this);
}

void ForLoopIteratorInvalidationCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *MatchedExpression = Result.Nodes.getNodeAs<CallExpr>(MethodCall);
  const auto *MatchedForStatement =
      Result.Nodes.getNodeAs<CXXForRangeStmt>(ForStatement);
  const auto *MethodDecl =
      Result.Nodes.getNodeAs<CXXMethodDecl>(MethodDeclaration);

  // In general, it's hard to tell if method call will invalidate container
  // iterators because it's implementation dependent. Listing all unsafe pairs
  // (container, method) is exhausting task and do not generalize good to
  // user-defined containers. Listing all unsafe containers would be better
  // but would lead to more false positives - std::vector has both safe and
  // unsafe methods.
  // To solve this problem we apply following heuristics:
  // We say that function is likely to invalidate iterators if this function
  // is not const and container doesn't have function with the same name, same
  // parameters (except 'this' parameter) but marked as const.
  // Together with configurable SafeTypes list this check has reasonable number
  // of false positives and automatically generalizes to user-defined
  // containers.
  if (hasEquivalentConstSubstitute(MethodDecl))
    return;

  CFG::BuildOptions Options;

  // buildCFG takes Stmt* instead of const Stmt* so we need to const_cast
  // TODO remove this cast and pass MatchedForStatement to buildCFG instead when
  // buildCFG will be taking const Stmt*.
  auto *ForStmt = const_cast<CXXForRangeStmt *>(MatchedForStatement);

  // Construct a CFG and ExprSequence to find possible sequences of operations.
  const std::unique_ptr<CFG> TheCFG =
      CFG::buildCFG(nullptr, ForStmt, Result.Context, Options);
  StmtToBlockMap BlockMap(TheCFG.get(), Result.Context);

  auto *CallCFGBlock = BlockMap.blockContainingStmt(MatchedExpression);
  auto *ForIncCFGBlock =
      BlockMap.blockContainingStmt(MatchedForStatement->getInc());

  llvm::BitVector Reachable(TheCFG->getNumBlockIDs());
  reachable_code::ScanReachableFromBlock(CallCFGBlock, Reachable);

  if (Reachable[ForIncCFGBlock->getBlockID()]) {
    diag(MatchedExpression->getLocStart(),
         "this call may lead to iterator invalidation");
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
