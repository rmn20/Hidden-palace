package obj2ds;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;

/**
 *
 * @author Roman Lahin
 */
public class Mesh {
    
    public static int defScale = 4096 * 8;
    
    public String name;
    public ArrayList<Polygon> pols;
    
    public boolean hasColor;
    
    public Mesh(String name) {
        this.name = name;
        pols = new ArrayList();
    }
    
    public ArrayList<Vector3D> getVertices() {
        ArrayList<Vector3D> list = new ArrayList<>();
        getVertices(list);
        return list;
    }
    
    public void getVertices(ArrayList<Vector3D> list) {
        for(Polygon pol : pols) {
            for(Vector3D v : pol.pos) {
                list.add(v);
            }
        }
    }

    public byte[] export(boolean exportPolyData, boolean exportCmdList) {
        double min[] = new double[]{Double.MAX_VALUE, Double.MAX_VALUE, Double.MAX_VALUE};
        double max[] = new double[]{-Double.MAX_VALUE, -Double.MAX_VALUE, -Double.MAX_VALUE};
        
        int splitted = 0;
        for(int i=0; i<pols.size(); i++) {
            Polygon pol = pols.get(i);
            
            if(pol.split(pols)) {
                splitted++;
                i--;
            }
        }
        
        if(splitted > 0) System.out.println("Polygons splitted: "+splitted);
        
        ArrayList<Polygon> polv4 = new ArrayList();
        ArrayList<Polygon> polv3 = new ArrayList();
        for(Polygon pol : pols) {
            if(pol.pos.length == 4) polv4.add(pol);
            else if(pol.pos.length == 3) polv3.add(pol);
            
            pol.aabb(min, max);
        }
        
        int[] minL = new int[]{
                (int) Math.floor(min[0] * 4096),
                (int) Math.floor(min[1] * 4096),
                (int) Math.floor(min[2] * 4096)
            };
        
        long[] scale;
        //We don't use dynamic scaling by default so we have less gaps
        boolean dynamicScaling = false;
        int defScale = Mesh.defScale;
        
        for(int i=0; i<3; i++) {
            int size = (int) Math.round((max[i] * 4096. - minL[i]) * 4096. / defScale + Short.MIN_VALUE);
            
            if(size > Short.MAX_VALUE) {
                dynamicScaling = true;
                System.out.println("Using dynamic scaling...");
                break;
            }
        }
        
        if(dynamicScaling) {
            scale = new long[3];
            
            for(int i=0; i<3; i++) {
                double length = max[i] * 4096 - minL[i];
                //65535 чтобы число влезло а не было на границе
                //Ceil чтобы округлилось в большую сторону
                double sc = Math.max(1, Math.ceil(length * 4096 / 65535.));

                if(sc > Integer.MAX_VALUE) {
                    System.out.println("Warning! Object is too large!");
                    sc = Integer.MAX_VALUE;
                }

                scale[i] = (long) sc;
            }
        } else {
            scale = new long[]{defScale, defScale, defScale};
        }
        
        //Added at the last moment
        for(Polygon pol : pols) {
            pol.prepare(minL, scale, hasColor);
        }
        
        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            NDSOutputStream dos = new NDSOutputStream(baos);
            
            int flags = hasColor?1:0;
            if(exportPolyData) flags |= 2;
            if(exportCmdList) flags |= 4;
            
            dos.writeIntLE(flags);
            
            dos.writeIntLE(Float.floatToIntBits((float) min[0]));
            dos.writeIntLE(Float.floatToIntBits((float) min[1]));
            dos.writeIntLE(Float.floatToIntBits((float) min[1]));
            dos.writeIntLE(Float.floatToIntBits((float) max[0]));
            dos.writeIntLE(Float.floatToIntBits((float) max[1]));
            dos.writeIntLE(Float.floatToIntBits((float) max[1]));
            
            dos.writeIntLE(minL[0]);
            dos.writeIntLE(minL[1]);
            dos.writeIntLE(minL[2]);
            
            dos.writeIntLE((int) scale[0]);
            dos.writeIntLE((int) scale[1]);
            dos.writeIntLE((int) scale[2]);
            
            if(exportPolyData) {
                dos.writeShortLE(polv4.size());
                dos.writeShortLE(polv3.size());
            
                for(Polygon pol : polv4) pol.write(dos, minL, scale, hasColor);
                for(Polygon pol : polv3) pol.write(dos, minL, scale, hasColor);
                
                //Padding
                if((dos.size() & 3) != 0) dos.write(new byte[4 - (dos.size() & 3)]);
            }
            
            if(exportCmdList) exportCommandList(dos, hasColor, polv4, polv3);
            
            byte[] data = baos.toByteArray();
            baos.close();
            
            return data;
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        return null;
    }
    
    private static final int FIFO_BEGIN = 0x40 | 0x0100, FIFO_END = 0x41 | 0x0000,
            FIFO_NOP = 0x00 | 0x0100, GL_TRIANGLES = 0x00 | 0x0100, GL_QUADS = 0x01 | 0x0100,
            FIFO_NORMAL = 0x21 | 0x0100, FIFO_TEX_COORD = 0x22 | 0x0100, FIFO_COLOR = 0x20 | 0x0100,
            
            FIFO_VERTEX10 = 0x24 | 0x0100, FIFO_VERTEX16 = 0x23 | 0x0200, 
            FIFO_VERTEX_XY = 0x25 | 0x0100, FIFO_VERTEX_XZ = 0x26 | 0x0100, FIFO_VERTEX_YZ = 0x27 | 0x0100,
            FIFO_POLY_FORMAT = 0x29 | 0x0100;
    
