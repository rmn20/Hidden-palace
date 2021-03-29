package obj2ds;

/**
 *
 * @author Roman Lahin
 */
public class Vector2D {
    
    public double x, y;
    public int xExport, yExport;
    
    public Vector2D(double x, double y) {
        this.x = x;
        this.y = y;
    }
    
    public boolean equals(Vector2D v) {
        if(v == null) return false;
        return v.xExport == xExport && v.yExport == yExport;
    }
    
    public void asTexcoord() {
        int x = (int) Math.round(this.x * 16 * 1024);
        int y = (int) Math.round(this.y * 16 * 1024);
        
        xExport = x & 0xffff;
        yExport = y & 0xffff;
    }

}
