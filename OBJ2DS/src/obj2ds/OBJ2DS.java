package obj2ds;

import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Scanner;
import javax.imageio.ImageIO;

/**
 *
 * @author Roman Lahin
 */
public class OBJ2DS {
    
    private static final int SKY_FLAG = 1, MUSIC_FLAG = 2, MUSIC_STOP_FLAG = 4;
    
    private static boolean map, exportPolyData = true, exportCmdList = true;

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        
        //Info
        if(args.length == 0) {
            System.out.println("Roman Lahin OBJ2DS util");
            System.out.println("Converts obj files to dsmd model format");
            System.out.println("Or to dsmp map format\n");
            
            System.out.println("100 obj units will be converted to 4096 ds units");
            System.out.println("Be careful with uvm, it will be multiplied with 16384 (16 bit limit is -32768 +32767)\n");
            
            System.out.println("-map (0)/1 to convert obj files to maps");
            System.out.println("-scale ("+Mesh.defScale+") to specify default scaling level");
            System.out.println("(Higher scaling is better for details, "
                    + "but the model may not fit within the 16 bit limit and dynamic scalling"
                    + "will be enabled)\n");
            
            System.out.println("-polydata 0/(1) adds polygon data to model file "
                    + "(used for model morphing, which wont work with dynamic scaling)");
            
            System.out.println("-cmdlist 0/(1) adds command list to model file (Used for fast rendering)");
            
            //dont ask
            (new Scanner(System.in)).nextLine();
            return;
        }
        
        //Read args
        for(int t=0; t<args.length; t++) {
            String arg = args[t];
            //Skip options
            if(arg.startsWith("-")) {
                
                if(arg.equals("-map")) {
                    map = Integer.parseInt(args[t+1]) == 1;
                } else if(arg.equals("-scale")) {
                    Mesh.defScale = Integer.parseInt(args[t+1]);
                } else if(arg.equals("-polydata")) {
                    exportPolyData = Integer.parseInt(args[t+1]) == 1;
                } else if(arg.equals("-cmdlist")) {
                    exportCmdList = Integer.parseInt(args[t+1]) == 1;
                }
                
                t++;
            }
        }
        
