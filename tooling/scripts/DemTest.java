import ghidra.app.script.GhidraScript;
import ghidra.app.util.demangler.*;
import ghidra.app.util.demangler.gnu.*;
import java.util.*;
public class DemTest extends GhidraScript {
  public void run() throws Exception {
    var gnu = new GnuDemangler();
    var opts = new GnuDemanglerOptions(GnuDemanglerFormat.GNU, true); opts.setDemangleOnlyKnownPatterns(false);
    var cw = new gamecubeloader.common.CodeWarriorDemangler();
    String[] names = {"CalculateScore__24AIActionAccidentRecoveryP18DrivingInformation","GetRelativePositionTo__15AICarControllerP3CarPfT2","_._11DriftDriver",
      "LineSegmentIsInBounds__FP8bVector3T0P12FuturePointsii","Alloc__11MyAllocatorUiRCQ22EA12TagValuePair","__Q24RCMP11RCMP_SYSTEM","_._Q24RCMP11RCMP_SYSTEM",
      "FindHandle__11CAnimPlayer13eAnimCategory","SetGameState__17EAXS_PFController13eEAXGameStatei","bGetTicker__Fv","Update__8EAXSoundf","__ls__FR7ostreamPCc","_4RCMP.rcmp_sys","__vt$11DriftDriver","__tf11DriftDriver"};
    for (String n : names) {
      String g, c;
      try { var o = gnu.demangle(n, opts); g = o == null ? "null" : o.getSignature(false); } catch (Exception e) { g = "ERR " + e.getMessage(); }
      try { var o = gamecubeloader.common.CodeWarriorDemangler.demangleSymbol(n); c = o == null ? "null" : o.getSignature(false); } catch (Exception e) { c = "ERR"; }
      println("DEM " + n + "\n      gnu-v2: " + g + "\n      cw    : " + c);
    }
    // whole-program success rates
    int tot=0, gok=0, cok=0; List<String> fails = new ArrayList<>();
    for (var s : currentProgram.getSymbolTable().getAllSymbols(false)) {
      for (String n : new String[]{ s.getName() }) {
        if (!(n.contains("__") || n.startsWith("_._"))) continue;
        tot++;
        try { if (gnu.demangle(n, opts) != null) gok++; else fails.add(n); } catch (Exception e) { fails.add(n + "  [" + e.getMessage() + "]"); }
        try { if (gamecubeloader.common.CodeWarriorDemangler.demangleSymbol(n) != null) cok++; } catch (Exception e) {}
      }
    }
    for (String n : fails.subList(0, Math.min(15, fails.size()))) println("DEMFAIL " + n);
    println("DEMRATE candidates=" + tot + " gnu-v2=" + gok + " cw=" + cok);
  }
}
