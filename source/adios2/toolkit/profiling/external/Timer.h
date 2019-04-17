/*
   Copyright (c) 2019 University of Oregon
   Distributed under the OSI-approved Apache License, Version 2.0.  See
   accompanying file Copyright.txt for details.
 */

#pragma once

/* This code won't compile on windows.  Disable it */
#if !defined(_WIN32) && !defined(_WIN64) // && !defined(__APPLE__)
#define ADIOST_USE_TIMERS
#endif

/* ------------------------------------------------------------------ */
/* Define the C++ API and ADIOS glue class first */
/* ------------------------------------------------------------------ */

#ifdef __cplusplus

#if defined(ADIOST_USE_TIMERS)

#include <memory>
#include <sstream>
#include <string>

namespace external
{

namespace profiler
{

class Timer
{
public:
    static void RegisterThread(void);
    static void Start(const char *timer_name);
    static void Start(const std::string &timer_name);
    static void Stop(const char *timer_name);
    static void Stop(const std::string &timer_name);
    static void SampleCounter(const char *name, const double value);
    static void MetaData(const char *name, const char *value);
    // The only way to get an instance of this class
    static Timer &Get(void);
    // destructor
    ~Timer(void);
    bool m_Initialized;
    static thread_local bool m_ThreadSeen;

private:
    static void _RegisterThread(void);
    // Private constructor to prevent construction
    Timer(void);
    // Prevent copies
    Timer(const Timer &old);
    const Timer &operator=(const Timer &old);
};

class ScopedTimer
{
private:
    std::string m_Name;

public:
    ScopedTimer(const std::string &name) : m_Name(name)
    {
        Timer::Start(m_Name);
    }
    ~ScopedTimer() { Timer::Stop(m_Name); }
};

} // namespace external

} // namespace profiler

#define ADIOST_INIT() external::profiler::Timer::Get();
#define ADIOST_REGISTER_THREAD() external::profiler::Timer::RegisterThread();
#define ADIOST_START(_timer_name) external::profiler::Timer::Start(_timer_name);
#define ADIOST_STOP(_timer_name) external::profiler::Timer::Stop(_timer_name);
#define ADIOST_SAMPLE_COUNTER(_name, _value)                                      \
    external::profiler::Timer::SampleCounter(_name, _value);
#define ADIOST_METADATA(_name, _value) external::profiler::Timer::MetaData(_name, _value);
#define ADIOST_SCOPED_TIMER(__name) external::profiler::ScopedTimer __var##finfo(__name);
#define ADIOST_SCOPED_TIMER_FUNC()                                                \
    std::stringstream __ss##finfo;                                             \
    __ss##finfo << __func__ << " [{" << __FILE__ << "} {" << __LINE__          \
                << ",0}]";                                                     \
    external::profiler::ScopedTimer __var##finfo(__ss##finfo.str());

#else // defined(ADIOST_USE_TIMERS)

#define ADIOST_INIT()
#define ADIOST_REGISTER_THREAD()
#define ADIOST_START(_timer_name)
#define ADIOST_STOP(_timer_name)
#define ADIOST_START_FUNC()
#define ADIOST_STOP_FUNC()
#define ADIOST_SAMPLE_COUNTER(_name, _value)
#define ADIOST_METADATA(_name, _value)
#define ADIOST_SCOPED_TIMER(__name)
#define ADIOST_SCOPED_TIMER_FUNC()

#endif // defined(ADIOST_USE_TIMERS)

#else // ifdef __cplusplus

/* ------------------------------------------------------------------ */
/* Now define the C API */
/* ------------------------------------------------------------------ */

#if defined(ADIOST_USE_TIMERS)

/* regular C API */

void TimerRegisterThread();
void TimerStart(const char *timerName);
void TimerStop(const char *timerName);
void TimerSampleCounter(const char *name, const double value);
void TimerMetaData(const char *name, const char *value);

/*
    Macro API for option of entirely disabling at compile time
    To use this API, set the Macro ADIOST_USE_TIMERS on the command
    line or in a config.h file, however your project does it
 */

#define ADIOST_REGISTER_THREAD() TimerRegisterThread();
#define ADIOST_START(_timer_name) TimerStart(_timer_name);
#define ADIOST_STOP(_timer_name) TimerStop(_timer_name);
#define ADIOST_START_FUNC()                                                       \
    char __adiostFuncName[1024];                                                  \
    sprintf(__adiostFuncName, "%s [{%s} {%d,0}]", __func__, __FILE__, __LINE__);  \
    TimerStart(__adiostFuncName);
#define ADIOST_STOP_FUNC() TimerStop(__adiostFuncName);
#define ADIOST_SAMPLE_COUNTER(_name, _value) TimerSampleCounter(_name, _value);
#define ADIOST_METADATA(_name, _value) TimerMetaData(_name, _value);

#else // defined(ADIOST_USE_TIMERS)

#define ADIOST_REGISTER_THREAD()
#define ADIOST_START(_timer_name)
#define ADIOST_STOP(_timer_name)
#define ADIOST_START_FUNC()
#define ADIOST_STOP_FUNC()
#define ADIOST_SAMPLE_COUNTER(_name, _value)
#define ADIOST_METADATA(_name, _value)

#endif // defined(ADIOST_USE_TIMERS)

#endif // ifdef __cplusplus

