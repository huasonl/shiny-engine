/* 
 * File:   XqxTableIndexS.h
 * Author: Jehu Shaw
 * 
 * Created on 2017_7_5, 23:15
 */
#ifndef __XQXTABLEINDEXS_H__
#define __XQXTABLEINDEXS_H__

#include <assert.h>
#include <stdexcept>
#include "SpinRWLock.h"
#include "ScopedRWLock.h"

namespace util {

#define XQXTABLEINDEXS_ALLOCATE_SIZE 8
#define XQXTABLEINDEXS_INDEX_NIL (size_t)-1

class CXqxTableIndexS
{
	class CXqxTableIterator {
	public:
		CXqxTableIterator(const CXqxTableIndexS* pXqxTable, size_t nIndex)
			: m_pXqxTable(pXqxTable)
			, m_nIndex(nIndex)
			, m_bEnd(false)
		{
			assert(m_pXqxTable);
			if(pXqxTable && pXqxTable->m_nCurSize == nIndex) {
				m_bEnd = true;
			}
		}

		bool operator==(const CXqxTableIterator& x) const {
			thd::CScopedReadLock rrLock(m_pXqxTable->m_rwLock);
			thd::CScopedReadLock rlLock(x.m_pXqxTable->m_rwLock);

			if(m_pXqxTable != x.m_pXqxTable) {
				throw std::logic_error("The table is not the same.");
			}
			if(m_bEnd && m_pXqxTable->m_nCurSize != m_nIndex) {
				throw std::logic_error("The end iterator of this is invalid !");
			}
			if(x.m_bEnd && x.m_pXqxTable->m_nCurSize != x.m_nIndex) {
				throw std::logic_error("The end iterator of x is invalid !");
			}
			return m_nIndex == x.m_nIndex;
		}

		bool operator!=(const CXqxTableIterator& x) const {
			thd::CScopedReadLock rrLock(m_pXqxTable->m_rwLock);
			thd::CScopedReadLock rlLock(x.m_pXqxTable->m_rwLock);

			if(m_pXqxTable != x.m_pXqxTable) {
				throw std::logic_error("The table is not the same.");
			}
			if(m_bEnd && m_pXqxTable->m_nCurSize != m_nIndex) {
				throw std::logic_error("The end iterator of this is invalid !");
			}
			if(x.m_bEnd && x.m_pXqxTable->m_nCurSize != x.m_nIndex) {
				throw std::logic_error("The end iterator of x is invalid !");
			}
			return m_nIndex != x.m_nIndex;
		}

		CXqxTableIterator& operator++() {
			thd::CScopedReadLock rLock(m_pXqxTable->m_rwLock);
			IncreaseIndex();
			return *this;
		}

		CXqxTableIterator operator++(int) {
			thd::CScopedReadLock rLock(m_pXqxTable->m_rwLock);
			CXqxTableIterator tmp(*this);
			IncreaseIndex();
			return tmp;
		}

		CXqxTableIterator& operator--() {
			thd::CScopedReadLock rLock(m_pXqxTable->m_rwLock);
			DecreaseIndex();
			return *this;
		}

		CXqxTableIterator operator--(int) {
			thd::CScopedReadLock rLock(m_pXqxTable->m_rwLock);
			CXqxTableIterator tmp(*this);
			DecreaseIndex();
			return tmp;
		}

		size_t GetIndex() const {
			thd::CScopedReadLock rLock(m_pXqxTable->m_rwLock);
			return m_pXqxTable->m_arrItems[m_nIndex];
		}

