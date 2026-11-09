// PC (MSVC x86): splits functions that Ghidra merged. An address inside a function body is treated as a
// real function start when it is
//   - the target of a direct CALL instruction, or
//   - an entry of a vtable-like table (code-referenced run of pointers into .text), or
//   - listed in the optional address file (e.g. nfsu2-re's documented functions).
// The enclosing function is recreated at its own entry; its name, namespace, comment and signature
// source are kept. Usage: -postScript FixFunctionBoundaries.java [addresses.txt]
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import java.nio.file.*;
import java.util.*;

public class FixFunctionBoundaries extends GhidraScript {
  public void run() throws Exception {
    Listing listing = currentProgram.getListing();
    ReferenceManager refs = currentProgram.getReferenceManager();
    Memory mem = currentProgram.getMemory();
    MemoryBlock text = mem.getBlock(".text");
    Map<Address, String> starts = new TreeMap<>();

    // 1. direct call targets
    for (Instruction ins : listing.getInstructions(text.getStart(), true)) {
      if (!text.contains(ins.getAddress())) break;
      if (!ins.getFlowType().isCall()) continue;
      for (Address t : ins.getFlows()) {
        Function f = getFunctionContaining(t);
        if (f != null && !f.getEntryPoint().equals(t)) starts.putIfAbsent(t, "call");
      }
    }
    // 2. vtable entries
    for (MemoryBlock b : mem.getBlocks()) {
      if (b.isExecute() || !b.isInitialized() || b.getName().equals(".rsrc")) continue;
      for (Address a = b.getStart(); a.compareTo(b.getEnd().subtract(8)) < 0; a = a.add(4)) {
        boolean codeRef = false;
        for (Reference r : refs.getReferencesTo(a)) if (getInstructionAt(r.getFromAddress()) != null) { codeRef = true; break; }
        if (!codeRef) continue;
        int n = 0;
        for (Address p = a; p.compareTo(b.getEnd().subtract(4)) < 0; p = p.add(4), n++) {
          Address t = toAddr(mem.getInt(p) & 0xFFFFFFFFL);
          if (!text.contains(t)) break;
          if (n > 0) {
            boolean next = false;
            for (Reference r : refs.getReferencesTo(p)) if (getInstructionAt(r.getFromAddress()) != null) next = true;
            if (next) break;
          }
          Function f = getFunctionContaining(t);
          if (f != null && !f.getEntryPoint().equals(t)) starts.putIfAbsent(t, "vtable");
        }
      }
    }
    // 3. listed addresses
    if (getScriptArgs().length > 0) {
      for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
        line = line.trim();
        if (line.isEmpty()) continue;
        Address t = toAddr(Long.parseLong(line, 16));
        Function f = getFunctionContaining(t);
        if (f != null && !f.getEntryPoint().equals(t)) starts.putIfAbsent(t, "list");
      }
    }
    println("BOUNDARY candidates: " + starts.size());

    Map<String, Integer> byKind = new TreeMap<>();
    int split = 0, failed = 0;
    for (var e : starts.entrySet()) {
      if (monitor.isCancelled()) break;
      Address t = e.getKey();
      Function outer = getFunctionContaining(t);
      if (outer == null || outer.getEntryPoint().equals(t)) continue;
      // must be an instruction boundary inside the function
      if (getInstructionAt(t) == null) { failed++; continue; }
      Address entry = outer.getEntryPoint();
      String name = outer.getName();
      SourceType src = outer.getSymbol().getSource();
      Namespace ns = outer.getParentNamespace();
      String plate = outer.getComment();
      String plateAt = getPlateComment(entry);
      try {
        removeFunction(outer);
        boolean okInner = new CreateFunctionCmd(t).applyTo(currentProgram, monitor);
        boolean okOuter = new CreateFunctionCmd(entry).applyTo(currentProgram, monitor);
        Function o = getFunctionAt(entry);
        if (o != null) {
          if (src != SourceType.DEFAULT) { o.setParentNamespace(ns); o.setName(name, src); }
          if (plate != null) o.setComment(plate);
          if (plateAt != null) setPlateComment(entry, plateAt);
        }
        if (okInner && okOuter && o != null && getFunctionAt(t) != null) {
          split++;
          byKind.merge(e.getValue(), 1, Integer::sum);
        } else failed++;
      } catch (Exception ex) {
        failed++;
        if (failed < 10) println("BOUNDARY fail " + t + ": " + ex.getMessage());
      }
    }
    println("BOUNDARY split " + split + " " + byKind + ", failed " + failed + ", functions now "
        + currentProgram.getFunctionManager().getFunctionCount());
  }
}