    private static final int standartPolyFormat = 0x1f0080,
            polIdAdress = 0x1000000;
    
    private static Vector3D prevPos, prevNorm;
    private static Vector2D prevUV;
    private static Color prevColor;
    private static int prevPolId;

    private void exportCommandList(NDSOutputStream dos, 
            boolean hasColor,
            ArrayList<Polygon> polv4, ArrayList<Polygon> polv3) throws Exception {
        
        prevPos = prevNorm = null;
        prevUV = null;
        prevColor = null;
        prevPolId = 0;
       
        ArrayList<Integer> commands = new ArrayList();
        ArrayList<Integer> list = new ArrayList();
        
        if(polv4.size() > 0) {
            commands.add(FIFO_BEGIN);
            list.add(GL_QUADS);
            
            add(polv4, hasColor, commands, list);
        }
        
        if(polv3.size() > 0) {
            commands.add(FIFO_BEGIN);
            list.add(GL_TRIANGLES);
            
            add(polv3, hasColor, commands, list);
        }
        
        if(commands.size() > 0) {
            commands.add(FIFO_END);
            //list.add(0);
            
            if(prevPolId != 0) {
                commands.add(FIFO_POLY_FORMAT);
                list.add(standartPolyFormat);
            }
        }
        
        int cmds = list.size() + (commands.size()/4) + ((commands.size()&3) > 0 ? 1: 0);
        dos.writeIntLE(cmds);
        System.out.println("size: "+cmds);
        System.out.println("commands: "+commands.size());
        
        int listI = 0;
        for(int i=0; i<commands.size(); i++) {
            
            int command = commands.get(i);
            if((i&3) == 0) {
                int commandsPacked = 0;
                
                commandsPacked |= command & 0xff;
                if(i + 1 < commands.size()) commandsPacked |= (commands.get(i + 1) & 0xff) << 8;
                if(i + 2 < commands.size()) commandsPacked |= (commands.get(i + 2) & 0xff) << 16;
                if(i + 3 < commands.size()) commandsPacked |= (commands.get(i + 3) & 0xff) << 24;
                
                dos.writeIntLE(commandsPacked);
            }
            
            for(int x=0; x<command>>8; x++, listI++) {
                dos.writeIntLE(list.get(listI));
            }
        }
        
    }
    
    private void add(ArrayList<Polygon> pols, boolean hasColor, ArrayList<Integer> commands, ArrayList<Integer> list) {
        
        //Lazy space optimization thing
        if(!hasColor) pols.sort((Polygon lhs, Polygon rhs) -> 
            // -1 - less than, 1 - greater than, 0 - equal
            lhs.normalWeight() < rhs.normalWeight() ? -1 : (lhs.normalWeight() > rhs.normalWeight()) ? 1 : 0
        );
        
        if(hasColor) pols.sort((Polygon lhs, Polygon rhs) -> 
            // -1 - less than, 1 - greater than, 0 - equal
            lhs.colorWeight() < rhs.colorWeight() ? -1 : (lhs.colorWeight() > rhs.colorWeight()) ? 1 : 0
        );
        
        for(Polygon pol : pols) {
                
            //Polygon ID (used for AA)
            if(pol.id != prevPolId) {
                commands.add(FIFO_POLY_FORMAT);
                list.add(standartPolyFormat | (polIdAdress * pol.id));
                prevPolId = pol.id;
            }
                
            for(int i = 0; i < pol.pos.length; i++) {
                Vector3D norm = pol.norm[i];
                Vector2D uv = pol.uv[i];
                Color col = null;
                if(hasColor) {
                    col = pol.col[i];
                    if(col == null) col = Color.WHITE;
                }
                Vector3D pos = pol.pos[i];
                
                //Normal definition actually works as a color definition
                //Just color is calculated using material & light parameters
                //So there's no need for declaring normal and color at the same time
                if(!hasColor && !norm.equals(prevNorm)) {
                    commands.add(FIFO_NORMAL);
                    list.add(norm.xExport | (norm.yExport << 10) | (norm.yExport << 20));
                    prevNorm = norm;
                } else if(hasColor && !col.equals(prevColor)) {
                    commands.add(FIFO_COLOR);
                    list.add(col.export);
                    prevColor = col;
                }

                //UVM
                if(!uv.equals(prevUV)) {
                    commands.add(FIFO_TEX_COORD);
                    list.add(uv.xExport | (uv.yExport << 16));
                    prevUV = uv;
                }

                //Position
                if(prevPos != null && prevPos.xExport == pos.xExport) {
                    commands.add(FIFO_VERTEX_YZ);
                    list.add(pos.yExport | (pos.zExport << 16));
                } else if(prevPos != null && prevPos.yExport == pos.yExport) {
                    commands.add(FIFO_VERTEX_XZ);
                    list.add(pos.xExport | (pos.zExport << 16));
                } else if(prevPos != null && prevPos.zExport == pos.zExport) {
                    commands.add(FIFO_VERTEX_XY);
                    list.add(pos.xExport | (pos.yExport << 16));
                } else {

                    if(pos.xExport > 1023 || pos.xExport < -1024
                            || pos.yExport > 1023 || pos.yExport < -1024
                            || pos.zExport > 1023 || pos.zExport < -1024) {
                        commands.add(FIFO_VERTEX16);
                        list.add(pos.xExport | (pos.yExport << 16));
                        list.add(pos.zExport);
                    } else {
                        commands.add(FIFO_VERTEX10);
                        list.add(
                                (pos.xExport & 0x3ff)
                                | ((pos.yExport & 0x3ff) << 10)
                                | ((pos.zExport & 0x3ff) << 20)
                        );
                    }

                }

                prevPos = pos;
            }
        }
    }

}
