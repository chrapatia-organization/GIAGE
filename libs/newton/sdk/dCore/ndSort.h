/* Copyright (c) <2003-2021> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __ND_SORT_H__
#define __ND_SORT_H__

#include "ndCoreStdafx.h"
#include "ndHeap.h"
#include "ndProfiler.h"
#include "ndThreadPool.h"

template <class T, class CompareKey>
void ndBinarySearch(T* const array, ndInt32 elements, void* const context = nullptr)
{
	dAssert(0);
	//ndInt32 index0 = 0;
	//ndInt32 index2 = elements - 1;
	//
	//while ((index2 - index0) > 4) 
	//{
	//	ndInt32 index1 = (index0 + index2) >> 1;
	//	ndInt32 test = compare(&array[index1], &entry, context);
	//	if (test < 0) 
	//	{
	//		index0 = index1;
	//	}
	//	else 
	//	{
	//		index2 = index1;
	//	}
	//}
	//
	//index0 = (index0 > 0) ? index0 - 1 : 0;
	//index2 = ((index2 + 1) < elements) ? index2 + 1 : elements;
	//ndInt32 index = index0 - 1;
	//for (ndInt32 i = index0; i < index2; i++) 
	//{
	//	ndInt32 test = compare(&array[i], &entry, context);
	//	if (!test) 
	//	{
	//		return i;
	//	}
	//	else if (test > 0) 
	//	{
	//		break;
	//	}
	//	index = i;
	//}
	//return index;
}

template <class T, class dCompareKey>
void ndSort(T* const array, ndInt32 elements, void* const context = nullptr)
{
	D_TRACKTIME();
	const ndInt32 batchSize = 8;
	ndInt32 stack[1024][2];

	stack[0][0] = 0;
	stack[0][1] = elements - 1;
	ndInt32 stackIndex = 1;
	const dCompareKey comparator;
	while (stackIndex)
	{
		stackIndex--;
		ndInt32 lo = stack[stackIndex][0];
		ndInt32 hi = stack[stackIndex][1];
		if ((hi - lo) > batchSize)
		{
			ndInt32 mid = (lo + hi) >> 1;
			if (comparator.Compare(array[lo], array[mid], context) > 0)
			{
				dSwap(array[lo], array[mid]);
			}
			if (comparator.Compare(array[mid], array[hi], context) > 0)
			{
				dSwap(array[mid], array[hi]);
			}
			if (comparator.Compare(array[lo], array[mid], context) > 0)
			{
				dSwap(array[lo], array[mid]);
			}
			ndInt32 i = lo + 1;
			ndInt32 j = hi - 1;
			const T pivot(array[mid]);
			do
			{
				while (comparator.Compare(array[i], pivot, context) < 0) i++;
				while (comparator.Compare(array[j], pivot, context) > 0) j--;

				if (i <= j)
				{
					dSwap(array[i], array[j]);
					i++;
					j--;
				}
			} while (i <= j);

			if (i < hi)
			{
				stack[stackIndex][0] = i;
				stack[stackIndex][1] = hi;
				stackIndex++;
			}
			if (lo < j)
			{
				stack[stackIndex][0] = lo;
				stack[stackIndex][1] = j;
				stackIndex++;
			}
			dAssert(stackIndex < ndInt32(sizeof(stack) / (2 * sizeof(stack[0][0]))));
		}
	}

	ndInt32 stride = batchSize + 1;
	if (elements < stride)
	{
		stride = elements;
	}
	for (ndInt32 i = 1; i < stride; i++)
	{
		if (comparator.Compare(array[0], array[i], context) > 0)
		{
			dSwap(array[0], array[i]);
		}
	}

	for (ndInt32 i = 1; i < elements; i++)
	{
		ndInt32 j = i;
		const T tmp(array[i]);
		for (; comparator.Compare(array[j - 1], tmp, context) > 0; j--)
		{
			dAssert(j > 0);
			array[j] = array[j - 1];
		}
		array[j] = tmp;
	}

	#ifdef _DEBUG
		for (ndInt32 i = 0; i < (elements - 1); i++)
		{
			dAssert(comparator.Compare(array[i], array[i + 1], context) <= 0);
		}
	#endif
}

template <class T, ndInt32 bits, class nEvaluateKey, class dKey>
void ndCountingSort(T* const array, T* const scratchBuffer, ndInt32 elementsCount, ndInt32 digitLocation)
{
	dAssert(0);
//	ndInt32 scanCount[256];
//	ndInt32 histogram[256][4];
//
//	dAssert(radixPass >= 1);
//	dAssert(radixPass <= 4);
//
//	memset(histogram, 0, sizeof(histogram));
//	for (ndInt32 i = 0; i < elements; i++)
//	{
//		ndInt32 key = getRadixKey(&array[i], context);
//		for (ndInt32 j = 0; j < radixPass; j++)
//		{
//			ndInt32 radix = (key >> (j << 3)) & 0xff;
//			histogram[radix][j] = histogram[radix][j] + 1;
//		}
//	}
//
//	for (ndInt32 radix = 0; radix < radixPass; radix += 2)
//	{
//		scanCount[0] = 0;
//		for (ndInt32 i = 1; i < 256; i++)
//		{
//			scanCount[i] = scanCount[i - 1] + histogram[i - 1][radix];
//		}
//		ndInt32 radixShift = radix << 3;
//		for (ndInt32 i = 0; i < elements; i++)
//		{
//			ndInt32 key = (getRadixKey(&array[i], context) >> radixShift) & 0xff;
//			ndInt32 index = scanCount[key];
//			tmpArray[index] = array[i];
//			scanCount[key] = index + 1;
//		}
//
//		if ((radix + 1) < radixPass)
//		{
//			scanCount[0] = 0;
//			for (ndInt32 i = 1; i < 256; i++) {
//				scanCount[i] = scanCount[i - 1] + histogram[i - 1][radix + 1];
//			}
//
//			ndInt32 radixShift = (radix + 1) << 3;
//			for (ndInt32 i = 0; i < elements; i++)
//			{
//				ndInt32 key = (getRadixKey(&array[i], context) >> radixShift) & 0xff;
//				ndInt32 index = scanCount[key];
//				array[index] = tmpArray[i];
//				scanCount[key] = index + 1;
//			}
//		}
//		else
//		{
//			memcpy(array, tmpArray, elements * sizeof(T));
//		}
//	}
//
//#ifdef _DEBUG
//	for (ndInt32 i = 0; i < (elements - 1); i++)
//	{
//		dAssert(getRadixKey(&array[i], context) <= getRadixKey(&array[i + 1], context));
//	}
//#endif
}

#endif
