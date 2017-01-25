//===--- InvalidatedIteratorsCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "InvalidatedIteratorsCheck.h"
#include "../utils/ExprSequence.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Analysis/CFG.h"
#include <iostream>

using namespace clang::ast_matchers;
using namespace clang::tidy::utils;
using namespace clang::ast_type_traits;

namespace clang {
namespace tidy {
namespace misc {

namespace {
const char BlockFuncName[] = "BlockFunc";
const char DeclStmtName[] = "DeclarationStmt";
const char DeclName[] = "Declaration";
const char ExprName[] = "Expr";
const char VectorName[] = "TheVector";

const char CallExprName[] = "CallExpr";
const char CallName[] = "InvalidatingCall";
const char CalledDeclName[] = "FuncDecl";
const char CallArgName[] = "CallArg";
}

void InvalidatedIteratorsCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus)
    return;

  Finder->addMatcher(
      declRefExpr(
          hasDeclaration(
              varDecl(allOf(hasAncestor(functionDecl().bind(BlockFuncName)),
                            hasParent(declStmt().bind(DeclStmtName)),
                            hasType(referenceType()),
                            hasDescendant(declRefExpr(
                                hasType(cxxRecordDecl(hasName("std::vector"))),
                                hasDeclaration(varDecl().bind(VectorName))))))
                  .bind(DeclName)))
          .bind(ExprName),
      this);
}

InvalidatedIteratorsCheck::ExprMatcherType
InvalidatedIteratorsCheck::getModifyingMatcher(const VarDecl *VectorDecl) {
  // An invalidation might occur by directly calling a risky method.
  auto InvalidateMatcher =
      cxxMemberCallExpr(has(memberExpr(hasDeclaration(cxxMethodDecl(hasAnyName(
          "push_back", "emplace_back", "clear",
          "insert", "emplace"))),
                                       has(declRefExpr(hasDeclaration(
                                           varDecl(equalsNode(VectorDecl))))))))
          .bind(CallName);
  // It may also occur by calling a method through a non-const reference.
  auto FuncCallMatcher =
      callExpr(hasDeclaration(functionDecl().bind(CalledDeclName)),
               hasAnyArgument(
                   declRefExpr(hasDeclaration(varDecl(equalsNode(VectorDecl))))
                       .bind(CallArgName)))
          .bind(CallExprName);
  auto ModifyingMatcher = expr(eachOf(InvalidateMatcher, FuncCallMatcher));

  return ModifyingMatcher;
}

bool InvalidatedIteratorsCheck::canFuncInvalidate(const FunctionDecl *Func,
                                                  unsigned ArgId,
                                                  unsigned NumCallArgs,
                                                  ASTContext *Context) {
  // Try to find the result of some previous canFuncInvalidate call
  // which matches our arguments.
  const auto MemoTuple = std::make_tuple(Func, ArgId, NumCallArgs);
  const auto MemoChkIter = CanFuncInvalidateMemo.find(MemoTuple);
  if (MemoChkIter != CanFuncInvalidateMemo.end())
    return MemoChkIter->second;

  // Temporarily insert a negative answer so that recursive functions
  // won't loop our method.
  const auto InsertResult =
      CanFuncInvalidateMemo.insert(std::make_pair(MemoTuple, false));
  assert(InsertResult.second);
  const auto MemoIter = InsertResult.first;

  if (!Func->hasBody()) {
    // We cannot assume anything about the function, not knowing its
    // body; it possibly invalidates our iterators.
    return (MemoIter->second = true);
  }

  if (Func->getNumParams() != NumCallArgs) {
    // Sometimes number of call arguments is not equal to the number
    // of declaration arguments. As far as we know, it happens only when
    // calling some class operators. In this case, there is also an
    // additional *this argument as the first argument of the call.
    // We should get rid of it so as to match the declaration arguments.
    assert(Func->getNumParams() == NumCallArgs - 1 &&
           "Number of function parameters should be equal to number of call "
           "arguments or call arguments minus one");

    if (ArgId == 0) {
      // We don't care about member calls. These should be covered in other
      // matchers.
      return false;
    }
    ArgId--;
  }

  const auto *Arg = Func->parameters()[ArgId];

  // Get all possible invalidations of the container Arg.
  auto ModifyingMatcher = functionDecl(hasDescendant(getModifyingMatcher(Arg)));

  const auto Matches = match(ModifyingMatcher, *Func, *Context);
  for (const auto &Match : Matches) {
    // A dangerous method possibly invalidates iterators.
    const bool IsInvalidatingCall =
        Match.getNodeAs<CXXMemberCallExpr>(CallName) != nullptr;
    if (IsInvalidatingCall)
      return (MemoIter->second = true);

    // A recursive call might invalidate.
    const auto *CallUse = Match.getNodeAs<CallExpr>(CallExprName);
    assert(CallUse);

    if (canCallInvalidate(CallUse, Match, Context))
      return (MemoIter->second = true);
  }
  return false;
}

bool InvalidatedIteratorsCheck::canCallInvalidate(
    const CallExpr *Call, ast_matchers::BoundNodes Match, ASTContext *Context) {
  const auto *FuncArg = Match.getNodeAs<Expr>(CallArgName);
  const auto *FuncDecl = Match.getNodeAs<FunctionDecl>(CalledDeclName);

  // Go through all call arguments and check if any of them matches to
  // our argument and can invalidate iterators pointing to it.
  for (unsigned i = 0; i < Call->getNumArgs(); ++i) {
    const auto *Arg = Call->getArg(i);

    if (Arg == FuncArg &&
        canFuncInvalidate(FuncDecl, i, Call->getNumArgs(), Context))
      return true;
  }

  return false;
}

void InvalidatedIteratorsCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *RefUse = Result.Nodes.getNodeAs<DeclRefExpr>(ExprName);
  const auto *RefDecl = Result.Nodes.getNodeAs<VarDecl>(DeclName);
  const auto *RefDeclStmt = Result.Nodes.getNodeAs<DeclStmt>(DeclStmtName);
  const auto *Func = Result.Nodes.getNodeAs<FunctionDecl>(BlockFuncName);
  const auto *VectorDecl = Result.Nodes.getNodeAs<VarDecl>(VectorName);
  Stmt *FuncBody = Func->getBody();

