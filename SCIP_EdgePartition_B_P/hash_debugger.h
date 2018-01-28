/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#ifndef __HASH_DEBUGGER_H__
#define __HASH_DEBUGGER_H__

typedef unsigned long long HASHCODE;

struct HashMem
{
	HASHCODE mem;
};

/*
create a new HashMem
*/
void createHashMem(
	HashMem** hashmem
	);

/*
release a HashMem
*/
void releaseHashMem(
	HashMem** hashmem
	);

/*
calculate the hash val for the buffer,
this hash val will add the information stored in the hashmem,
after calculating the hash value, this method will update the hashmem to the new hash value
*/
HASHCODE calcHash(
	const void* buffer, 
	size_t length, 
	HashMem* hashmem
);

HASHCODE calcHash_wrap(
	const void* buffer, 
	size_t length
);

void printHash(
	HashMem* hashmem
	);

void printHash_wrap(
	const char* title
	);

#endif