	private:
		void IncreaseIndex() {
			if(sizeof(m_nIndex) == sizeof(unsigned long)) {
				size_t nIndex;
				do {
					nIndex = (size_t)m_nIndex;
					if(nIndex >= m_pXqxTable->m_nCurSize) {
						atomic_xchg8(&m_bEnd, true);
						atomic_xchg(&m_nIndex, m_pXqxTable->m_nCurSize);
						return;
					}
				} while (atomic_cmpxchg(&m_nIndex, nIndex + 1, nIndex) != nIndex);
				if((nIndex + 1) == m_pXqxTable->m_nCurSize) {
					atomic_xchg8(&m_bEnd, true);
				}
			} else if(sizeof(m_nIndex) == sizeof(uint64_t)) {
				size_t nIndex;
				do {
					nIndex = (size_t)m_nIndex;
					if(nIndex >= m_pXqxTable->m_nCurSize) {
						atomic_xchg8(&m_bEnd, true);
						atomic_xchg64(&m_nIndex, m_pXqxTable->m_nCurSize);
						return;
					}
				} while (atomic_cmpxchg64(&m_nIndex, nIndex + 1, nIndex) != nIndex);
				if((nIndex + 1) == m_pXqxTable->m_nCurSize) {
					atomic_xchg8(&m_bEnd, true);
				}
			} else if(sizeof(m_nIndex) == sizeof(uint16_t)) {
				size_t nIndex;
				do {
					nIndex = (size_t)m_nIndex;
					if(nIndex >= m_pXqxTable->m_nCurSize) {
						atomic_xchg8(&m_bEnd, true);
						atomic_xchg16(&m_nIndex, m_pXqxTable->m_nCurSize);
						return;
					}
				} while (atomic_cmpxchg16(&m_nIndex, nIndex + 1, nIndex) != nIndex);
				if((nIndex + 1) == m_pXqxTable->m_nCurSize) {
					atomic_xchg8(&m_bEnd, true);
				}
			} else if(sizeof(m_nIndex) == sizeof(uint8_t)) {
				size_t nIndex;
				do {
					nIndex = (size_t)m_nIndex;
					if(nIndex >= m_pXqxTable->m_nCurSize) {
						atomic_xchg8(&m_bEnd, true);
						atomic_xchg8(&m_nIndex, m_pXqxTable->m_nCurSize);
						return;
					}
				} while (atomic_cmpxchg8(&m_nIndex, nIndex + 1, nIndex) != nIndex);
				if((nIndex + 1) == m_pXqxTable->m_nCurSize) {
					atomic_xchg8(&m_bEnd, true);
				}
			} else {
				throw std::logic_error("No implement");
			}
		}

		void DecreaseIndex() {
			if(sizeof(m_nIndex) == sizeof(unsigned long)) {
				size_t nIndex;
				do {
					nIndex = (size_t)m_nIndex;
					if(0 == nIndex) {
						atomic_xchg8(&m_bEnd, false);
						return;
					}
				} while (atomic_cmpxchg(&m_nIndex, nIndex - 1, nIndex) != nIndex);
				if(nIndex == m_pXqxTable->m_nCurSize) {
					atomic_xchg8(&m_bEnd, false);
				}
			} else if(sizeof(m_nIndex) == sizeof(uint64_t)) {
				size_t nIndex;
				do {
					nIndex = (size_t)m_nIndex;
					if(0 == nIndex) {
						atomic_xchg8(&m_bEnd, false);
						return;
					}
				} while (atomic_cmpxchg64(&m_nIndex, nIndex - 1, nIndex) != nIndex);
				if(nIndex == m_pXqxTable->m_nCurSize) {
					atomic_xchg8(&m_bEnd, false);
				}
			} else if(sizeof(m_nIndex) == sizeof(uint16_t)) {
				size_t nIndex;
				do {
					nIndex = (size_t)m_nIndex;
					if(0 == nIndex) {
						atomic_xchg8(&m_bEnd, false);
						return;
					}
				} while (atomic_cmpxchg16(&m_nIndex, nIndex - 1, nIndex) != nIndex);
				if(nIndex == m_pXqxTable->m_nCurSize) {
					atomic_xchg8(&m_bEnd, false);
				}
			} else if(sizeof(m_nIndex) == sizeof(uint8_t)) {
				size_t nIndex;
				do {
					nIndex = (size_t)m_nIndex;
					if(0 == nIndex) {
						atomic_xchg8(&m_bEnd, false);
						return;
					}
				} while (atomic_cmpxchg8(&m_nIndex, nIndex - 1, nIndex) != nIndex);
				if(nIndex == m_pXqxTable->m_nCurSize) {
					atomic_xchg8(&m_bEnd, false);
				}
			} else {
				throw std::logic_error("No implement");
			}
		}

