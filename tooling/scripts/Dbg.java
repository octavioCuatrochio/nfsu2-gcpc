import ghidra.app.script.GhidraScript;
public class Dbg extends GhidraScript {
  public void run() throws Exception {
    var b = currentProgram.getMemory().getBlock(toAddr(0x80351f20L));
    println("DBG block " + b.getName() + " r=" + b.isRead() + " w=" + b.isWrite() + " x=" + b.isExecute() + " vol=" + b.isVolatile());
    var it = currentProgram.getListing().getInstructions(toAddr(0x802e7be0L), true);
    for (int i = 0; i < 40 && it.hasNext(); i++) { var ins = it.next(); println("DBG " + ins.getAddress() + " " + ins); }
    println("DBG data@ " + currentProgram.getListing().getDataAt(toAddr(0x80351f20L)));
    for (var r : getReferencesTo(toAddr(0x80351f20L))) println("DBG ref " + r);
  }
}
