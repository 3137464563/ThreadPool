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
public:
	ThreadPool(size_t minNumOfThread = 2, size_t maxNumOfThread = 0);
	~ThreadPool();

	BOOL QueueTaskItem(shared_ptr<TaskBase> task, shared_ptr<TaskCallbackBase> taskCb, BOOL longFun = FALSE);
	size_t getPoolSize() { return threadList.size(); }

private:
	// �߳���(�ڲ���)
	class Thread
	{
	public:
		Thread();
		~Thread();

		BOOL isBusy();											// �Ƿ���������ִ��
		void ExecuteTask(shared_ptr<TaskBase> t, shared_ptr<TaskCallbackBase> tcb);		// ִ������

	private:
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
		WaitTask(shared_ptr<TaskBase> task, shared_ptr<TaskCallbackBase> taskCb, BOOL bLong = FALSE) 
		{
			this->task = task;
			this->taskCb = taskCb;
			this->bLong = bLong;
		}
		~WaitTask() { task = NULL; taskCb = NULL; }

		shared_ptr<TaskBase>	task;					// Ҫִ�е�����
		shared_ptr<TaskCallbackBase> taskCb;			// �ص�������
		BOOL bLong;										// �Ƿ�ʱ������
	};

	// �������б�ȡ���������
	class GetTaskTask : TaskBase
	{
	public:
		GetTaskTask(ThreadPool *threadPool) { wParam = (WPARAM)threadPool; }
		~GetTaskTask() { }

		int Run()
		{
			ThreadPool *threadPool = (ThreadPool *)wParam;
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

			return 1;
		}

	};

	void GetTaskExcute();

	CRITICAL_SECTION csThreadLock;
	list<shared_ptr<Thread>> threadList;				// �߳��б�

	CRITICAL_SECTION csWaitTaskLock;
	queue<shared_ptr<WaitTask>> waitTaskList;			// �����б�

	HANDLE					stopEvent;					// ֪ͨ�߳��˳���ʱ��
	HANDLE					completionPort;				// ��ɶ˿�
	size_t					maxNumOfThread;
	size_t					minNumOfThread;
	shared_ptr<Thread>		dispatchTaskthread;		// �ַ�������߳�
};



