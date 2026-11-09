import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.parallel.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;

public class ExportDecomp extends GhidraScript {
  static String fileFor(Function f) {
    Namespace ns = f.getParentNamespace();
    if (ns == null || ns.isGlobal()) return "_global";
    String n = ns.getName(true).replace("::", "__").replaceAll("[^A-Za-z0-9_.-]", "_");
    if (n.length() > 150) n = n.substring(0, 150) + "_" + Integer.toHexString(n.hashCode());
    return n;
  }

  public void run() throws Exception {
    Path out = Paths.get(getScriptArgs()[0]);
    Files.createDirectories(out);
    ConcurrentHashMap<Function, String> code = new ConcurrentHashMap<>();
    List<Function> failed = Collections.synchronizedList(new ArrayList<>());
    var cfg = new DecompileConfigurer() { public void configure(DecompInterface d) {
      var o = new DecompileOptions(); o.grabFromProgram(currentProgram); d.setOptions(o); } };
    var cb = new DecompilerCallback<Void>(currentProgram, cfg) {
      public Void process(DecompileResults r, ghidra.util.task.TaskMonitor m) {
        if (r.getDecompiledFunction() == null) failed.add(r.getFunction());
        else code.put(r.getFunction(), r.getDecompiledFunction().getC());
        return null;
      }
    };
    List<Function> fs = new ArrayList<>();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) if (!f.isExternal()) fs.add(f);
    ParallelDecompiler.decompileFunctions(cb, fs, monitor);
    cb.dispose();

    // retry failures with a long timeout
    var ifc = new DecompInterface(); var o = new DecompileOptions(); o.grabFromProgram(currentProgram);
    ifc.setOptions(o); ifc.openProgram(currentProgram);
    List<Function> stillFailed = new ArrayList<>();
    for (Function f : new ArrayList<>(failed)) {
      var r = ifc.decompileFunction(f, 600, monitor);
      if (r.getDecompiledFunction() != null) code.put(f, r.getDecompiledFunction().getC());
      else { stillFailed.add(f); code.put(f, "/* " + f.getName(true) + " @ " + f.getEntryPoint() + ": decompilation failed: " + String.valueOf(r.getErrorMessage()).trim() + " */\n"); }
    }
    ifc.dispose();

    TreeMap<String, List<Function>> groups = new TreeMap<>();
    for (Function f : fs) groups.computeIfAbsent(fileFor(f), k -> new ArrayList<>()).add(f);
    StringBuilder index = new StringBuilder("address\tfile\tfunction\n");
    for (var e : groups.entrySet()) {
      e.getValue().sort(Comparator.comparing(Function::getEntryPoint));
      try (var w = Files.newBufferedWriter(out.resolve(e.getKey() + ".c"))) {
        w.write("// Ghidra decompilation of Speed.elf\n#include \"../types.h\"\n\n");
        for (Function f : e.getValue()) {
          w.write("// " + f.getName(true) + " @ " + f.getEntryPoint() + "\n");
          w.write(code.getOrDefault(f, "/* missing */\n"));
          w.write("\n");
          index.append(f.getEntryPoint()).append('\t').append(e.getKey()).append(".c\t").append(f.getName(true)).append('\n');
        }
      }
    }
    Files.writeString(out.resolveSibling("functions.tsv"), index.toString());

    // data types header
    var dtm = currentProgram.getDataTypeManager();
    try (var fw = new FileWriter(out.resolveSibling("types.h").toFile())) {
      var hw = new ghidra.program.model.data.DataTypeWriter(dtm, fw, false);
      var all = new ArrayList<ghidra.program.model.data.DataType>(); dtm.getAllDataTypes(all);
      hw.write(all, monitor);
    }

    println("EXPORT functions=" + fs.size() + " files=" + groups.size() + " stillFailed=" + stillFailed.size());
    for (Function f : stillFailed) println("EXPORT FAIL " + f.getName(true));
  }
}
