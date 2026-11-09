// Types remaining vtable-pointer fields of game classes: constructors that store
// "<Class>_virtual_table" into this->field_0xNN get that field typed __vtbl_ptr_type * _vptr.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import java.util.regex.*;
public class FixVptrs extends GhidraScript {
  public void run() throws Exception {
    var dtm = currentProgram.getDataTypeManager();
    DataType entry = dtm.getDataType(new CategoryPath("/DWARF"), "__vtbl_ptr_type");
    var ifc = new DecompInterface(); var o = new DecompileOptions(); o.grabFromProgram(currentProgram); ifc.setOptions(o); ifc.openProgram(currentProgram);
    Pattern pat = Pattern.compile("^\\s*this->field_0x([0-9a-f]+) = \\(?[\\w ]*\\*?\\)?\\w*virtual_table;", Pattern.MULTILINE);
    int typed = 0;
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      if (!(f.getParentNamespace() instanceof GhidraClass c) || !f.getName().equals(c.getName())) continue;
      Parameter tp = null; for (Parameter q : f.getParameters()) if ("this".equals(q.getName())) tp = q;
      if (tp == null || !(tp.getDataType() instanceof Pointer p) || !(p.getDataType() instanceof Structure s)) continue;
      if (s.getCategoryPath().getPath().startsWith("/DWARF")) continue;
      var r = ifc.decompileFunction(f, 30, monitor); if (r.getDecompiledFunction() == null) continue;
      Matcher m = pat.matcher(r.getDecompiledFunction().getC());
      while (m.find()) {
        int off = Integer.parseInt(m.group(1), 16);
        if (off + 4 > s.getLength()) continue;
        var comp = s.getComponentAt(off);
        if (comp != null && comp.getDataType() instanceof Pointer pp && pp.getDataType() == entry) continue;
        s.replaceAtOffset(off, new PointerDataType(entry, dtm), 4, "_vptr", null);
        typed++;
      }
    }
    println("VPTRFIX typed " + typed + " more vtable pointers");
  }
}
