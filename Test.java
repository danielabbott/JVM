public class Test {	

    public static void main(String[] args) {
		System.out.println(2);
		for(int i = 3; i < 100; i += 2) {
			boolean isPrime = true;
			for(int j = 3; j < i/2; j += 2) {
				if((i % j) == 0) {
					isPrime = false;
					break;
				}
			}
			if(isPrime)
				System.out.println(i);
		}
    }

}