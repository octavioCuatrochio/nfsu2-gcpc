// Grows class structs that PC code accesses past their end (the decompiler then shows `this[N].field_..`
// or `(this + N)`), because the class is bigger on this platform than the layout we have. Decompiles every
// __thiscall method, takes the furthest access per class, and grows the struct to cover it.
// Prints the grown class names as "GROWN <name>" so FillClassStructs can be re-run on just those.
// Usage: -postScript GrowClassStructs.java
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.parallel.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.regex.*;

public class GrowClassStructs extends GhidraScript {
  public void run() throws Exception {
    Map<Structure, Integer> need = new ConcurrentHashMap<>();
    Map<Structure, String> cls = new ConcurrentHashMap<>();
    Pattern idx = Pattern.compile("\\bthis\\[(\\d+)\\]\\.(\\w+)");
    Pattern plus = Pattern.compile("\\(this \\+ (\\d+)\\)");
    List<Function> fs = new ArrayList<>();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true))
      if ("__thiscall".equals(f.getCallingConventionName()) && f.getParentNamespace() instanceof GhidraClass) fs.add(f);
    var cfg = new DecompileConfigurer() { public void configure(DecompInterface d) {
      var o = new DecompileOptions(); o.grabFromProgram(currentProgram); d.setOptions(o); } };
    var cb = new DecompilerCallback<Void>(currentProgram, cfg) {
      public Void process(DecompileResults r, ghidra.util.task.TaskMonitor m) {
        if (r.getDecompiledFunction() == null) return null;
        Function f = r.getFunction();
        Parameter tp = null;
        for (Parameter q : f.getParameters()) if ("this".equals(q.getName())) tp = q;
        if (tp == null || !(tp.getDataType() instanceof Pointer p) || !(p.getDataType() instanceof Structure s)) return null;
        if (s.getCategoryPath().getPath().startsWith("/DWARF")) return null;
        String c = r.getDecompiledFunction().getC();
        int len = s.getLength(), max = 0;
        Matcher a = idx.matcher(c);
        while (a.find()) {
          int n = Integer.parseInt(a.group(1));
          DataTypeComponent comp = null;
          for (DataTypeComponent x : s.getDefinedComponents()) if (a.group(2).equals(x.getFieldName())) { comp = x; break; }
          int off = comp != null ? comp.getOffset() + comp.getLength() : 4;
          max = Math.max(max, n * len + off);
        }
        Matcher b = plus.matcher(c);
        while (b.find()) max = Math.max(max, Integer.parseInt(b.group(1)) * len + 4);
        if (max > len) { need.merge(s, max, Math::max); cls.put(s, f.getParentNamespace().getName(true)); }
        return null;
      }
    };
    ParallelDecompiler.decompileFunctions(cb, fs, monitor);
    cb.dispose();
    int grown = 0;
    for (var e : need.entrySet()) {
      Structure s = e.getKey();
      int target = (e.getValue() + 3) & ~3;
      if (target > 0x100000) continue;  // nonsense
      s.growStructure(target - s.getLength());
      s.setDescription((s.getDescription() == null ? "" : s.getDescription() + "\n") + "Grown to PC size 0x" + Integer.toHexString(target));
      println("GROWN " + cls.get(s) + " 0x" + Integer.toHexString(target));
      grown++;
    }
    if (getScriptArgs().length > 0) {
      try (var w = java.nio.file.Files.newBufferedWriter(java.nio.file.Paths.get(getScriptArgs()[0]))) {
        for (var e : need.entrySet()) { w.write(cls.get(e.getKey())); w.write('\n'); }
      }
    }
    println("GROW " + grown + " class structs grown (of " + fs.size() + " methods scanned)");
  }
}
