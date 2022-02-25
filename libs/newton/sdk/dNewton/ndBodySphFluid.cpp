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

#include "ndCoreStdafx.h"
#include "ndNewtonStdafx.h"
#include "ndWorld.h"
#include "ndBodySphFluid.h"


/*
{
ndFloat32 xxx[6][6][6];
for (ndInt32 i = 0; i < 6 * 6 * 6; i++)
{
ndFloat32* yyy = &xxx[0][0][0];
yyy[i] = 1.0f;
}
for (ndInt32 i = 0; i < uniqueCount; i++)
{
ndInt32 x = m_hashGridMap[i].m_x;
ndInt32 y = m_hashGridMap[i].m_y;
ndInt32 z = m_hashGridMap[i].m_z;

xxx[z][y][x] = 0.0f;
}

dIsoSurfaceOld isoSurcase;
isoSurcase.GenerateSurface(&xxx[0][0][0], 0.5f, 5, 5, 5, gridSize, gridSize, gridSize);
cellCount *= 1;
}
*/

ndBodySphFluid::ndBodySphFluid()
	:ndBodyParticleSet()
	,m_box0(ndFloat32(-1e10f))
	,m_box1(ndFloat32(1e10f))
	,m_hashGridMap(1024)
	,m_particlesPairs(1024)
	,m_hashGridMapScratchBuffer(1024)
//	,m_gridScans(1024)
{
}

ndBodySphFluid::ndBodySphFluid(const ndLoadSaveBase::ndLoadDescriptor& desc)
	:ndBodyParticleSet(desc)
	,m_box0(ndFloat32(-1e10f))
	,m_box1(ndFloat32(1e10f))
	,m_hashGridMap()
	,m_hashGridMapScratchBuffer()
{
	// nothing was saved
	dAssert(0);
}

ndBodySphFluid::~ndBodySphFluid()
{
}

//void ndBodySphFluid::Save(const dLoadSaveBase::dSaveDescriptor& desc) const
void ndBodySphFluid::Save(const ndLoadSaveBase::ndSaveDescriptor&) const
{
	dAssert(0);
	//nd::TiXmlElement* const paramNode = CreateRootElement(rootNode, "ndBodySphFluid", nodeid);
	//ndBodyParticleSet::Save(paramNode, assetPath, nodeid, shapesCache);
}

void ndBodySphFluid::AddParticle(const ndFloat32, const ndVector& position, const ndVector&)
{
	ndVector point(position);
	point.m_w = ndFloat32(1.0f);
	m_posit.PushBack(point);
}

void ndBodySphFluid::CaculateAABB(const ndWorld* const, ndVector& boxP0, ndVector& boxP1) const
{
	D_TRACKTIME();
	ndVector box0(ndFloat32(1e20f));
	ndVector box1(ndFloat32(-1e20f));
	for (ndInt32 i = m_posit.GetCount() - 1; i >= 0; i--)
	{
		box0 = box0.GetMin(m_posit[i]);
		box1 = box1.GetMax(m_posit[i]);
	}
	boxP0 = box0;
	boxP1 = box1;
}

void ndBodySphFluid::Update(const ndWorld* const world, ndFloat32)
{
	ndVector boxP0;
	ndVector boxP1;
	CaculateAABB(world, boxP0, boxP1);
	const ndFloat32 gridSize = CalculateGridSize();
	m_box0 = boxP0 - ndVector(gridSize);
	m_box1 = boxP1 + ndVector(gridSize);

	CreateGrids(world);
	SortGrids(world);
	BuildPairs(world);
	CalculateAccelerations(world);
}

void ndBodySphFluid::SortByCenterType()
{
	D_TRACKTIME();
	const ndInt32 count = m_hashGridMap.GetCount();

	ndInt32 histogram[2];
	memset(histogram, 0, sizeof(histogram));

	const ndGridHash* const srcArray = &m_hashGridMap[0];
	for (ndInt32 i = 0; i < count; i++)
	{
		const ndGridHash& entry = srcArray[i];
		const ndInt32 index = ndInt32(entry.m_cellType);
		histogram[index] = histogram[index] + 1;
	}

	ndInt32 acc = 0;
	for (ndInt32 i = 0; i < 2; i++)
	{
		const ndInt32 n = histogram[i];
		histogram[i] = acc;
		acc += n;
	}

	ndGridHash* const dstArray = &m_hashGridMapScratchBuffer[0];
	for (ndInt32 i = 0; i < count; i++)
	{
		const ndGridHash& entry = srcArray[i];
		const ndInt32 key = ndInt32(entry.m_cellType);
		const ndInt32 index = histogram[key];
		dstArray[index] = entry;
		histogram[key] = index + 1;
	}
	m_hashGridMap.Swap(m_hashGridMapScratchBuffer);
}

