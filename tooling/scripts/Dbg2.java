import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.data.*;
public class Dbg2 extends GhidraScript {
  String dec(String tag) throws Exception {
    var ifc = new DecompInterface(); var o = new DecompileOptions(); o.grabFromProgram(currentProgram); o.setRespectReadOnly(true); ifc.setOptions(o); ifc.openProgram(currentProgram);
    var f = getFunctionAt(toAddr(0x802e7be0L));
    String c = ifc.decompileFunction(f, 60, monitor).getDecompiledFunction().getC();
    ifc.dispose();
    for (String l : c.split("\n")) if (l.contains("fVar5 =") || l.contains("fVar2 = fVar2")) println("DBG2 [" + tag + "] " + l.trim());
    return c;
  }
  public void run() throws Exception {
    dec("block read-only, undefined4");
    var a = toAddr(0x80351f20L);
    clearListing(a, a.add(3));
    var d = createData(a, FloatDataType.dataType);
    dec("float data");
    MutabilitySettingsDefinition.DEF.setChoice(d, MutabilitySettingsDefinition.CONSTANT);
    dec("float + constant mutability");
  }
}
