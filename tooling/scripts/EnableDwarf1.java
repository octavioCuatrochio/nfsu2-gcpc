import ghidra.app.script.GhidraScript;
public class EnableDwarf1 extends GhidraScript {
  public void run() throws Exception {
    setAnalysisOption(currentProgram, "DWARF1", "true");
    println("DWARF1 enabled: " + getCurrentAnalysisOptionsAndValues(currentProgram).get("DWARF1") + " lang=" + currentProgram.getLanguageID());
  }
}