void ndBodySphFluid::SortSingleThreaded()
{
	D_TRACKTIME();
	const ndInt32 count = m_hashGridMap.GetCount();

	ndInt32 histograms[6][1 << D_RADIX_DIGIT_SIZE];
	memset(histograms, 0, sizeof(histograms));

	ndGridHash* srcArray = &m_hashGridMap[0];
	for (ndInt32 i = 0; i < count; i++)
	{
		const ndGridHash& entry = srcArray[i];

		const ndInt32 xlow = entry.m_xLow;
		histograms[0][xlow] = histograms[0][xlow] + 1;

		const ndInt32 xHigh = entry.m_xHigh;
		histograms[1][xHigh] = histograms[1][xHigh] + 1;

		const ndInt32 ylow = entry.m_yLow;
		histograms[2][ylow] = histograms[2][ylow] + 1;

		const ndInt32 yHigh = entry.m_yHigh;
		histograms[3][yHigh] = histograms[3][yHigh] + 1;

		const ndInt32 zlow = entry.m_zLow;
		histograms[4][zlow] = histograms[4][zlow] + 1;

		const ndInt32 zHigh = entry.m_zHigh;
		histograms[5][zHigh] = histograms[5][zHigh] + 1;
	}

	ndInt32 acc[6];
	memset(acc, 0, sizeof(acc));
	for (ndInt32 i = 0; i < (1 << D_RADIX_DIGIT_SIZE); i++)
	{
		for (ndInt32 j = 0; j < 6; j++)
		{
			const ndInt32 n = histograms[j][i];
			histograms[j][i] = acc[j];
			acc[j] += n;
		}
	}

	ndInt32 shiftbits = 0;
	ndUnsigned64 mask = (1 << D_RADIX_DIGIT_SIZE) - 1;
	ndGridHash* dstArray = &m_hashGridMapScratchBuffer[0];
	for (ndInt32 radix = 0; radix < 3; radix++)
	{
		ndInt32* const scan0 = &histograms[radix * 2][0];
		for (ndInt32 i = 0; i < count; i++)
		{
			const ndGridHash& entry = srcArray[i];
			const ndInt32 key = ndUnsigned32((entry.m_gridHash & mask) >> shiftbits);
			const ndInt32 index = scan0[key];
			dstArray[index] = entry;
			scan0[key] = index + 1;
		}
		mask <<= D_RADIX_DIGIT_SIZE;
		shiftbits += D_RADIX_DIGIT_SIZE;
		dSwap(dstArray, srcArray);

		if (m_upperDigisIsValid[radix])
		{
			ndInt32* const scan1 = &histograms[radix * 2 + 1][0];
			for (ndInt32 i = 0; i < count; i++)
			{
				const ndGridHash& entry = dstArray[i];
				const ndInt32 key = ndUnsigned32((entry.m_gridHash & mask) >> shiftbits);
				const ndInt32 index = scan1[key];
				srcArray[index] = entry;
				scan1[key] = index + 1;
			}
			dSwap(dstArray, srcArray);
		}
		mask <<= D_RADIX_DIGIT_SIZE;
		shiftbits += D_RADIX_DIGIT_SIZE;
	}

	if (srcArray != &m_hashGridMap[0])
	{
		m_hashGridMap.Swap(m_hashGridMapScratchBuffer);
	}
	dAssert(srcArray == &m_hashGridMap[0]);
}

void ndBodySphFluid::AddCounters(const ndWorld* const world, ndContext& context) const
{
	D_TRACKTIME();

	ndInt32 acc = 0;
	for (ndInt32 i = 0; i < ndInt32 (sizeof(context.m_scan) / sizeof(ndInt32)); i++)
	{
		ndInt32 sum = context.m_scan[i];
		context.m_scan[i] = acc;
		acc += sum;
	}

	ndInt32 accTemp[1 << D_RADIX_DIGIT_SIZE];
	memset(accTemp, 0, sizeof(accTemp));

	const ndInt32 count = sizeof(context.m_scan) / sizeof(ndInt32);
	const ndInt32 threadCount = world->GetThreadCount();
	for (ndInt32 threadId = 0; threadId < threadCount; threadId++)
	{
		for (ndInt32 i = 0; i < count; i++)
		{
			ndInt32 a = context.m_histogram[threadId][i];
			context.m_histogram[threadId][i] = accTemp[i] + context.m_scan[i];
			accTemp[i] += a;
		}
	}
}

