// GCC 2.x destructors take a hidden second argument, int __in_chrg (in-charge flag: also delete
// the object / destroy virtual bases). The mangled name does not show it, so demangled
// destructors miss it and the decompiler reports a stray in_r4. DWARF-typed ones already have it.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.symbol.SourceType;
import java.util.*;
public class FixDestructors extends GhidraScript {
  public void run() throws Exception {
    int fixed = 0;
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      if (!(f.getParentNamespace() instanceof GhidraClass) || !f.getName().startsWith("~")) continue;
      if (f.getSignatureSource() == SourceType.IMPORTED || f.getSignatureSource() == SourceType.USER_DEFINED) continue;
      List<Parameter> formal = new ArrayList<>();
      boolean has = false;
      for (Parameter p : f.getParameters()) {
        if ("__in_chrg".equals(p.getName())) has = true;
        if (!p.isAutoParameter()) formal.add(p);
      }
      if (has) continue;
      boolean thiscall = "__thiscall".equals(f.getCallingConventionName());
      if (!thiscall && formal.isEmpty()) continue;           // no 'this' at all: leave it
      if ((thiscall ? formal.size() : formal.size() - 1) != 0) continue;  // only this-only destructors
      List<Variable> params = new ArrayList<>(formal);
      params.add(new ParameterImpl("__in_chrg", IntegerDataType.dataType, currentProgram));
      f.updateFunction(f.getCallingConventionName(), new ReturnParameterImpl(VoidDataType.dataType, currentProgram), params,
          FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS, true, f.getSignatureSource());
      fixed++;
    }
    println("DTORFIX added __in_chrg to " + fixed + " destructors");
  }
}
