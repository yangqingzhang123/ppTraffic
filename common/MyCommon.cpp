#include "MyCommon.h"

using namespace std;

namespace MyCommon {

CTimer cctimer;

long get_time_us()
{
     timeval t1; 
     gettimeofday(&t1,NULL);
     return t1.tv_sec*1000*1000 + t1.tv_usec;
}
int tokenize(const std::string &src,std::vector<int> &tokens,const char delim)
{
    std::string::size_type pos1=0,pos2=0;
    while (true)
    {   
        pos1=src.find(delim,pos2);
        if(pos1==std::string::npos){
            tokens.push_back(pos1);
            break;
        }else{
            tokens.push_back(pos1);
        }   
        pos2=pos1+1;
    }   
    return tokens.size();
}
int tokenize(const std::string &src,std::vector<std::string> &tokens,const char delim)
{
	string::const_iterator pos1=src.begin();
	string::const_iterator pos2=src.begin();
	while(true)
	{
		while(*pos2!=delim&&pos2!=src.end())
			pos2++;
		tokens.push_back(string(pos1,pos2));
		if(pos2==src.end())
			break;
		pos2++;
		pos1=pos2;
	}
    return tokens.size();
}
int tokenize(const std::string &src,std::vector<std::string> &tokens,const std::string &delim)
{
    std::string::size_type pos1=0,pos2=0;
    while (true)
    {   
        pos1=src.find_first_of(delim,pos2);
        if(pos1==std::string::npos){
            string tmp = src.substr(pos2);
            tokens.push_back(tmp);
            break;
        }else{
            string tmp = src.substr(pos2,pos1-pos2);
            tokens.push_back(tmp);
        }   
        pos2=pos1+1;
    }   
    return tokens.size();
}


int join(const std::vector<std::string> & val, const std::string & sep,std::string & ret)
{
	if(val.size()==0)return 0;
	std::ostringstream oss;
	oss<<val[0];
	for(int i=1;i<val.size();i++)
		oss<<"|"<<val[i];
	ret=oss.str();
	return 0;
}

int count(const char* src, const char ch)
{
	int cnt = 0;
	if(src==NULL)
		return cnt;
	while(*src != '\0')
	{
		if(*src == ch)
			cnt++;
		src++;
	}
	return cnt;
}

double getSphereDist(double lng1,double lat1,double lng2,double lat2)
{
        double radlat1 = lat1 * PI / 180.0;
        double radlat2 = lat2 * PI / 180.0;
        double a = radlat1 - radlat2;
        double b = (lng1 - lng2) * PI / 180.0;
        double s = 2 * asin(sqrt(pow(sin(a/2),2) + cos(radlat1)*cos(radlat2)*pow(sin(b/2),2))) * EARTH_RADIUS_MI;
        return s;
}

bool md5(string& str, string& md5_result) 
{  
	char buff[33]={'\0'};
	unsigned char md5[16] = {0};  
	char tmp[3]={'\0'};  

	MD5_CTX ctx;  
	MD5_Init(&ctx);  
	MD5_Update(&ctx, str.c_str(), str.size());  
	MD5_Final(md5, &ctx);  

	for(int i = 0; i<16; i++)  
	{ 
		sprintf(tmp, "%02x", md5[i]);  
		strcat(buff, tmp);  
	}
	md5_result = buff;
	return true;  
}  

int double2Seconds(const double double_time)
{
    int hour = double_time;
    int min = round((double_time - hour)*100);
    return (hour*3600 + min*60);
}

int PrintInfo::PrintLog(const char* format, ...) {
    return 1;
    time_t t;
    time(&t);
    char buff[MAXLOGLEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buff, MAXLOGLEN, format, args);
    va_end(args);
    //      fprintf(stderr, "[MIOJI][LOG][%s]%s\n", MJ::MyTime::toString(t,8,"%Y%m%d_%H:%M:%S").c_str(), buff);
    fprintf(stderr, "[LOG][%s]%s\n", MJ::MyTime::toString(t,8,"%m%d_%H:%M:%S").c_str(), buff);
    return 0;
}


int PrintInfo::PrintDbg(const char* format, ...) {
    return 1;
    time_t t;
    time(&t);
    char buff[MAXLOGLEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buff, MAXLOGLEN, format, args);
    va_end(args);
    //      fprintf(stderr, "[MIOJI][DBG][%s]%s\n", MJ::MyTime::toString(t,8,"%Y%m%d_%H:%M:%S").c_str(), buff);
    fprintf(stderr, "[DBG][%s]%s\n", MJ::MyTime::toString(t,8,"%m%d_%H:%M:%S").c_str(), buff);
    return 0;
}


int PrintInfo::PrintErr(const char* format, ...) {
    return 1;
    time_t t;
    time(&t);
    char buff[MAXLOGLEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buff, MAXLOGLEN, format, args);
    va_end(args);
    //      fprintf(stderr, "[MIOJI][ERR][%s]%s\n", MJ::MyTime::toString(t,8,"%Y%m%d_%H:%M:%S").c_str(), buff);
    fprintf(stderr, "[ERR][%s]%s\n", MJ::MyTime::toString(t,8,"%m%d_%H:%M:%S").c_str(), buff);
    return 0;
}

} // end of namespace