void ndBodySphFluid::SortParallel(const ndWorld* const world)
{
	D_TRACKTIME();
	class ndBodySphFluidCountDigits : public ndScene::ndBaseJob
	{
		virtual void Execute()
		{
			D_TRACKTIME();

			ndWorld* const world = m_owner->GetWorld();
			ndContext* const context = (ndContext*)m_context;
			ndBodySphFluid* const fluid = context->m_fluid;
			const ndInt32 threadId = GetThreadId();
			const ndInt32 threadCount = world->GetThreadCount();
			
			const ndInt32 count = fluid->m_hashGridMap.GetCount();
			const ndInt32 size = count / threadCount;
			const ndInt32 start = threadId * size;
			const ndInt32 batchSize = (threadId == threadCount - 1) ? count - start : size;

			ndGridHash* const hashArray = &fluid->m_hashGridMap[0];
			ndInt32* const histogram = context->m_histogram[threadId];

			memset(histogram, 0, sizeof(context->m_scan));
			const ndInt32 shiftbits = context->m_pass * D_RADIX_DIGIT_SIZE;
			const ndUnsigned64 mask = (ndUnsigned64((1 << D_RADIX_DIGIT_SIZE) - 1)) << shiftbits;

			for (ndInt32 i = 0; i < batchSize; i++)
			{
				const ndGridHash& entry = hashArray[i + start];
				const ndInt32 key = ndUnsigned32((entry.m_gridHash & mask) >> shiftbits);
				histogram[key] += 1;
			}
		}
	};

	class ndBodySphFluidAddPartialSum : public ndScene::ndBaseJob
	{
		virtual void Execute()
		{
			D_TRACKTIME();
			ndWorld* const world = m_owner->GetWorld();
			ndContext* const context = (ndContext*)m_context;
			const ndInt32 threadId = GetThreadId();
			const ndInt32 threadCount = world->GetThreadCount();

			const ndInt32 count = sizeof (context->m_scan) / sizeof (context->m_scan[0]);

			const ndInt32 size = count / threadCount;
			const ndInt32 start = threadId * size;
			const ndInt32 batchSize = (threadId == threadCount - 1) ? count - start : size;

			ndInt32* const scan = context->m_scan;
			for (ndInt32 i = 0; i < batchSize; i++)
			{
				ndInt32 acc = 0;
				for (ndInt32 j = 0; j < threadCount; j++)
				{
					acc += context->m_histogram[j][i + start];
				}
				scan[i + start] = acc;
			}
		}
	};

	class ndBodySphFluidReorderBuckets: public ndScene::ndBaseJob
	{
		virtual void Execute()
		{
			D_TRACKTIME();
			ndWorld* const world = m_owner->GetWorld();
			ndContext* const context = (ndContext*)m_context;
			ndBodySphFluid* const fluid = context->m_fluid;
			const ndInt32 threadId = GetThreadId();
			const ndInt32 threadCount = world->GetThreadCount();

			const ndInt32 count = fluid->m_hashGridMap.GetCount();
			const ndInt32 size = count / threadCount;
			const ndInt32 start = threadId * size;
			const ndInt32 batchSize = (threadId == threadCount - 1) ? count - start : size;

			ndGridHash* const srcArray = &fluid->m_hashGridMap[0];
			ndGridHash* const dstArray = &fluid->m_hashGridMapScratchBuffer[0];

			const ndInt32 shiftbits = context->m_pass * D_RADIX_DIGIT_SIZE;
			const ndUnsigned64 mask = (ndUnsigned64((1 << D_RADIX_DIGIT_SIZE)) - 1) << shiftbits;

			ndInt32* const histogram = context->m_histogram[threadId];
			for (ndInt32 i = 0; i < batchSize; i++)
			{
				const ndGridHash& entry = srcArray[i + start];
				const ndInt32 key = ndUnsigned32((entry.m_gridHash & mask) >> shiftbits);
				const ndInt32 index = histogram[key];
				dstArray[index] = entry;
				histogram[key] = index + 1;
			}
		}
	};

	ndContext context;
	context.m_fluid = this;
	ndScene* const scene = world->GetScene();
	for (ndInt32 pass = 0; pass < 6; pass++)
	{
		if (!(pass & 1) || m_upperDigisIsValid[pass >> 1])
		{
			D_TRACKTIME();
			context.m_pass = pass;
			scene->SubmitJobs<ndBodySphFluidCountDigits>(&context);
			scene->SubmitJobs<ndBodySphFluidAddPartialSum>(&context);

			AddCounters(world, context);

			scene->SubmitJobs<ndBodySphFluidReorderBuckets>(&context);
			m_hashGridMap.Swap(m_hashGridMapScratchBuffer);
		}
	}	
}


