// After auto-filling class layouts, an embedded (by-value) struct field can become larger than
// the slot it was placed in, which makes the parent invalid ("does not fit") and breaks
// decompilation. Re-apply such fields at their real size when the following bytes are free,
// otherwise turn them back into plain bytes.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import java.util.*;
public class RepairStructs extends GhidraScript {
  public void run() throws Exception {
    var dtm = currentProgram.getDataTypeManager();
    int grown = 0, cleared = 0, pass = 0, bad;
    do {
      bad = 0; pass++;
      List<Structure> all = new ArrayList<>();
      for (var it = dtm.getAllStructures(); it.hasNext();) all.add(it.next());
      for (Structure s : all) {
        if (s.getCategoryPath().getPath().startsWith("/DWARF")) continue;
        for (DataTypeComponent c : s.getDefinedComponents()) {
          DataType dt = c.getDataType();
          if (!(dt instanceof Composite) || dt.getLength() == c.getLength()) continue;
          bad++;
          int off = c.getOffset(), need = dt.getLength();
          boolean free = off + need <= s.getLength();
          for (int o = off + c.getLength(); free && o < off + need; o++) {
            var other = s.getComponentContaining(o);
            if (other != null && other.getDataType() != DataType.DEFAULT) free = false;
          }
          String name = c.getFieldName(), cmt = c.getComment();
          try {
            if (free && need > 0) { s.replaceAtOffset(off, dt, need, name, cmt); grown++; }
            else { s.clearAtOffset(off); cleared++; }
          } catch (Exception e) { s.clearAtOffset(off); cleared++; }
        }
      }
    } while (bad > 0 && pass < 10);
    println("REPAIR grew " + grown + " embedded fields, cleared " + cleared + " that no longer fit (" + pass + " passes)");
  }
}
