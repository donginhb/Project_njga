#ifndef __AUTOLOCK__H
#define __AUTOLOCK__H
// wrapper for whatever critical section we have
class CCritSec {

    // make copy constructor and assignment operator inaccessible

    CCritSec(const CCritSec &refCritSec);
    CCritSec &operator=(const CCritSec &refCritSec);

    CRITICAL_SECTION m_CritSec;
	
public:
    CCritSec() 
	{
        InitializeCriticalSection(&m_CritSec);
    };

    ~CCritSec() 
	{
        DeleteCriticalSection(&m_CritSec);
    };

    void Lock() 
	{
        EnterCriticalSection(&m_CritSec);
    };

    void Unlock() 
	{
        LeaveCriticalSection(&m_CritSec);
    };

private:


};

//
// To make deadlocks easier to track it is useful to insert in the
// code an assertion that says whether we own a critical section or
// not.  We make the routines that do the checking globals to avoid
// having different numbers of member functions in the debug and
// retail class implementations of CCritSec.  In addition we provide
// a routine that allows usage of specific critical sections to be
// traced.  This is NOT on by default - there are far too many.
//


    #define CritCheckIn(x) TRUE
    #define CritCheckOut(x) TRUE
    #define DbgLockTrace(pc, fT)

// locks a critical section, and unlocks it automatically
// when the lock goes out of scope
class CAutoLock {

    // make copy constructor and assignment operator inaccessible

    CAutoLock(const CAutoLock &refAutoLock);
    CAutoLock &operator=(const CAutoLock &refAutoLock);

protected:
    CCritSec * m_pLock;

public:
    CAutoLock(CCritSec * plock)
    {
        m_pLock = plock;
		try
		{
			//AfxMessageBox("11");
			if(m_pLock)
        m_pLock->Lock();
		}
		catch (...)
		{
			//AfxMessageBox("1");
			
		}
		
        
#ifdef _DEBUG
//		TRACE("%s Lock %d \n",m_pLock->GetName(),m_pLock->GetRef());
#endif
    };

    ~CAutoLock() 
	{
		try
		{
			//AfxMessageBox("11");
			if(m_pLock)
        m_pLock->Unlock();
		}
		catch (...)
		{
		}
#ifdef _DEBUG
//		TRACE("%s UnLock %d\n",m_pLock->GetName(),m_pLock->GetRef());
#endif
    };
};
#endif

