package obj2ds;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;

/**
 *
 * @author Roman Lahin
 */
public class NDSOutputStream extends DataOutputStream {

    public NDSOutputStream(OutputStream out) {
        super(out);
    }
    
    public void writeIntLE(int data) throws IOException {
        write(data&0xff);
        write((data>>8)&0xff);
        write((data>>16)&0xff);
        write((data>>>24)&0xff);
    }

    public void writeShortLE(int data) throws IOException {
        write(data&0xff);
        write((data>>8)&0xff);
    }

    public void writeAscii(String line) throws IOException {
        for(int i=0; i<line.length(); i++) {
            write(line.charAt(i));
        }
        
        write('\0');
    }

    public void writeAscii(String line, int len) throws IOException {
        for(int i=0; i<len; i++) {
            char c = (i < line.length() && i != len-1) ? line.charAt(i) : '\0';
            
            write(c);
        }
    }
    
}
