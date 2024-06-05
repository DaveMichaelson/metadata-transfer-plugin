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

template<typename Node>
void attachJSONMetadata(ASTContext *Context, Node *N, llvm::json::Object &&Object) {
  std::string s;
  llvm::raw_string_ostream os(s);
  os << llvm::json::Value(std::move(Object));
  attachMetadata(Context, N, os.str());
}

bool isFunctionPointerType(Decl *Decleration) {
  QualType Ty;
  if (const auto *D = dyn_cast<ValueDecl>(Decleration))
    Ty = D->getType();
  else if (const auto *D = dyn_cast<TypedefNameDecl>(Decleration))
    Ty = D->getUnderlyingType();
  else
    return false;
  
  return Ty.getCanonicalType()->isFunctionPointerType();
}


class FunctionPointerTypeVisitor
  : public RecursiveASTVisitor<FunctionPointerTypeVisitor> {
public:
  explicit FunctionPointerTypeVisitor(ASTContext *Context) 
    : Context(Context) {}

  bool VisitCallExpr(CallExpr *CE) {
    if (!CE->getCalleeDecl())
      return true;
    if (!isFunctionPointerType(CE->getCalleeDecl()))
      return true;

    std::stringstream FPType;
    FPType << "(";
    
    Expr **Args = CE->getArgs();
    for(unsigned int Iarg = 0; Iarg < CE->getNumArgs(); ++Iarg) {
      FPType << Args[Iarg]->getType().getAsString();
      if (Iarg < CE->getNumArgs() - 1) {
        FPType << ",";
      }
    }
    
    FPType << ")->" << CE->getType().getAsString();
    
    attachMetadata(Context, CE, "CallSignature", FPType.str());
    
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *FD) {
    std::stringstream FPType;
    FPType << "(";
    
    for(unsigned int Iarg = 0; Iarg < FD->getNumParams(); ++Iarg) {
      FPType << FD->getParamDecl(Iarg)->getType().getAsString();
      if (Iarg < FD->getNumParams() - 1) {
        FPType << ",";
      }
    }
    
    FPType << ")->" << FD->getReturnType().getAsString();
    
    attachMetadata(Context, FD, "CallSignature", FPType.str());
    
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

  std::string computeRecordTypeName(CXXRecordDecl *D) {
    llvm::SmallString<256> TypeName;
    llvm::raw_svector_ostream OS(TypeName);
    OS << D->getKindName() << ".";

    PrintingPolicy Policy = D->getASTContext().getPrintingPolicy();
    Policy.SuppressInlineNamespace = false;

    if (D->getIdentifier())
      D->printQualifiedName(OS, Policy);
    else if (const TypedefNameDecl *TDD = D->getTypedefNameForAnonDecl())
      TDD->printQualifiedName(OS, Policy);
    else
      OS << "anon";

    return OS.str().str();
  }

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
    FuncPointerVisitor(Context) {}

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
