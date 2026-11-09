import ghidra.app.script.GhidraScript;
public class DisableDwarf1 extends GhidraScript {
  public void run() throws Exception { setAnalysisOption(currentProgram, "DWARF1", "false"); }
}
