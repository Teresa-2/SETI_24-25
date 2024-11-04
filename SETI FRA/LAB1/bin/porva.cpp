#include <iostream>

int main()
{
	double time = 3000.5555;
	

	
	long dec = time;
	double virgola = time % dec;
	std::cout << virgola << std::endl;
	
	
	return 0;
}
