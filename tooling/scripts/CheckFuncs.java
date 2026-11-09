import ghidra.app.script.GhidraScript;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;
public class CheckFuncs extends GhidraScript {
  public void run() throws Exception {
    var log = new ghidra.app.util.importer.MessageLog();
    new com.github.rafalh.ghidra.dwarfone.DWARF1Analyzer().added(currentProgram, currentProgram.getMemory(), monitor, log);
    int n = 0;
    for (String line : Files.readAllLines(Path.of(getScriptArgs()[0]))) {
      String[] p = line.split(" ");
      var a = toAddr(Long.parseLong(p[0], 16));
      var f = getFunctionAt(a);
      String why = f == null ? (getFunctionContaining(a) == null ? "NO FUNCTION" : "INSIDE " + getFunctionContaining(a).getName(true)) : (f.getSignatureSource() != SourceType.IMPORTED ? "NOT IMPORTED sig=" + f.getSignatureSource() + " name=" + f.getName(true) : null);
      if (why != null) { n++; if (n <= 40) println("MISS " + line + " -> " + why + " block=" + currentProgram.getMemory().getBlock(a)); }
    }
    println("MISS total " + n);
  }
}