	private:
		const CXqxTableIndexS* m_pXqxTable;
		size_t m_nIndex;
		bool m_bEnd;
	};

public:
	typedef CXqxTableIterator iterator;

public:
	CXqxTableIndexS(size_t nAllcSize = 0)
		: m_arrItems(NULL)
		, m_nAllcSize(0)
		, m_nCurSize(0)
	{
		if(0 == nAllcSize) {
			nAllcSize = XQXTABLEINDEXS_ALLOCATE_SIZE;
		}
		thd::CScopedWriteLock wLock(m_rwLock);
		AllcoTable(nAllcSize);
	}

	CXqxTableIndexS(const CXqxTableIndexS& orig)
	{
		if(this == &orig) {
			return;
		}
		thd::CScopedReadLock rLock(orig.m_rwLock);
		thd::CScopedWriteLock wLock(m_rwLock);

		m_arrItems = NULL;
		m_nAllcSize = 0;
		m_nCurSize = 0;

		if(0 == orig.m_nAllcSize) {
			AllcoTable(XQXTABLEINDEXS_ALLOCATE_SIZE);
		} else {
			AllcoTable(orig.m_nAllcSize);
		}

		if(NULL != m_arrItems) {
			if(orig.m_nCurSize > m_nAllcSize) {
				assert(false);
			} else {
				for(size_t i = 0; i < orig.m_nCurSize; ++i) {
					size_t nCurIdx = orig.m_arrItems[i];
					if(nCurIdx >= m_nAllcSize) {
						assert(false);
						continue;
					}

					if(nCurIdx != i) {
						m_arrItems[i] = nCurIdx;
						m_arrItems[nCurIdx] = i;
					} else {
						m_arrItems[i] = i;
					}	
				}
				m_nCurSize = orig.m_nCurSize;
			}
		}
	}

	~CXqxTableIndexS()
	{
		thd::CScopedWriteLock wLock(m_rwLock);
		DestoryTable();
	}

	size_t Add()
	{
		thd::CScopedWriteLock wLock(m_rwLock);
		AllcoTable(m_nCurSize + 1);
		size_t nSubIdx = m_nCurSize;
		if(nSubIdx >= m_nAllcSize) {
			return XQXTABLEINDEXS_INDEX_NIL;
		}
		size_t nCurIdx = nSubIdx;
		if(m_nCurSize > 0) {
			size_t nPreIdx = m_arrItems[nCurIdx];
			if(nCurIdx > nPreIdx) {
				assert(nCurIdx == m_arrItems[nPreIdx]);
				nCurIdx = nPreIdx;
				m_arrItems[nSubIdx] = nSubIdx;
			}
		}
		m_arrItems[nCurIdx] = nCurIdx;

		++m_nCurSize;
		return nCurIdx;
	}

	bool Remove(size_t nIndex)
	{
		thd::CScopedWriteLock wLock(m_rwLock);
		if(nIndex >= m_nAllcSize) {
			assert(false);
			return false;
		}

		size_t nCurIdx = nIndex;
		size_t nSubIdx = m_arrItems[nCurIdx];
		size_t nLastIdx = m_nCurSize - 1;
		if(nSubIdx > nLastIdx) {
			return false;
		}

		if(nCurIdx > nLastIdx) {
			m_arrItems[nCurIdx] = nCurIdx;
			m_arrItems[nSubIdx] = nSubIdx;
			nCurIdx = nSubIdx;	
		}

		size_t nSubIdx2 = m_arrItems[nLastIdx];	
		if(nSubIdx2 != nLastIdx) {
			m_arrItems[nLastIdx] = nLastIdx;
			m_arrItems[nSubIdx2] = nCurIdx;
			m_arrItems[nCurIdx] = nSubIdx2;
		} else if(nCurIdx != nLastIdx) {
			m_arrItems[nCurIdx] = nLastIdx;
			m_arrItems[nLastIdx] = nCurIdx;
		}
		--m_nCurSize;
		return true;
	}

	void Clear() 
	{
		thd::CScopedWriteLock wLock(m_rwLock);
		size_t nSize = m_nCurSize;
		m_nCurSize = 0;
		for(size_t i = 0; i < nSize; ++i) {
			size_t nCurIdx = m_arrItems[i];
			if(nCurIdx >= m_nAllcSize) {
				assert(false);
				continue;
			}
			if(nCurIdx != i) {
				m_arrItems[i] = i;
				m_arrItems[nCurIdx] = nCurIdx;
			}
		}
	}

