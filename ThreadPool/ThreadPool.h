/*
==========================================================================
* ��ThreadPool�Ǳ�����ĺ����࣬�����Զ�ά���̳߳صĴ�����������е�����

* ���е�TaskBase���Ƿ�װ��������
* ���е�TaskBase���Ƿ�װ��������ɺ�Ļص�������

*�÷�������һ��ThreadPool����������TaskBase���TaskBase�࣬�����������е�Run()������Ȼ�����ThreadPool��QueueTaskItem()��������

Author: TTGuoying

Date: 2018/02/09 19:55

==========================================================================
*/
#pragma once
#include <Windows.h>
#include <list>
#include <queue>
#include <memory>

using std::list;
using std::queue;
using std::shared_ptr;

#define THRESHOLE_OF_WAIT_TASK  20

class TaskBase
{
public:
	TaskBase() { wParam = NULL;  lParam = NULL; }
	TaskBase(WPARAM wParam, LPARAM lParam) { this->wParam = wParam; this->lParam = lParam;  }
	virtual ~TaskBase() { }

	WPARAM wParam;              // ����1
	LPARAM lParam;              // ����2
	virtual int Run() = 0;		// ������

};

class TaskCallbackBase
{
public:
	TaskCallbackBase() {}
	virtual ~TaskCallbackBase() {}

	virtual void Run(int result) = 0; // �ص�����

};

class ThreadPool
{
private:
	// �߳���(�ڲ���)
	class Thread
	{
	public:
		Thread(ThreadPool *threadPool);
		~Thread();

		BOOL isBusy();											// �Ƿ���������ִ��
		void ExecuteTask(shared_ptr<TaskBase> t, shared_ptr<TaskCallbackBase> tcb);		// ִ������

	private:
		ThreadPool *threadPool;									// �����̳߳�
		BOOL	busy;											// �Ƿ���������ִ��
		BOOL    exit;											// �Ƿ��˳�
		HANDLE  thread;											// �߳̾��
		shared_ptr<TaskBase>	task;							// Ҫִ�е�����
		shared_ptr<TaskCallbackBase> taskCb;					// �ص�������
		static unsigned int __stdcall ThreadProc(PVOID pM);		// �̺߳���
	};

	// IOCP��֪ͨ����
	enum WAIT_OPERATION_TYPE
	{
		GET_TASK,
		EXIT
	};

	// ��ִ�е�������
	class WaitTask
	{
	public:
		WaitTask(shared_ptr<TaskBase> task, shared_ptr<TaskCallbackBase> taskCb, BOOL bLong)
		{
			this->task = task;
			this->taskCb = taskCb;
			this->bLong = bLong;
		}
		~WaitTask() { task = NULL; taskCb = NULL; bLong = FALSE; }

		shared_ptr<TaskBase>	task;					// Ҫִ�е�����
		shared_ptr<TaskCallbackBase> taskCb;			// �ص�������
		BOOL bLong;										// �Ƿ�ʱ������
	};

	// �������б�ȡ������̺߳���
	static unsigned int __stdcall GetTaskThreadProc(PVOID pM)
	{
		ThreadPool *threadPool = (ThreadPool *)pM;
		BOOL bRet = FALSE;
		DWORD dwBytes = 0;
		WAIT_OPERATION_TYPE opType;
		OVERLAPPED *ol;
		while (WAIT_OBJECT_0 != WaitForSingleObject(threadPool->stopEvent, 0))
		{
			BOOL bRet = GetQueuedCompletionStatus(threadPool->completionPort, &dwBytes, (PULONG_PTR)&opType, &ol, INFINITE);
			// �յ��˳���־
			if (EXIT == (DWORD)opType)
			{
				break;
			}
			else if (GET_TASK == (DWORD)opType)
			{
				threadPool->GetTaskExcute();
			}
		}
		return 0;
	}

	//�߳��ٽ�����
	class CriticalSectionLock
	{
	private:
		CRITICAL_SECTION cs;//�ٽ���
	public:
		CriticalSectionLock() { InitializeCriticalSection(&cs); }
		~CriticalSectionLock() { DeleteCriticalSection(&cs); }
		void Lock() { EnterCriticalSection(&cs); }
		void UnLock() { LeaveCriticalSection(&cs); }
	};


public:
	ThreadPool(size_t minNumOfThread = 2, size_t maxNumOfThread = 10);
	~ThreadPool();

	BOOL QueueTaskItem(shared_ptr<TaskBase> task, shared_ptr<TaskCallbackBase> taskCb = NULL, BOOL longFun = FALSE);	   // �������

private:
	size_t getCurNumOfThread() { return getIdleThreadNum() + getBusyThreadNum(); }	// ��ȡ�̳߳��еĵ�ǰ�߳���
	size_t GetMaxNumOfThread() { return maxNumOfThread - numOfLongFun; }			// ��ȡ�̳߳��е�����߳���
	void SetMaxNumOfThread(size_t size)			// �����̳߳��е�����߳���
	{ 
		if (size < numOfLongFun)
		{
			maxNumOfThread = size + numOfLongFun;
		}
		else
			maxNumOfThread = size; 
	}					
	size_t GetMinNumOfThread() { return minNumOfThread; }							// ��ȡ�̳߳��е���С�߳���
	void SetMinNumOfThread(size_t size) { minNumOfThread = size; }					// �����̳߳��е���С�߳���

	size_t getIdleThreadNum() { return idleThreadList.size(); }	// ��ȡ�̳߳��е��߳���
	size_t getBusyThreadNum() { return busyThreadList.size(); }	// ��ȡ�̳߳��е��߳���
	void CreateIdleThread(size_t size);							// ���������߳�
	void DeleteIdleThread(size_t size);							// ɾ�������߳�
	Thread *GetIdleThread();									// ��ȡ�����߳�
	void MoveBusyThreadToIdleList(Thread *busyThread);			// æµ�̼߳�������б�
	void MoveThreadToBusyList(Thread *thread);					// �̼߳���æµ�б�
	void GetTaskExcute();										// �����������ȡ����ִ��

	CriticalSectionLock idleThreadLock;							// �����߳��б���
	list<Thread *> idleThreadList;								// �����߳��б�
	CriticalSectionLock busyThreadLock;							// æµ�߳��б���
	list<Thread *> busyThreadList;								// æµ�߳��б�

	CriticalSectionLock waitTaskLock;
	list<shared_ptr<WaitTask>> waitTaskList;					// �����б�

	HANDLE					dispatchThrad;						// �ַ������߳�
	HANDLE					stopEvent;							// ֪ͨ�߳��˳���ʱ��
	HANDLE					completionPort;						// ��ɶ˿�
	size_t					maxNumOfThread;						// �̳߳��������߳���
	size_t					minNumOfThread;						// �̳߳�����С���߳���
	size_t					numOfLongFun;						// �̳߳�����С���߳���
};



