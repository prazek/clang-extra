//===--- UnnecessaryMutableCheck.cpp - clang-tidy--------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "UnnecessaryMutableCheck.h"
#include "../utils/LexerUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ParentMap.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void UnnecessaryMutableCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      fieldDecl(anyOf(isPrivate(), allOf(isProtected(),
                                         hasParent(cxxRecordDecl(isFinal())))),
                unless(anyOf(isImplicit(), isInstantiated(),
                             hasParent(cxxRecordDecl(isUnion())))))
          .bind("field"),
      this);
}

class FieldUseVisitor : public RecursiveASTVisitor<FieldUseVisitor> {
public:
  explicit FieldUseVisitor(FieldDecl *SoughtField)
      : SoughtField(SoughtField), FoundNonConstUse(false) {}

  void RunSearch(Decl *Declaration) {
    auto *Body = Declaration->getBody();
    ParMap = new ParentMap(Body);
    TraverseStmt(Body);
    delete ParMap;
  }

  bool VisitExpr(Expr *GenericExpr) {
    MemberExpr *Expression = dyn_cast<MemberExpr>(GenericExpr);
    if (Expression == nullptr)
      return true;

    if (Expression->getMemberDecl() != SoughtField)
      return true;

    // Check if expr is a member of const thing.
    bool IsConstObj = false;
    for (auto *ChildStmt : Expression->children()) {
      Expr *ChildExpr = dyn_cast<Expr>(ChildStmt);
      if (ChildExpr) {
        IsConstObj |= ChildExpr->getType().isConstQualified();

        // If member expression dereferences, we need to check
        // whether pointer type is const.
        if (Expression->isArrow()) {
          auto &WrappedType = *ChildExpr->getType();
          if (!WrappedType.isPointerType()) {
            // It's something weird. Just to be sure, assume we're const.
            IsConstObj = true;
          } else {
            IsConstObj |= WrappedType.getPointeeType().isConstQualified();
          }
        }
      }
    }

    // If it's not, mutableness changes nothing.
    if (!IsConstObj)
      return true;

    // By a const operation on a member expression we mean a MemberExpr
    // whose parent is ImplicitCastExpr to rvalue or something constant.
    bool HasRvalueCast = false;
    bool HasConstCast = false;
    if (ParMap->hasParent(Expression)) {
      const auto *Cast =
          dyn_cast<ImplicitCastExpr>(ParMap->getParent(Expression));
      if (Cast != nullptr) {
        HasRvalueCast = Cast->getCastKind() == CastKind::CK_LValueToRValue;
        HasConstCast = Cast->getType().isConstQualified();
      }
    }

    if (!HasRvalueCast && !HasConstCast) {
      FoundNonConstUse = true;
      return false;
    }

    return true;
  }

  bool IsNonConstUseFound() const { return FoundNonConstUse; }

private:
  FieldDecl *SoughtField;
  ParentMap *ParMap;
  bool FoundNonConstUse;
};

class ClassMethodVisitor : public RecursiveASTVisitor<ClassMethodVisitor> {
public:
  ClassMethodVisitor(ASTContext &Context, FieldDecl *SoughtField)
      : SM(Context.getSourceManager()), SoughtField(SoughtField),
        NecessaryMutable(false) {}

  bool VisitDecl(Decl *GenericDecl) {
    // As for now, we can't track friends.
    if (dyn_cast<FriendDecl>(GenericDecl)) {
      NecessaryMutable = true;
      return false;
    }

    FunctionDecl *Declaration = dyn_cast<FunctionDecl>(GenericDecl);

    if (Declaration == nullptr)
      return true;

    // All decls need their definitions in main file.
    if (!Declaration->hasBody() ||
        !SM.isInMainFile(Declaration->getBody()->getLocStart())) {
      NecessaryMutable = true;
      return false;
    }

    FieldUseVisitor FieldVisitor(SoughtField);
    FieldVisitor.RunSearch(Declaration);
    if (FieldVisitor.IsNonConstUseFound()) {
      NecessaryMutable = true;
      return false;
    }

    return true;
  }

  bool IsMutableNecessary() const { return NecessaryMutable; }

private:
  SourceManager &SM;
  FieldDecl *SoughtField;
  bool NecessaryMutable;
};

// Checks if 'mutable' keyword can be removed; for now; we do it only if
// it is the only declaration in a declaration chain.
static bool CheckRemoval(SourceManager &SM, const SourceLocation &LocStart,
                         const SourceLocation &LocEnd, ASTContext &Context,
                         SourceRange &ResultRange) {

  FileID FID = SM.getFileID(LocEnd);
  llvm::MemoryBuffer *Buffer = SM.getBuffer(FID, LocEnd);
  Lexer DeclLexer(SM.getLocForStartOfFile(FID), Context.getLangOpts(),
                  Buffer->getBufferStart(), SM.getCharacterData(LocStart),
                  Buffer->getBufferEnd());

  Token DeclToken;
  bool result = false;

  while (!DeclLexer.LexFromRawLexer(DeclToken)) {

    if (DeclToken.getKind() == tok::TokenKind::semi) {
      break;
    }

    if (DeclToken.getKind() == tok::TokenKind::comma) {
      return false;
    }

    if (DeclToken.isOneOf(tok::TokenKind::identifier,
                          tok::TokenKind::raw_identifier)) {
      auto TokenStr = DeclToken.getRawIdentifier().str();

      // "mutable" cannot be used in any other way than to mark mutableness
      if (TokenStr == "mutable") {
        ResultRange =
            SourceRange(DeclToken.getLocation(), DeclToken.getEndLoc());
        result = true;
      }
    }
  }

  assert(result && "No mutable found, weird");

  return result;
}

void UnnecessaryMutableCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MD = Result.Nodes.getNodeAs<FieldDecl>("field");
  const auto *ClassMatch = dyn_cast<CXXRecordDecl>(MD->getParent());
  auto &Context = *Result.Context;
  auto &SM = *Result.SourceManager;

  if (!MD->getDeclName() || ClassMatch->isDependentContext() ||
      !MD->isMutable())
    return;

  ClassMethodVisitor Visitor(Context, const_cast<FieldDecl *>(MD));
  Visitor.TraverseDecl(const_cast<CXXRecordDecl *>(ClassMatch));

  if (Visitor.IsMutableNecessary())
    return;

  auto Diag =
      diag(MD->getLocation(), "'mutable' modifier is unnecessary for field %0")
      << MD->getDeclName();

  SourceRange RemovalRange;

  if (CheckRemoval(SM, MD->getLocStart(), MD->getLocEnd(), Context,
                   RemovalRange)) {
    Diag << FixItHint::CreateRemoval(RemovalRange);
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
