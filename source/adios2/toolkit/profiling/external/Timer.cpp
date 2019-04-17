/*
   Copyright (c) 2019 University of Oregon
   Distributed under the OSI-approved Apache License, Version 2.0.  See
   accompanying file Copyright.txt for details.
 */

#include "Timer.h"

/* If not enabled, macro out all of the code in this file. */
#if defined(ADIOST_USE_TIMERS)

#include <unistd.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <thread>

#define ADIOST_SUCCESS 0
#define ADIOST_FAILURE 1

/* Make sure that the Timer singleton is constructed when the
 * library is loaded.  This will ensure (on linux, anyway) that
 * we can assert that we have m_Initialized on the main thread. */
//static void __attribute__((constructor)) InitializeLibrary(void);

/* Function pointer types */

/* Logistical functions */
typedef void (*AdiostInitType)(void);
typedef void (*AdiostRegisterThreadType)(void);
typedef void (*AdiostExitType)(void);
/* Data entry functions */
typedef void (*AdiostStartType)(const char *);
typedef void (*AdiostStopType)(const char *);
typedef void (*AdiostSampleCounterType)(const char *, double);
typedef void (*AdiostMetadataType)(const char *, const char *);
// typedef std::vector<double> (*AdiostGetProfileType)(const char *, bool allThreads);

/* Function pointers */

void (*MyAdiostInit)(void) = nullptr;
void (*MyAdiostRegisterThread)(void) = nullptr;
void (*MyAdiostExit)(void) = nullptr;
void (*MyAdiostStart)(const char *) = nullptr;
void (*MyAdiostStop)(const char *) = nullptr;
void (*MyAdiostSampleCounter)(const char *, double) = nullptr;
void (*MyAdiostMetadata)(const char *, const char *) = nullptr;
// std::vector<double> (*MyAdiostGetProfile)(const char *, bool) = nullptr;

static void InitializeLibrary(void)
{
    // initialize the library by creating the singleton
    static external::profiler::Timer &tt = external::profiler::Timer::Get();
}

void OpenPreloadLibraries(void)
{
/* We might be in a static executable.  Get the ld_preload variable */
#if defined(__APPLE__) && defined(__MACH__)
    const char *envvar = "DYLD_PRELOAD";
#else
    const char *envvar = "LD_PRELOAD";
#endif
    const char *preload = getenv(envvar);
    if (preload != NULL)
    {
        // fprintf(stderr, "%s:\n%s\n", envvar, preload);
        /* tokenize it */
        char *token = strtok((char *)preload, ":");
        while (token)
        {
            printf("token: %s\n", token);
            /* then, dlopen() first and re-try the dlsym() call. */
            dlopen(token, RTLD_LAZY);
            token = strtok(NULL, ":");
        }
    }
    // fprintf(stderr, "%s not set!\n", envvar);
}

int AssignFunctionPointers(void)
{
    MyAdiostInit = (AdiostInitType)dlsym(RTLD_DEFAULT, "perftool_init");
    if (MyAdiostInit == nullptr) {
        // still do this?  Static executables could crash.
        OpenPreloadLibraries();
        MyAdiostInit = (AdiostInitType)dlsym(RTLD_DEFAULT, "perftool_init");
        if (MyAdiostInit == nullptr) {
            return ADIOST_FAILURE;
        }
    }
    MyAdiostRegisterThread =
        (AdiostRegisterThreadType)dlsym(RTLD_DEFAULT, "perftool_register_thread");
    MyAdiostStart = (AdiostStartType)dlsym(RTLD_DEFAULT, "perftool_start");
    MyAdiostStop = (AdiostStopType)dlsym(RTLD_DEFAULT, "perftool_stop");
    MyAdiostExit = (AdiostExitType)dlsym(RTLD_DEFAULT, "perftool_exit");
    MyAdiostSampleCounter = (AdiostSampleCounterType)dlsym(
        RTLD_DEFAULT, "perftool_sample_counter");
    MyAdiostMetadata = (AdiostMetadataType)dlsym(RTLD_DEFAULT, "perftool_metadata");
    return ADIOST_SUCCESS;
}

int AdiostStubInitializeSimple(void)
{
    if (AssignFunctionPointers() == ADIOST_FAILURE) {
        return ADIOST_FAILURE;
    }
    MyAdiostInit();
    return ADIOST_SUCCESS;
}


namespace external
{

namespace profiler
{

thread_local bool Timer::m_ThreadSeen(false);

// constructor
Timer::Timer(void) : m_Initialized(false)
{
    if (AdiostStubInitializeSimple() == ADIOST_SUCCESS) {
        m_Initialized = true;
    }
    m_ThreadSeen = true;
}

// The only way to get an instance of this class
Timer &Timer::Get(void)
{
    static std::unique_ptr<Timer> instance(new Timer);
    if (!m_ThreadSeen && instance.get()->m_Initialized)
    {
        _RegisterThread();
    }
    return *instance;
}

// used internally to the class
inline void Timer::_RegisterThread(void)
{
    if (!m_ThreadSeen)
    {
        MyAdiostRegisterThread();
        m_ThreadSeen = true;
    }
}

// external API call
void Timer::RegisterThread(void)
{
    if (!Timer::Get().m_Initialized)
        return;
    _RegisterThread();
}

void Timer::Start(const char *timer_name)
{
    if (!Timer::Get().m_Initialized)
        return;
    MyAdiostStart(timer_name);
}

void Timer::Start(const std::string &timer_name)
{
    if (!Timer::Get().m_Initialized)
        return;
    MyAdiostStart(timer_name.c_str());
}

void Timer::Stop(const char *timer_name)
{
    if (!Timer::Get().m_Initialized)
        return;
    MyAdiostStop(timer_name);
}

void Timer::Stop(const std::string &timer_name)
{
    if (!Timer::Get().m_Initialized)
        return;
    MyAdiostStop(timer_name.c_str());
}

void Timer::SampleCounter(const char *name, const double value)
{
    if (!Timer::Get().m_Initialized)
        return;
    MyAdiostSampleCounter(const_cast<char *>(name), value);
}

void Timer::MetaData(const char *name, const char *value)
{
    if (!Timer::Get().m_Initialized)
        return;
    MyAdiostMetadata(name, value);
}

Timer::~Timer(void)
{ 
    if (MyAdiostExit == nullptr)
        return;
    MyAdiostExit();
}

} // namespace profiler

} // namespace external

/* Expose the API to C */

extern "C" { // C Bindings

void TimerInit() {
    InitializeLibrary();
}

void TimerRegisterThread() {
    external::profiler::Timer::RegisterThread();
}

void TimerStart(const char *timerName)
{
    external::profiler::Timer::Start(timerName);
}

void TimerStop(const char *timerName)
{
    external::profiler::Timer::Stop(timerName);
}

void TimerSampleCounter(const char *name, const double value)
{
    external::profiler::Timer::SampleCounter(name, value);
}

void TimerMetaData(const char *name, const char *value)
{
    external::profiler::Timer::MetaData(name, value);
}

/* End of C function definitions */

} // extern "C"

#endif // defined(ADIOST_USE_TIMERS)
