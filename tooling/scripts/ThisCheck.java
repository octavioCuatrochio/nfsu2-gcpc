import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.util.*;
public class ThisCheck extends GhidraScript {
  public void run() throws Exception {
    for (String n : new String[]{"AICarController::GetRelativePositionTo","RaceCarController::ShouldWeAvoidThisAvoidable"}) {
      for (Function f : currentProgram.getFunctionManager().getFunctions(true)) if (f.getName(true).equals(n)) {
        var syms = currentProgram.getSymbolTable().getSymbols(f.getEntryPoint());
        List<String> names = new ArrayList<>(); for (Symbol s : syms) names.add(s.getName(true) + "[" + s.getSource() + "]");
        println("THIS " + n + " cc=" + f.getCallingConventionName() + " sigSrc=" + f.getSignatureSource() + " sig=" + f.getPrototypeString(true,false) + " syms=" + names);
      }
    }
    Map<String,Integer> cc = new TreeMap<>(); int memberish = 0;
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      if (!(f.getParentNamespace() instanceof GhidraClass)) continue;
      cc.merge(f.getCallingConventionName() + "/" + f.getSignatureSource(), 1, Integer::sum);
    }
    println("THIS class-function conventions: " + cc);
    println("THIS cspec conventions: " + Arrays.toString(currentProgram.getCompilerSpec().getCallingConventions()));
  }
}