D_NEWTON_API void ndBodySphFluid::GenerateIsoSurface(const ndWorld* const)
{
return;
#if 0
	D_TRACKTIME();
	dVector boxP0;
	dVector boxP1;
	CaculateAABB(world, boxP0, boxP1);

	dFloat32 gridSize = m_radius * dFloat32(2.0f);
	const dVector invGridSize (dFloat32 (1.0f) / gridSize);

	dVector padd(dFloat32(2.0f) * gridSize);
	boxP0 -= padd & dVector::m_triplexMask;
	boxP1 += padd & dVector::m_triplexMask;

	m_hashGridMap.SetCount(m_posit.GetCount());
	m_hashGridMapScratchBuffer.SetCount(m_posit.GetCount());
	const dVector* const posit = &m_posit[0];
	
	for (dInt32 i = m_posit.GetCount() - 1; i >= 0; i--)
	{
		dAssert(0);
		//dVector r(posit[i] - boxP0);
		//dVector p(r * invGridSize);
		//ndGridHash hashKey(p, i, ndHomeGrid);
		//m_hashGridMap[i] = hashKey;
	}

	ndContext context;
	context.m_fluid = this;

	SortSingleThreaded(world);
	dInt32 uniqueCount = 0;
	for (dInt32 i = 0; i < m_hashGridMap.GetCount();)
	{
		dUnsigned64 key0 = m_hashGridMap[i].m_gridHash;
		m_hashGridMap[uniqueCount].m_gridHash = m_hashGridMap[i].m_gridHash;
		uniqueCount++;
		for (i ++; (i < m_hashGridMap.GetCount()) && (key0 == m_hashGridMap[i].m_gridHash); i++);
	}

	dAssert(0);
	//ndGridHash hashBox0(dVector::m_zero, 0, ndHomeGrid);
	//ndGridHash hashBox1((boxP1 - boxP0) * invGridSize, 0, ndHomeGrid);
	//
	//dUnsigned64 cellCount = (hashBox1.m_z - hashBox0.m_z) * (hashBox1.m_y - hashBox0.m_y) * (hashBox1.m_x - hashBox0.m_x);
	//
	////if (cellCount <= 128)
	//if (cellCount <= 256)
	//{
	//	dAssert((hashBox1.m_z - hashBox0.m_z) > 1);
	//	dAssert((hashBox1.m_y - hashBox0.m_y) > 1);
	//	dAssert((hashBox1.m_x - hashBox0.m_x) > 1);
	//
	//	const dInt32 x_ = 6;
	//	const dInt32 y_ = 6;
	//	const dInt32 z_ = 20;
	//	dFloat32 xxx[z_][y_][x_];
	//	memset(xxx, 0, sizeof(xxx));
	//	for (dInt32 i = 0; i < uniqueCount; i++)
	//	{
	//		dInt32 x = m_hashGridMap[i].m_x;
	//		dInt32 y = m_hashGridMap[i].m_y;
	//		dInt32 z = m_hashGridMap[i].m_z;
	//		xxx[z][y][x] = 1.0f;
	//	}
	//	
	//	dInt32 gridCountX = dInt32(hashBox1.m_x - hashBox0.m_x) + 32;
	//	dInt32 gridCountY = dInt32(hashBox1.m_y - hashBox0.m_y) + 32;
	//	dInt32 gridCountZ = dInt32(hashBox1.m_z - hashBox0.m_z) + 32;
	//	m_isoSurcase.Begin(boxP0, dFloat32(0.5f), gridSize, gridCountX, gridCountY, gridCountZ);
	//	
	//	dIsoSurface::dIsoCell cell;
	//	for (dInt32 z = 0; z < z_-1; z++)
	//	{
	//		cell.m_z = z;
	//		for (dInt32 y = 0; y < y_-1; y++)
	//		{
	//			cell.m_y = y;
	//			for (dInt32 x = 0; x < x_-1; x++)
	//			{
	//				cell.m_x = x;
	//				cell.m_isoValues[0][0][0] = xxx[z + 0][y + 0][x + 0];
	//				cell.m_isoValues[0][0][1] = xxx[z + 0][y + 0][x + 1];
	//				cell.m_isoValues[0][1][0] = xxx[z + 0][y + 1][x + 0];
	//				cell.m_isoValues[0][1][1] = xxx[z + 0][y + 1][x + 1];
	//				cell.m_isoValues[1][0][0] = xxx[z + 1][y + 0][x + 0];
	//				cell.m_isoValues[1][0][1] = xxx[z + 1][y + 0][x + 1];
	//				cell.m_isoValues[1][1][0] = xxx[z + 1][y + 1][x + 0];
	//				cell.m_isoValues[1][1][1] = xxx[z + 1][y + 1][x + 1];
	//				m_isoSurcase.ProcessCell(cell);
	//			}
	//		}
	//	}
	//	m_isoSurcase.End();
	//}
#endif
}


void ndBodySphFluid::CalculateScansDebug(ndArray<ndInt32>& gridScans)
{
	ndInt32 count = 0;
	gridScans.SetCount(0);
	ndUnsigned64 gridHash0 = m_hashGridMap[0].m_gridHash;
	for (ndInt32 i = 0; i < m_hashGridMap.GetCount(); i++)
	{
		ndUnsigned64 gridHash = m_hashGridMap[i].m_gridHash;
		if (gridHash != gridHash0)
		{
			gridScans.PushBack(count);
			count = 0;
			gridHash0 = gridHash;
		}
		count++;
	}
	gridScans.PushBack(count);

	ndInt32 acc = 0;
	for (ndInt32 i = 0; i < gridScans.GetCount(); i++)
	{
		ndInt32 sum = gridScans[i];
		gridScans[i] = acc;
		acc += sum;
	}
	gridScans.PushBack(acc);
}

