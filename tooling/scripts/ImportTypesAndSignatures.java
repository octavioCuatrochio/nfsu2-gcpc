// PC side of type transfer (MSVC x86):
//  1. copies all types from the GameCube archive (gc_types.gdt) into the program
//  2. converts class layouts from GCC 2.95 to MSVC: GCC puts the vtable pointer after the fields of the
//     first polymorphic class, MSVC at offset 0 -> vptr moves to 0, fields that were before it move up by 4
//  3. applies GameCube signatures to matched PC functions: methods become __thiscall (this in ECX), the
//     hidden struct-return pointer becomes the first stack argument, callee-cleanup free functions __stdcall
// Usage: -postScript ImportTypesAndSignatures.java <gc_types.gdt> <signatures.jsonl> <pairs.tsv>
//   pairs.tsv: pc_addr_hex<TAB>gc_addr_hex
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.symbol.*;
import com.google.gson.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ImportTypesAndSignatures extends GhidraScript {
  DataTypeManager dtm;
  Map<String, DataType> cache = new HashMap<>();

  DataType resolveType(String path) {
    if (path == null) return Undefined4DataType.dataType;
    if (cache.containsKey(path)) return cache.get(path);
    DataType dt = null;
    String p = path.trim();
    int stars = 0;
    while (p.endsWith("*")) { stars++; p = p.substring(0, p.length() - 1).trim(); }
    List<Integer> dims = new ArrayList<>();
    while (p.endsWith("]")) {
      int i = p.lastIndexOf('[');
      dims.add(0, Integer.parseInt(p.substring(i + 1, p.length() - 1)));
      p = p.substring(0, i).trim();
    }
    dt = dtm.getDataType(p);
    if (dt == null) {
      String name = p.substring(p.lastIndexOf('/') + 1);
      dt = switch (name) {
        case "void" -> VoidDataType.dataType;
        case "bool" -> BooleanDataType.dataType;
        case "char" -> CharDataType.dataType;
        case "uchar", "byte" -> ByteDataType.dataType;
        case "short" -> ShortDataType.dataType;
        case "ushort", "word" -> UnsignedShortDataType.dataType;
        case "int" -> IntegerDataType.dataType;
        case "uint", "dword" -> UnsignedIntegerDataType.dataType;
        case "long" -> IntegerDataType.dataType;   // 32-bit on both targets
        case "ulong" -> UnsignedIntegerDataType.dataType;
        case "longlong" -> LongLongDataType.dataType;
        case "ulonglong" -> UnsignedLongLongDataType.dataType;
        case "float" -> FloatDataType.dataType;
        case "double" -> DoubleDataType.dataType;
        case "undefined1" -> Undefined1DataType.dataType;
        case "undefined2" -> Undefined2DataType.dataType;
        case "undefined4", "undefined" -> Undefined4DataType.dataType;
        case "undefined8" -> Undefined8DataType.dataType;
        default -> null;
      };
    }
    if (dt == null) { cache.put(path, null); return null; }
    for (int d = dims.size() - 1; d >= 0; d--) dt = new ArrayDataType(dt, dims.get(d), dt.getLength());
    for (int i = 0; i < stars; i++) dt = new PointerDataType(dt, 4, dtm);
    cache.put(path, dt);
    return dt;
  }

  Set<String> copiedPaths = new HashSet<>();

  int convertVptrLayouts() throws Exception {
    int n = 0;
    List<Structure> todo = new ArrayList<>();
    for (Iterator<Structure> it = dtm.getAllStructures(); it.hasNext(); ) {
      Structure s = it.next();
      // only layouts that came from the GameCube archive in this run: everything else is already PC-shaped,
      // and a PC layout may legitimately hold a vptr at offset > 0 (secondary base class)
      if (!copiedPaths.contains(s.getPathName())) continue;
      // idempotent: a converted (or PC-native) layout may legitimately hold a vptr at offset > 0
      // (secondary base class), so never convert twice
      if (s.getDescription() != null && s.getDescription().contains("Layout converted")) continue;
      for (DataTypeComponent c : s.getDefinedComponents())
        if ("_vptr".equals(c.getFieldName()) && c.getOffset() > 0) { todo.add(s); break; }
    }
    for (Structure s : todo) {
      DataTypeComponent vptr = null;
      List<DataTypeComponent> comps = new ArrayList<>();
      for (DataTypeComponent c : s.getDefinedComponents()) {
        if ("_vptr".equals(c.getFieldName()) && vptr == null) vptr = c; else comps.add(c);
      }
      int x = vptr.getOffset();
      StructureDataType ns = new StructureDataType(s.getCategoryPath(), s.getName(), s.getLength(), dtm);
      ns.replaceAtOffset(0, vptr.getDataType(), 4, "_vptr", "moved from GCC offset 0x" + Integer.toHexString(x));
      boolean ok = true;
      for (DataTypeComponent c : comps) {
        int off = c.getOffset() < x ? c.getOffset() + 4 : c.getOffset();
        if (off + c.getLength() > ns.getLength()) { ok = false; break; }
        try {
          ns.replaceAtOffset(off, c.getDataType(), c.getLength(), c.getFieldName(),
              c.getOffset() != off ? "GCC offset 0x" + Integer.toHexString(c.getOffset()) : c.getComment());
        } catch (Exception e) { ok = false; break; }
      }
      if (!ok) continue;
      s.replaceWith(ns);
      s.setDescription((s.getDescription() == null ? "" : s.getDescription() + "\n")
          + "Layout converted from GCC 2.95 (vptr at 0x" + Integer.toHexString(x) + ") to MSVC (vptr at 0)");
      n++;
    }
    return n;
  }

  boolean calleeCleans(Function f) {
    for (Instruction ins : currentProgram.getListing().getInstructions(f.getBody(), true))
      if (ins.getMnemonicString().equals("RET") && ins.getNumOperands() == 1) return true;
    return false;
  }

  public void run() throws Exception {
    String[] a = getScriptArgs();
    dtm = currentProgram.getDataTypeManager();
    // 1. types
    FileDataTypeManager arch = FileDataTypeManager.openFileArchive(new File(a[0]), false);
    List<DataType> all = new ArrayList<>();
    arch.getAllDataTypes(all);
    int copied = 0;
    for (DataType dt : all) {
      if (dt instanceof BuiltInDataType) continue;
      if (dtm.getDataType(dt.getDataTypePath()) != null) continue;
      DataType r = dtm.resolve(dt, DataTypeConflictHandler.KEEP_HANDLER);
      copiedPaths.add(r.getPathName());
      copied++;
    }
    arch.close();
    println("TRANSFER types copied: " + copied);
    // 2. layouts
    println("TRANSFER layouts converted to MSVC vptr placement: " + convertVptrLayouts());
    // 3. signatures
    Map<Long, JsonObject> sigs = new HashMap<>();
    for (String l : Files.readAllLines(Paths.get(a[1]))) {
      JsonObject o = JsonParser.parseString(l).getAsJsonObject();
      sigs.put(o.get("addr").getAsLong(), o);
    }
    int applied = 0, skipped = 0, partial = 0;
    for (String l : Files.readAllLines(Paths.get(a[2]))) {
      String[] c = l.split("\t");
      Function f = getFunctionAt(toAddr(Long.parseLong(c[0], 16)));
      JsonObject s = sigs.get(Long.parseLong(c[1], 16));
      if (f == null || s == null || f.getSignatureSource() == SourceType.USER_DEFINED) { skipped++; continue; }
      JsonArray ps = s.getAsJsonArray("params");
      JsonObject thisP = null, retP = null;
      List<JsonObject> rest = new ArrayList<>();
      for (JsonElement e : ps) {
        JsonObject p = e.getAsJsonObject();
        String n = p.get("name").getAsString();
        if (n.equals("__in_chrg") || n.equals("__vtt_parm")) continue;  // GCC 2.95 hidden destructor/ctor flags
        if (n.equals("this") && thisP == null) thisP = p;
        else if ((n.equals("__return_storage_ptr__") || n.equals("__return_ptr") || n.equals("ret_ptr") || n.startsWith("__retptr")) && retP == null) retP = p;
        else rest.add(p);
      }
      boolean method = thisP != null;
      List<Variable> params = new ArrayList<>();
      boolean missing = false;
      try {
        List<JsonObject> order = new ArrayList<>();
        if (method) order.add(thisP);
        if (retP != null) order.add(retP);
        order.addAll(rest);
        for (JsonObject p : order) {
          DataType dt = resolveType(p.get("type").getAsString());
          if (dt == null) { dt = Undefined4DataType.dataType; missing = true; }
          String pname = p.get("name").getAsString();
          if (pname.equals("__return_storage_ptr__")) pname = "__return_ptr";
          params.add(new ParameterImpl(pname, dt, currentProgram));
        }
        String retPath = s.get("ret").getAsString();
        DataType ret = retPath.equals("/undefined") ? f.getReturnType() : resolveType(retPath);
        if (ret == null) { ret = f.getReturnType(); missing = true; }
        String cc = method ? "__thiscall" : (calleeCleans(f) ? "__stdcall" : "__cdecl");
        f.updateFunction(cc, new ReturnParameterImpl(ret, currentProgram), params,
            FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.IMPORTED);
        f.setVarArgs(s.get("varargs").getAsBoolean());
        applied++;
        if (missing) partial++;
      } catch (Exception e) {
        skipped++;
        if (skipped < 15) println("TRANSFER skip " + f.getEntryPoint() + " " + f.getName() + ": " + e.getMessage());
      }
    }
    println("TRANSFER signatures applied " + applied + " (" + partial + " with unresolved types), skipped " + skipped);
  }
}
