package java.lang;

public final class StringBuilder {

	public static native void jvm_ntv_print_var (long i);
	public static native void jvm_ntv_print_var (int i);
	public static native void jvm_ntv_print_var (double i);
	public static native void jvm_ntv_print_var (float i);
	public static native void jvm_ntv_print_var (short i);
	public static native void jvm_ntv_print_var (char i);
	public static native void jvm_ntv_print_var (byte i);
	public static native void jvm_ntv_print_var (char[] c);
	public static native void jvm_ntv_print_var (int[] i);
	public static native void jvm_ntv_print_var (String s);

	private String string = "";

	public StringBuilder append(int i)
	{
		return append(Integer.toString(i));
	}

	public StringBuilder append(String s)
	{
		char[] newChars = new char[string.length() + s.length()];

		for(int i = 0; i < string.length(); i++)
			newChars[i] = string.charAt(i);

		for(int i = string.length(), j = 0; i < string.length() + s.length(); i++, j++)
			newChars[i] = s.charAt(j);

		string = new String(newChars);

		return this;
	}

	public String toString()
	{
		return string;
	}

}