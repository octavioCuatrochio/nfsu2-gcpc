import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
public class Decomp extends GhidraScript {
  public void run() throws Exception {
    var ifc = new DecompInterface(); var o = new DecompileOptions(); o.grabFromProgram(currentProgram); o.setRespectReadOnly(true); ifc.setOptions(o); ifc.openProgram(currentProgram);
    for (String w : getScriptArgs()) {
      Function f = null;
      for (Function g : currentProgram.getFunctionManager().getFunctions(true)) if (g.getName(true).equals(w)) { f = g; break; }
      if (f == null) { println("DECOMP ===== " + w + " NOT FOUND"); continue; }
      var r = ifc.decompileFunction(f, 60, monitor);
      println("DECOMP ===== " + f.getName(true) + " @ " + f.getEntryPoint() + "\n" + (r.getDecompiledFunction() == null ? r.getErrorMessage() : r.getDecompiledFunction().getC()));
    }
  }
}
