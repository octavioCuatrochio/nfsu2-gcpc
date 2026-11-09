import ghidra.app.script.GhidraScript;
public class VtSyms extends GhidraScript {
  public void run() throws Exception {
    int n = 0;
    for (var s : currentProgram.getSymbolTable().getSymbols(toAddr(0x80000000L).getAddressSpace().getMinAddress() == null ? null : currentProgram.getMemory(), toAddr(0x80000000L), true)) {
      if (n > 6) break;
    }
    var it = currentProgram.getSymbolTable().getSymbolIterator("*vt*", true);
    while (it.hasNext() && n < 12) { var s = it.next(); println("VT " + s.getName(true) + " @ " + s.getAddress() + " src=" + s.getSource()); n++; }
    var it2 = currentProgram.getSymbolTable().getSymbolIterator("*RaceCarController*", true);
    while (it2.hasNext()) { var s = it2.next(); if (s.getSymbolType() != ghidra.program.model.symbol.SymbolType.FUNCTION) println("RCC " + s.getName(true) + " @ " + s.getAddress() + " " + s.getSymbolType()); }
  }
}
