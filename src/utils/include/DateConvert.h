#include <string>  
#include <time.h>  
/*
string to time_t
ʱ���ʽ  2009-3-24
*/
class DateConvert
{
private:
	DateConvert() {}
	~DateConvert() {}

public:
	static  int API_StringToDate(const std::string &strDateStr, time_t &timeData)
	{
		char *pBeginPos = (char*)strDateStr.c_str();
		char *pPos = strstr(pBeginPos, "-");
		if (pPos == NULL)
		{
			return -1;
		}
		int iYear = atoi(pBeginPos);
		int iMonth = atoi(pPos + 1);

		pPos = strstr(pPos + 1, "-");
		if (pPos == NULL)
		{
			return -1;
		}

		int iDay = atoi(pPos + 1);

		struct tm sourcedate;
		memset((void*)&sourcedate, 0, sizeof(sourcedate));
		sourcedate.tm_mday = iDay;
		sourcedate.tm_mon = iMonth - 1;
		sourcedate.tm_year = iYear - 1900;

		timeData = mktime(&sourcedate);

		return 0;
	};

	/*
	time_t to string
	*/
	static int API_DateToString(std::string &strDateStr, const time_t &timeData)
	{
		char chTmp[15];
		memset(chTmp, 0, sizeof(chTmp));

		struct tm *p;
		p = localtime(&timeData);

		p->tm_year = p->tm_year + 1900;

		p->tm_mon = p->tm_mon + 1;


		snprintf(chTmp, sizeof(chTmp), "%04d-%02d-%02d",
			p->tm_year, p->tm_mon, p->tm_mday);

		strDateStr = chTmp;
		return 0;
	};

	//    struct tm  
	//    {  
	//        int tm_sec;  
	//        int tm_min;  
	//        int tm_hour;  
	//        int tm_mday;  
	//        int tm_mon;  
	//        int tm_year;  
	//        int tm_wday;  
	//        int tm_yday;  
	//        int tm_isdst;  
	//    };  
	//int tm_sec ����Ŀǰ������������ΧΪ0-59����������61��  
	//int tm_min ����Ŀǰ��������Χ0-59  
	//int tm_hour ����ҹ�����ʱ������ΧΪ0-23  
	//int tm_mday Ŀǰ�·ݵ���������Χ01-31  
	//int tm_mon ����Ŀǰ�·ݣ���һ�����𣬷�Χ��0-11  
	//int tm_year ��1900 ���������������  
	//int tm_wday һ���ڵ�������������һ���𣬷�ΧΪ0-6  
	//int tm_yday �ӽ���1��1�������������������ΧΪ0-365  
	//int tm_isdst �չ��Լʱ������  

	/*
	string to time_t
	ʱ���ʽ 2009-3-24 0:00:08 �� 2009-3-24
	*/
	static int API_StringToDateEX(const std::string &strDateStr, time_t &timeData)
	{
		char *pBeginPos = (char*)strDateStr.c_str();
		char *pPos = strstr(pBeginPos, "-");
		if (pPos == NULL)
		{
			printf("strDateStr[%s] err \n", strDateStr.c_str());
			return -1;
		}
		int iYear = atoi(pBeginPos);
		int iMonth = atoi(pPos + 1);
		pPos = strstr(pPos + 1, "-");
		if (pPos == NULL)
		{
			printf("strDateStr[%s] err \n", strDateStr.c_str());
			return -1;
		}
		int iDay = atoi(pPos + 1);
		int iHour = 0;
		int iMin = 0;
		int iSec = 0;
		pPos = strstr(pPos + 1, " ");
		//Ϊ�˼�����Щû��ȷ��ʱ�����  
		if (pPos != NULL)
		{
			iHour = atoi(pPos + 1);
			pPos = strstr(pPos + 1, ":");
			if (pPos != NULL)
			{
				iMin = atoi(pPos + 1);
				pPos = strstr(pPos + 1, ":");
				if (pPos != NULL)
				{
					iSec = atoi(pPos + 1);
				}
			}
		}

		struct tm sourcedate;
		memset((void*)&sourcedate, 0, sizeof(sourcedate));
		sourcedate.tm_sec = iSec;
		sourcedate.tm_min = iMin;
		sourcedate.tm_hour = iHour;
		sourcedate.tm_mday = iDay;
		sourcedate.tm_mon = iMonth - 1;
		sourcedate.tm_year = iYear - 1900;
		timeData = mktime(&sourcedate);
		return 0;
	}
	/*
	time_t to string ʱ���ʽ 2009-3-24 0:00:08
	*/
	static int API_DateToStringEX(std::string &strDateStr, const time_t &timeData)
	{
		char chTmp[100];
		memset(chTmp, 0, sizeof(chTmp));
		struct tm *p;
		p = localtime(&timeData);
		p->tm_year = p->tm_year + 1900;
		p->tm_mon = p->tm_mon + 1;

		snprintf(chTmp, sizeof(chTmp), "%04d-%02d-%02d %02d:%02d:%02d",
			p->tm_year, p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
		strDateStr = chTmp;
		return 0;
	}

};