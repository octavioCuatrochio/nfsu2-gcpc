// Moves cfront/GCC2-style qualified data symbols into their namespaces:
//   _4RCMP.rcmp_sys          -> RCMP::rcmp_sys
//   _Q24RCMP6Inner.value     -> RCMP::Inner::value
// (CodeWarrior's own function mangling is handled by the GameCube loader's demangler.)
import ghidra.app.script.GhidraScript;
import ghidra.app.util.NamespaceUtils;
import ghidra.program.model.symbol.*;
import java.util.*;

public class FixCfrontNames extends GhidraScript {
  /** returns [qualifiedClass, member] or null */
  static String[] parse(String s) {
    if (!s.startsWith("_") || s.length() < 4) return null;
    int i = 1; List<String> parts = new ArrayList<>();
    int count = 1;
    if (s.charAt(i) == 'Q') { i++; if (i >= s.length() || !Character.isDigit(s.charAt(i))) return null; count = s.charAt(i) - '0'; i++; }
    for (int k = 0; k < count; k++) {
      int j = i; while (j < s.length() && Character.isDigit(s.charAt(j))) j++;
      if (j == i) return null;
      int n = Integer.parseInt(s.substring(i, j));
      if (j + n > s.length()) return null;
      String part = s.substring(j, j + n);
      if (!part.matches("[A-Za-z_][A-Za-z0-9_]*")) return null;
      parts.add(part); i = j + n;
    }
    if (i >= s.length() || (s.charAt(i) != '.' && s.charAt(i) != '$')) return null;
    String member = s.substring(i + 1);
    if (!member.matches("[A-Za-z_][A-Za-z0-9_]*")) return null;
    return new String[] { String.join("::", parts), member };
  }
  public void run() throws Exception {
    var st = currentProgram.getSymbolTable(); int fixed = 0, failed = 0;
    List<Symbol> todo = new ArrayList<>();
    for (Symbol s : st.getAllSymbols(false)) {
      if (s.getSymbolType() != SymbolType.LABEL || !s.getParentNamespace().isGlobal()) continue;
      if (parse(s.getName()) != null) todo.add(s);
    }
    for (Symbol s : todo) {
      String[] p = parse(s.getName());
      try {
        Namespace ns = NamespaceUtils.createNamespaceHierarchy(p[0], null, currentProgram, SourceType.IMPORTED);
        s.setNameAndNamespace(p[1], ns, SourceType.IMPORTED);
        fixed++;
      } catch (Exception e) { failed++; if (failed < 5) println("CFRONT fail " + s.getName() + ": " + e.getMessage()); }
    }
    println("CFRONT renamed " + fixed + " data symbols, " + failed + " failed");
  }
}
