/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include "utils.h"

using namespace std;


void printArrayDouble(const char* title, double* arr, int size)
{
	SCIPdebugMessage("<-------------%s------------->\n", title);
	for(int i = 0; i < size; ++i)
	{
		SCIPdebugMessage("%d ", arr[i]);
	}
	SCIPdebugMessage("\n");
	return;
}