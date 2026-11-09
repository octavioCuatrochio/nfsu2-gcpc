// Analysis options for SN Systems GCC 2.95 GameCube ELFs.
import ghidra.app.script.GhidraScript;
public class ConfigureAnalysis extends GhidraScript {
  public void run() throws Exception {
    var p = currentProgram;
    // GCC 2.x mangling (Name__5ClassArgs, _._Class dtors, _4NS.var statics) needs the legacy demangler
    setAnalysisOption(p, "Demangler GNU", "true");
    setAnalysisOption(p, "Demangler GNU.Use Deprecated Demangler", "true");
    setAnalysisOption(p, "Demangler GNU.Demangler Format", "GNU");
    setAnalysisOption(p, "Demangler GNU.Demangle Only Known Mangled Symbols", "false");
    // the CodeWarrior demangler mis-parses GCC 2.x names
    setAnalysisOption(p, "Demangle CodeWarrior", "false");
    // DWARF1 debug info (GCC dwarfout)
    setAnalysisOption(p, "DWARF1", String.valueOf(!"nodwarf1".equals(getScriptArgs().length > 0 ? getScriptArgs()[0] : "")));
    getCurrentAnalysisOptionsAndValues(p).forEach((k, v) -> { if (k.startsWith("Demangl") || k.equals("DWARF1")) println("CONFIG " + k + " = " + v); });
  }
}
