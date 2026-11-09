// GCC 2.x mangling encodes enums exactly like classes, so the demangler creates an empty struct
// for every enum used in a signature. Passed by value, such an empty "struct" cannot be given a
// register and the real argument shows up as in_rN. A demangler placeholder that is passed by
// value and never received any fields (not a class with a layout) is an enum: make it a 4-byte enum.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import java.util.*;
public class FixEnumParams extends GhidraScript {
  public void run() throws Exception {
    var dtm = currentProgram.getDataTypeManager();
    Set<DataType> byValue = new LinkedHashSet<>();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true))
      for (Parameter p : f.getParameters()) {
        DataType d = p.getDataType();
        if (d.getCategoryPath().getPath().startsWith("/DWARF")) continue;
        // empty struct placeholder, or the demangler's 1-byte "typedef undefined" placeholder
        if (d instanceof Structure s && s.getNumDefinedComponents() == 0) byValue.add(d);
        else if (d instanceof TypeDef t && d.getCategoryPath().getPath().startsWith("/Demangler")
                 && (t.getBaseDataType() == DataType.DEFAULT || t.getBaseDataType() instanceof Undefined)) byValue.add(d);
      }
    // a type that names a class with methods is a class, never an enum
    Set<String> classesWithMethods = new HashSet<>();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true))
      if (f.getParentNamespace() instanceof GhidraClass c) classesWithMethods.add(c.getName());
    int n = 0;
    for (DataType s : byValue) {
      if (classesWithMethods.contains(s.getName())) continue;
      var e = new EnumDataType(s.getCategoryPath(), s.getName(), 4, dtm);
      try { dtm.replaceDataType(s, e, true); n++; } catch (Exception ex) { println("ENUMFIX fail " + s.getPathName() + ": " + ex.getMessage()); }
    }
    println("ENUMFIX converted " + n + " by-value placeholder types to 4-byte enums");
  }
}
