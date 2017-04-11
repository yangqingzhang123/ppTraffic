#ifndef __ROUTE_CONFIG_H__
#define __ROUTE_CONFIG_H__

#include <string>
#include <vector>


class RouteConfig{
public:
	RouteConfig();
	~RouteConfig();
	static bool init(const std::string& path);
public:
	/*多线程配置参数*/
	static size_t thread_num;
	static size_t thread_stack_size;

	/*子服务相关参数*/
	static std::string cache_server_addr;
	static std::string traffic_server_addr;
	static std::string view_server_addr;
	static std::string hotel_server_addr;

	static int traffic_server_timeout;
	static int view_server_timeout;
	static int hotel_server_timeout;
	
	/*onlinedb的静态数据*/	
	static std::string db_host;
	static std::string db_user;
	static std::string db_passwd;
	static std::string db_name;

	/*数据文件*/
	static std::string data_path;
	static std::string city_file_name;
	
	/*路径得分计算相关参数*/
	static int cost_traffic_weight;	//交通花费所占得分比重
	static int cost_hotel_weight;		//酒店花费所占得分比重
	static int time_traffic_weight;	//交通时间所占得分比重
	static int depart_daytime;				//白天出发的附加得分
	static int depart_nighttime;			//夜间出发的附加得分
	
	
	/*Route路径规划使用参数*/
	static int max_size_bfs;	//路径规划过程中保留的最大路径数
	static int cheapest_price_num;	//根据价格剪枝保留的路径数目 一定要大于return_num_bfs
	static int max_pre_head_tail_expand_num;	//每条确定首尾交通的预规划路径保留的结果个数
	static int max_pre_expand_num;		//每条预规划路径保留的结果个数
	static int return_num_bfs;	//路径规划最终返回的结果数
	static int return_num;		//城市级别规划返回给客户端的结果数
	
	/*其他*/
	static int debug_level;	//调试输出的级别
	static int runtime_error;	//运行时出现异常的处理方式 0：继续 1：exi

	static int max_size_bfs_pre;
	static int return_num_bfs_pre;
};


#endif	//__ROUTE_CONFIG_H__



