// Resets the signatures of the listed functions to "unknown" (no declared parameters, decompiler infers
// them per call site), keeping name, namespace and comments. Used for imported GameCube signatures that
// make most of their PC callers decompile worse (e.g. float arguments passed differently by MSVC).
// Usage: -postScript ResetSignatures.java <addresses.txt>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.symbol.*;
import java.nio.file.*;
import java.util.*;

public class ResetSignatures extends GhidraScript {
  public void run() throws Exception {
    int n = 0;
    for (String l : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
      if (l.isBlank()) continue;
      Function f = getFunctionAt(toAddr(Long.parseLong(l.trim(), 16)));
      if (f == null || f.getSignatureSource() == SourceType.USER_DEFINED) continue;
      f.updateFunction("unknown", new ReturnParameterImpl(Undefined.getUndefinedDataType(4), currentProgram),
          new ArrayList<Variable>(), FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.DEFAULT);
      f.setSignatureSource(SourceType.DEFAULT);
      String c = f.getComment();
      f.setComment((c == null ? "" : c + "\n") + "GameCube signature not applied: it made most PC callers decompile worse");
      n++;
    }
    println("RESET " + n + " signatures to unknown");
  }
}
