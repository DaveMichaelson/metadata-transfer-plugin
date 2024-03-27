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

class AttributeMetadataVisitor
  : public RecursiveASTVisitor<AttributeMetadataVisitor> {
public:
  explicit AttributeMetadataVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *Declaration) {
    return true;
  }

  bool VisitFriendDecl(FriendDecl *Declaration) {
    return true;
  }

  bool VisitFieldDecl(FieldDecl *Declaration) {
    return true;
  }

  bool VisitDecl(Decl *Declaration) {
    return true;
  }

  bool VisitExpr(Expr *Expression) {
    return true;
  }

private:
  ASTContext *Context;
};

class AttributeMetadataConumser : public clang::ASTConsumer {
public:
  explicit AttributeMetadataConumser(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  AttributeMetadataVisitor Visitor;
};

class AttributeMetadataAction : public clang::PluginASTAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
    return std::make_unique<AttributeMetadataConumser>(&Compiler.getASTContext());
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
static FrontendPluginRegistry::Add<AttributeMetadataAction> X("fincClassDecls", "my plugin description");
