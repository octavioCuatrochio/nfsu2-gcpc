import ghidra.app.script.GhidraScript;
public class ListOpts extends GhidraScript {
  public void run() throws Exception {
    getCurrentAnalysisOptionsAndValues(currentProgram).forEach((k,v) -> { if (k.toLowerCase().contains("demangl")) println("OPT " + k + " = " + v); });
  }
}