void ndBodySphFluid::CalculateScans(const ndWorld* const world)
{
	D_TRACKTIME();

	class ndCalculateScans : public ndScene::ndBaseJob
	{
		public:
		class ndContext
		{
			public:
			ndContext(ndBodySphFluid* const fluid, const ndWorld* const world)
				:m_fluid(fluid)
			{
				memset(m_scan, 0, sizeof(m_scan));

				const ndInt32 threadCount = world->GetThreadCount();
				ndInt32 particleCount = m_fluid->m_hashGridMap.GetCount();

				ndInt32 acc0 = 0;
				ndInt32 stride = particleCount / threadCount;
				const ndGridHash* const hashGridMap = &m_fluid->m_hashGridMap[0];
				for (ndInt32 threadIndex = 0; threadIndex < threadCount; threadIndex++)
				{
					m_scan[threadIndex] = acc0;
					acc0 += stride;
					while (acc0 < particleCount && (hashGridMap[acc0].m_gridHash == hashGridMap[acc0 - 1].m_gridHash))
					{
						acc0++;
					}
				}
				m_scan[threadCount] = particleCount;
			}

			ndBodySphFluid* m_fluid;
			ndInt32 m_scan[D_MAX_THREADS_COUNT + 1];
		};

		virtual void Execute()
		{
			D_TRACKTIME();
			//ndWorld* const world = m_owner->GetWorld();
			ndContext* const context = (ndContext*)m_context;
			//const ndInt32 threadCount = world->GetThreadCount();
		
			const ndInt32 threadIndex = GetThreadId();
			ndBodySphFluid* const fluid = context->m_fluid;
			const ndGridHash* const hashGridMap = &context->m_fluid->m_hashGridMap[0];
				
			const ndInt32 start = context->m_scan[threadIndex];
			const ndInt32 strideCount = context->m_scan[threadIndex + 1] - start;
			ndArray<ndInt32>& gridScans = fluid->m_gridScans[threadIndex];
			ndUnsigned64 gridHash0 = hashGridMap[start].m_gridHash;

			ndInt32 count = 0;
			gridScans.SetCount(0);
			for (ndInt32 i = 0; i < strideCount; i++)
			{
				ndUnsigned64 gridHash = hashGridMap[start + i].m_gridHash;
				if (gridHash != gridHash0)
				{
					gridScans.PushBack(count);
					count = 0;
					gridHash0 = gridHash;
				}
				count++;
			}
			gridScans.PushBack(count);
		}
	};

	ndCalculateScans::ndContext context(this, world);
	ndScene* const scene = world->GetScene();
	scene->SubmitJobs<ndCalculateScans>(&context);

	ndInt32 acc = 0;
	ndArray<ndInt32>& gridScans = m_gridScans[0];
	for (ndInt32 i = 0; i < gridScans.GetCount(); i++)
	{
		ndInt32 sum = gridScans[i];
		gridScans[i] = acc;
		acc += sum;
	}

	ndInt32 threadCount = scene->GetThreadCount();
	for (ndInt32 threadIndex = 1; threadIndex < threadCount; threadIndex++)
	{
		ndArray<ndInt32>& dstGridScans = m_gridScans[0];
		const ndArray<ndInt32>& srcGridScans = m_gridScans[threadIndex];
		const ndInt32 base = dstGridScans.GetCount();
		dstGridScans.SetCount(base + srcGridScans.GetCount());

		ndInt32* const dst = &dstGridScans[base];
		for (ndInt32 i = 0; i < srcGridScans.GetCount(); i++)
		{
			ndInt32 sum = srcGridScans[i];
			dst[i] = acc;
			acc += sum;
		}
	}
	gridScans.PushBack(acc);

	#ifdef _DEBUG
	CalculateScansDebug(m_gridScans[1]);
	dAssert(m_gridScans[1].GetCount() == m_gridScans[0].GetCount());
	for (ndInt32 i = 0; i < m_gridScans[0].GetCount(); i++)
	{
		dAssert(m_gridScans[1][i] == m_gridScans[0][i]);
	}
	#endif
}

