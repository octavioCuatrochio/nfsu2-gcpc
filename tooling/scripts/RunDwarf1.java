import ghidra.app.script.GhidraScript;
import ghidra.app.util.importer.MessageLog;
import ghidra.app.decompiler.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.util.*;
public class RunDwarf1 extends GhidraScript {
  public void run() throws Exception {
    var log = new MessageLog();
    long t = System.currentTimeMillis();
    new com.github.rafalh.ghidra.dwarfone.DWARF1Analyzer().added(currentProgram, currentProgram.getMemory(), monitor, log);
    println("DW1TIME " + (System.currentTimeMillis() - t) + "ms");
    // group log messages
    Map<String,Integer> groups = new TreeMap<>(); List<String> examples = new ArrayList<>();
    for (String line : log.toString().split("\n")) {
      if (line.isBlank()) continue;
      String key = line.replaceAll("[0-9a-fA-F]{5,}", "#").replaceAll("\\{.*", "{..}");
      key = key.length() > 110 ? key.substring(0, 110) : key;
      if (groups.merge(key, 1, Integer::sum) == 1 && examples.size() < 25) examples.add(line.length() > 300 ? line.substring(0,300) : line);
    }
    groups.entrySet().stream().sorted((a,b)->b.getValue()-a.getValue()).limit(30).forEach(e -> println("DW1LOG " + e.getValue() + "x " + e.getKey()));
    for (String e : examples) println("DW1EX " + e);
    // type stats
    var dtm = currentProgram.getDataTypeManager();
    int st=0, un=0, en=0, td=0, fd=0, undefMembers=0, members=0, bitf=0;
    for (var it = dtm.getAllDataTypes(); it.hasNext();) {
      DataType dt = it.next();
      if (!dt.getCategoryPath().getPath().startsWith("/DWARF")) continue;
      if (dt instanceof Structure s) { st++; for (var c : s.getDefinedComponents()) { members++; if (c.isBitFieldComponent()) bitf++; if (c.getDataType() == DataType.DEFAULT || c.getDataType().getName().startsWith("undefined")) undefMembers++; } }
      else if (dt instanceof Union) un++; else if (dt instanceof ghidra.program.model.data.Enum) en++; else if (dt instanceof TypeDef) td++; else if (dt instanceof FunctionDefinition) fd++;
    }
    println("DW1TYPES structs=" + st + " unions=" + un + " enums=" + en + " typedefs=" + td + " funcdefs=" + fd + " members=" + members + " bitfields=" + bitf + " undefinedMembers=" + undefMembers);
    int imported=0, undefParams=0, params=0, classNs=0;
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      if (f.getSignatureSource() != SourceType.IMPORTED) continue;
      imported++; if (f.getParentNamespace() instanceof GhidraClass) classNs++;
      for (Parameter p : f.getParameters()) { params++; if (p.getDataType().getName().startsWith("undefined")) undefParams++; }
    }
    println("DW1FUNCS importedSigs=" + imported + " inClass=" + classNs + " params=" + params + " undefinedParams=" + undefParams);
    // decompile samples
    String[] want = (getScriptArgs().length > 0 ? getScriptArgs() : new String[]{}) ;
    var ifc = new DecompInterface(); var dopt = new DecompileOptions(); dopt.grabFromProgram(currentProgram); dopt.setRespectReadOnly(true); ifc.setOptions(dopt); ifc.openProgram(currentProgram);
    List<Function> picks = new ArrayList<>();
    for (String w : want) for (Function f : currentProgram.getFunctionManager().getFunctions(true)) if (f.getName(true).equals(w)) { picks.add(f); break; }
    for (Function f : picks) {
      var r = ifc.decompileFunction(f, 60, monitor);
      println("DW1DECOMP ===== " + f.getName(true) + " @ " + f.getEntryPoint() + "\n" + (r.getDecompiledFunction() == null ? r.getErrorMessage() : r.getDecompiledFunction().getC()));
    }
  }
}
