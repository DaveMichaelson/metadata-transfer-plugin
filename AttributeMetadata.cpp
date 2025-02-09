#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Sema/Sema.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Mangle.h"
#include "llvm/IR/Attributes.h"
#include "llvm/Support/JSON.h"
#include <algorithm>
#include <numeric>
#include <string>

using namespace clang;

void attachMetadata(ASTContext *Context, Decl *D, StringRef Metadata) {
  D->addAttr(AnnotateAttr::CreateImplicit(*Context, Metadata, nullptr, 0));
}

void attachMetadata(ASTContext *Context, Stmt *S, StringRef Metadata) {
  S->addAttr(*Context, AttachMetadataAttr::CreateImplicit(*Context, Metadata));
}

template<typename Node>
void attachMetadata(ASTContext *Context, Node *N, std::string Kind, StringRef Metadata) {
  attachMetadata(Context, N, Kind + "=" + Metadata.str());
}

std::string getStringFromType(QualType type) {
  return type.getCanonicalType().getAsString();
}

void eraseFromString(std::string &S, std::string SubString) {
  size_t pos = S.find(SubString);
  if (pos == std::string::npos)
    return;
  S.erase(pos, SubString.size());
}

template<typename Node>
void attachJSONMetadata(ASTContext *Context, Node *N, llvm::json::Object &&Object) {
  std::string s;
  llvm::raw_string_ostream os(s);
  os << llvm::json::Value(std::move(Object));
  attachMetadata(Context, N, os.str());
}


class FunctionPointerTypeVisitor
  : public RecursiveASTVisitor<FunctionPointerTypeVisitor> {
public:
  explicit FunctionPointerTypeVisitor(ASTContext *Context) 
    : Context(Context) {}

  template<typename Node>
  void attachFunctionTypeMetadata(Node *N, llvm::json::Array &TypeList, std::string ReturnType) {
    llvm::json::Object object;
    object.try_emplace("ReturnType", ReturnType);
    object.try_emplace("TypeList", std::move(TypeList));
    attachJSONMetadata(Context, N, std::move(object));
  }

  void attachFunctionTypeMetadataForFunctionDecl(FunctionDecl *N, llvm::json::Array &TypeList, std::string ReturnType) {
    llvm::json::Object object;
    object.try_emplace("ReturnType", ReturnType);
    object.try_emplace("TypeList", std::move(TypeList));
    attachJSONMetadata(Context, N, std::move(object));
  }

  bool VisitCallExpr(CallExpr *CE) {
    Expr **Args = CE->getArgs();
    llvm::json::Array TypeList;

    if (auto MCE = llvm::dyn_cast<CXXMemberCallExpr>(CE)) {
      TypeList.push_back(MCE->getMethodDecl()->getParent()->getDeclName().getAsString() + " *");
    }

    for(unsigned int Iarg = 0; Iarg < CE->getNumArgs(); ++Iarg) {
      TypeList.push_back(getStringFromType(Args[Iarg]->getType()));
    }
    
    attachFunctionTypeMetadata(CE, TypeList, getStringFromType(CE->getType()));
        
    return true;
  }

  virtual bool VisitFunctionDecl(FunctionDecl *FD) {    
    llvm::json::Array TypeList;

    if (auto MD = llvm::dyn_cast<CXXMethodDecl>(FD)) {
      // add this suffix for the this-pointer
      std::string suffix = " *";
      if (llvm::isa<CXXConstructorDecl>(MD))
        // this is suffix is not needed for constructor calls
        suffix = "";
      if (MD->isInstance()) {
        TypeList.push_back(MD->getParent()->getDeclName().getAsString() + suffix);
      }
    }

    for(unsigned int Iarg = 0; Iarg < FD->getNumParams(); ++Iarg) {
      TypeList.push_back(getStringFromType(FD->getParamDecl(Iarg)->getType()));
    }
        
    attachFunctionTypeMetadataForFunctionDecl(FD, TypeList, getStringFromType(FD->getReturnType()));
    
    return true;
  }

  bool VisitCXXConstructExpr(CXXConstructExpr *CE) {    
    Expr **Args = CE->getArgs();
    llvm::json::Array TypeList;
    
    TypeList.push_back(CE->getConstructor()->getParent()->getDeclName().getAsString());
    

    for(unsigned int Iarg = 0; Iarg < CE->getNumArgs(); ++Iarg) {
      TypeList.push_back(getStringFromType(Args[Iarg]->getType()));
    }
    
    std::string ClassType = getStringFromType(CE->getType());
    if (CE->getConstructor()->isDefaultConstructor()) {
      ClassType = CE->getConstructor()->getParent()->getDeclName().getAsString();
    }
    attachFunctionTypeMetadata(CE, TypeList, ClassType);    
    return true;
  }

private:
  ASTContext *Context;
};