void ndBodySphFluid::CreateGrids(const ndWorld* const world)
{
	D_TRACKTIME();
	class ndCreateGrids : public ndScene::ndBaseJob
	{
		public:
		class ndContext
		{
			public:
			ndContext(ndBodySphFluid* const fluid)
				:m_fluid(fluid)
				, m_lock()
			{
			}

			ndBodySphFluid* m_fluid;
			ndSpinLock m_lock;
		};

		// size of the level one cache minus few values for local variables.
		#define D_SCRATCH_BUFFER_SIZE		(1024 * 24 / sizeof (ndGridHash))
		#define D_SCRATCH_BUFFER_SIZE_PADD	(32)

		class ndHashCacheBuffer : public ndFixSizeArray<ndGridHash, D_SCRATCH_BUFFER_SIZE + D_SCRATCH_BUFFER_SIZE_PADD>
		{
			public:
			ndHashCacheBuffer()
				:ndFixSizeArray<ndGridHash, D_SCRATCH_BUFFER_SIZE + D_SCRATCH_BUFFER_SIZE_PADD>()
				, m_size(0)
			{
				// check the local scratch buffer is smaller than level one cache
				dAssert(GetCapacity() * sizeof(ndGridHash) < 32 * 1024);
			}

			void PushBack(const ndGridHash& element)
			{
				ndInt32 index = m_size;
				m_size++;
				(*this)[index] = element;
			}

			void AddCell(ndInt32 count, const ndGridHash& origin, const ndGridHash& cell, const ndGridHash* const neigborgh)
			{
				#ifdef _DEBUG 
				ndInt32 debugCheck = 0;
				#endif

				for (ndInt32 j = 0; j < count; j++)
				{
					ndGridHash quadrand(cell);
					quadrand.m_gridHash += neigborgh[j].m_gridHash;
					quadrand.m_cellType = ndGridType(quadrand.m_gridHash == origin.m_gridHash);
					dAssert(quadrand.m_cellType == ((quadrand.m_gridHash == origin.m_gridHash) ? ndHomeGrid : ndAdjacentGrid));

					PushBack(quadrand);
					#ifdef _DEBUG 
					debugCheck += quadrand.m_cellType ? 1 : 0;
					#endif
				}
				dAssert(debugCheck == 1);
			}

			ndInt32 m_size;
		};

		virtual void Execute()
		{
			D_TRACKTIME();

			ndVector m_neighborkDirs[8];

			ndBodySphFluid* const fluid = ((ndContext*)m_context)->m_fluid;
			const ndFloat32 radius = fluid->m_radius;

			//onst ndInt32 threadIndex = GetThreadId();
			//onst ndInt32 threadCount = m_owner->GetThreadCount();
			//onst ndInt32 particleCount = fluid->m_posit.GetCount();
			//
			//onst ndInt32 step = particleCount / threadCount;
			//onst ndInt32 start = threadIndex * step;
			//onst ndInt32 count = ((threadIndex + 1) < threadCount) ? step : particleCount - start;

			const ndFloat32 gridSize = fluid->CalculateGridSize();

			const ndVector origin(fluid->m_box0);
			const ndVector invGridSize(ndFloat32(1.0f) / gridSize);
			const ndVector* const posit = &fluid->m_posit[0];

			const ndVector box0(-radius, -radius, -radius, ndFloat32(0.0f));
			const ndVector box1(radius, radius, radius, ndFloat32(0.0f));

			ndGridHash stepsCode_xyz[8];
			stepsCode_xyz[0] = ndGridHash(0, 0, 0);
			stepsCode_xyz[1] = ndGridHash(1, 0, 0);
			stepsCode_xyz[2] = ndGridHash(0, 1, 0);
			stepsCode_xyz[3] = ndGridHash(1, 1, 0);
			stepsCode_xyz[4] = ndGridHash(0, 0, 1);
			stepsCode_xyz[5] = ndGridHash(1, 0, 1);
			stepsCode_xyz[6] = ndGridHash(0, 1, 1);
			stepsCode_xyz[7] = ndGridHash(1, 1, 1);

			ndGridHash stepsCode_xy[4];
			stepsCode_xy[0] = ndGridHash(0, 0, 0);
			stepsCode_xy[1] = ndGridHash(1, 0, 0);
			stepsCode_xy[2] = ndGridHash(0, 1, 0);
			stepsCode_xy[3] = ndGridHash(1, 1, 0);

			ndGridHash stepsCode_yz[4];
			stepsCode_yz[0] = ndGridHash(0, 0, 0);
			stepsCode_yz[1] = ndGridHash(0, 1, 0);
			stepsCode_yz[2] = ndGridHash(0, 0, 1);
			stepsCode_yz[3] = ndGridHash(0, 1, 1);

			ndGridHash stepsCode_xz[4];
			stepsCode_xz[0] = ndGridHash(0, 0, 0);
			stepsCode_xz[1] = ndGridHash(1, 0, 0);
			stepsCode_xz[2] = ndGridHash(0, 0, 1);
			stepsCode_xz[3] = ndGridHash(1, 0, 1);

			ndGridHash stepsCode_x[2];
			stepsCode_x[0] = ndGridHash(0, 0, 0);
			stepsCode_x[1] = ndGridHash(1, 0, 0);

			ndGridHash stepsCode_y[2];
			stepsCode_y[0] = ndGridHash(0, 0, 0);
			stepsCode_y[1] = ndGridHash(0, 1, 0);

			ndGridHash stepsCode_z[2];
			stepsCode_z[0] = ndGridHash(0, 0, 0);
			stepsCode_z[1] = ndGridHash(0, 0, 1);

			ndHashCacheBuffer bufferOut;
			//for (ndInt32 i = 0; i < count; i++)
			const ndStartEnd startEnd(fluid->m_posit.GetCount(), GetThreadId(), m_owner->GetThreadCount());
			for (ndInt32 i = startEnd.m_start; i < startEnd.m_end; ++i)
			{
				ndVector r(posit[i] - origin);
				ndVector p(r * invGridSize);
				ndGridHash hashKey(p, i);

				fluid->m_upperDigisIsValid[0] |= hashKey.m_xHigh;
				fluid->m_upperDigisIsValid[1] |= hashKey.m_yHigh;
				fluid->m_upperDigisIsValid[2] |= hashKey.m_zHigh;
				ndVector p0((r + box0) * invGridSize);
				ndVector p1((r + box1) * invGridSize);

				ndGridHash box0Hash(p0, i);
				ndGridHash box1Hash(p1, i);
				ndGridHash codeHash(box1Hash.m_gridHash - box0Hash.m_gridHash);

				dAssert(codeHash.m_x <= 1);
				dAssert(codeHash.m_y <= 1);
				dAssert(codeHash.m_z <= 1);
				ndUnsigned32 code = ndUnsigned32(codeHash.m_z * 4 + codeHash.m_y * 2 + codeHash.m_x);

				switch (code)
				{
					case 0:
					{
						box0Hash.m_cellType = ndHomeGrid;
						bufferOut.PushBack(box0Hash);
						break;
					}

					case 1:
					{
						// this grid goes across all cell.
						bufferOut.AddCell(2, hashKey, box0Hash, stepsCode_x);
						break;
					}

					case 2:
					{
						// this grid goes across all cell.
						bufferOut.AddCell(2, hashKey, box0Hash, stepsCode_y);
						break;
					}

					case 4:
					{
						// this grid goes across all cell.
						bufferOut.AddCell(2, hashKey, box0Hash, stepsCode_z);
						break;
					}


					case 3:
					{
						// this grid goes across all cell.
						bufferOut.AddCell(4, hashKey, box0Hash, stepsCode_xy);
						break;
					}

					case 5:
					{
						// this grid goes across all cell.
						bufferOut.AddCell(4, hashKey, box0Hash, stepsCode_xz);
						break;
					}

					case 6:
					{
						// this grid goes across all cell.
						bufferOut.AddCell(4, hashKey, box0Hash, stepsCode_yz);
						break;
					}

					case 7:
					{
						// this grid goes across all cell.
						bufferOut.AddCell(8, hashKey, box0Hash, stepsCode_xyz);
						break;
					}

					default:
						dAssert(0);
				}

				if (bufferOut.m_size > ndInt32 (D_SCRATCH_BUFFER_SIZE))
				{
					D_TRACKTIME();
					ndScopeSpinLock criticalLock(((ndContext*)m_context)->m_lock);
					ndInt32 dstIndex = fluid->m_hashGridMap.GetCount();
					fluid->m_hashGridMap.SetCount(dstIndex + bufferOut.m_size);
					memcpy(&fluid->m_hashGridMap[dstIndex], &bufferOut[0], bufferOut.m_size * sizeof(ndGridHash));
					bufferOut.m_size = 0;
				}
			}

			if (bufferOut.m_size)
			{
				D_TRACKTIME();
				ndScopeSpinLock criticalLock(((ndContext*)m_context)->m_lock);
				ndInt32 dstIndex = fluid->m_hashGridMap.GetCount();
				fluid->m_hashGridMap.SetCount(dstIndex + bufferOut.m_size);
				memcpy(&fluid->m_hashGridMap[dstIndex], &bufferOut[0], bufferOut.m_size * sizeof(ndGridHash));
			}
		}
	};

	ndScene* const scene = world->GetScene();
	m_upperDigisIsValid[0] = 0;
	m_upperDigisIsValid[1] = 0;
	m_upperDigisIsValid[2] = 0;

	ndCreateGrids::ndContext context(this);
	m_hashGridMap.SetCount(0);
	scene->SubmitJobs<ndCreateGrids>(&context);
}

