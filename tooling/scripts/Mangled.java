import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import java.util.*;
public class Mangled extends GhidraScript {
  public void run() throws Exception {
    int total=0, mangled=0, demangledNs=0; Map<String,Integer> kinds = new TreeMap<>(); List<String> ex = new ArrayList<>();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      total++;
      String n = f.getName();
      if (!f.getParentNamespace().isGlobal()) demangledNs++;
      if (n.contains("__") && !n.startsWith("__") || n.startsWith("_._") || n.startsWith("__ct__") || n.startsWith("__dt__")) {
        mangled++;
        String k = n.startsWith("_._") ? "dtor _._" : n.startsWith("__") ? "operator/ctor __xx__" : n.matches(".*__[0-9].*") ? "member Name__<n>Class..." : n.matches(".*__F.*") ? "free func __F" : n.matches(".*__Q.*") ? "qualified __Q" : "other";
        kinds.merge(k, 1, Integer::sum);
        if (ex.size() < 400) ex.add(n);
      }
    }
    println("MANG total=" + total + " stillMangled=" + mangled + " inNamespace=" + demangledNs + " kinds=" + kinds);
    Collections.shuffle(ex, new Random(1)); for (int i = 0; i < 25 && i < ex.size(); i++) println("MANGEX " + ex.get(i));
  }
}
