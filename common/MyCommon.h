#ifndef COMMON_CSY_H
#define COMMON_CSY_H

#include "time.h"
#include <sys/time.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "openssl/md5.h"
#include <string.h>
#include <stdio.h>
#include "MJCommon.h"

#define REDIS_DEBUG 0
#define DEBUG 0

#define EARTH_RADIUS_MI 6378137
#define PI 3.1415927

#define ERR_THRES 0.00001
#define RUN_OK 0
#define RUN_ERR -1

#define MAXQUERYLEN 510         // 255 chars
#define MAXNAMELEN 510          // 255 chars
#define MAXWORDLEN 510          // max (MAXQUERYLEN, MAXNAMELEN)

#define SSDB_KEY_WEEK_DAY 1

#define MAXLOGLEN 2048000

using namespace std;

namespace MyCommon{

long get_time_us();
int tokenize(const std::string &src,std::vector<std::string> &tokens,const std::string &delim);
int tokenize(const std::string &src,std::vector<std::string> &tokens,const char delim);
int tokenize(const std::string &src,std::vector<int> &tokens,const char delim);
int double2Seconds(const double double_time);
static int ParseInt(std::string& Value, int& Ret) {
    if (Value == "") {
        Ret = 0;
        return RUN_ERR;
    }
    Ret = atoi(Value.c_str());
    return RUN_OK;
}

static int ParseDouble(std::string& Value, double& Ret) {
    if (Value == "") {
        Ret = 0;
        return RUN_ERR;
    }
    Ret = atof(Value.c_str());
    return RUN_OK;
}

int join(const std::vector<std::string> & val, const std::string & sep,std::string & ret);
int count(const char* src, const char ch);
double getSphereDist(double lng1,double lat1,double lng2,double lat2);
bool md5(std::string& str, std::string& md5_result);  

class CTimer{
private:
	struct timeval _last;
	long cost;
public:
	CTimer(){
		cost=0;
	}
	void reset(){
		cost=0;
	};
	void start(){
		gettimeofday(&_last,NULL);
	};
	void pause(){
		struct timeval _this;
		gettimeofday(&_this,NULL);
		cost+=(_this.tv_sec-_last.tv_sec)*1000000+(_this.tv_usec-_last.tv_usec);
	};
	long get_cost(){
		return cost;
	}
};

extern CTimer cctimer;

class PrintInfo {
    private:
        static int NEED_STD_LOG;
        static int NEED_ERR_LOG;
    public:
        static int NEED_DUMP_LOG;
        static int NEED_DBG_LOG;
        static int SwitchDbg(int need_dbg, int need_log, int need_err, int need_dump);
        static int PrintLog(const char* format, ...);
        static int PrintDbg(const char* format, ...);
        static int PrintErr(const char* format, ...);
};

} // end namespace 
#endif
