import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.util.*;
public class VtDiag extends GhidraScript {
  public void run() throws Exception {
    var st = currentProgram.getSymbolTable();
    List<Symbol> vts = new ArrayList<>();
    for (Symbol s : st.getAllSymbols(false)) if (s.getName().contains("virtual_table") || s.getName().startsWith("_vt.")) vts.add(s);
    println("VTD vtable symbols: " + vts.size() + " e.g. " + (vts.isEmpty() ? "" : vts.get(0).getName(true) + " ns=" + vts.get(0).getParentNamespace().getName(true)));
    var ifc = new DecompInterface(); var o = new DecompileOptions(); o.grabFromProgram(currentProgram); ifc.setOptions(o); ifc.openProgram(currentProgram);
    int shown = 0; Map<String,Integer> fromKinds = new TreeMap<>();
    for (Symbol v : vts) {
      for (Reference r : getReferencesTo(v.getAddress())) {
        Function f = getFunctionContaining(r.getFromAddress());
        String k = f == null ? "no function" : (f.getParentNamespace() instanceof GhidraClass c ? (f.getName().equals(c.getName()) ? "ctor" : f.getName().startsWith("~") ? "dtor" : "other method") : "non-method");
        fromKinds.merge(k, 1, Integer::sum);
        if (shown < 8 && f != null && (k.equals("ctor") || k.equals("dtor"))) {
          var res = ifc.decompileFunction(f, 30, monitor);
          if (res.getDecompiledFunction() == null) continue;
          for (String line : res.getDecompiledFunction().getC().split("\n")) if (line.contains("virtual_table")) { println("VTD " + f.getName(true) + ": " + line.trim()); shown++; }
        }
      }
    }
    println("VTD refs by function kind: " + fromKinds);
  }
}
