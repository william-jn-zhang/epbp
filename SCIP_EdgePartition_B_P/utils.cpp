/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include <iostream>

#include "utils.h"

using namespace std;


void printArray(const char* title, double* arr, int size)
{
	cout << "<-------------" << title << "------------->" << endl;
	for(int i = 0; i < size; ++i)
	{
		cout << arr[i] << ' ';
	}
	cout << endl;
	return;
}