	size_t Find(size_t nIndex) const 
	{
		thd::CScopedReadLock rLock(m_rwLock);
		if(nIndex >= m_nAllcSize) {
			return XQXTABLEINDEXS_INDEX_NIL;
		}
		if(NULL == m_arrItems) {
			assert(false);
			return XQXTABLEINDEXS_INDEX_NIL;
		}
		return m_arrItems[nIndex];
	}

	size_t Size() const {
		thd::CScopedReadLock rLock(m_rwLock);
		return m_nCurSize;
	}

	bool Empty() const {
		thd::CScopedReadLock rLock(m_rwLock);
		return m_nCurSize == 0;
	}

	iterator Begin() {
		thd::CScopedReadLock rLock(m_rwLock);
		return iterator(this, 0);
	}

	iterator End() {
		thd::CScopedReadLock rLock(m_rwLock);
		return iterator(this, m_nCurSize);
	}

	CXqxTableIndexS& operator = (const CXqxTableIndexS& right) {
		if(this == &right) {
			return *this;
		}
		thd::CScopedReadLock rLock(right.m_rwLock);
		thd::CScopedWriteLock wLock(m_rwLock);

		DestoryTable();

		if(0 == right.m_nAllcSize) {
			AllcoTable(XQXTABLEINDEXS_ALLOCATE_SIZE);
		} else {
			AllcoTable(right.m_nAllcSize);
		}

		if(NULL != m_arrItems) {
			if(right.m_nCurSize > m_nAllcSize) {
				assert(false);
			} else {
				for(size_t i = 0; i < right.m_nCurSize; ++i) {
					size_t nCurIdx = right.m_arrItems[i];
					if(nCurIdx >= m_nAllcSize) {
						assert(false);
						continue;
					}

					if(nCurIdx != i) {
						m_arrItems[i] = nCurIdx;
						m_arrItems[nCurIdx] = i;
					} else {
						m_arrItems[i] = i;
					}	
				}
				m_nCurSize = right.m_nCurSize;
			}
		}
		return *this;
	}

private:
	void AllcoTable(size_t nNeedSize) {
		if(NULL == m_arrItems) {
			size_t nByteSize = sizeof(size_t) * nNeedSize;
			m_arrItems = (size_t*)malloc(nByteSize);
			m_nAllcSize = nNeedSize;
			if(NULL != m_arrItems) {
				for(size_t i = 0; i < m_nAllcSize; ++i) {
					m_arrItems[i] = XQXTABLEINDEXS_INDEX_NIL;
				}
			} else {
				assert(m_arrItems);
			}
		} else {
			if(nNeedSize > m_nAllcSize) {
				size_t nOldSize = m_nAllcSize;
				size_t nNewSize = m_nAllcSize * 2;
				if(nNeedSize > nNewSize) {
					nNewSize = nNeedSize;
				}
				size_t nByteSize = sizeof(size_t) * nNewSize;
				size_t* pNew = (size_t*)realloc(m_arrItems, nByteSize);
				if(NULL == pNew) {
					if(nNeedSize < nNewSize) {
						nByteSize = sizeof(size_t) * nNeedSize;
						pNew = (size_t*)realloc(m_arrItems, nByteSize);
						if(NULL != pNew) {
							m_arrItems = pNew;
							m_nAllcSize = nNeedSize;
						}
					}
					assert(pNew);
				} else {
					m_arrItems = pNew;
					m_nAllcSize = nNewSize;
				}
				if(NULL != m_arrItems) {
					for(size_t i = nOldSize; i < m_nAllcSize; ++i) {
						m_arrItems[i] = XQXTABLEINDEXS_INDEX_NIL;
					}
				} else {
					assert(m_arrItems);
				}
			}
		}
	}

	void DestoryTable() {
		if(NULL != m_arrItems) {
			m_nCurSize = 0;
			free(m_arrItems);
			m_arrItems = NULL;
		}
		m_nAllcSize = 0;
	}

private:
	size_t* m_arrItems;
	size_t m_nAllcSize;
	size_t m_nCurSize;
	thd::CSpinRWLock m_rwLock;
};

}

#endif // __XQXTABLEINDEXS_H__