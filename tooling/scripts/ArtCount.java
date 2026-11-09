import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.parallel.*;
import ghidra.program.model.listing.*;
import java.util.*;
import java.util.regex.*;
public class ArtCount extends GhidraScript {
  public void run() throws Exception {
    Map<String,Integer> funcs = new TreeMap<>();
    var cfg = new DecompileConfigurer() { public void configure(DecompInterface d) { var o = new DecompileOptions(); o.grabFromProgram(currentProgram); d.setOptions(o); d.toggleSyntaxTree(false); } };
    var pat = Pattern.compile("\\b(unaff|extraout|in)_(r|f|cr|GQR)\\d+\\b");
    var cb = new DecompilerCallback<Void>(currentProgram, cfg) {
      public Void process(DecompileResults r, ghidra.util.task.TaskMonitor m) {
        if (r.getDecompiledFunction() == null) return null;
        Set<String> kinds = new HashSet<>(); var mm = pat.matcher(r.getDecompiledFunction().getC());
        while (mm.find()) kinds.add(mm.group(1) + "_" + mm.group(2));
        synchronized (funcs) { for (String k : kinds) funcs.merge(k, 1, Integer::sum); }
        return null;
      }
    };
    List<Function> fs = new ArrayList<>(); currentProgram.getFunctionManager().getFunctions(true).forEach(fs::add);
    ParallelDecompiler.decompileFunctions(cb, fs, monitor); cb.dispose();
    println("ARTCOUNT functions containing artifact kind: " + funcs);
  }
}
