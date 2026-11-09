// Exports functions, globals and data types of the current program as JSON Lines, for knowledge.db.
// Usage (headless): -postScript ExportKnowledge.java <output-dir>
// Writes <output-dir>/functions.jsonl, globals.jsonl, types.jsonl
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import com.google.gson.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ExportKnowledge extends GhidraScript {
  Gson gson = new GsonBuilder().disableHtmlEscaping().create();

  String str(Data d) {
    Object v = d.getValue();
    return v instanceof String s ? s : null;
  }

  public void run() throws Exception {
    Path out = Paths.get(getScriptArgs()[0]);
    Files.createDirectories(out);
    Listing listing = currentProgram.getListing();
    ReferenceManager refs = currentProgram.getReferenceManager();
    FunctionManager fm = currentProgram.getFunctionManager();

    try (var w = Files.newBufferedWriter(out.resolve("functions.jsonl"))) {
      for (Function f : fm.getFunctions(true)) {
        if (monitor.isCancelled()) break;
        JsonObject o = new JsonObject();
        o.addProperty("addr", f.getEntryPoint().getOffset());
        o.addProperty("name", f.getName());
        o.addProperty("full_name", f.getName(true));
        Namespace ns = f.getParentNamespace();
        o.addProperty("namespace", ns == null || ns.isGlobal() ? "" : ns.getName(true));
        o.addProperty("size", f.getBody().getNumAddresses());
        o.addProperty("signature", f.getPrototypeString(true, false));
        o.addProperty("callconv", f.getCallingConventionName());
        o.addProperty("sig_source", f.getSignatureSource().toString());
        o.addProperty("name_source", f.getSymbol().getSource().toString());
        o.addProperty("thunk", f.isThunk());
        o.addProperty("nparams", f.getParameterCount());
        JsonArray callees = new JsonArray();
        for (Function c : f.getCalledFunctions(monitor)) callees.add(c.getEntryPoint().getOffset());
        o.add("callees", callees);
        JsonArray strings = new JsonArray();
        JsonArray callSeq = new JsonArray();   // direct call targets in address order, with repeats
        JsonArray dataRefs = new JsonArray();  // non-string data addresses referenced, first-use order
        Set<String> seen = new LinkedHashSet<>();
        Set<Long> seenData = new HashSet<>();
        for (Instruction ins : listing.getInstructions(f.getBody(), true)) {
          for (Reference r : refs.getReferencesFrom(ins.getAddress())) {
            Address to = r.getToAddress();
            if (!to.isMemoryAddress()) continue;
            if (r.getReferenceType().isCall()) {
              Function c = fm.getFunctionAt(to);
              if (c != null) callSeq.add(to.getOffset());
              continue;
            }
            if (r.getReferenceType().isFlow()) continue;
            Data d = listing.getDataContaining(to);
            String s = d != null && d.getAddress().equals(to) ? str(d) : null;
            if (s != null) { if (seen.add(s) && seen.size() <= 64) strings.add(s); }
            else if (fm.getFunctionAt(to) == null && seenData.add(to.getOffset()) && seenData.size() <= 256)
              dataRefs.add(to.getOffset());
          }
        }
        o.add("strings", strings);
        o.add("call_seq", callSeq);
        o.add("data_refs", dataRefs);
        w.write(gson.toJson(o)); w.write('\n');
      }
    }

    try (var w = Files.newBufferedWriter(out.resolve("globals.jsonl"))) {
      for (Symbol s : currentProgram.getSymbolTable().getAllSymbols(false)) {
        if (s.getSymbolType() != SymbolType.LABEL || !s.getAddress().isMemoryAddress()) continue;
        if (fm.getFunctionAt(s.getAddress()) != null) continue;
        Data d = listing.getDataAt(s.getAddress());
        JsonObject o = new JsonObject();
        o.addProperty("addr", s.getAddress().getOffset());
        o.addProperty("name", s.getName(true));
        o.addProperty("source", s.getSource().toString());
        if (d != null) {
          o.addProperty("type", d.getDataType().getPathName());
          o.addProperty("size", d.getLength());
          String v = str(d);
          if (v != null) o.addProperty("string", v.length() > 512 ? v.substring(0, 512) : v);
        }
        o.addProperty("nrefs", refs.getReferenceCountTo(s.getAddress()));
        w.write(gson.toJson(o)); w.write('\n');
      }
    }

    try (var w = Files.newBufferedWriter(out.resolve("types.jsonl"))) {
      List<DataType> all = new ArrayList<>();
      currentProgram.getDataTypeManager().getAllDataTypes(all);
      for (DataType dt : all) {
        JsonObject o = new JsonObject();
        o.addProperty("path", dt.getPathName());
        o.addProperty("name", dt.getName());
        o.addProperty("kind", dt instanceof Structure ? "struct" : dt instanceof Union ? "union" : dt instanceof ghidra.program.model.data.Enum ? "enum"
            : dt instanceof TypeDef ? "typedef" : dt instanceof FunctionDefinition ? "funcdef" : "other");
        o.addProperty("size", dt.getLength());
        if (dt instanceof Composite c) {
          JsonArray fields = new JsonArray();
          for (DataTypeComponent m : c.getDefinedComponents()) {
            JsonObject fo = new JsonObject();
            fo.addProperty("off", m.getOffset());
            fo.addProperty("name", m.getFieldName());
            fo.addProperty("type", m.getDataType().getName());
            fo.addProperty("size", m.getLength());
            fields.add(fo);
          }
          o.add("fields", fields);
        } else if (dt instanceof ghidra.program.model.data.Enum e) {
          JsonObject vals = new JsonObject();
          for (String n : e.getNames()) vals.addProperty(n, e.getValue(n));
          o.add("values", vals);
        } else if (dt instanceof TypeDef t) {
          o.addProperty("base", t.getBaseDataType().getPathName());
        }
        w.write(gson.toJson(o)); w.write('\n');
      }
    }
    println("KNOWLEDGE exported to " + out);
  }
}
