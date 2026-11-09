import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.parallel.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.util.*;
import java.util.regex.*;
public class Artifacts extends GhidraScript {
  public void run() throws Exception {
    Map<String,Integer> unaff = new TreeMap<>(), inr = new TreeMap<>();
    List<String> ex = Collections.synchronizedList(new ArrayList<>());
    var cfg = new DecompileConfigurer() { public void configure(DecompInterface d) { var o = new DecompileOptions(); o.grabFromProgram(currentProgram); d.setOptions(o); d.toggleSyntaxTree(false); } };
    var cb = new DecompilerCallback<Void>(currentProgram, cfg) {
      public Void process(DecompileResults r, ghidra.util.task.TaskMonitor m) {
        if (r.getDecompiledFunction() == null) return null;
        Function f = r.getFunction(); String c = r.getDecompiledFunction().getC();
        String kind = f.getSignatureSource() == SourceType.IMPORTED ? "dwarf" : f.getParentNamespace() instanceof GhidraClass ? "class:" + f.getCallingConventionName() : f.getParentNamespace().isGlobal() ? (f.getSignatureSource() == SourceType.DEFAULT ? "plain-nosig" : "free-demangled") : "namespace";
        Set<String> seenU = new HashSet<>(), seenR = new HashSet<>();
        Matcher m1 = Pattern.compile("\\bunaff_(\\w+)").matcher(c); while (m1.find()) seenU.add(m1.group(1));
        Matcher m2 = Pattern.compile("\\bin_(r\\d+|f\\d+)\\b").matcher(c); while (m2.find()) seenR.add(m2.group(1));
        synchronized (unaff) {
          for (String u : seenU) unaff.merge(u.replaceAll("\\d+$", "N") + " [" + kind + "]", 1, Integer::sum);
          if (!seenR.isEmpty()) inr.merge(kind, 1, Integer::sum);
          if (!seenR.isEmpty() && ex.size() < 40 && !f.hasVarArgs()) ex.add(kind + " " + f.getName(true) + " " + f.getPrototypeString(false,false) + " uses " + seenR);
        }
        return null;
      }
    };
    List<Function> fs = new ArrayList<>(); currentProgram.getFunctionManager().getFunctions(true).forEach(fs::add);
    ParallelDecompiler.decompileFunctions(cb, fs, monitor); cb.dispose();
    println("ART unaff by reg/kind: " + unaff);
    println("ART in_r functions by kind: " + inr);
    for (String e : ex) println("ARTEX " + e);
  }
}