class EnumerateBaseClassVisitor 
  : public RecursiveASTVisitor<EnumerateBaseClassVisitor> {
public:
  explicit EnumerateBaseClassVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *D) {
    if (!D->hasDefinition()) {
      return true;
    }
    llvm::json::Array array;

    for (auto base = D->bases_begin(); base != D->bases_end(); ++base) {
      CXXRecordDecl *BaseDecl = base->getType()->getAsCXXRecordDecl();
      if (!BaseDecl) {
        continue;
      }
      array.push_back(BaseDecl->getDeclName().getAsString());
    }
    llvm::json::Object object;
    object.try_emplace("ClassName", D->getDeclName().getAsString());
    object.try_emplace("BaseClasses", std::move(array));
    attachJSONMetadata(Context, D, std::move(object));
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
    if (IsMacro(D)) {
      MarkMacro(D);
    }
    
    return true;
  }

  bool TraverseStmt(Stmt *S) {
    MacroStack.push_back("#");
    bool result = RecursiveASTVisitor<MarkMacroVisitor>::TraverseStmt(S);
    while(MacroStack.pop_back_val() != "#");
    return result;
  }

  bool VisitStmt(Stmt *S) {
    if (IsMacro(S)) {
      StringRef MacroName = GetMacroName(S);
      if (std::find(MacroStack.begin(), MacroStack.end(), MacroName) == MacroStack.end()) {
        MacroStack.push_back(MacroName);
        MarkMacro(S);
      }
    }
    return true;
  }

  bool VisitExpr(Expr *Expression) {
    return true;
  }

private:
  ASTContext *Context;
  llvm::SmallVector<StringRef, 4> MacroStack;

  template <typename Node>
  bool IsMacro(Node *N) {
    return N->getBeginLoc().isMacroID() && N->getEndLoc().isMacroID();
  }

  template<typename Node>
  void addMacroNameAsAttribute(Node *N, StringRef MacroName) {
    attachMetadata(Context, N, "MacroName", MacroName);
  }

  template <typename Node>
  StringRef GetMacroName(Node *N) {
    return Lexer::getImmediateMacroName(N->getBeginLoc(), 
      Context->getSourceManager(), Context->getLangOpts());
  }

  template <typename Node>
  void MarkMacro(Node *N) {
    StringRef MacroName = GetMacroName(N);
    addMacroNameAsAttribute(N, MacroName);
  }

};

class AttributeMetadataConsumer : public clang::ASTConsumer {
public:
  explicit AttributeMetadataConsumer(ASTContext *Context)
    : MacroVisitor(Context), BaseClassVisitor(Context), 
    FuncPointerVisitor(Context) {
      llvm::outs() << "-------------------PLUGIN CALLED---------------------\n";
    }

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    for (auto D : DG) {
      MacroVisitor.TraverseDecl(D);
      BaseClassVisitor.TraverseDecl(D);
      FuncPointerVisitor.TraverseDecl(D);
    }
    return true;
  }
private:
  MarkMacroVisitor MacroVisitor;
  EnumerateBaseClassVisitor BaseClassVisitor;
  FunctionPointerTypeVisitor FuncPointerVisitor;
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