  CFG::BuildOptions Options;

  auto *Context = Result.Context;

  // Construct a CFG and ExprSequence so that it is possible to find possible
  // sequences of operations.
  const std::unique_ptr<CFG> TheCFG =
      CFG::buildCFG(nullptr, FuncBody, Result.Context, Options);
  const std::unique_ptr<StmtToBlockMap> BlockMap(
      new StmtToBlockMap(TheCFG.get(), Result.Context));
  const std::unique_ptr<ExprSequence> Sequence(
      new ExprSequence(TheCFG.get(), Result.Context));
  const CFGBlock *Block = BlockMap->blockContainingStmt(RefUse);
  if (!Block) {
    return;
  }

  auto ModifyingMatcher = getModifyingMatcher(VectorDecl);

  for (const auto &BlockElem : *Block) {
    // In each block, find all possible invalidating operations and process
    // all of them.
    const auto StmtPtr = BlockElem.getAs<CFGStmt>();
    if (!StmtPtr.hasValue())
      continue;
    auto Results = match(ModifyingMatcher, *(StmtPtr->getStmt()), *Context);

    for (const auto &Match : Results) {
      const auto *InvalidatingUse =
          Match.getNodeAs<CXXMemberCallExpr>(CallName);
      const auto *CallUse = Match.getNodeAs<CallExpr>(CallExprName);

      // If the candidate invalidating operation is calling the function,
      // it should be able to invalidate.
      if (CallUse && !canCallInvalidate(CallUse, Match, Context))
        continue;

      const Expr *Use = (InvalidatingUse != nullptr ? cast<Expr>(InvalidatingUse)
                                                    : cast<Expr>(CallUse));

      // The incorrect order of operations is:
      //   1. Declare a reference.
      //   2. Make a possibly invalidating use of the container.
      //   3. Use the reference.
      if (Sequence->potentiallyAfter(Use, RefDeclStmt) &&
          Sequence->potentiallyAfter(RefUse, Use)) {
        diag(RefUse->getLocation(), "%0 might be invalidated before the access")
            << RefDecl;
        // TODO: add a whole possible chain of invalidations.
        diag(Use->getExprLoc(), "possible place of invalidation",
             DiagnosticIDs::Note);
        return;
      }
    }
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
