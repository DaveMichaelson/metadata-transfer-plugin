#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Sema/Sema.h"
#include "clang/AST/Attr.h"
#include "llvm/IR/Attributes.h"

using namespace clang;

class EnumerateBaseClassVisitor 
  : public RecursiveASTVisitor<EnumerateBaseClassVisitor> {
public:
  explicit EnumerateBaseClassVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *D) {
    llvm::outs() << "Declaration of " << D->Decl::getDeclKindName() << "\n";
    return true;
  }

private:
  ASTContext *Context;
};

class MarkMacroVisitor
  : public RecursiveASTVisitor<MarkMacroVisitor> {
public:
  explicit MarkMacroVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitDecl(Decl *D) {
    llvm::outs() << "Declaration of " << D->Decl::getDeclKindName() << "\n";
    D->addAttr(AnnotateAttr::CreateImplicit(
      D->getASTContext(), D->Decl::getDeclKindName(), nullptr, 0));

    if (IsMacro(D)) {
      MarkMacro(D);
    }
    
    return true;
  }

  bool TraverseStmt(Stmt *S) {
    if (skip) {
      return true;
    }
    bool result = RecursiveASTVisitor::TraverseStmt(S);
    skip = false;
    return result;
  }

  bool VisitStmt(Stmt *S) {
    if (IsMacro(S)) {
      skip = true;
      MarkMacro(S);
    }
    return true;
  }

  bool VisitExpr(Expr *Expression) {
    return true;
  }

private:
  ASTContext *Context;
  bool skip = false;

  template <typename Node>
  bool IsMacro(Node *N) {
    return N->getBeginLoc().isMacroID() && N->getEndLoc().isMacroID();
  }

  template <typename Node>
  void MarkMacro(Node *N) {
    StringRef MacroName = Lexer::getImmediateMacroName(N->getBeginLoc(), 
      Context->getSourceManager(), Context->getLangOpts());
    llvm::outs() << "MacroName=" << MacroName << "\n";
    attachMetadata(N, MacroName);
  }

  void attachMetadata(Decl *D, StringRef Metadata) {
    D->addAttr(AnnotateAttr::Create(*Context, Metadata, nullptr, 0));
  }

  void attachMetadata(Stmt *S, StringRef Metadata) {
    S->addAttr(*Context, AttachMetadataAttr::Create(*Context, Metadata));
  }
  
};

class AttributeMetadataConsumer : public clang::ASTConsumer {
public:
  explicit AttributeMetadataConsumer(ASTContext *Context)
    : MacroVisitor(Context), BaseClassVisitor(Context) {}

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    llvm::outs() << "HandleTopLevelDecl" << "\n";
    for (auto D : DG) {
      llvm::outs() << D->getDeclKindName() << "\n";
      llvm::outs() << "MacroVisitor" << "\n";
      MacroVisitor.TraverseDecl(D);
      llvm::outs() << "BaseClassVisitor" << "\n";
      BaseClassVisitor.TraverseDecl(D);
    }
    return true;
  }
private:
  MarkMacroVisitor MacroVisitor;
  EnumerateBaseClassVisitor BaseClassVisitor;
};

class AttributeMetadataAction : public clang::PluginASTAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
    return std::make_unique<AttributeMetadataConsumer>(&Compiler.getASTContext());
  }

  bool ParseArgs(const CompilerInstance &CI,
               const std::vector<std::string>& args) override {
    return true;
  }

  PluginASTAction::ActionType getActionType() override {
    return AddBeforeMainAction;
  }
};


//-----------------------------------------------------------------------------
// Registration
//-----------------------------------------------------------------------------
static FrontendPluginRegistry::Add<AttributeMetadataAction> X("metadataTransfer", "my plugin description");
