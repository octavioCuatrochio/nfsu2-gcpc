// Replaces the empty placeholder types the demangler creates (e.g. /Demangler/RCMP/FRAME)
// with the real DWARF definition of the same name, so every signature and variable that used
// the placeholder now uses the full struct/enum/typedef.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import java.util.*;

public class MergeDemanglerTypes extends GhidraScript {
  private static boolean isPlaceholder(DataType dt) {
    if (dt instanceof Structure s) return s.isNotYetDefined() || s.getNumDefinedComponents() == 0;
    if (dt instanceof Union u) return u.getNumComponents() == 0;
    return false;
  }
  /** struct/union/enum, or a typedef of one: what a demangler class placeholder can stand for */
  private static boolean isAggregateLike(DataType dt) {
    while (dt instanceof TypeDef t) dt = t.getDataType();
    return dt instanceof Composite || dt instanceof ghidra.program.model.data.Enum;
  }
  public void run() throws Exception {
    var dtm = currentProgram.getDataTypeManager();
    // DWARF types by simple name; the root /DWARF category wins over per-CU collision categories
    Map<String, DataType> dwarf = new HashMap<>();
    List<DataType> demangler = new ArrayList<>();
    for (var it = dtm.getAllDataTypes(); it.hasNext();) {
      DataType dt = it.next();
      String cat = dt.getCategoryPath().getPath();
      if (cat.equals("/DWARF")) dwarf.put(dt.getName(), dt);
      else if (cat.startsWith("/DWARF/")) dwarf.putIfAbsent(dt.getName(), dt);
      else if (cat.startsWith("/Demangler")) demangler.add(dt);
    }
    int merged = 0, skipped = 0;
    for (DataType d : demangler) {
      if (!isPlaceholder(d)) continue;
      DataType real = dwarf.get(d.getName());
      if (real == null || isPlaceholder(real) || !isAggregateLike(real)) { skipped++; continue; }
      try {
        dtm.replaceDataType(d, real, false);
        merged++;
      } catch (Exception e) {
        println("MERGE fail " + d.getPathName() + ": " + e.getMessage());
      }
    }
    println("MERGE replaced " + merged + " demangler placeholders with DWARF types (" + skipped + " have no DWARF definition)");
  }
}
