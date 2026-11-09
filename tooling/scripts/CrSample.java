import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.util.*;
import java.util.regex.*;
public class CrSample extends GhidraScript {
  public void run() throws Exception {
    var ifc = new DecompInterface(); var o = new DecompileOptions(); o.grabFromProgram(currentProgram); ifc.setOptions(o); ifc.openProgram(currentProgram);
    Map<String,Integer> regs = new TreeMap<>(); int shown = 0, n = 0;
    var pat = Pattern.compile("\\b(unaff|in)_(cr\\d|r\\d+|f\\d+)\\b");
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      if (n++ % 3 != 0) continue; // sample a third of the program
      var r = ifc.decompileFunction(f, 20, monitor); if (r.getDecompiledFunction() == null) continue;
      String c = r.getDecompiledFunction().getC(); var m = pat.matcher(c); Set<String> s = new TreeSet<>();
      while (m.find()) s.add(m.group());
      for (String x : s) regs.merge(x, 1, Integer::sum);
      if (shown < 3 && s.stream().anyMatch(x -> x.contains("_cr"))) {
        shown++; println("CRS ===== " + f.getName(true) + " " + s);
        for (String line : c.split("\n")) if (line.matches(".*\\b(unaff|in)_cr\\d.*")) println("CRS    " + line.trim());
        var it = currentProgram.getListing().getInstructions(f.getBody(), true); int k = 0;
        while (it.hasNext() && k < 400) { var ins = it.next(); k++; String t = ins.toString(); if (t.contains("cr") || t.startsWith("mfcr") || t.startsWith("mtcr")) println("CRS    asm " + ins.getAddress() + " " + t); }
      }
    }
    println("CRS register usage (sampled): " + regs);
  }
}
