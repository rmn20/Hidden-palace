package obj2ds;

/**
 *
 * @author Roman Lahin
 */
public class Color {
    
    public static Color WHITE = new Color(0xffffff);
    
    public int r, g, b;
    public int export;
    
    public Color(int col) {
        r = Math.round((((col >> 16) & 0xff) & 0xff) * 32f / 256f);
        g = Math.round(((col >> 8) & 0xff) * 32f / 256f);
        b = Math.round(((col >> 0) & 0xff) * 32f / 256f);
        
        r = Math.max(0, Math.min(31, r));
        g = Math.max(0, Math.min(31, g));
        b = Math.max(0, Math.min(31, b));
    }
    
    public boolean equals(Color c) {
        if(c == null) return false;
        return c.export == export;
    }
    
    public void prepare() {
        export = 0x8000 | r | (g<<5) | (b<<10);
    }

}
