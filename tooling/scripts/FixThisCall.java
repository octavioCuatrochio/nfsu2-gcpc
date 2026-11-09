// GCC 2.x mangling does not distinguish static from non-static member functions, and Ghidra's
// demangler assigns __thiscall inconsistently. For class functions left without 'this', decompile
// them and switch to __thiscall when the body reads one more integer argument register than the
// declared parameters use (i.e. r3 is really 'this'). Functions without that evidence are untouched.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.util.regex.*;

public class FixThisCall extends GhidraScript {
  static boolean isFloat(DataType dt) {
    while (dt instanceof TypeDef t) dt = t.getBaseDataType();
    return dt instanceof AbstractFloatDataType;
  }
  public void run() throws Exception {
    var ifc = new DecompInterface(); var o = new DecompileOptions(); o.grabFromProgram(currentProgram); ifc.setOptions(o);
    ifc.toggleSyntaxTree(false); ifc.openProgram(currentProgram);
    int checked = 0, fixed = 0;
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      if (monitor.isCancelled()) break;
      if (!(f.getParentNamespace() instanceof GhidraClass)) continue;
      if (!"__stdcall".equals(f.getCallingConventionName())) continue;
      if (f.getSignatureSource() == SourceType.IMPORTED || f.getSignatureSource() == SourceType.USER_DEFINED) continue; // DWARF is authoritative
      int nInt = 0;
      for (Parameter p : f.getParameters()) if (!isFloat(p.getDataType())) nInt++;
      if (nInt >= 8) continue;
      checked++;
      var r = ifc.decompileFunction(f, 30, monitor);
      if (r.getDecompiledFunction() == null) continue;
      String c = r.getDecompiledFunction().getC();
      if (Pattern.compile("\\bin_r" + (3 + nInt) + "\\b").matcher(c).find()) {
        f.setCallingConvention("__thiscall");
        fixed++;
      }
    }
    ifc.dispose();
    println("THISFIX checked " + checked + " stdcall class functions, switched " + fixed + " to __thiscall");
  }
}
