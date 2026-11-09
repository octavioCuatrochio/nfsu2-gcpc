import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import java.nio.file.*;
import java.util.*;
public class VerifyLayouts extends GhidraScript {
  public void run() throws Exception {
    String json = Files.readString(Path.of(getScriptArgs()[0]));
    var gson = new com.google.gson.Gson();
    Map<String, Map<String, Object>> exp = gson.fromJson(json, Map.class);
    var dtm = currentProgram.getDataTypeManager();
    int ok = 0, missing = 0, badSize = 0, badMember = 0; List<String> problems = new ArrayList<>();
    for (var e : exp.entrySet()) {
      DataType dt = dtm.getDataType(new CategoryPath("/DWARF"), e.getKey());
      if (!(dt instanceof Structure s)) { missing++; problems.add("MISSING " + e.getKey() + " got " + dt); continue; }
      int size = ((Number) e.getValue().get("size")).intValue();
      if (s.getLength() != size) { badSize++; problems.add("SIZE " + e.getKey() + " ghidra=" + s.getLength() + " dwarf=" + size); continue; }
      boolean good = true;
      for (Object mo : (List<?>) e.getValue().get("members")) {
        List<?> m = (List<?>) mo; String name = (String) m.get(0); Number off = (Number) m.get(1); Number bits = (Number) m.get(2);
        if (off == null || name == null) continue;
        boolean found = false;
        for (var c : s.getComponents()) {
          if (c.getOffset() != off.intValue() && !(bits != null && c.isBitFieldComponent() && c.getOffset() >= off.intValue() && c.getOffset() < off.intValue() + 4)) continue;
          String cn = c.getFieldName() == null ? "" : c.getFieldName();
          String cc = c.getComment() == null ? "" : c.getComment();
          if (name.equals("<base>") ? cn.startsWith("base_") : (cn.equals(name) || cc.equals(name))) { found = true; break; }
        }
        if (!found) { good = false; problems.add("MEMBER " + e.getKey() + "." + name + " @" + off + (bits != null ? " bits=" + bits : "")); }
      }
      if (good) ok++; else badMember++;
    }
    println("VERIFY ok=" + ok + " missing=" + missing + " badSize=" + badSize + " badMembers=" + badMember);
    Map<String,Integer> kinds = new TreeMap<>();
    for (String p : problems) kinds.merge(p.split(" ")[0], 1, Integer::sum);
    println("VERIFY problem kinds " + kinds);
    Collections.shuffle(problems, new Random(2));
    for (int i = 0; i < Math.min(30, problems.size()); i++) println("VPROB " + problems.get(i));
  }
}
