// GameCube side of type transfer: writes every data type of the program into a Ghidra data type archive,
// and every function signature (with type paths) as JSON Lines.
// Usage: -postScript ExportTypesAndSignatures.java <out-dir>   -> <out-dir>/gc_types.gdt, signatures.jsonl
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import com.google.gson.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ExportTypesAndSignatures extends GhidraScript {
  public void run() throws Exception {
    Path out = Paths.get(getScriptArgs()[0]);
    Files.createDirectories(out);
    File gdt = out.resolve("gc_types.gdt").toFile();
    if (gdt.exists()) gdt.delete();
    FileDataTypeManager arch = FileDataTypeManager.createFileArchive(gdt);
    DataTypeManager src = currentProgram.getDataTypeManager();
    int tx = arch.startTransaction("copy");
    int n = 0;
    try {
      List<DataType> all = new ArrayList<>();
      src.getAllDataTypes(all);
      for (DataType dt : all) {
        if (dt instanceof BuiltInDataType) continue;
        arch.resolve(dt, DataTypeConflictHandler.KEEP_HANDLER);
        n++;
      }
    } finally {
      arch.endTransaction(tx, true);
    }
    arch.save();
    arch.close();

    Gson gson = new Gson();
    int s = 0;
    try (var w = Files.newBufferedWriter(out.resolve("signatures.jsonl"))) {
      for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
        if (f.getSignatureSource() == ghidra.program.model.symbol.SourceType.DEFAULT) continue;
        JsonObject o = new JsonObject();
        o.addProperty("addr", f.getEntryPoint().getOffset());
        o.addProperty("callconv", f.getCallingConventionName());
        o.addProperty("ret", f.getReturnType().getPathName());
        o.addProperty("varargs", f.hasVarArgs());
        o.addProperty("source", f.getSignatureSource().toString());
        JsonArray ps = new JsonArray();
        for (Parameter p : f.getParameters()) {
          JsonObject po = new JsonObject();
          po.addProperty("name", p.getName());
          po.addProperty("type", p.getDataType().getPathName());
          po.addProperty("auto", p.isAutoParameter());
          ps.add(po);
        }
        o.add("params", ps);
        w.write(gson.toJson(o)); w.write('\n');
        s++;
      }
    }
    println("TYPES exported " + n + " types to " + gdt + ", " + s + " signatures");
  }
}
