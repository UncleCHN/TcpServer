#include"utils/include/CmiLog.h"

int CmiLog::rowNum = 0;
BUILD_SHARE(CmiLog)
CmiLog::CmiLog()
{
	SharedAppenderPtr pConsoleAppender(new ConsoleAppender());
	//SharedAppenderPtr pFileAppender(new FileAppender(buf2));

	// ����һ��PatternLayout,���󶨵� pConsoleAppender  
	//auto_ptr<Layout> pPatternLayout(new PatternLayout(_T("%d{%m/%d/%y %H:%M:%S} - %m [%l]%n")));
	//pConsoleAppender->setLayout(pPatternLayout);
	//pFileAppender->setLayout(pPatternLayout);

	// ����Logger,���������ȼ�   
	Logger logger = Logger::getInstance(_T("Log"));

	// ����Ҫ����Logger��Appender��ӵ�Logger��   
	logger.addAppender(pConsoleAppender);
	//logger.addAppender(pFileAppender);
}

CmiLog::~CmiLog()
{
}

 void __cdecl CmiLog::debug(const char *format, ...)
{
	char buf[4096], *p = buf;
	va_list args;
	va_start(args, format);
	p += _vsnprintf(p, sizeof(buf) - 1, format, args);
	va_end(args);

	LOG4CPLUS_DEBUG(Log(), ++rowNum<<":"<<buf);
}


 void __cdecl CmiLog::err(const char *format, ...)
 {
	 char buf[4096], *p = buf;
	 va_list args;
	 va_start(args, format);
	 p += _vsnprintf(p, sizeof(buf) - 1, format, args);
	 va_end(args);

	 LOG4CPLUS_ERROR(Log(), ++rowNum << ":" << buf);
 }

 void __cdecl CmiLog::warn(const char *format, ...)
 {
	 char buf[4096], *p = buf;
	 va_list args;
	 va_start(args, format);
	 p += _vsnprintf(p, sizeof(buf) - 1, format, args);
	 va_end(args);

	 LOG4CPLUS_WARN(Log(), ++rowNum << ":" << buf);
 }