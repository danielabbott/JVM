package java.io;

public abstract class OutputStream {

	private int file;

	abstract void write(int b);

	void close(){}
	void flush(){}

	void write(byte[] b)
	{
		write(b, 0, b.length);
	}

	void write(byte[]b, int off, int len)
	{
		// TODO throw exceptions when parameters are invalid
		if(b != null || off < 0 || off + len > b.length || len <= 0 || off + len < 0)
			return;
		for(int i = 0; i < len; i++)
			write(b[off+i]);
	}

}
