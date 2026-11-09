// Applies field names/comments to struct layouts (from tooling/knowledge/field_names.py), and renames
// auto-named fields (field_0xNN) whose offset changed in the GCC->MSVC layout conversion.
// Rows: struct_path<TAB>offset<TAB>name<TAB>ctype<TAB>comment
// A named row renames the component starting at that offset (or creates one for simple types in an
// undefined area); an empty name only adds the comment.
// Usage: -postScript ApplyFieldKnowledge.java <fields.tsv>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import java.nio.file.*;
import java.util.*;

public class ApplyFieldKnowledge extends GhidraScript {
  DataType simpleType(String ctype, DataTypeManager dtm) {
    String t = ctype.trim();
    if (t.contains("[")) return null;
    if (t.endsWith("*")) return new PointerDataType(VoidDataType.dataType, 4, dtm);
    return switch (t) {
      case "int", "signed int", "long" -> IntegerDataType.dataType;
      case "unsigned int", "unsigned long", "unsigned" -> UnsignedIntegerDataType.dataType;
      case "short" -> ShortDataType.dataType;
      case "unsigned short" -> UnsignedShortDataType.dataType;
      case "char" -> CharDataType.dataType;
      case "unsigned char" -> ByteDataType.dataType;
      case "float" -> FloatDataType.dataType;
      case "double" -> DoubleDataType.dataType;
      default -> null;
    };
  }

  public void run() throws Exception {
    DataTypeManager dtm = currentProgram.getDataTypeManager();
    int renamed = 0, created = 0, commented = 0, autoRenamed = 0, missing = 0;
    // 1. auto names that no longer match their offset after layout conversion
    for (Iterator<Structure> it = dtm.getAllStructures(); it.hasNext(); ) {
      Structure s = it.next();
      for (DataTypeComponent c : s.getDefinedComponents()) {
        String n = c.getFieldName();
        if (n == null || !n.matches("field_0x[0-9a-f]+")) continue;
        String want = "field_0x" + Integer.toHexString(c.getOffset());
        if (!n.equals(want)) { try { c.setFieldName(want); autoRenamed++; } catch (Exception e) { } }
      }
    }
    // vtable pointers always keep the name _vptr (also repairs earlier runs)
    for (Iterator<Structure> it = dtm.getAllStructures(); it.hasNext(); ) {
      for (DataTypeComponent c : it.next().getDefinedComponents()) {
        if (c.getDataType().getName().contains("__vtbl_ptr_type") && !"_vptr".equals(c.getFieldName())) {
          try { c.setFieldName("_vptr"); } catch (Exception e) { }
        }
      }
    }
    // 2. names and notes
    for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
      String[] f = line.split("\t", -1);
      if (f.length < 5) continue;
      DataType dt = dtm.getDataType(f[0]);
      if (!(dt instanceof Structure s)) { missing++; continue; }
      int off = Integer.parseInt(f[1]);
      String name = f[2], comment = f[4];
      if (off >= s.getLength()) continue;
      DataTypeComponent c = s.getComponentContaining(off);
      if (c != null && c.getOffset() == off && c.getDataType() != DataType.DEFAULT) {
        if (!name.isEmpty() && !c.getDataType().getName().contains("__vtbl_ptr_type")) {
          try { c.setFieldName(name); renamed++; } catch (Exception e) { c.setFieldName(name + "_"); renamed++; }
        }
        c.setComment(comment); commented++;
      } else if (!name.isEmpty()) {
        DataType t = simpleType(f[3], dtm);
        if (t != null && off + t.getLength() <= s.getLength()) {
          boolean free = true;
          for (int i = off; i < off + t.getLength(); i++) {
            DataTypeComponent x = s.getComponentContaining(i);
            if (x != null && x.getDataType() != DataType.DEFAULT) { free = false; break; }
          }
          if (free) { s.replaceAtOffset(off, t, t.getLength(), name, comment); created++; }
        }
      }
    }
    println("FIELDS renamed=" + renamed + " created=" + created + " commented=" + commented
        + " autoRenamed=" + autoRenamed + " missingStructs=" + missing);
  }
}
