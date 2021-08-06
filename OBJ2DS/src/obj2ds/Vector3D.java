package obj2ds;

/**
 *
 * @author Roman Lahin
 */
public class Vector3D extends Vector2D {
    
    public double z;
    public int zExport;

    public Vector3D(double x, double y) {
        super(x, y);
    };

    public Vector3D(double x, double y, double z) {
        super(x, y);
        this.z = z;
    }
    
    public void max(Vector3D v) {
        if(v.x > x) x = v.x;
        if(v.y > y) y = v.y;
        if(v.z > z) z = v.z;
    }
    
    public void min(Vector3D v) {
        if(v.x < x) x = v.x;
        if(v.y < y) y = v.y;
        if(v.z < z) z = v.z;
    }
    
    public boolean equals(Vector3D v) {
        if(v == null) return false;
        return v.xExport == xExport && v.yExport == yExport && v.zExport == zExport;
    }
    
    public void calcNormal(Vector3D a, Vector3D b, Vector3D c) {
        double x = (b.y - a.y) * (c.z - a.z) - (b.z - a.z) * (c.y - a.y);
        double y = (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z);
        double z = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        
        double sqrt = Math.sqrt(x * x + y * y + z * z);
        
        this.x = (x / sqrt);
        this.y = (y / sqrt);
        this.z = (z / sqrt);
    }
    
    public void asPos(double[] min, long[] scale) {
        int x = (int) (Math.round(this.x * 4096 * 4096 / scale[0]) 
                - Math.round(min[0] * 4096 * 4096 / scale[0])) + Short.MIN_VALUE;
        int y = (int) (Math.round(this.y * 4096 * 4096 / scale[1]) 
                - Math.round(min[1] * 4096 * 4096 / scale[1])) + Short.MIN_VALUE;
        int z = (int) (Math.round(this.z * 4096 * 4096 / scale[2]) 
                - Math.round(min[2] * 4096 * 4096 / scale[2])) + Short.MIN_VALUE;
        
        x = Math.max(Short.MIN_VALUE,  Math.min(Short.MAX_VALUE, x));
        y = Math.max(Short.MIN_VALUE,  Math.min(Short.MAX_VALUE, y));
        z = Math.max(Short.MIN_VALUE,  Math.min(Short.MAX_VALUE, z));
        
        xExport = x & 0xffff;
        yExport = y & 0xffff;
        zExport = z & 0xffff;
    }
    
    public byte[] asPosf32() {
        int x = (int) Math.round(this.x * 4096);
        int y = (int) Math.round(this.y * 4096);
        int z = (int) Math.round(this.z * 4096);
        
        x = Math.max(Integer.MIN_VALUE,  Math.min(Integer.MAX_VALUE, x));
        y = Math.max(Integer.MIN_VALUE,  Math.min(Integer.MAX_VALUE, y));
        z = Math.max(Integer.MIN_VALUE,  Math.min(Integer.MAX_VALUE, z));
        
        return new byte[]{
            (byte)(x&0xff), (byte)((x>>>8)&0xff), (byte)((x>>>16)&0xff), (byte)((x>>>24)&0xff), 
            (byte)(y&0xff), (byte)((y>>>8)&0xff), (byte)((y>>>16)&0xff), (byte)((y>>>24)&0xff), 
            (byte)(z&0xff), (byte)((z>>>8)&0xff), (byte)((z>>>16)&0xff), (byte)((z>>>24)&0xff)
        };
    }
    
    public void asNorm() {
        int x = (int) Math.round(this.x * 512);
        int y = (int) Math.round(this.y * 512);
        int z = (int) Math.round(this.z * 512);
        
        x = Math.max(-512, Math.min(511, x));
        y = Math.max(-512, Math.min(511, y));
        z = Math.max(-512, Math.min(511, z));
        
        xExport = x & 0x3ff;
        yExport = y & 0x3ff;
        zExport = z & 0x3ff;
    }

}
