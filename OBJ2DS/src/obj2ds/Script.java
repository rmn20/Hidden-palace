package obj2ds;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 *
 * @author Roman Lahin
 */
public class Script {

    public static byte[] read(String code) {
        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            NDSOutputStream dos = new NDSOutputStream(baos);
            
            String[] lines = code.split(";");
            
            for(String line : lines) {
                String[] args = line.split("_");

                for(int i = 0; i < args.length; i++) {
                    String arg = args[i];

                    if(arg.equals("map")) {
                        int flags = 0;
                        if(args.length >= 5) flags |= 1; //Player pos
                        if(args.length >= 7) flags |= 2; //Player rot
                        
                        if((flags & 2) == 2) {
                            dos.write(Opcode.PUSHN);
                            dos.write(2 * 2);
                            dos.writeShortLE((int) Math.round(Double.parseDouble(args[i + 5]) * 32768 / 360));
                            dos.writeShortLE((int) Math.round(Double.parseDouble(args[i + 6]) * 32768 / 360));
                        }
                        
                        if((flags & 1) == 1) {
                            dos.write(Opcode.PUSHN);
                            dos.write(4 * 3);
                            dos.writeIntLE((int) Math.round(Double.parseDouble(args[i + 2]) * 0.01 * 4096));
                            dos.writeIntLE((int) Math.round(Double.parseDouble(args[i + 3]) * 0.01 * 4096));
                            dos.writeIntLE((int) Math.round(Double.parseDouble(args[i + 4]) * 0.01 * 4096));
                        }
                        
                        dos.write(Opcode.PUSHN);
                        dos.write(args[i + 1].length() + 1);
                        dos.writeAscii(args[i + 1]);
                        
                        dos.write(Opcode.PUSH);
                        dos.write(flags);

                        dos.write(Opcode.FUNC);
                        dos.write(Func.MAP);
                        i++;
                    } else if(arg.equals("playerpos")) {
                        dos.write(Opcode.PUSHN);
                        dos.write(4 * 3);
                        dos.writeIntLE((int) Math.round(Double.parseDouble(args[i + 3]) * 0.01 * 4096));
                        dos.writeIntLE((int) Math.round(Double.parseDouble(args[i + 2]) * 0.01 * 4096));
                        dos.writeIntLE((int) Math.round(Double.parseDouble(args[i + 1]) * 0.01 * 4096));

                        dos.write(Opcode.FUNC);
                        dos.write(Func.PLAYERPOS);
                        i += 3;
                    } else if(arg.equals("playerrot")) {
                        dos.write(Opcode.PUSHN);
                        dos.write(2 * 2);
                        dos.writeShortLE((int) Math.round(Double.parseDouble(args[i + 2]) * 32768 / 360));
                        dos.writeShortLE((int) Math.round(Double.parseDouble(args[i + 1]) * 32768 / 360));

                        dos.write(Opcode.FUNC);
                        dos.write(Func.PLAYERROT);

                        i += 2;
                    }
                }
            }

            dos.write(Opcode.END);

            while((baos.size() & 3) != 0) dos.write(Opcode.NOP);

            byte[] data = baos.toByteArray();
            baos.close();
            return data;
        } catch (IOException e) {
            e.printStackTrace();
        }
        
        return new byte[]{(byte)Opcode.END, (byte)Opcode.NOP, (byte)Opcode.NOP, (byte)Opcode.NOP};
    }

}
