// GCC 2.x emits a local "gcc2_compiled." label at the start of every object file's .text, at the
// same address as that file's first function, and Ghidra may pick it as the function's name.
// Pre-analysis: delete the labels. Post-analysis: give misnamed functions the real symbol name.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;
import java.util.*;

public class RemoveGccMarkers extends GhidraScript {
  static boolean isMarker(String n) { return n.equals("gcc2_compiled.") || n.equals("gcc2_compiled_") || n.equals("__gnu_compiled_cplusplus") || n.equals("__gnu_compiled_c"); }
  public void run() throws Exception {
    var st = currentProgram.getSymbolTable(); int deleted = 0, renamed = 0;
    List<Symbol> markers = new ArrayList<>();
    for (Symbol s : st.getAllSymbols(true)) if (isMarker(s.getName())) markers.add(s);
    for (Symbol s : markers) {
      if (s.getSymbolType() == SymbolType.FUNCTION) {
        Function f = (Function) s.getObject();
        Symbol best = null;
        for (Symbol o : st.getSymbols(s.getAddress())) {
          if (o == s || isMarker(o.getName()) || o.getSource() == SourceType.DEFAULT) continue;
          if (best == null || (!o.getParentNamespace().isGlobal() && best.getParentNamespace().isGlobal())) best = o;
        }
        if (best == null) { f.getSymbol().setName(null, SourceType.DEFAULT); continue; }
        String name = best.getName(); Namespace ns = best.getParentNamespace();
        best.delete();
        f.getSymbol().setNameAndNamespace(name, ns, SourceType.IMPORTED);
        renamed++;
      } else {
        s.delete(); deleted++;
      }
    }
    println("MARKERS deleted " + deleted + " gcc2_compiled labels, renamed " + renamed + " functions");
  }
}