        for(int t=0; t<args.length; t++) {
            String arg = args[t];
            //Skip options
            if(arg.startsWith("-")) {
                t++; continue;
            }
            
            Mesh[] loaded = ObjLoader.loadOBJ(arg);
            ArrayList<Mesh> export = new ArrayList<>();
            Mesh exportMesh = new Mesh("export");
            
            //Map stuff
            ArrayList<Vector3D> boundingBoxes = new ArrayList<>();
            String skybox = null;
            int musicID = -1;
            boolean stopMusic = false;
            ArrayList<Vector3D> scriptBoxes = new ArrayList<>();
            ArrayList<Integer> scriptFlags = new ArrayList<>();
            ArrayList<byte[]> scripts = new ArrayList<>();
            
            for(Mesh mesh : loaded) {
                String name = mesh.name;
                if(name.startsWith("col_")) {
                    Mesh prev = export.get(export.size()-1);
                    prev.hasColor = true;
                    exportMesh.hasColor = true;
                    
                    String tex = mesh.name.substring(4);
                    int[][] cols = loadPNG(arg, tex);
                    
                    for(int i=0; i<mesh.pols.size(); i++) {
                        Polygon pol = mesh.pols.get(i);
                        pol.col = new Color[pol.pos.length];
                        prev.pols.get(i).col = pol.col;
                        
                        for(int x=0; x<pol.col.length; x++) {
                            pol.col[x] = getCol(cols, pol.uv[x]);
                        }
                    }
                    
                } else if(name.startsWith("gamephys")) {
                    Vector3D min = new Vector3D(Double.MAX_VALUE, Double.MAX_VALUE, Double.MAX_VALUE);
                    Vector3D max = new Vector3D(-Double.MAX_VALUE, -Double.MAX_VALUE, -Double.MAX_VALUE);
                    boundingBoxes.add(min);
                    boundingBoxes.add(max);
                    
                    ArrayList<Vector3D> verts = mesh.getVertices();
                    for(Vector3D v : verts) {
                        min.min(v);
                        max.max(v);
                    }
                    
                } else if(name.startsWith("gamesky_")) {
                    skybox = name.substring(8);
                    
                } else if(name.startsWith("gamemusic_stop")) {
                    stopMusic = true;
                    
                } else if(name.startsWith("gamemusic_")) {
                    musicID = Integer.parseInt(name.substring(10));
                    
                } else if(name.startsWith("gameclickable_")) {
                    Vector3D min = new Vector3D(Double.MAX_VALUE, Double.MAX_VALUE, Double.MAX_VALUE);
                    Vector3D max = new Vector3D(-Double.MAX_VALUE, -Double.MAX_VALUE, -Double.MAX_VALUE);
                    scriptBoxes.add(min);
                    scriptBoxes.add(max);
                    
                    ArrayList<Vector3D> verts = mesh.getVertices();
                    for(Vector3D v : verts) {
                        min.min(v);
                        max.max(v);
                    }
                    scriptFlags.add(0);
                    scripts.add(Script.read(name.substring(14)));
                    
                } else if(name.startsWith("gameinvis")) {
                    //skip
                    
                } else {
                    export.add(mesh);
                    exportMesh.pols.addAll(mesh.pols);
                    
                }
            }
            
            if(!map) {
                byte[] data = exportMesh.export(exportPolyData, exportCmdList);
                
                try {
                    File file = new File(arg.substring(0, arg.lastIndexOf('.')) + ".dsmd");
                    if(!file.exists()) file.createNewFile();
                    FileOutputStream fos = new FileOutputStream(file);
                    fos.write(data);
                    fos.close();
                } catch(Exception e) {
                    e.printStackTrace();
                }
                
            } else {
                LinkedHashMap<String, Mesh> matMeshes = new LinkedHashMap<>();
                ArrayList<String> materials = new ArrayList<>();
                
                //Split mesh by materials
                for(Polygon p : exportMesh.pols) {
                    Mesh m = matMeshes.get(p.material);
                    
                    if(m == null) {
                        m = new Mesh(p.material);
                        m.hasColor = exportMesh.hasColor;
                        
                        matMeshes.put(p.material, m);
                        materials.add(p.material);
                    }
                    
                    m.pols.add(p);
                }
                
                try {
                    ByteArrayOutputStream baos = new ByteArrayOutputStream();
                    NDSOutputStream dos = new NDSOutputStream(baos);
                    
                    int flags = (skybox == null ? 0 : SKY_FLAG);
                    if(stopMusic) flags |= MUSIC_STOP_FLAG;
                    if(musicID != -1) flags |= MUSIC_FLAG;
                    dos.writeIntLE(flags);
                    
                    if(skybox != null) {
                        int index = materials.indexOf(skybox);
                        
                        if(index == -1) {
                            index = materials.size();
                            materials.add(skybox);
                        }
                        
                        dos.writeIntLE(index);
                    }
                    if(musicID != -1) {
                        dos.writeIntLE(musicID);
                    }
                    
                    //Write bounding boxes
                    dos.writeIntLE(boundingBoxes.size() / 2);
                    
                    for(Vector3D v : boundingBoxes) {
                        dos.write(v.asPosf32());
                    }
                    
                    //Write script boxes
                    dos.writeIntLE(scriptBoxes.size() / 2);
                    
                    for(Vector3D v : scriptBoxes) {
                        dos.write(v.asPosf32());
                    }
                    
                    for(int flag : scriptFlags) {
                        dos.writeIntLE(flag);
                    }
                    
                    for(byte[] script : scripts) {
                        dos.writeIntLE(script.length / 4);
                        dos.write(script);
                    }
                    
                    //Write materials
                    dos.writeIntLE(materials.size());
                    
                    for(String mat : materials) {
                        dos.writeAscii(mat, 32);
                    }
                    
                    //Write meshes materials
                    dos.writeIntLE(matMeshes.size());
                    
                    for(int i=0; i<matMeshes.size(); i++) {
                        dos.writeIntLE(i);
                    }
                    
                    //Write meshes
                    Mesh[] mapMeshes = new Mesh[matMeshes.size()];
                    matMeshes.values().toArray(mapMeshes);
                    
                    for(int i=0; i<mapMeshes.length; i++) {
                        Mesh m = mapMeshes[i];
                        byte[] mesh = m.export(false, true);
                        dos.writeIntLE(mesh.length / 4);
                        dos.write(mesh);
                    }
                    
                    File file = new File(arg.substring(0, arg.lastIndexOf('.')) + ".dsmp");
                    if(!file.exists()) file.createNewFile();
                    FileOutputStream fos = new FileOutputStream(file);
                    fos.write(baos.toByteArray());
                    fos.close();
                    baos.close();
                } catch(Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private static Color getCol(int[][] cols, Vector2D uv) {
        int w = cols.length, h = cols[0].length;
        int x = (int) ((uv.x - Math.floor(uv.x)) * w);
        int y = (int) ((uv.y - Math.floor(uv.y)) * h);
        
        return new Color(cols[x][y]);
    }
    
    private static int[][] loadPNG(String model, String tex) {
        try {
            File file = new File(model.substring(0, model.lastIndexOf(File.separator) + 1) + tex);
            if(!file.exists()) throw new Exception("File " + file.getAbsolutePath() + "doesn't exist!");

            BufferedImage img = ImageIO.read(file);
            int imgW = img.getWidth(), imgH = img.getHeight();

            int[] rawColors = new int[imgW * imgH];
            img.getRGB(0, 0, imgW, imgH, rawColors, 0, imgW);

            int[][] colors = new int[imgW][imgH];
            for(int i = 0; i < rawColors.length; i++) {
                colors[i % imgW][i / imgW] = rawColors[i];
            }

            return colors;
            
        } catch (Exception e) {
            e.printStackTrace();
            
            return new int[][]{{0}};
        }
    }
    
}
