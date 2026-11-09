// PC (MSVC x86): creates functions Ghidra's analysis missed because they are only reached indirectly.
//  1. vtable entries: in read-only data, a code-referenced address followed by >=2 pointers into .text
//  2. extra entry points from a file (one hex address per line), e.g. nfsu2-re's documented functions
// Usage: -postScript CreateMissingFunctions.java [addresses.txt]
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import java.nio.file.*;
import java.util.*;

public class CreateMissingFunctions extends GhidraScript {
  int created = 0, failed = 0;

  boolean ensureFunction(Address a) {
    if (getFunctionAt(a) != null) return false;
    Function containing = getFunctionContaining(a);
    if (containing != null) return false;  // inside another function: don't split it blindly
    if (getInstructionAt(a) == null) {
      var dis = new DisassembleCommand(a, null, true);
      dis.applyTo(currentProgram, monitor);
    }
    var cmd = new CreateFunctionCmd(a);
    if (cmd.applyTo(currentProgram, monitor) && getFunctionAt(a) != null) { created++; return true; }
    failed++;
    return false;
  }

  public void run() throws Exception {
    Memory mem = currentProgram.getMemory();
    MemoryBlock text = mem.getBlock(".text");
    ReferenceManager refs = currentProgram.getReferenceManager();
    List<Address> entries = new ArrayList<>();
    for (MemoryBlock b : mem.getBlocks()) {
      if (b.isExecute() || !b.isInitialized() || b.getName().equals(".rsrc")) continue;
      Address a = b.getStart().add((4 - b.getStart().getOffset() % 4) % 4);
      while (a.compareTo(b.getEnd().subtract(8)) < 0) {
        if (monitor.isCancelled()) return;
        boolean codeRef = false;
        for (Reference r : refs.getReferencesTo(a)) {
          if (getInstructionAt(r.getFromAddress()) != null) { codeRef = true; break; }
        }
        if (codeRef) {
          List<Address> run = new ArrayList<>();
          Address p = a;
          while (p.compareTo(b.getEnd().subtract(4)) < 0) {
            long v = mem.getInt(p) & 0xFFFFFFFFL;
            Address t = toAddr(v);
            if (!text.contains(t)) break;
            if (!run.isEmpty() && refs.getReferenceCountTo(p) > 0) {
              boolean fromCode = false;
              for (Reference r : refs.getReferencesTo(p)) if (getInstructionAt(r.getFromAddress()) != null) fromCode = true;
              if (fromCode) break;  // next table starts here
            }
            run.add(t);
            p = p.add(4);
          }
          if (run.size() >= 2) { entries.addAll(run); a = p; continue; }
        }
        a = a.add(4);
      }
    }
    println("MISSING vtable-like entries: " + entries.size());
    for (Address t : new LinkedHashSet<>(entries)) ensureFunction(t);
    int fromVt = created;
    if (getScriptArgs().length > 0) {
      for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
        line = line.trim();
        if (line.isEmpty()) continue;
        ensureFunction(toAddr(Long.parseLong(line, 16)));
      }
    }
    println("MISSING created " + created + " functions (" + fromVt + " from vtables, " + (created - fromVt)
        + " from list), " + failed + " failed");
  }
}
