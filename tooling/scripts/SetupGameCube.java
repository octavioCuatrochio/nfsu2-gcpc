// Prepares a CodeWarrior GameCube ELF for analysis:
//  - r13 = _SDA_BASE_, r2 = _SDA2_BASE_ (small-data base registers, constant for the whole program)
//  - GQR0 = 0 (paired-single save/restore quantizer)
//  - constant sections marked read-only so the decompiler folds float/int constants
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Symbol;
import java.math.BigInteger;

public class SetupGameCube extends GhidraScript {
  private Long sym(String name) {
    for (Symbol s : currentProgram.getSymbolTable().getGlobalSymbols(name)) return s.getAddress().getOffset();
    return null;
  }
  public void run() throws Exception {
    Long sda = sym("_SDA_BASE_"), sda2 = sym("_SDA2_BASE_");
    if (sda == null || sda2 == null) { println("SETUP: _SDA_BASE_/_SDA2_BASE_ not found, skipping register setup"); }
    else {
      Register r13 = currentProgram.getRegister("r13"), r2 = currentProgram.getRegister("r2");
      var ctx = currentProgram.getProgramContext();
      for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
        if (!b.isExecute()) continue;
        ctx.setValue(r13, b.getStart(), b.getEnd(), BigInteger.valueOf(sda));
        ctx.setValue(r2, b.getStart(), b.getEnd(), BigInteger.valueOf(sda2));
        // GQR0 is zeroed by __OSPSInit and never changed: it is the "no scaling, float" quantizer
        // used by every psq_l/psq_st that saves/restores paired-single registers in prologues.
        Register gqr0 = currentProgram.getRegister("GQR0");
        if (gqr0 != null) ctx.setValue(gqr0, b.getStart(), b.getEnd(), BigInteger.ZERO);
        println("SETUP: " + b.getName() + " r13=" + Long.toHexString(sda) + " r2=" + Long.toHexString(sda2) + (gqr0 != null ? " GQR0=0" : ""));
      }
    }
    for (String n : new String[] {".rodata", ".sdata2", ".sbss2", ".ctors", ".dtors"}) {
      MemoryBlock b = currentProgram.getMemory().getBlock(n);
      if (b != null && b.isInitialized()) { b.setWrite(false); println("SETUP: " + n + " -> read-only"); }
    }
  }
}
