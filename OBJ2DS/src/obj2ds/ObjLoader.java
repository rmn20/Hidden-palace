package obj2ds;

import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;

/**
 *
 * @author Roman Lahin
 */
public class ObjLoader {
    
    public static Mesh[] loadOBJ(String path) {
        try {
            File file = new File(path);
            FileInputStream fis = new FileInputStream(file);
            
            byte[] bytes = new byte[fis.available()];
            fis.read(bytes);
            fis.close();
            
            String loadedFile = new String(bytes, "UTF-8");
            String[] lines = loadedFile.split("\n");
            
            ArrayList<Vector3D> pos = new ArrayList();
            pos.add(new Vector3D(0, 0, 0));
            
            ArrayList<Vector3D> norm = new ArrayList();
            norm.add(new Vector3D(0, 0, 0));
            
            ArrayList<Vector2D> uv = new ArrayList();
            uv.add(new Vector2D(0, 0));
            
            ArrayList<Mesh> meshes = new ArrayList();
            Mesh currentMesh = null;
            
            String material = "null";
            int polyId = 0;
            
            for(String line : lines) {
                line = line.trim();
                
                if(line.startsWith("o ") || line.startsWith("g ")) {
                    currentMesh = new Mesh(line.substring(2));
                    meshes.add(currentMesh);
                    
                    String[] data = currentMesh.name.split(";");
                    for(String dataLine : data) {
                        int index = dataLine.indexOf('=');
                        if(index == -1) continue;
                        
                        String name = dataLine.substring(0, index);
                        String val = dataLine.substring(index+1);
                        
                        if(name.equals("poly_id")) polyId = Integer.parseInt(val);
                    }
                    
                } else if(line.startsWith("v ")) {
                    String[] coords = line.split(" ");
                    
                    //Santimeters to meters
                    pos.add(new Vector3D(
                            getDouble(coords[1]) * 0.01,
                            getDouble(coords[2]) * 0.01,
                            getDouble(coords[3]) * 0.01
                    ));
                    
                } else if(line.startsWith("vt ")) {
                    String[] coords = line.split(" ");
                    
                    uv.add(new Vector2D(
                            getDouble(coords[1]),
                            1f-getDouble(coords[2])
                    ));
                    
                } else if(line.startsWith("vn ")) {
                    String[] coords = line.split(" ");
                    
                    norm.add(new Vector3D(
                            getDouble(coords[1]),
                            getDouble(coords[2]),
                            getDouble(coords[3])
                    ));
                    
                } else if(line.startsWith("usemtl ")) {
                    material = line.substring(7);
                    
                } else if(line.startsWith("f ")) {
                    String[] verts = line.split(" ");
                    
                    Polygon pol = new Polygon();
                    currentMesh.pols.add(pol);
                    
                    pol.material = material;
                    pol.id = polyId;
                    
                    pol.pos = new Vector3D[verts.length-1];
                    pol.norm = new Vector3D[verts.length-1];
                    pol.uv = new Vector2D[verts.length-1];
                    pol.col = new Color[verts.length-1];
                    
                    for(int i=1; i<verts.length; i++) {
                        String[] vert = verts[i].split("/", -1);
                        
                        pol.pos[i-1] = pos.get(getInt(vert[0]));
                        pol.norm[i-1] = norm.get(vert.length>=3?getInt(vert[2]):0);
                        pol.uv[i-1] = uv.get(vert.length>=2?getInt(vert[1]):0);
                        pol.col[i-1] = Color.WHITE;
                    }
                    
                }
            }
            
            
            return meshes.toArray(new Mesh[meshes.size()]);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        
        return null;
    }
    
    private static double getDouble(String str) {
        if(str.isEmpty()) return 0;
        return Double.valueOf(str);
    }
    
    private static int getInt(String str) {
        if(str.isEmpty()) return 0;
        return Integer.valueOf(str);
    }

}
