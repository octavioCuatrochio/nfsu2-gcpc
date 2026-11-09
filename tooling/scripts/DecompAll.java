import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.parallel.*;
import ghidra.program.model.listing.*;
import java.util.*;
import java.util.concurrent.atomic.*;
public class DecompAll extends GhidraScript {
  public void run() throws Exception {
    AtomicInteger ok = new AtomicInteger(), fail = new AtomicInteger(), warnUnaff = new AtomicInteger(), inR = new AtomicInteger();
    List<String> errs = Collections.synchronizedList(new ArrayList<>());
    var cfg = new DecompileConfigurer() { public void configure(DecompInterface d) { var o = new DecompileOptions(); o.grabFromProgram(currentProgram); d.setOptions(o); d.toggleSyntaxTree(false); } };
    var cb = new DecompilerCallback<Void>(currentProgram, cfg) {
      public Void process(DecompileResults r, ghidra.util.task.TaskMonitor m) {
        if (r.getDecompiledFunction() == null) { fail.incrementAndGet(); if (errs.size() < 15) errs.add(r.getFunction().getName(true) + ": " + String.valueOf(r.getErrorMessage()).trim()); }
        else { ok.incrementAndGet(); String c = r.getDecompiledFunction().getC(); if (c.contains("unaff_")) warnUnaff.incrementAndGet(); if (c.matches("(?s).*\\bin_r\\d+\\b.*")) inR.incrementAndGet(); }
        return null;
      }
    };
    List<Function> fs = new ArrayList<>(); currentProgram.getFunctionManager().getFunctions(true).forEach(fs::add);
    ParallelDecompiler.decompileFunctions(cb, fs, monitor);
    cb.dispose();
    println("DALL ok=" + ok + " failed=" + fail + " withUnaff=" + warnUnaff + " withInR=" + inR);
    for (String e : errs) println("DERR " + e);
  }
}
