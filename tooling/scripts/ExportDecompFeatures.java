// Decompiles every function (in parallel) and exports architecture-independent features for matching:
//   consts : integer constants the decompiled code uses (|v| >= 0x100, not addresses into the program)
//   cbr    : number of conditional branches, loops (back edges), call count, return-value use
// Output: <dir>/features.jsonl  — one {"addr", "consts", "cbr", "loops", "calls", "ops"} per function.
// Usage: -postScript ExportDecompFeatures.java <dir>
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.parallel.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.pcode.*;
import ghidra.program.model.block.*;
import com.google.gson.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;

public class ExportDecompFeatures extends GhidraScript {
  public void run() throws Exception {
    Path out = Paths.get(getScriptArgs()[0]);
    Files.createDirectories(out);
    var mem = currentProgram.getMemory();
    var space = currentProgram.getAddressFactory().getDefaultAddressSpace();
    ConcurrentLinkedQueue<String> lines = new ConcurrentLinkedQueue<>();
    Gson gson = new Gson();
    var cfg = new DecompileConfigurer() { public void configure(DecompInterface d) {
      var o = new DecompileOptions(); o.grabFromProgram(currentProgram); d.setOptions(o);
      d.toggleCCode(false); d.toggleSyntaxTree(true); d.setSimplificationStyle("decompile"); } };
    var cb = new DecompilerCallback<Void>(currentProgram, cfg) {
      public Void process(DecompileResults r, ghidra.util.task.TaskMonitor m) {
        HighFunction hf = r.getHighFunction();
        if (hf == null) return null;
        TreeSet<Long> consts = new TreeSet<>();
        int cbr = 0, calls = 0, ops = 0;
        Iterator<PcodeOpAST> it = hf.getPcodeOps();
        while (it.hasNext()) {
          PcodeOpAST op = it.next();
          ops++;
          int oc = op.getOpcode();
          if (oc == PcodeOp.CBRANCH) cbr++;
          if (oc == PcodeOp.CALL || oc == PcodeOp.CALLIND) calls++;
          if (oc == PcodeOp.CALL || oc == PcodeOp.BRANCH || oc == PcodeOp.CBRANCH) continue;
          for (int i = 0; i < op.getNumInputs(); i++) {
            Varnode v = op.getInput(i);
            if (v == null || !v.isConstant()) continue;
            if (oc == PcodeOp.PTRSUB || oc == PcodeOp.PTRADD) continue;  // field offsets / element sizes differ by ABI
            long val = v.getOffset();
            int size = v.getSize();
            if (size < 4 && size > 0) { long mask = (1L << (8 * size)) - 1; val &= mask; }
            long sval = size == 4 ? (int) val : val;
            if (Math.abs(sval) < 0x100) continue;
            if (size == 4 && mem.contains(space.getAddress(val & 0xFFFFFFFFL))) continue;  // pointer
            consts.add(val & 0xFFFFFFFFL);
          }
        }
        int loops = 0;
        for (PcodeBlockBasic b : hf.getBasicBlocks())
          for (int i = 0; i < b.getOutSize(); i++)
            if (b.getOut(i).getStart().compareTo(b.getStart()) <= 0) loops++;
        JsonObject o = new JsonObject();
        o.addProperty("addr", r.getFunction().getEntryPoint().getOffset());
        JsonArray c = new JsonArray(); consts.stream().limit(128).forEach(c::add);
        o.add("consts", c);
        o.addProperty("cbr", cbr); o.addProperty("loops", loops); o.addProperty("calls", calls); o.addProperty("ops", ops);
        lines.add(gson.toJson(o));
        return null;
      }
    };
    List<Function> fs = new ArrayList<>();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) if (!f.isExternal() && !f.isThunk()) fs.add(f);
    ParallelDecompiler.decompileFunctions(cb, fs, monitor);
    cb.dispose();
    try (var w = Files.newBufferedWriter(out.resolve("features.jsonl"))) {
      for (String l : lines) { w.write(l); w.write('\n'); }
    }
    println("FEATURES " + lines.size() + " of " + fs.size() + " functions");
  }
}
