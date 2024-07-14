# metadata-transfer-plugin

Dieses Clang Plugin attribuiert AST-Knoten um Informationen über 
die Vererbungshierarchie (BaseClassVisitor), Makros (MacroVisitor) und 
Zeigertypen (FuncPointerVisitor) in die LLVM-IR mittels Metadaten zu übertragen.

## Bauen
Bevor das Plugin gebaut werden kann muss in `CMakeLists.txt` das Build-Verzeichnis
vom von mir angepassten LLVM Projekt angegeben werden. Dafür muss die `CT_LLVM_INSTALL_DIR`
Variable entsprechend gesetzt werden.

Anschließend kann das Plugin mit folgendem Befehl gebaut werden:
`cmake . && make`

Beim Übersetzen eines Programms kann das Plugin mit der `-fplugin` Option dem `clang`-Aufruf
mitgegeben werden:
`-fplugin=lib/libAttributeMetadata.so`

## Reihenfolge der Ausführung

Die ASTMainAction ist die
Codegenerierung, da das Plugin Attribute hinzufügt, die in der Codegenerierung verarbeitet
werden sollen, muss das Plugin vor der ASTMainAction ausgeführt werden. Um dies zu gewähr-
leisten werden die RecursiveASTVisitor in der HandleTopLevelDecl-Methode vom ASTConsumer
ausgeführt. Dies hat den Grund, dass die Codegenerierung von Funktionen ebenfalls in der
HandleTopLevelDecl-Methode vom BackendConsumer, der Codegenerierung, ausgeführt wird
und die HandleTopLevelDecl-Methode vor der HandleTranslationUnit-Methode ausgeführt wird
und nur auf diese Weise die RecursiveASTVisitor des Plugins vor der Codegenerierung der Funk-
tionen ausgeführt wird. Dies ist notwendig damit die Attribute, die vom Plugin, innerhalb
von Funktionen hinzugefügt werden, auch von der Codegenerierung verarbeitet werden.