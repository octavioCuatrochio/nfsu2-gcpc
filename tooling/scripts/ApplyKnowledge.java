// Applies knowledge.db matches to the PC program: GameCube names (with C++ class namespaces) on matched
// functions, vtables and globals, plus a plate comment with the GameCube signature and nfsu2-re's notes.
// Input: a TSV written by tooling/knowledge/apply_pc.py
//   kind<TAB>pc_addr_hex<TAB>name<TAB>comment     (kind = func | label)
// Usage: -postScript ApplyKnowledge.java <tsv>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.nio.file.*;
import java.util.*;

public class ApplyKnowledge extends GhidraScript {
  Namespace namespaceFor(String qualified) throws Exception {
    Namespace ns = currentProgram.getGlobalNamespace();
    String[] parts = qualified.split("::");
    for (int i = 0; i < parts.length - 1; i++) {
      Namespace next = currentProgram.getSymbolTable().getNamespace(parts[i], ns);
      if (next == null) next = currentProgram.getSymbolTable().createClass(ns, parts[i], SourceType.IMPORTED);
      ns = next;
    }
    return ns;
  }

  public void run() throws Exception {
    int funcs = 0, labels = 0, skipped = 0;
    for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
      String[] c = line.split("\t", -1);
      if (c.length < 4) continue;
      Address a = toAddr(Long.parseLong(c[1], 16));
      String full = c[2];
      String simple = full.contains("::") ? full.substring(full.lastIndexOf("::") + 2) : full;
      // operator names etc. may contain characters Ghidra rejects; keep them in the comment only
      simple = simple.replaceAll("[\\s]", "_");
      try {
        Namespace ns = namespaceFor(full.contains("::") ? full.substring(0, full.lastIndexOf("::") + 2) + "x" : "x");
        if (c[0].equals("func")) {
          Function f = getFunctionAt(a);
          if (f == null) { skipped++; continue; }
          if (f.getSymbol().getSource() == SourceType.USER_DEFINED) { skipped++; continue; }  // never override user names
          f.setParentNamespace(ns);
          f.setName(simple, SourceType.IMPORTED);
          funcs++;
        } else {
          Symbol s = currentProgram.getSymbolTable().getPrimarySymbol(a);
          if (s != null && s.getSource() == SourceType.USER_DEFINED) { skipped++; continue; }
          currentProgram.getSymbolTable().createLabel(a, simple, ns, SourceType.IMPORTED).setPrimary();
          labels++;
        }
        if (!c[3].isEmpty()) setPlateComment(a, c[3].replace("\\n", "\n"));
      } catch (Exception e) {
        skipped++;
        if (skipped < 20) println("APPLY skip " + c[1] + " " + full + ": " + e.getMessage());
      }
    }
    // names this script applied earlier (recognizable by the "GC " plate comment) whose match is gone: reset
    Set<Long> current = new HashSet<>();
    for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
      String[] c = line.split("\t", -1);
      if (c.length >= 2) current.add(Long.parseLong(c[1], 16));
    }
    int reset = 0;
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      Address a = f.getEntryPoint();
      String plate = getPlateComment(a);
      if (plate == null || !(plate.startsWith("GC ") || plate.startsWith("nfsu2-re:") || plate.contains("[match.py]")) || current.contains(a.getOffset())) continue;
      if (f.getSymbol().getSource() == SourceType.USER_DEFINED) continue;
      f.setParentNamespace(currentProgram.getGlobalNamespace());
      f.getSymbol().setName(SymbolUtilities.getDefaultFunctionName(a), SourceType.DEFAULT);
      setPlateComment(a, null);
      reset++;
    }
    println("APPLY functions=" + funcs + " labels=" + labels + " skipped=" + skipped + " stale names reset=" + reset);
  }
}
