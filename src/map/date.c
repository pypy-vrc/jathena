
#include "date.h"
#include <time.h>

int date_get_year()
{
	time_t t;
	struct tm * lt;
	t = time(NULL);
	lt = localtime(&t);
	return lt->tm_year+1900;
}
int date_get_month()
{
	time_t t;
	struct tm * lt;
	t = time(NULL);
	lt = localtime(&t);
	return lt->tm_mon+1;
}
int date_get_day()
{
	time_t t;
	struct tm * lt;
	t = time(NULL);
	lt = localtime(&t);
	return lt->tm_mday;
}
int date_get_hour()
{
	time_t t;
	struct tm * lt;
	t = time(NULL);
	lt = localtime(&t);
	return lt->tm_hour;
}

int date_get_min()
{
	time_t t;
	struct tm * lt;
	t = time(NULL);
	lt = localtime(&t);
	return lt->tm_min;
}

int date_get_sec()
{
	time_t t;
	struct tm * lt;
	t = time(NULL);
	lt = localtime(&t);
	return lt->tm_sec;
}

int is_day_of_sun()
{
	return date_get_day()%2 == 0;
}

int is_day_of_moon()
{
	return date_get_day()%2 == 1;
}

int is_day_of_star()
{
	return date_get_day()%5 == 0;
}

