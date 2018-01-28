/**@file   
 * @brief  
 * @author william-jn-zhang
 *
 * 
 */

#include "hash_debugger.h"
#include "xxhash.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>

HashMem* HASHMEM;

void createHashMem(
	HashMem** hashmem
	)
{
	*hashmem = (HashMem*)malloc(sizeof(HashMem));
	(*hashmem) -> mem = 0L;
}

void releaseHashMem(
	HashMem** hashmem
	)
{
	free(*hashmem);
}

HASHCODE calcHash(
	const void* buffer, 
	size_t length, 
	HashMem* hashmem
)
{
	HASHCODE seed = 0;
	void* _buffer = NULL;
	size_t newlength = length + sizeof(HASHCODE);

	_buffer = malloc(newlength);
	memset(_buffer, 0, newlength);

	memcpy(_buffer, buffer, length);
	memcpy((unsigned char*)_buffer + length, &(hashmem -> mem), sizeof(HASHCODE));

	HASHCODE hashcode = XXH64(_buffer, newlength, seed);

	hashmem -> mem = hashcode;

	return hashcode;
}

HASHCODE calcHash_wrap(
	const void* buffer, 
	size_t length
)
{
	extern HashMem* HASHMEM;
	return calcHash(buffer, length, HASHMEM);
}

void printHash(
	HashMem* hashmem
	)
{
	printf("%lld\n", hashmem -> mem);
}

void printHash_wrap(const char* title)
{
	extern HashMem* HASHMEM;
	printf(title);
	printf("%lld\n", HASHMEM -> mem);
}