void ndBodySphFluid::SortGrids(const ndWorld* const world)
{
	D_TRACKTIME();
	const ndInt32 threadCount = world->GetThreadCount();
	m_hashGridMapScratchBuffer.SetCount(m_hashGridMap.GetCount());

	SortByCenterType();
	if (threadCount <= 1)
	{
		dAssert(threadCount == 1);
		SortSingleThreaded();
	}
	else
	{
		SortParallel(world);
	}

	#ifdef _DEBUG
	for (ndInt32 i = 0; i < (m_hashGridMap.GetCount() - 1); i++)
	{
		const ndGridHash& entry0 = m_hashGridMap[i + 0];
		const ndGridHash& entry1 = m_hashGridMap[i + 1];
		ndUnsigned64 gridHashA = entry0.m_gridHash;
		ndUnsigned64 gridHashB = entry1.m_gridHash;
		dAssert(gridHashA <= gridHashB);
	}
	#endif
}

void ndBodySphFluid::BuildPairs(const ndWorld* const world)
{
	D_TRACKTIME();
	class ndBodySphFluidCreatePair : public ndScene::ndBaseJob
	{
		public:
		class ndContext
		{
			public:
			ndContext(ndBodySphFluid* const fluid)
				:m_fluid(fluid)
				, m_lock()
			{
			}

			ndBodySphFluid* m_fluid;
			ndSpinLock m_lock;
		};

		#define D_SCRATCH_PAIR_BUFFER_SIZE		(1024 * 24 / sizeof (ndParticlePair))
		#define D_SCRATCH_PAIR_BUFFER_SIZE_PADD (256)

		class ndParticlePairCacheBuffer : public ndFixSizeArray<ndParticlePair, D_SCRATCH_PAIR_BUFFER_SIZE + D_SCRATCH_PAIR_BUFFER_SIZE_PADD>
		{
			public:
			ndParticlePairCacheBuffer()
				:ndFixSizeArray<ndParticlePair, D_SCRATCH_PAIR_BUFFER_SIZE + D_SCRATCH_PAIR_BUFFER_SIZE_PADD>()
				, m_size(0)
			{
				// check the local scratch buffer is smaller than level one cache
				dAssert(GetCapacity() * sizeof(ndParticlePair) < 32 * 1024);
			}

			void PushBack(ndInt32 m0, ndInt32 m1)
			{
				dAssert(0);
				ndInt32 index = m_size;
				m_size++;
				dAssert(m_size < GetCapacity());
				ndParticlePair& pair = (*this)[index];
				pair.m_m0 = m0;
				pair.m_m1 = m1;
			}
			ndInt32 m_size;
		};

		virtual void Execute()
		{
			D_TRACKTIME();
			ndWorld* const world = m_owner->GetWorld();
			ndBodySphFluid* const fluid = ((ndContext*)m_context)->m_fluid;
			const ndInt32 threadId = GetThreadId();
			const ndInt32 threadCount = world->GetThreadCount();
			
			const ndArray<ndInt32>& gridCounts = fluid->m_gridScans[0];
			const ndInt32 count = gridCounts.GetCount() - 1;
			const ndInt32 size = count / threadCount;
			const ndInt32 start = threadId * size;
			const ndInt32 batchSize = (threadId == threadCount - 1) ? count - start : size;
			const ndGridHash* const srcArray = &fluid->m_hashGridMap[0];

			//const ndVector* const positions = &fluid->m_posit[0];
			//const ndFloat32 diameter = ndFloat32(2.0f) * fluid->m_radius;
			//const ndFloat32 diameter2 = diameter * diameter;
			
			ndParticlePairCacheBuffer buffer;
			for (ndInt32 i = 0; i < batchSize; i++)
			{
				const ndInt32 cellStart = gridCounts[i + start];
				const ndInt32 cellCount = gridCounts[i + start + 1] - cellStart;
			
				const ndGridHash* const ptr = &srcArray[cellStart];
				for (ndInt32 j = cellCount - 1; j > 0; j--)
				{
					const ndGridHash& cell0 = ptr[j];
					if (cell0.m_cellType == ndHomeGrid)
					{
						const ndInt32 m0 = cell0.m_particleIndex;
						//const ndVector& posit0 = positions[m0];
			
						for (ndInt32 k = j - 1; k >= 0; k--)
						{
							const ndGridHash& cell1 = ptr[k];
							const ndInt32 m1 = cell1.m_particleIndex;
							bool test = (cell1.m_cellType == ndHomeGrid);
							dAssert(0);
							//const ndVector& posit1 = positions[m1];
							//const ndVector dist(posit1 - posit0);
							//ndFloat32 dist2 = dist.DotProduct(dist).GetScalar();
							//test = test | (cell0.m_particleIndex <= cell1.m_gridHash);
							//test = test & (dist2 <= diameter2);
							if (test)
							{
								buffer.PushBack(m0, m1);
							}
						}
					}
				}
			
				if (buffer.m_size > ndInt32 (D_SCRATCH_PAIR_BUFFER_SIZE))
				{
					ndScopeSpinLock criticalLock(((ndContext*)m_context)->m_lock);
					ndInt32 dstIndex = fluid->m_particlesPairs.GetCount();
					fluid->m_particlesPairs.SetCount(dstIndex + buffer.m_size);
					memcpy(&fluid->m_particlesPairs[dstIndex], &buffer[0], buffer.m_size * sizeof(ndParticlePair));
					buffer.m_size = 0;
				}
			}
			
			if (buffer.m_size)
			{
				D_TRACKTIME();
				ndScopeSpinLock criticalLock(((ndContext*)m_context)->m_lock);
				ndInt32 dstIndex = fluid->m_particlesPairs.GetCount();
				fluid->m_particlesPairs.SetCount(dstIndex + buffer.m_size);
				memcpy(&fluid->m_particlesPairs[dstIndex], &buffer[0], buffer.m_size * sizeof(ndParticlePair));
			}
		}
	};

	CalculateScans(world);
	ndScene* const scene = world->GetScene();
	m_particlesPairs.SetCount(0);
	ndBodySphFluidCreatePair::ndContext context(this);
	scene->SubmitJobs<ndBodySphFluidCreatePair>(&context);
}

