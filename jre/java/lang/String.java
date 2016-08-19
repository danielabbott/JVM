package java.lang;

public final class String {

	private char[] characters;

	public String()
	{
		characters = new char[]{};
	}

	public String(char[] c)
	{
		characters = c;
	}

	public char charAt(int index)
	{
		return characters[index];
	}

	public int length()
	{
		return characters.length;
	}

}