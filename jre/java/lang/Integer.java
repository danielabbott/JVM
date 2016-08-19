package java.lang;

public final class Integer {

	public static final int MAX_VALUE = 2147483647;
	public static final int MIN_VALUE = -2147483647;
	public static final int SIZE = 32;

	public static String toString(int i) 
	{
		int i_ = i;

		char[] chars = new char[11];
		int j = 0;

		if(i < 0) i = -i;

		if(i == 0) {
			chars[j++] = '0';
		} else {
			while(i != 0) {
				chars[j] = '0';
				chars[j++] += ((char)(i % 10));
		
				i /= 10;
			}
		}

		if(i_ < 0)
			chars[j++] = '-';

		char[] chars2 = new char[j];
		for(int k = 0; k < j; k++) {
			chars2[k] = chars[j-k-1];
		}

		return new String(chars2);
	}

}