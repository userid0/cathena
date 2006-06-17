#include "basetypes.h"
#include "basetime.h"


///////////////////////////////////////////////////////////////////////////////
/// missing wchar time defines (ie. cygwin)
#if defined(__CYGWIN__) && !defined(_WTIME_DEFINED)

#ifdef __GNUC__ 
// message convention for gnu compilers
#warning "there are no WTIME system function defined, using empty stubs"
#else // no __GNUC__ 
// message convention for visual c compilers
#pragma message ( "there are no WTIME system function defined, using empty stubs" )
#endif

// have a rudimentary definition
inline wchar_t * __cdecl _wasctime(const struct tm *)	{ return NULL; }
inline wchar_t * __cdecl _wctime(const time_t *)		{ return NULL; }
inline size_t __cdecl wcsftime(wchar_t *, size_t, const wchar_t *, const struct tm *)	{ return 0; }
inline wchar_t * __cdecl _wstrdate(wchar_t *)		{ return NULL; }
inline wchar_t * __cdecl _wstrtime(wchar_t *)		{ return NULL; }

#define _WTIME_DEFINED
#endif


NAMESPACE_BEGIN(basics)

//////////////////////////////////////////////////////////////////////////
// datetime type: 
// 64-bit int defined as number of milliseconds since midnight 01/01/0001
// derived from PTypes (C++ Portable Types Library)
//////////////////////////////////////////////////////////////////////////
datetime makedt(int days, int msecs)
{
	return int64(days) * _msecsmax + msecs;
}


bool isvalid(datetime d)
{
	return d >= 0 && d < _datetimemax;
}


bool isleapyear(int year)
{
	return year > 0 && year % 4 == 0 
		&& (year % 100 != 0 || year % 400 == 0);
}


int daysinmonth(int year, int month)
{
	if (month < 1 || month > 12)
		return 0;
	static const int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int res = mdays[month - 1];
	if( (month == 2) && isleapyear(year) )
			res++;
	return res;
}


