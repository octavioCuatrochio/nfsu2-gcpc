// PowerPC SVR4/EABI returns structs through a hidden pointer in r3, shifting 'this' and all
// arguments by one register. GCC 2.x mangled names give the complete parameter list but no
// return type, so a demangled function that reads exactly one more integer argument register
// than its declared parameters (two for a class function still lacking 'this') returns a struct.
// Give those an explicit __return_storage_ptr__ first parameter.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.symbol.SourceType;
import java.util.*;
import java.util.regex.*;
public class FixStructReturns extends GhidraScript {
  static boolean isFloat(DataType dt) { while (dt instanceof TypeDef t) dt = t.getBaseDataType(); return dt instanceof AbstractFloatDataType; }
  public void run() throws Exception {
    var ifc = new DecompInterface(); var o = new DecompileOptions(); o.grabFromProgram(currentProgram); ifc.setOptions(o); ifc.toggleSyntaxTree(false); ifc.openProgram(currentProgram);
    Pattern inr = Pattern.compile("\\bin_r(\\d+)\\b");
    int fixed = 0, withThis = 0;
    List<Function> todo = new ArrayList<>(); currentProgram.getFunctionManager().getFunctions(true).forEach(todo::add);
    for (Function f : todo) {
      if (monitor.isCancelled()) break;
      SourceType src = f.getSignatureSource();
      if (src != SourceType.ANALYSIS || f.hasVarArgs()) continue;          // demangled signatures only
      Parameter[] ps = f.getParameters();
      int nInt = 0; for (Parameter p : ps) if (!isFloat(p.getDataType())) nInt++;
      if (nInt >= 7) continue;
      var r = ifc.decompileFunction(f, 30, monitor); if (r.getDecompiledFunction() == null) continue;
      int maxReg = 0; Matcher m = inr.matcher(r.getDecompiledFunction().getC());
      while (m.find()) maxReg = Math.max(maxReg, Integer.parseInt(m.group(1)));
      int extra = maxReg - (2 + nInt);
      boolean isClass = f.getParentNamespace() instanceof GhidraClass;
      boolean thiscall = "__thiscall".equals(f.getCallingConventionName());
      boolean addThis;
      if (extra == 1) addThis = false;
      else if (extra == 2 && isClass && !thiscall) addThis = true;
      else continue;
      List<Variable> params = new ArrayList<>();
      DataType retPtr = new PointerDataType(DataType.DEFAULT, currentProgram.getDataTypeManager());
      params.add(new ParameterImpl("__return_ptr", retPtr, currentProgram));
      if (addThis || thiscall) {
        DataType thisType = null;
        for (Parameter p : ps) if (p.isAutoParameter() && "this".equals(p.getName())) thisType = p.getDataType();
        if (thisType == null) {
          var cls = (GhidraClass) f.getParentNamespace();
          DataType st = VariableUtilities.findExistingClassStruct(cls, currentProgram.getDataTypeManager());
          thisType = new PointerDataType(st != null ? st : DataType.DEFAULT, currentProgram.getDataTypeManager());
        }
        params.add(new ParameterImpl("this", thisType, currentProgram));
        if (addThis) withThis++;
      }
      for (Parameter p : ps) if (!p.isAutoParameter()) params.add(new ParameterImpl(p.getName(), p.getDataType(), currentProgram));
      f.updateFunction("__stdcall", new ReturnParameterImpl(retPtr, currentProgram), params, FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS, true, SourceType.ANALYSIS);
      fixed++;
    }
    ifc.dispose();
    println("STRUCTRET gave " + fixed + " functions a hidden return pointer (" + withThis + " also gained 'this')");
  }
}
