#ifndef _LYCONSTDATA_H_
#define _LYCONSTDATA_H_

#include <string>
#include <map>
#include "json/json.h"
#include "define.h"
#include "City.h"
#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <set>
#include "RouteConfig.h"
#include <mysql/mysql.h>

//存储限定的固化数据到程序内存中
class LYConstData{
public:
	/*构造析构初始化*/
	LYConstData() {}
	~LYConstData() {
		Destroy();
	}
	/*全局初始化函数*/
	static int Init();

	/*全局析构函数*/
	static bool Destroy(){
		std::tr1::unordered_map<int, City*>::iterator mi;
                for (mi = m_ID2CityMap.begin(); mi != m_ID2CityMap.end(); mi ++) {
                        delete mi->second;
                }
                m_ID2CityMap.clear();
		pthread_mutex_destroy(&m_MutexLocker);
	}


	static std::tr1::unordered_map<int, City*> m_ID2CityMap;
private:

	// 加载数据的入口
	static int LoadData();
	// 加载城市数据
	static int LoadCity();

        static pthread_mutex_t m_MutexLocker;
        static MYSQL m_Mysql;
};

#endif //_LYCONSTDATA_H_
