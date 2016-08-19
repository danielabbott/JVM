package java.io;

import java.lang.Integer;

public final class PrintStream extends OutputStream {

	public PrintStream(OutputStream o)
	{
	
	}

	private static native void jvm_ntv_write_stdout (int b);

	public void write(int b) 
	{
		jvm_ntv_write_stdout(b);
	}

	private void putString(String s)
	{
		for(int i = 0; i < s.length(); i++) {
			write(s.charAt(i));
		}
	}

	public void println(String x)
	{
		putString(x);
		write((int)'\n');
    }

	public void println(int i)
	{
        putString(Integer.toString(i));
		write((int)'\n');
    }

}