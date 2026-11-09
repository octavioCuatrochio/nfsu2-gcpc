// PC (MSVC x86): checks imported signatures against what the machine code proves, and fixes them.
//  - __thiscall/__stdcall callees pop their own stack arguments ("ret N"), so N bytes = stack parameters.
//    Too many parameters -> extras dropped from the end; too few -> undefined4 parameters appended.
//    A function that pops nothing is not __stdcall (-> __cdecl).
//  - functions listed in the optional file (class members that read ECX undeclared, found in the
//    decompiled output as in_ECX) become __thiscall with `this` typed as their class.
// Writes <out>/signature_mismatches.tsv (pc_addr, name, declared_bytes, ret_bytes) for knowledge.db.
// Usage: -postScript ValidateSignatures.java <out-dir> [ecx_methods.txt]
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ValidateSignatures extends GhidraScript {
  int retBytes(Function f) {
    int purge = -1;
    for (Instruction ins : currentProgram.getListing().getInstructions(f.getBody(), true)) {
      if (!ins.getMnemonicString().equals("RET")) continue;
      int n = 0;
      if (ins.getNumOperands() == 1 && ins.getScalar(0) != null) n = (int) ins.getScalar(0).getUnsignedValue();
      purge = Math.max(purge, n);
    }
    return purge;  // -1: no RET found (tail call / noreturn): unknown
  }

  /** smallest "add esp, N" directly after a direct call to f (caller cleanup = cdecl argument bytes); -1 if none */
  int callerCleanup(Function f) {
    int min = Integer.MAX_VALUE, sites = 0;
    for (var r : currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint())) {
      if (!r.getReferenceType().isCall()) continue;
      Instruction call = getInstructionAt(r.getFromAddress());
      if (call == null) continue;
      Instruction next = call.getNext();
      sites++;
      if (next != null && next.getMnemonicString().equals("ADD") && next.getNumOperands() == 2
          && next.getDefaultOperandRepresentation(0).equals("ESP") && next.getScalar(1) != null)
        min = Math.min(min, (int) next.getScalar(1).getValue());
      // a site without "add esp" right after proves nothing: MSVC may defer and merge the cleanup
    }
    return min == Integer.MAX_VALUE ? -1 : min;
  }

  public void run() throws Exception {
    Path out = Paths.get(getScriptArgs()[0]);
    Set<Long> ecx = new HashSet<>();
    if (getScriptArgs().length > 1)
      for (String l : Files.readAllLines(Paths.get(getScriptArgs()[1]))) if (!l.isBlank()) ecx.add(Long.parseLong(l.trim(), 16));
    int toThis = 0, trimmed = 0, padded = 0, toCdecl = 0, ok = 0, cdeclTrimmed = 0;
    try (var w = Files.newBufferedWriter(out.resolve("signature_mismatches.tsv"))) {
      for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
        if (f.getSignatureSource() != SourceType.IMPORTED || f.hasVarArgs()) continue;
        List<ParameterImpl> ps = new ArrayList<>();
        // all parameters including the auto `this` of __thiscall (it is re-declared explicitly on update)
        for (Parameter p : f.getParameters()) ps.add(new ParameterImpl(p.getName(), p.getDataType(), currentProgram));
        String cc = f.getCallingConventionName();
        boolean changed = false;
        // class member reading ECX undeclared -> method
        if (ecx.contains(f.getEntryPoint().getOffset()) && !"__thiscall".equals(cc) && f.getParentNamespace() instanceof GhidraClass c) {
          DataType cls = null;
          for (DataType d : findStructs(c.getName())) { cls = d; break; }
          DataType tt = cls != null ? new PointerDataType(cls, 4, currentProgram.getDataTypeManager()) : new PointerDataType();
          ps.add(0, new ParameterImpl("this", tt, currentProgram));
          cc = "__thiscall"; changed = true; toThis++;
        }
        int purge = retBytes(f);
        if (purge >= 0 && ("__thiscall".equals(cc) || "__stdcall".equals(cc))) {
          int first = "__thiscall".equals(cc) && !ps.isEmpty() && "this".equals(ps.get(0).getName()) ? 1 : 0;  // `this` in ECX
          int declared = 0;
          for (int i = first; i < ps.size(); i++) declared += (ps.get(i).getLength() + 3) & ~3;
          if (purge == 0 && "__stdcall".equals(cc)) { cc = "__cdecl"; changed = true; toCdecl++; }
          else if (declared != purge) {
            w.write(String.format("%X\t%s\t%d\t%d%n", f.getEntryPoint().getOffset(), f.getName(true), declared, purge));
            while (declared > purge && ps.size() > first) {
              declared -= (ps.get(ps.size() - 1).getLength() + 3) & ~3;
              ps.remove(ps.size() - 1);
              trimmed++;
            }
            int k = 0;
            while (declared < purge) { ps.add(new ParameterImpl("param_extra" + (k++), Undefined4DataType.dataType, currentProgram)); declared += 4; padded++; }
            changed = true;
          } else ok++;
        }
        // __cdecl: never declare more stack bytes than any caller cleans up right after the call
        if ("__cdecl".equals(cc)) {
          int clean = callerCleanup(f);
          int declared = 0;
          for (ParameterImpl q : ps) declared += (q.getLength() + 3) & ~3;
          if (clean > 0 && declared > clean) {
            {
              w.write(String.format("%X\t%s\t%d\t%d\tcdecl%n", f.getEntryPoint().getOffset(), f.getName(true), declared, clean));
              while (declared > clean && !ps.isEmpty()) {
                declared -= (ps.get(ps.size() - 1).getLength() + 3) & ~3;
                ps.remove(ps.size() - 1);
                cdeclTrimmed++;
              }
              changed = true;
            }
          }
        }
        if (changed) {
          try {
            f.updateFunction(cc, f.getReturn(), ps, FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.IMPORTED);
          } catch (Exception e) { println("VALIDATE fail " + f.getName(true) + ": " + e.getMessage()); }
        }
      }
    }
    println("VALIDATE ok=" + ok + " madeThiscall=" + toThis + " paramsTrimmed=" + trimmed + " paramsAdded=" + padded
        + " stdcallToCdecl=" + toCdecl + " cdeclParamsTrimmed=" + cdeclTrimmed);
  }

  List<DataType> findStructs(String name) {
    List<DataType> r = new ArrayList<>();
    currentProgram.getDataTypeManager().findDataTypes(name, r);
    r.removeIf(d -> !(d instanceof Structure));
    r.sort(Comparator.comparing(d -> d.getCategoryPath().getPath().startsWith("/Demangler") ? 0 : 1));
    return r;
  }
}
