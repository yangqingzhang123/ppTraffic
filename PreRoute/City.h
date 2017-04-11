#ifndef _CITY_H_
#define _CITY_H_
#include "define.h"
#include "QuadTree.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/format.hpp>

class City {
public:
	City(Coords& LeftTop, Coords& RightBot) {
		m_ID = -1;
		InsertNum = 0;
		m_Name = "";
		Coords leftTop;
		Coords rightBot;
		Coordinate::LatLon2XY(LeftTop, leftTop);
		Coordinate::LatLon2XY(RightBot, rightBot);
		m_QuadTree = new QuadTree(leftTop, rightBot, 0);
		m_LeftTop = LeftTop; 
		m_RightBot = RightBot;
		m_Center.m_X = (m_LeftTop.m_X + m_RightBot.m_X)/2;
		m_Center.m_Y = (m_LeftTop.m_Y + m_RightBot.m_Y)/2;
		Release();
		
		for (int i = TRAFFICLINE_TYPE_BEGIN; i < TRAFFICLINE_TYPE_COUNT; i ++) {	// id start from 1, push NULL for pos0
			m_TrafficLines[i].push_back(NULL);
		}
		m_ID2Node.push_back(NULL);
	}
	virtual ~City() {
		Release();
	}

	int m_ID;
	std::string m_Name;
	Coords m_LeftTop;
	Coords m_RightBot;
	Coords m_Center;
	int m_TimeZone;
	int InsertNum;

	// 地图点数据
	std::vector<MapNode*> m_ID2Node;
	std::tr1::unordered_set<std::string> m_NodeLinkable;							// 辅助数据: 两点是否交通直接可达
	std::tr1::unordered_map<int, std::tr1::unordered_set<EntityNode*> > m_NID2EPMap[NODE_TYPE_COUNT]; 
	std::tr1::unordered_map<int, EntityNode* > m_ID2StationEntityNode;	
	std::tr1::unordered_map<int,std::tr1::unordered_map<int,std::string > > m_ID2StationInfo;	// Node对应EntityNode的映射关系

	// 实体点数据
	std::tr1::unordered_map<std::string, EntityNode*> m_ViewMap;				// 景点列表
	std::tr1::unordered_map<std::string, EntityNode*> m_HotelMap;				// 酒店列表
	std::tr1::unordered_map<std::string, EntityNode*> m_RestMap;				// 饭店列表
	std::tr1::unordered_map<std::string, EntityNode*> m_AirportMap;				// 机场列表
	std::tr1::unordered_map<std::string, EntityNode*> m_ShopMap;				// 购物列表 
	std::tr1::unordered_map<std::string, EntityNode*> m_ActivityMap;			// 活动列表


	//加预处理来减轻压力
	

	//优化

	QuadTree* m_QuadTree;									// 空间索引

	// 线数据
	std::vector<StationNodeLine*> m_TrafficLines[TRAFFICLINE_TYPE_COUNT];			// 交通线路,从1开始遍历
	std::tr1::unordered_map<int, std::vector<int> > m_NID2TLIDMap[TRAFFICLINE_TYPE_COUNT];	// Node对应Bus或Subway等的一种, 但可属于多条交通线
	EntityNode* GetEntityPointNode(std::string& ID, std::string& Type) {
		std::tr1::unordered_map<std::string, EntityNode*>::iterator mi;
		if (Type == "view") mi = m_ViewMap.find(ID);
		if (Type == "hotel") mi = m_HotelMap.find(ID);
		if (Type == "restaurant") mi = m_RestMap.find(ID);
		if (Type == "shop") mi = m_ShopMap.find(ID);
		if (Type == "activity") mi = m_ActivityMap.find(ID);
		if ((mi != m_ViewMap.end()) && (mi != m_HotelMap.end()) && (mi != m_RestMap.end()) && (mi != m_ShopMap.end()) && (mi != m_ActivityMap.end())) {
			return mi->second;
		}
		return NULL;
	}

	int Release() {
		for (int i = TRAFFICLINE_TYPE_BEGIN; i < TRAFFICLINE_TYPE_COUNT; i ++) {
                        for (int j = 1; j < m_TrafficLines[i].size(); j ++) {
                                delete m_TrafficLines[i][j];
                        }
                        m_TrafficLines[i].clear();
                }

		for (int i = 1; i < m_ID2Node.size(); i ++) {
			delete m_ID2Node[i];
		}
		m_ID2Node.clear();

		if (m_QuadTree) m_QuadTree->Release();

		std::tr1::unordered_map<std::string, EntityNode*>::iterator mi;
		for (mi = m_ViewMap.begin(); mi != m_ViewMap.end(); mi ++) {
			delete mi->second;
		}
		for (mi = m_HotelMap.begin(); mi != m_HotelMap.end(); mi ++) {
                        delete mi->second;
                }
		for (mi = m_RestMap.begin(); mi != m_RestMap.end(); mi ++) {
                        delete mi->second;
                }
		for (mi = m_ShopMap.begin(); mi != m_ShopMap.end(); mi ++) {
                        delete mi->second;
                }
		for (mi = m_ActivityMap.begin(); mi != m_ActivityMap.end(); mi ++) {
                        delete mi->second;
                }
		return RUN_OK;
	}

