#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "RouteConfig.h"
//#include "CommonFuc.h"


using namespace std;

/*���߳����ò���*/
size_t RouteConfig::thread_num=1;
size_t RouteConfig::thread_stack_size=102400000;

/*�ӷ�����ز���*/
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

/*�����ļ�*/
std::string RouteConfig::data_path;
std::string RouteConfig::city_file_name;

/*·���÷ּ�����ز���*/
int RouteConfig::cost_traffic_weight;	//��ͨ������ռ�÷ֱ���
int RouteConfig::cost_hotel_weight;		//�Ƶ껨����ռ�÷ֱ���
int RouteConfig::time_traffic_weight;	//��ͨʱ����ռ�÷ֱ���
int RouteConfig::depart_daytime;				//��������ĸ��ӵ÷�
int	RouteConfig::depart_nighttime;			//ҹ������ĸ��ӵ÷�


/*Route·���滮ʹ�ò���*/
int RouteConfig::max_size_bfs_pre;	//·��Ԥ�滮���������·����
int RouteConfig::return_num_bfs_pre;	//·��Ԥ�滮���շ��صĽ����
int RouteConfig::max_size_bfs;	//·���滮���������·����
int RouteConfig::cheapest_price_num;	//���ݼ۸��֦������·����Ŀ һ��Ҫ����return_num_bfs
int RouteConfig::max_pre_head_tail_expand_num;	//ÿ��ȷ����β��ͨ��Ԥ�滮·�������Ľ������
int RouteConfig::max_pre_expand_num;		//ÿ��Ԥ�滮·�������Ľ������
int RouteConfig::return_num_bfs;	//·���滮���շ��صĽ����
int RouteConfig::return_num;		//���м���滮���ظ��ͻ��˵Ľ����

/*����*/
int RouteConfig::debug_level;	//��������ļ���
int RouteConfig::runtime_error;	//����ʱ�����쳣�Ĵ���ʽ 0������ 1��exit

RouteConfig::RouteConfig(){
}
RouteConfig::~RouteConfig(){
}

bool RouteConfig::init(const std::string& path){
	//��ֵ��·��
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
