//===--- UnnecessaryMutableCheck.cpp - clang-tidy--------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../utils/LexerUtils.h"
#include "UnnecessaryMutableCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ParentMap.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
// TODO: remove all debug things
//#include <iostream>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {


void UnnecessaryMutableCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(fieldDecl(anyOf(isPrivate(), allOf(isProtected(), hasParent(cxxRecordDecl(isFinal())))),
        unless(anyOf(isImplicit(), isInstantiated(), hasParent(cxxRecordDecl(isUnion()))))).bind("field"), this);
}

class FieldUseVisitor : public RecursiveASTVisitor<FieldUseVisitor> {
public:
  explicit FieldUseVisitor(FieldDecl *SoughtField) : SoughtField(SoughtField),
      FoundNonConstUse(false) {}

  void RunSearch(Decl *Declaration) {
    //std::cerr << "New search!" << std::endl;
    //Declaration->dumpColor();
    auto *Body = Declaration->getBody();
    //if (Body) { Body->dumpColor(); }
    //std::cerr << std::endl;
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

    //Expression->dumpColor();
    //Expression->getMemberDecl()->dumpColor();
    
    // By a const operation on a member expression we mean a MemberExpr
    // whose parent is ImplicitCastExpr to rvalue or something constant.
    bool HasRvalueCast = false;
    bool HasConstCast = false;
    if (ParMap->hasParent(Expression)) {
        const auto *Cast = dyn_cast<ImplicitCastExpr>(ParMap->getParent(Expression));
        if (Cast != nullptr) {
          //Cast->dumpColor();
          HasRvalueCast = Cast->getCastKind() == CastKind::CK_LValueToRValue;
          HasConstCast = Cast->getType().isConstQualified();
        }
    }

    //std::cerr << HasRvalueCast << " " << HasConstCast << std::endl;

    if (!HasRvalueCast && !HasConstCast) {
      FoundNonConstUse = true;
      return false;
    }

    return true;
  }

  bool IsNonConstUseFound() const {
    return FoundNonConstUse;
  }

private:
  FieldDecl *SoughtField;
  ParentMap *ParMap;
  bool FoundNonConstUse;

};



class ClassMethodVisitor : public RecursiveASTVisitor<ClassMethodVisitor> {
public:
  ClassMethodVisitor(ASTContext &Context, FieldDecl *SoughtField)
      : SM(Context.getSourceManager()), SoughtField(SoughtField), NecessaryMutable(false) {}

  bool VisitDecl(Decl *GenericDecl) {
    CXXMethodDecl *Declaration = dyn_cast<CXXMethodDecl>(GenericDecl);

    if (Declaration == nullptr)
      return true;

    // Mutable keyword does not influence non-const methods.
    if (!Declaration->isConst())
      return true;

    // All const  decls need definitions in main file.
    if (!Declaration->hasBody() || !SM.isInMainFile(Declaration->getBody()->getLocStart())) {
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

  bool IsMutableNecessary() const {
    return NecessaryMutable;
  }

private:
  SourceManager &SM;
  FieldDecl *SoughtField;
  bool NecessaryMutable;
};


// Checks if 'mutable' keyword can be removed; for now; we do it only if
// it is the only declaration in a declaration chain.
static bool CheckRemoval(SourceManager &SM,
                         const SourceLocation &LocStart,
                         const SourceLocation &LocEnd,
                         ASTContext &Context,
                         SourceRange &ResultRange) {
  
  FileID FID = SM.getFileID(LocEnd);
  llvm::MemoryBuffer *Buffer = SM.getBuffer(FID, LocEnd);
  Lexer DeclLexer(SM.getLocForStartOfFile(FID), Context.getLangOpts(),
                  Buffer->getBufferStart(),
                  SM.getCharacterData(LocStart),
                  Buffer->getBufferEnd());


  Token DeclToken;
  bool result = false;

  while (!DeclLexer.LexFromRawLexer(DeclToken)) {

    if (DeclToken.getKind() == tok::TokenKind::semi) {
      //std::cerr << "Found semi!\n";
      break;
    }

    if (DeclToken.getKind() == tok::TokenKind::comma) {
      //std::cerr << "Found comma, it's a multiple declaration!\n";
      return false;
    }

    if (DeclToken.isOneOf(tok::TokenKind::identifier, tok::TokenKind::raw_identifier)) {
      //std::cerr << "Found identifier!\n";
      auto TokenStr = DeclToken.getRawIdentifier().str();
      //std::cerr << "Token: " << TokenStr << "\n";

      // "mutable" cannot be used in any other way than to mark mutableness
      if (TokenStr == "mutable") {
        ResultRange = SourceRange(DeclToken.getLocation(), DeclToken.getEndLoc());
        result = true;
      }
    }
  }

  assert(result && "No mutable found, weird");

  return result;
}


void UnnecessaryMutableCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MD = Result.Nodes.getNodeAs<FieldDecl>("field");
  /*std::cerr << MD->isImplicit() << "\n" <<
               MD->getDeclName().getAsString() << "\n" <<
               MD->hasAttr<UnusedAttr>() << "\n" <<
               MD->getParent()->isDependentContext() << "\n" <<
               MD->isMutable() << "\n" <<
               MD->isReferenced() << "\n" <<
               MD->isThisDeclarationReferenced() << "\n";
  MD->dumpColor();*/

  const auto *ClassMatch = dyn_cast<CXXRecordDecl>(MD->getParent());
  auto &Context = *Result.Context;
  auto &SM = *Result.SourceManager;

  //ClassMatch->dumpColor();

  if (!MD->getDeclName() || ClassMatch->isDependentContext() || !MD->isMutable())
    return;

  ClassMethodVisitor Visitor(Context, const_cast<FieldDecl*>(MD));
  Visitor.TraverseDecl(const_cast<CXXRecordDecl*>(ClassMatch));
  //std::cerr << std::endl;

  if (Visitor.IsMutableNecessary())
    return;


  //std::cerr << "Unnecessary mutable!" << std::endl;
  
  auto Diag = diag(MD->getLocation(), "'mutable' modifier is unnecessary for field %0")
                    << MD->getDeclName();

  SourceRange RemovalRange;
  
  if (CheckRemoval(SM, MD->getLocStart(),
                   MD->getLocEnd(),
                   Context,
                   RemovalRange)) {
    //std::cerr << "WANNA REMOVE!" << std::endl;
    Diag << FixItHint::CreateRemoval(RemovalRange);
  }



  /*const auto *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("x");
  if (MatchedDecl->getName().startswith("awesome_"))
    return;
  diag(MatchedDecl->getLocation(), "function '%0' is insufficiently awesome")
      << MatchedDecl->getName()
      << FixItHint::CreateInsertion(MatchedDecl->getLocation(), "awesome_");*/
}

} // namespace misc
} // namespace tidy
} // namespace clang
