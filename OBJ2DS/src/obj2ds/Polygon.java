package obj2ds;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;

/**
 *
 * @author Roman Lahin
 */
public class Polygon {
    
    public String material;
    
    public Vector3D[] pos;
    public Vector3D[] norm;
    public Vector2D[] uv;
    public Color[] col;
    
    int id;

    public void prepare(int[] minL, long[] scale, boolean hasColor) {
        for(int i=0; i<pos.length; i++) {
            if(!hasColor) norm[i].asNorm();
            else col[i].prepare();
            
            uv[i].asTexcoord();
            pos[i].asPos(minL, scale);
        }
    }

    public void write(NDSOutputStream dos, int[] minL, long[] scale, boolean hasColor) throws IOException {
        for(int i=0; i<pos.length; i++) {
            if(!hasColor) dos.writeIntLE(norm[i].xExport | (norm[i].yExport << 10) | (norm[i].zExport << 20));
            else dos.writeShortLE(col[i].export);
            
            dos.writeShortLE(uv[i].xExport);
            dos.writeShortLE(uv[i].yExport);
            
            dos.writeShortLE(pos[i].xExport);
            dos.writeShortLE(pos[i].yExport);
            dos.writeShortLE(pos[i].zExport);
        }
    }
    
    public int normalWeight() {
        for(int i=1; i<pos.length; i++) {
            if(!norm[0].equals(norm[i])) return Integer.MAX_VALUE;
        }
        
        return norm[0].xExport | (norm[0].yExport << 10) | (norm[0].yExport << 20);
    }
    
    public int colorWeight() {
        for(int i=1; i<pos.length; i++) {
            if(!col[0].equals(col[i])) return Integer.MAX_VALUE;
        }
        
        return col[0].export;
    }
    
    public Vector3D firstP() {
        return pos[pos.length-1];
    }
    
    public Vector3D lastP() {
        return pos[pos.length-1];
    }

    public void aabb(double[] min, double[] max) {
        for(Vector3D v : pos) {
            if(v.x < min[0]) min[0] = v.x;
            if(v.x > max[0]) max[0] = v.x;
            
            if(v.y < min[1]) min[1] = v.y;
            if(v.y > max[1]) max[1] = v.y;
            
            if(v.z < min[2]) min[2] = v.z;
            if(v.z > max[2]) max[2] = v.z;
        }
    }

    public boolean split(ArrayList<Polygon> pols) {
        if(pos.length <= 3) return false;
        
        Vector3D n1 = new Vector3D(0, 0, 0);
        n1.calcNormal(pos[0], pos[1], pos[2]);
        n1.asNorm();
        
        Vector3D n2 = new Vector3D(0, 0, 0);
        n2.calcNormal(pos[2], pos[3], pos[0]);
        n2.asNorm();
        
        if(!n1.equals(n2)) {
            Polygon p1 = new Polygon();
            p1.pos = new Vector3D[3];
            p1.norm = new Vector3D[3];
            p1.uv = new Vector2D[3];
            p1.col = new Color[3];
            
            Polygon p2 = new Polygon();
            p2.pos = new Vector3D[3];
            p2.norm = new Vector3D[3];
            p2.uv = new Vector2D[3];
            p2.col = new Color[3];
            
            for(int i=0; i<3; i++) {
                p1.pos[i] = pos[i];
                p2.pos[i] = pos[(i+2) & 3];
                
                p1.norm[i] = norm[i];
                p2.norm[i] = norm[(i+2) & 3];
                
                p1.uv[i] = uv[i];
                p2.uv[i] = uv[(i+2) & 3];
                
                p1.col[i] = col[i];
                p2.col[i] = col[(i+2) & 3];
            }
            
            pols.remove(this);
            pols.add(p1);
            pols.add(p2);
            
            return true;
        }
        
        return false;
    }

}