int daysinyear(int year, int month)
{
	if (month < 1 || month > 12)
		return 0;
	static const int ndays[12] = {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
	int res = ndays[month - 1];
	if( (month > 1) && isleapyear(year) )
			res++;
	return res;
}


int dayofweek(datetime d)
{
	return (days(d) + 1) % 7;
}


bool isdatevalid(int year, int month, int day)
{
	return year >= 1 && year <= 9999 
		&& month >= 1 && month <= 12
		&& day >= 1 && day <= daysinmonth(year, month);
}


datetime encodedate(int year, int month, int day)
{
	if (!isdatevalid(year, month, day))
		return invdatetime;
	int y = year - 1;
	return makedt(day				// days in this month
		+ daysinyear(year, month - 1)   // plus days since the beginning of the year
		+ y * 365					// plus "pure" days
		+ y / 4 - y / 100 + y / 400	// plus leap year correction
		- 1, 0);						// ... minus one (guess why :)
}

datetime encodedate(int year, int month, int day, int hour, int min, int sec, int msec)
{
	return encodedate(year, month, day) + encodetime(hour, min, sec, msec);
}


bool decodedate(datetime date, int& year, int& month, int& day)
{
	int d = days(date);
	if (d < 0 || d >= _daysmax)	// allowed date range is 01/01/0001 - 12/31/9999
	{
		year = 0;
		month = 0;
		day = 0;
		return false;
	}

	const int d1 = 365;			// number of days in 1 year
	const int d4 = d1 * 4 + 1;	// ... in 4 year period
	const int d100 = d4 * 25 - 1;   // ... in 100 year period
	const int d400 = d100 * 4 + 1;  // ... in 400 year period

	year = (d / d400) * 400 + 1;
	d %= d400;

	int t = d / d100;
	d %= d100;
	if (t == 4)
	{
		t--;
		d += d100;
	}
	year += t * 100;

	year += (d / d4) * 4;
	d %= d4;

	t = d / d1;
	d %= d1;
	if (t == 4)
	{
		t--;
		d += d1;
	}
	year += t;

	month = d / 29;					// approximate month no. (to avoid loops)
	if (d < daysinyear(year, month))	// month no. correction
		month--;

	day = d - daysinyear(year, month) + 1;
	month++;
	return true;
}


bool istimevalid(int hour, int min, int sec, int msec)
{
	return hour >= 0 && hour < 24 
		&& min >= 0 && min < 60 
		&& sec >= 0 && sec < 60 
		&& msec >= 0 && msec < 1000;
}


datetime encodetime(int hour, int min, int sec, int msec)
{
	int64 res = int64(hour) * 3600000 + int64(min) * 60000 + int64(sec) * 1000 + msec;
	if (!isvalid(res))
		res = invdatetime;
	return res;
}


bool decodetime(datetime t, int& hour, int& min, int& sec, int& msec)
{
	if (!isvalid(t))
	{
		hour = 0;
		min = 0;
		sec = 0;
		msec = 0;
		return false;
	}
	int m = msecs(t);
	hour = m / 3600000;
	m %= 3600000;
	min = m / 60000;
	m %= 60000;
	sec = m / 1000;
	msec = m % 1000;
	return true;
}


bool decodetime(datetime t, int& hour, int& min, int& sec)
{
	int msec;
	return decodetime(t, hour, min, sec, msec);
}


tm* dttotm(datetime dt, tm& t)
{
	memset(&t, 0, sizeof(tm));
	if (!decodedate(dt, t.tm_year, t.tm_mon, t.tm_mday) 
		|| !decodetime(dt, t.tm_hour, t.tm_min, t.tm_sec))
			return NULL;
	t.tm_mon--;
	t.tm_yday = daysinyear(t.tm_year, t.tm_mon) + t.tm_mday - 1;
	t.tm_wday = dayofweek(dt);
	t.tm_year -= 1900;
	return &t;
}


datetime now(bool utc)
{
#ifdef WIN32
	SYSTEMTIME t;
	if (utc)
		GetSystemTime(&t);
	else
		GetLocalTime(&t);
	return encodedate(t.wYear, t.wMonth, t.wDay) +
		encodetime(t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

#else   // Unix
	// we can't use localtime() and gmtime() here as they don't return
	// milliseconds which are needed for our datetime format. instead,
	// we call gettimeofday() which have microsecond precision, and then
	// adjust the time according to timzone info returned by localtime()
	// on BSD and Linux, and global variables altzone/timezone on SunOS.

	// NOTE: at the moment of passing the DST adjustment (twice a year)
	// the local time value returned by now() may be incorrect.
	// the application should call tzupdate() from time to time if it
	// is supposed to be running infinitely, e.g. if it's a daemon.

	// always rely on UTC time inside your application whenever possible.
	timeval tv;
	gettimeofday(&tv, NULL);
	int edays = tv.tv_sec / 86400  // days since Unix "Epoch", i.e. 01/01/1970
		+ 719162;				// plus days between 01/01/0001 and Unix Epoch
	int esecs = tv.tv_sec % 86400; // the remainder, i.e. seconds since midnight
	datetime res = makedt(edays, esecs * 1000 + tv.tv_usec / 1000);

	if (!utc)
		res += tzoffset() * 60 * 1000;
	return res;
#endif
}


void tzupdate()
{
	tzset();
}


int tzoffset()
{
#if defined(WIN32)
	TIME_ZONE_INFORMATION tz;
	DWORD res = GetTimeZoneInformation(&tz);
	if ((res == TIME_ZONE_ID_DAYLIGHT) && (tz.DaylightDate.wMonth != 0))
		return - (tz.Bias +  tz.DaylightBias);
	else
		return - tz.Bias;

#else   // UNIX
	time_t t0;
	time(&t0);

#if defined(__sun__)
#ifdef SINGLETHREAD
	// localtime_r() is not available without -D_REENTRANT
	tm* t = localtime(&t0);
	if(t->tm_isdst != 0 && daylight != 0) 
#else
	tm t;
	localtime_r(&t0, &t); 
	if(t.tm_isdst != 0 && daylight != 0) 
#endif
		return - altzone / 60; 
	else 
		return - timezone / 60; 

#elif defined(__CYGWIN__)
	time_t local_time = time(NULL);
	struct tm gt;
	gmtime_r(&local_time, &gt);
	time_t gmt_time = mktime(&gt);
	return (local_time - gmt_time) / 60;

#else   // other UNIX
	tm t;
	localtime_r(&t0, &t);
	return t.tm_gmtoff / 60;
#endif

#endif
}


datetime utodatetime(time_t u)
{
	return _unixepoch + int64(u) * 1000;
}


NAMESPACE_END(basics)