	int SetNodeLinkable(int IDA, int IDB) {
		char buff[100];
/*                if (IDA < IDB) snprintf(buff, 100, "%d|%d", IDA, IDB);
                else snprintf(buff, 100, "%d|%d", IDB, IDA);*/
		snprintf(buff, 100, "%d|%d", IDA, IDB);
		m_NodeLinkable.insert(buff);
		return RUN_OK;
	}

	bool IsNodeLinkable(int IDA, int IDB) {
		char buff[100];
/*		if (IDA < IDB) snprintf(buff, 100, "%d|%d", IDA, IDB);
		else snprintf(buff, 100, "%d|%d", IDB, IDA);*/
		snprintf(buff, 100, "%d|%d", IDA, IDB);
		if (m_NodeLinkable.find(buff) == m_NodeLinkable.end()) return false;
		return true;
	}

	bool ExpandByTrafficePath(TrafficPath* Path, int NewID, int NewTrafficType) {
                for (int i = 0; i < Path->m_TrafficList.size(); i ++) {
                        MJ::PrintInfo::PrintDbg("City::ExpandByTrafficePath, PathItem:%d, ptype:%s, plineid:%d, ntype:%s, nid:%d",
                                i, TRAFFIC_TYPE_NAME[Path->m_TrafficList[i]->m_TrafficType], Path->m_TrafficList[i]->m_ID, 
				TRAFFIC_TYPE_NAME[TRAFFICLINE_TYPE_MAP[NewTrafficType]], NewID);
                        if (((Path->m_TrafficList[i]->m_TrafficType == TRAFFIC_TYPE_BUS) || (Path->m_TrafficList[i]->m_TrafficType == TRAFFIC_TYPE_SUBWAY)) &&
                                ((NewTrafficType == TRAFFICLINE_TYPE_BUS) || (NewTrafficType == TRAFFICLINE_TYPE_SUBWAY)) &&
                                (IsNodeLinkable(Path->m_TrafficList[i]->m_Start->m_MapNode->m_ID, NewID)))
                                return true;
                }
                return false;
    }
	
	int LoadTrafficLineData(const char* FileName, int TrafficType);		// ID start from 1
	int LoadViewData(const char* FileName, int ViewType);
	int LoadRestaurantData(const char* FileName, int RestaurantType);
	int LoadHotelData(const char* FileName, int HotelType);
	int LoadAirportData(const char* FileName, int AirportType);
	int LoadShopData(const char* FileName, int ShopType);
	int LoadActivityData(const char* FileName, int ActivityType);
	int GetPath(int timeblock,int Cid,int TrafficPrefer, int DepartTime, std::tr1::unordered_map<int,PathSet > & pathes,int weekday);

	MapNode* GetMapNode(int ID) {
		if ((ID >= 0) && (ID < m_ID2Node.size())) return m_ID2Node[ID];
		MJ::PrintInfo::PrintErr("City::GetMapNode, city[%s], undefined nodeID:%d", m_Name.c_str(), ID);
		return NULL;
	}

/*	TrafficItem* GetTraffic(int IDA, int IDB, double Cost, int TimeCost, int TrafficType, int TrafficLineID) {
		MapNode* mapNodeA = NULL;
		MapNode* mapNodeB = NULL;
		if ((mapNodeA = GetMapNode(IDA)) && (mapNodeB = GetMapNode(IDB))) {
			return GetTraffic(mapNodeA, mapNodeB, TrafficType, Cost, TimeCost, TrafficLineID);
		}
		return NULL;
	}*/

	TrafficItem* GetTraffic(EntityNode* NodeA, EntityNode* NodeB, double Cost, int TimeCost, int TrafficType, int TrafficLineID, int Time, int WaitTime) {
		if ((!NodeA) || (!NodeB)) {
			MJ::PrintInfo::PrintErr("City::GetTraffic, city[%s], input node is null", m_Name.c_str());
			return NULL;
		}
		
		TrafficItem* trItem = new TrafficItem;
		trItem->SetPointsOld(NodeA, NodeB, Cost, TimeCost, TrafficType, TrafficLineID, Time, WaitTime);
		return trItem;
	}

	int Dump() {
		MJ::PrintInfo::PrintDbg("TrafficPath::Dump, city:%d(%s), leftTop(x:%.2f, y:%.2f), rightBot(x:%.2f, y:%.2f), TimeZone:%d", m_ID, m_Name.c_str(), 
			m_LeftTop.m_X, m_LeftTop.m_Y, m_RightBot.m_X, m_RightBot.m_Y, m_TimeZone);
/*		for (int i = TRAFFICLINE_TYPE_BEGIN; i < TRAFFICLINE_TYPE_COUNT; i ++) {
			MJ::PrintInfo::PrintDbg("TrafficPath::Dump, TrafficPath type:%s, path size:%d", TRAFFICLINE_TYPE_NAME[i], m_TrafficLines[i].size());
                        for (int j = 1; j < m_TrafficLines[i].size(); j ++) {
				m_TrafficLines[i][j]->Dump();
                        }
                }*/
		return 0;
	}
};

#endif		//__LY_CITY_H__
