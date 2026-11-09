import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.symbol.SourceType;
public class Stats extends GhidraScript {
  public void run() throws Exception {
    var fm = currentProgram.getFunctionManager();
    int total=0, withParams=0, typedRet=0, sigImported=0, withLocals=0;
    for (var f : fm.getFunctions(true)) {
      total++;
      if (f.getParameterCount() > 0) withParams++;
      var rt = f.getReturnType();
      if (!(rt instanceof Undefined) && !(rt instanceof DefaultDataType)) typedRet++;
      if (f.getSignatureSource() == SourceType.IMPORTED || f.getSignatureSource() == SourceType.USER_DEFINED) sigImported++;
      if (f.getLocalVariables().length > 0) withLocals++;
    }
    var dtm = currentProgram.getDataTypeManager();
    int structs=0; var it = dtm.getAllStructures(); while (it.hasNext()) { it.next(); structs++; }
    println("STAT lang=" + currentProgram.getLanguageID() + " functions=" + total + " sigFromDebug=" + sigImported + " withParams=" + withParams + " typedReturn=" + typedRet + " withLocals=" + withLocals);
    println("STAT datatypes=" + dtm.getDataTypeCount(true) + " structs=" + structs);
    int n=0;
    for (var f : fm.getFunctions(true)) {
      if (f.getSignatureSource() == SourceType.IMPORTED && f.getParameterCount() > 1 && n < 10) { println("SAMPLE " + f.getPrototypeString(false, false)); n++; }
    }
  }
}
