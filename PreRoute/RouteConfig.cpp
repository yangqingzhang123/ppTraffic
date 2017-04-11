#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "RouteConfig.h"
//#include "CommonFuc.h"


using namespace std;

/*多线程配置参数*/
size_t RouteConfig::thread_num=1;
size_t RouteConfig::thread_stack_size=102400000;

/*子服务相关参数*/
std::string RouteConfig::cache_server_addr;
std::string RouteConfig::traffic_server_addr;
std::string RouteConfig::hotel_server_addr;
std::string RouteConfig::view_server_addr;
std::string RouteConfig::db_host;
std::string RouteConfig::db_user;
std::string RouteConfig::db_passwd;
std::string RouteConfig::db_name;

int RouteConfig::traffic_server_timeout;
int RouteConfig::view_server_timeout;
int RouteConfig::hotel_server_timeout;

/*数据文件*/
std::string RouteConfig::data_path;
std::string RouteConfig::city_file_name;

/*路径得分计算相关参数*/
int RouteConfig::cost_traffic_weight;	//交通花费所占得分比重
int RouteConfig::cost_hotel_weight;		//酒店花费所占得分比重
int RouteConfig::time_traffic_weight;	//交通时间所占得分比重
int RouteConfig::depart_daytime;				//白天出发的附加得分
int	RouteConfig::depart_nighttime;			//夜间出发的附加得分


/*Route路径规划使用参数*/
int RouteConfig::max_size_bfs_pre;	//路径预规划保留的最大路径数
int RouteConfig::return_num_bfs_pre;	//路径预规划最终返回的结果数
int RouteConfig::max_size_bfs;	//路径规划保留的最大路径数
int RouteConfig::cheapest_price_num;	//根据价格剪枝保留的路径数目 一定要大于return_num_bfs
int RouteConfig::max_pre_head_tail_expand_num;	//每条确定首尾交通的预规划路径保留的结果个数
int RouteConfig::max_pre_expand_num;		//每条预规划路径保留的结果个数
int RouteConfig::return_num_bfs;	//路径规划最终返回的结果数
int RouteConfig::return_num;		//城市级别规划返回给客户端的结果数

/*其他*/
int RouteConfig::debug_level;	//调试输出的级别
int RouteConfig::runtime_error;	//运行时出现异常的处理方式 0：继续 1：exit

RouteConfig::RouteConfig(){
}
RouteConfig::~RouteConfig(){
}

bool RouteConfig::init(const std::string& path){
	//赋值根路径
	data_path = path;
	if (data_path[data_path.length()-1]!='/')
		data_path += "/";
	
	string fname = data_path + "route.conf";
	ifstream fin;
	fin.open(fname.c_str());
	if (!fin){
		cerr<<"can not open file "<<fname<<endl;
		return false;
	}
	string line = "";
	while(!fin.eof()){
		getline(fin,line);
		if (line.length()==0||line[0]=='#')
			continue;
		int pos = line.find("=");
		if (pos==std::string::npos){
			cerr<<"[WARNING]:format err->"<<line<<endl;
			continue;
		}
		string key = line.substr(0,pos);
		string val = line.substr(pos+1);
		if (key == "cost_traffic_weight")
			cost_traffic_weight = atoi(val.c_str());
		else if (key=="cost_hotel_weight")
			cost_hotel_weight = atoi(val.c_str());
		else if (key=="time_traffic_weight")
			time_traffic_weight = atoi(val.c_str());
		else if (key=="depart_daytime")
			depart_daytime = atoi(val.c_str());
		else if (key=="depart_nighttime")
			depart_nighttime = atoi(val.c_str());
		else if (key=="max_size_bfs_pre")
			max_size_bfs_pre = atoi(val.c_str());
		else if (key=="cheapest_price_num")
			cheapest_price_num = atoi(val.c_str());
		else if (key=="max_pre_head_tail_expand_num")
			max_pre_head_tail_expand_num = atoi(val.c_str());
		else if (key=="max_pre_expand_num")
			max_pre_expand_num = atoi(val.c_str());
		else if (key=="return_num_bfs_pre")
			return_num_bfs_pre = atoi(val.c_str());
		else if (key=="max_size_bfs")
			max_size_bfs = atoi(val.c_str());
		else if (key=="return_num_bfs")
			return_num_bfs = atoi(val.c_str());
		else if (key=="return_num")
			return_num = atoi(val.c_str());
		else if (key == "city_file_name")
			city_file_name = val;
		else if (key=="data_path"){
			data_path = val;
			if (data_path[data_path.length()-1]!='/')
				data_path += "/";
		}else if (key=="debug_level")
			debug_level = atoi(val.c_str());
		else if (key=="cache_server_addr")
			cache_server_addr = val;
		else if (key=="traffic_server_addr")
			traffic_server_addr = val;
		else if (key=="traffic_server_timeout")
			traffic_server_timeout = atoi(val.c_str());
		else if (key=="view_server_addr")
			view_server_addr = val;
		else if (key=="view_server_timeout")
			view_server_timeout = atoi(val.c_str());
		else if (key=="hotel_server_addr")
			hotel_server_addr = val;
		else if (key=="hotel_server_timeout")
			hotel_server_timeout = atoi(val.c_str());
		else if (key=="thread_num")
			thread_num = atoi(val.c_str());
		else if (key=="thread_stack_size")
			thread_stack_size = atoi(val.c_str());
		else if (key == "db_host")
			db_host = val;
		else if (key == "db_user")
			db_user = val;
		else if (key == "db_passwd")
			db_passwd = val;
		else if (key == "db_name")
			db_name = val;
	}
	
	return true;
	
}
