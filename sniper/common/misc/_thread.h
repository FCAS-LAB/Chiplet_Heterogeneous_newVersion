#ifndef THREAD_H
#define THREAD_H

//changed at 2020-4-19
#include<iostream>

class Runnable
{
public:
   virtual ~Runnable() { }
   virtual void run() = 0;
   static void threadFunc(void *vpRunnable)
   {
      Runnable *runnable = (Runnable*)vpRunnable;
      //changed at 2020-4-19
      //std::cout<<"Runnable used\n";
      runnable->run();
   }
};

class _Thread
{
public:
   typedef void (*ThreadFunc)(void*);

   static _Thread *create(ThreadFunc func, void *param);
   static _Thread *create(Runnable *runnable)
   {
      return create(Runnable::threadFunc, runnable);
   }

   virtual ~_Thread() { };

   virtual void run() = 0;
};

#endif // THREAD_H
