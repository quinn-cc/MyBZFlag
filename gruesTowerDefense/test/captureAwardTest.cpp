#include <iostream>
using namespace std;


// a capping b
int fairCap(int a, int b)
{
	return (int) (3*(float)b*((float)b/(float)a));
}

int unfairCap(int a, int b)
{
	return (int) -(3*(float)a*((float)a/(float)b));
}

int cap(int a, int b)
{
	if ( (float)a/(float)b <= 4.0/3.0 )
		return fairCap(a,b);
	else
		return unfairCap(a,b);
}


int main()
{
	cout << "  | 1   2   3   4   5   6   7   8   9   10" << endl;

	for (int a = 1; a <= 10; a++)
	{
		cout << a;
		
		if (a < 10)
			cout << " ";
		
		cout << "| ";
		
		for (int b = 1; b <= 10; b++)
		{
			cout << cap(a,b);
			
			if (cap(a,b) <= -100)
				cout << "";
			else if (cap(a,b) <= -10)
				cout << " ";
			else if (cap(a,b) < 0)
				cout << "  ";
			else if (cap(a,b) < 10)
				cout << "   ";
			else if (cap(a,b) < 100)
				cout << "  ";
			else
				cout << " ";
		}
		
		cout << endl;
	}
	
	return 0;
}
