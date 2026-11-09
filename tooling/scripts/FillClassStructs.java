// Builds layouts for classes the debug info does not describe (game code), using Ghidra's
// "Auto Fill in Class Structure" logic on the 'this' parameter of every method of the class.
// Offsets come from real loads/stores; field names are generic (field_0x..). Classes with a
// DWARF definition are left alone.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.util.FillOutStructureHelper;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.util.*;

public class FillClassStructs extends GhidraScript {
  public void run() throws Exception {
    Set<String> only = new HashSet<>(Arrays.asList(getScriptArgs()));
    var helper = new FillOutStructureHelper(currentProgram, monitor);
    var opts = new DecompileOptions(); opts.grabFromProgram(currentProgram);
    DecompInterface ifc = helper.setUpDecompiler(opts);
    ifc.openProgram(currentProgram);
    // group __thiscall methods by class
    Map<GhidraClass, List<Function>> byClass = new LinkedHashMap<>();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      if (f.getParentNamespace() instanceof GhidraClass c && "__thiscall".equals(f.getCallingConventionName())) {
        if (!only.isEmpty() && !only.contains(c.getName(true))) continue;
        byClass.computeIfAbsent(c, k -> new ArrayList<>()).add(f);
      }
    }
    // vtable pointer offsets: constructors store "<Class>_virtual_table" into this+off
    Map<GhidraClass, Set<Integer>> vptrs = new HashMap<>();
    var plain = new DecompInterface(); plain.setOptions(opts); plain.openProgram(currentProgram);
    var vpat = java.util.regex.Pattern.compile("\\(this \\+ (0x[0-9a-f]+)\\) = \\w*virtual_table|\\*\\(\\w+ \\*\\*?\\)this = \\w*virtual_table");
    for (var e : byClass.entrySet()) {
      for (Function f : e.getValue()) {
        if (!f.getName().equals(e.getKey().getName())) continue; // constructors only
        DecompileResults r;
        try { r = plain.decompileFunction(f, 30, monitor); } catch (Exception ex) { continue; }
        if (r == null || r.getDecompiledFunction() == null) continue;
        var m = vpat.matcher(r.getDecompiledFunction().getC());
        while (m.find()) vptrs.computeIfAbsent(e.getKey(), k -> new TreeSet<>()).add(m.group(1) == null ? 0 : Integer.decode(m.group(1)));
      }
    }
    plain.dispose();
    DataType vtblEntry = currentProgram.getDataTypeManager().getDataType(new CategoryPath("/DWARF"), "__vtbl_ptr_type");
    int classes = 0, methods = 0, skippedDwarf = 0, vptrTyped = 0, renamedFields = 0, failures = 0;
    monitor.initialize(byClass.size());
    for (var e : byClass.entrySet()) {
      if (monitor.isCancelled()) break;
      monitor.incrementProgress(1);
      boolean touched = false;
      for (Function f : e.getValue()) {
        int thisIdx = -1;
        for (int i = 0; i < f.getParameterCount(); i++) if ("this".equals(f.getParameter(i).getName())) { thisIdx = i; break; }
        Parameter thisParam = thisIdx >= 0 ? f.getParameter(thisIdx) : null;
        if (thisParam == null || !(thisParam.getDataType() instanceof Pointer p)) continue;
        DataType target = p.getDataType();
        if (target.getCategoryPath().getPath().startsWith("/DWARF")) { skippedDwarf++; break; }
        try {
          var res = ifc.decompileFunction(f, 30, monitor);
          if (res.getHighFunction() == null) continue;
          var lsm = res.getHighFunction().getLocalSymbolMap();
          if (lsm.getNumParams() <= thisIdx) continue;
          var sym = lsm.getParamSymbol(thisIdx);
          if (sym == null || sym.getHighVariable() == null) continue;
          helper.processStructure(sym.getHighVariable(), f, false, true, ifc);
          methods++; touched = true;
        } catch (Exception ex) { failures++; }
      }
      if (touched) classes++;
      Structure st = null;
      for (Function f : e.getValue()) {
        Parameter tp = null; for (Parameter q : f.getParameters()) if ("this".equals(q.getName())) tp = q;
        if (tp != null && tp.getDataType() instanceof Pointer p && p.getDataType() instanceof Structure s0
            && !s0.getCategoryPath().getPath().startsWith("/DWARF")) { st = s0; break; }
      }
      if (st == null) continue;
      for (int off : vptrs.getOrDefault(e.getKey(), Set.of())) {
        if (vtblEntry == null || off + 4 > st.getLength()) continue;
        try { st.replaceAtOffset(off, new PointerDataType(vtblEntry, currentProgram.getDataTypeManager()), 4, vptrs.get(e.getKey()).size() == 1 ? "_vptr" : "_vptr_" + Integer.toHexString(off), null); vptrTyped++; }
        catch (Exception ex) { }
      }
      for (DataTypeComponent c : st.getDefinedComponents()) {
        String fn = c.getFieldName();
        if (fn == null || fn.matches("field\\d+_0x[0-9a-f]+")) {
          try { c.setFieldName("field_0x" + Integer.toHexString(c.getOffset())); renamedFields++; } catch (Exception ex) { }
        }
      }
    }
    ifc.dispose();
    println("FILL vptr fields typed: " + vptrTyped + ", fields renamed: " + renamedFields);
    println("FILL extended " + classes + " class structures from " + methods + " methods (" + skippedDwarf + " DWARF classes left as-is, " + failures + " methods failed)");
  }
}
