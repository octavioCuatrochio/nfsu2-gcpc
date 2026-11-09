import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.util.*;
public class VtDiag2 extends GhidraScript {
  boolean hasVptr(Structure s) { for (var c : s.getDefinedComponents()) if (c.getDataType() instanceof Pointer p && p.getDataType().getName().equals("__vtbl_ptr_type")) return true; return false; }
  public void run() throws Exception {
    var ifc = new DecompInterface(); var o = new DecompileOptions(); o.grabFromProgram(currentProgram); ifc.setOptions(o); ifc.openProgram(currentProgram);
    int shown = 0, ctorsNoVptr = 0, ctorsTotal = 0; Map<String,Integer> why = new TreeMap<>();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      if (!(f.getParentNamespace() instanceof GhidraClass c) || !f.getName().equals(c.getName())) continue;
      if (f.getParameterCount() == 0 || !(f.getParameter(0).getDataType() instanceof Pointer p) || !(p.getDataType() instanceof Structure s)) { why.merge("no struct this (cc=" + f.getCallingConventionName() + ")", 1, Integer::sum); continue; }
      if (s.getCategoryPath().getPath().startsWith("/DWARF")) continue;
      ctorsTotal++;
      if (hasVptr(s)) continue;
      var res = ifc.decompileFunction(f, 30, monitor); if (res.getDecompiledFunction() == null) continue;
      String code = res.getDecompiledFunction().getC();
      if (!code.contains("virtual_table")) { why.merge("ctor has no vtable store", 1, Integer::sum); continue; }
      ctorsNoVptr++;
      if (shown < 10) for (String line : code.split("\n")) if (line.contains("virtual_table")) { println("VTD2 " + f.getName(true) + ": " + line.trim()); shown++; break; }
    }
    println("VTD2 game ctors=" + ctorsTotal + " with vtable store but untyped vptr=" + ctorsNoVptr + " other=" + why);
  }
}
