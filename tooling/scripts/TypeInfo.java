import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
public class TypeInfo extends GhidraScript {
  public void run() throws Exception {
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      if (!f.getName(true).equals("UIMain::NotifySoundMessage")) continue;
      println("TI cc=" + f.getCallingConventionName() + " src=" + f.getSignatureSource() + " custom=" + f.hasCustomVariableStorage());
      for (Parameter p : f.getParameters()) { DataType d = p.getDataType(); println("TI  " + p.getName() + " : " + d.getPathName() + " class=" + d.getClass().getSimpleName() + " len=" + d.getLength() + " storage=" + p.getVariableStorage() + (d instanceof Structure s ? " defined=" + s.getNumDefinedComponents() + " notYet=" + s.isNotYetDefined() : "")); }
    }
    var it = currentProgram.getDataTypeManager().getAllDataTypes(); int n = 0;
    while (it.hasNext()) { var d = it.next(); if (d.getName().equals("FrontendSoundEvent")) println("TI type " + d.getPathName() + " " + d.getClass().getSimpleName() + " len=" + d.getLength()); }
  }
}