void ndBodySphFluid::CalculateAccelerations(const ndWorld* const world)
{
	D_TRACKTIME();

	class ndCalculateDensity: public ndScene::ndBaseJob
	{
		public:
		class ndContext
		{
			public:
			ndContext(ndBodySphFluid* const fluid)
				:m_fluid(fluid)
			{
			}

			ndBodySphFluid* m_fluid;
		};


		virtual void Execute()
		{
			D_TRACKTIME();
			ndWorld* const world = m_owner->GetWorld();
			ndBodySphFluid* const fluid = ((ndContext*)m_context)->m_fluid;
			const ndInt32 threadId = GetThreadId();
			const ndInt32 threadCount = world->GetThreadCount();
			

			ndArray<ndParticlePair>& particlesPairs = fluid->m_particlesPairs;
			const ndInt32 count = particlesPairs.GetCount();
			const ndInt32 stride = count / threadCount;
			const ndInt32 start = threadId * stride;
			const ndInt32 batchStride = (threadId == threadCount - 1) ? count - start : stride;

			for (ndInt32 i = 0; i < batchStride; i++)
			{
				//ndParticlePair& pair = particlesPairs[i];
			}
		}
	};

	ndScene* const scene = world->GetScene();
	//m_particlesPairs.SetCount(0);
	//ndBodySphFluidCreatePair::ndContext context(this);

	ndCalculateDensity::ndContext densityContext(this);
	scene->SubmitJobs<ndCalculateDensity>(&densityContext);
}