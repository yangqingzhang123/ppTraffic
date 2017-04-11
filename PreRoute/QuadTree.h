#ifndef _QUADTREE_H_
#define _QUADTREE_H_
#include "define.h"
#include <queue>
#include <math.h>
#define QUADMAXLEVEL 10
#define NODEMAXELEMENT 50
#define NODETYPE_LEAF 0
#define NODETYPE_MIDDLE 1

const double q_EARTH_RADIUS = 6371.004;

class QuadTree {			// mercator coordinate-system
	public:
		QuadTree(Coords& LeftTop, Coords& RightBot, int Level) {
			m_LeftTop = LeftTop;
			m_RightBot = RightBot;
			m_Center.m_X = (m_LeftTop.m_X + m_RightBot.m_X)/2;
			m_Center.m_Y = (m_LeftTop.m_Y + m_RightBot.m_Y)/2;

			m_Level = Level;
			m_NodeType = NODETYPE_LEAF;
			for (int i = 0; i < 4; i ++) m_Childs[i] = NULL;

			Release();
		}
		virtual ~QuadTree() {
			Release();
		}

		int Release() {
			m_NodeList.clear();
			m_NodeCount = 0;
			for (int i = 0; i < 4; i ++) {
				if (m_Childs[i]) {
					m_Childs[i]->Release();
					delete m_Childs[i];
					m_Childs[i] = NULL;
				}
			}
			return RUN_OK;
		}

		int GetChildRange(int ChildIdx, Coords& LeftTop, Coords& RightBot) {
			if (ChildIdx == 3) { LeftTop = m_LeftTop; RightBot = m_Center; }
			if (ChildIdx == 2) { LeftTop.m_X = m_Center.m_X; LeftTop.m_Y = m_LeftTop.m_Y; RightBot.m_X = m_RightBot.m_X; RightBot.m_Y = m_Center.m_Y; }
			if (ChildIdx == 0) { LeftTop = m_Center; RightBot = m_RightBot; }
			if (ChildIdx == 1) { LeftTop.m_X = m_LeftTop.m_X; LeftTop.m_Y = m_Center.m_Y; RightBot.m_X = m_Center.m_X; RightBot.m_Y = m_RightBot.m_Y; }
			return RUN_OK;
		}

		// -  - - > x
		// | 3 2
		// | 1 0
		int GetChildIdx(Coords& NewCoords) {
            int ret = 0;
            if (NewCoords.m_X <= m_Center.m_X) ret |= 0x00000001;
            if (NewCoords.m_Y <= m_Center.m_Y) ret |= 0x00000002;
            return ret;
		}

/*		int GetChildIdx(MapNode* NewNode) {
			if (!NewNode) {
				MJ::PrintInfo::PrintErr("QuadTree::GetChildIdx, null node when getting childIdx");
				return RUN_ERR;
			}
			return GetChildIdx(NewNode->m_Coords);
		}*/

		int InsertNode(EntityNode* NewNode, Coords& NewCoords) {
			int childIdx = RUN_ERR;
			m_NodeCount ++;
			Coords leftTop;
			Coords rightBot;
			if (m_NodeType == NODETYPE_MIDDLE) {					// go down
				childIdx = GetChildIdx(NewCoords);
				if (childIdx == RUN_ERR) {
					MJ::PrintInfo::PrintErr("QuadTree::InsertNode, error when geting childIdx");
					return RUN_ERR;
				}
				GetChildRange(childIdx, leftTop, rightBot);
				if (!m_Childs[childIdx]) m_Childs[childIdx] = new QuadTree(leftTop, rightBot, m_Level + 1);
				int ret = RUN_OK;
				if ((ret = m_Childs[childIdx]->InsertNode(NewNode, NewCoords)) > 0) {
					m_NodeCount --;
					return ret;
				}
				return ret;
			}

			double dist = 0;
			int typeCmp = 0;
			int t_size = m_NodeList.size();
			for (int i = 0; i < t_size; i ++) {
				if ((NewNode->m_Type != m_NodeList[i]->m_Type) || (NewNode->m_EntityType != m_NodeList[i]->m_EntityType)) continue; 
				dist = TrafficCost::GetDist(NewNode->m_MapNode, m_NodeList[i]->m_MapNode, 0);
				if (((dist - POI_MERGE_THRES) < ERR_THRES) && (NewNode->m_Name == m_NodeList[i]->m_Name)) {			// duplicate node, must be same type
//					MJ::PrintInfo::PrintDbg("QuadTree::InsertNode, hit exist, name:[%s], thres:%.2f", NewNode->m_Name.c_str(), dist);
					m_NodeCount --;
					return m_NodeList[i]->m_MapNode->m_ID;
				}
			}
			m_NodeList.push_back(NewNode);
			if (m_NodeList.size() > NODEMAXELEMENT) {
				if (m_Level < QUADMAXLEVEL) {					// split tree node
					m_NodeType = NODETYPE_MIDDLE;
					for (int i = 0; i < m_NodeList.size(); i ++) {
						Coords xyCoords;
						Coordinate::LatLon2XY(m_NodeList[i]->m_MapNode->m_Coords, xyCoords);
						childIdx = GetChildIdx(xyCoords);
						GetChildRange(childIdx, leftTop, rightBot);
						if (!m_Childs[childIdx]) m_Childs[childIdx] = new QuadTree(leftTop, rightBot, m_Level + 1);
						m_Childs[childIdx]->InsertNode(m_NodeList[i], xyCoords);
					}
					m_NodeList.clear();
					return RUN_OK;
				}
			}
			return RUN_OK;
		}

		int GetNodeList(Coords& Point, Coords& PointXY, std::string& NodeName, double Radius, int Count, std::priority_queue<NodePair<EntityNode> >& Ret, int NodeType, int EntityType, int SortType) {		// use mercator-system
			int findCount = 0;
			if (m_NodeType == NODETYPE_MIDDLE) {
				for (int i = 0; i < 4; i ++) {
					if (m_Childs[i]) {
						bool isYHit = false;
						bool isXHit = false;
						if (PointXY.m_Y - Radius <= m_Childs[i]->m_LeftTop.m_Y) {
							if (PointXY.m_Y + Radius > m_Childs[i]->m_LeftTop.m_Y) isYHit = true;
						} else if (PointXY.m_Y - Radius <= m_Childs[i]->m_RightBot.m_Y) isYHit = true;
						if (PointXY.m_X - Radius <= m_Childs[i]->m_LeftTop.m_X) {
							if (PointXY.m_X + Radius > m_Childs[i]->m_LeftTop.m_X) isXHit = true;
						} else if (PointXY.m_X - Radius <= m_Childs[i]->m_RightBot.m_X) isXHit = true;
						
						if (isXHit && isYHit) {
							int ret = m_Childs[i]->GetNodeList(Point, PointXY, NodeName, Radius, Count, Ret, NodeType, EntityType, SortType);
							if (ret != RUN_ERR) findCount += ret;
						}
					}
				}
			} else {
				double score = 0;
				double dist = 0;
				int t_size = m_NodeList.size();
				double radAX,radAY,radBX,radBY,value;
				for (int i = 0; i < t_size; i ++) {
					findCount ++;
					double distForErr = 0;
					if (NodeName != m_NodeList[i]->m_Name) distForErr = DIST_FOR_ERR;
					//dist = TrafficCost::GetDist(Point, m_NodeList[i]->m_MapNode->m_Coords, distForErr);
					radAX = Point.m_X*PI/180;
					radAY = Point.m_Y*PI/180;
					radBX = m_NodeList[i]->m_MapNode->m_Coords.m_X*PI/180;
					radBY = m_NodeList[i]->m_MapNode->m_Coords.m_Y*PI/180;
					value = cos(radAY)*cos(radBY)*cos(radBX - radAX) + sin(radAY)*sin(radBY);
					if(value > 1) value = 1.0;
					if(value < -1) value = -1.0;
					value = q_EARTH_RADIUS*acos(value);
					if( value < distForErr) value = distForErr;
					score = 0;
					dist = value;
/*					if (("MERCEDE" == m_NodeList[i]->m_Name)||("DUE MACELLI- CAPO LE CASE"== m_NodeList[i]->m_Name))  {
						MJ::PrintInfo::PrintDbg("QuadTree::GetNodeList, (%s), dist:%.5f", m_NodeList[i]->m_Name.c_str(), dist);
					}*/
					if (((NodeType & (1<<m_NodeList[i]->m_Type)) > 0) && ((EntityType & (1<<m_NodeList[i]->m_EntityType)) > 0) && 
							(dist < Radius)) {
						score = - Ranker::LBSRanker(m_NodeList[i], SortType, dist);
//						MJ::PrintInfo::PrintDbg("QuadTree::GetNodeList, (%s), score:%.5f", m_NodeList[i]->m_Name.c_str(), score);
						Ret.push(NodePair<EntityNode>(m_NodeList[i], score));
					}
				}
			}
			return findCount;
		}

		int DumpCurNodeInfo() {
			if (m_NodeType == NODETYPE_MIDDLE) {
				MJ::PrintInfo::PrintDbg("QuadTree::Dump, [MiddNode], Level:%d, LeftTop:[%.5f,%.5f], RightBot:[%.5f,%.5f], nodeCount:%d", m_Level,
						m_LeftTop.m_X, m_LeftTop.m_Y, m_RightBot.m_X, m_RightBot.m_Y, m_NodeCount);
			} else {
				MJ::PrintInfo::PrintDbg("QuadTree::Dump, [LeafNode], Level:%d, LeftTop:[%.5f,%.5f], RightBot:[%.5f,%.5f], nodeCount:%d, leafnodes:", m_Level,
						m_LeftTop.m_X, m_LeftTop.m_Y, m_RightBot.m_X, m_RightBot.m_Y, m_NodeCount);
				for (int i = 0; i < m_NodeList.size(); i ++) {
					MJ::PrintInfo::PrintDbg("QuadTree::Dump, leafnode, %s", m_NodeList[i]->m_Name.c_str());
					m_NodeList[i]->Dump();
				}
			}
			return 0;
		}

		int DumpChildInfo() {
			for (int i = 0; i < 4; i ++) {
				if (!m_Childs[i]) {
					MJ::PrintInfo::PrintDbg("QuadTree::Dump, childs:%d, NULL", i);
				} else {
					MJ::PrintInfo::PrintDbg("QuadTree::Dump, childs:%d, LeftTop:[%.5f,%.5f], RightBot:[%.5f,%.5f], Level:%d, nodeCount:%d", i,
							m_Childs[i]->m_LeftTop.m_X, m_Childs[i]->m_LeftTop.m_Y, m_Childs[i]->m_RightBot.m_X, m_Childs[i]->m_RightBot.m_Y,
							m_Childs[i]->m_Level, m_Childs[i]->m_NodeCount);
				}
			}
			return 0;
		}

		int Dump() {
			MJ::PrintInfo::PrintDbg("QuadTree::Dump, Dumping quadtree");
			DumpCurNodeInfo();
			DumpChildInfo();
			for (int i = 0; i < 4; i ++) {
				if (!m_Childs[i]) continue;
				m_Childs[i]->Dump();
			}
			return RUN_OK;
		}

		int m_Level;
		int m_NodeType;
		int m_NodeCount;

		Coords m_LeftTop;
		Coords m_Center;
		Coords m_RightBot;
		QuadTree* m_Childs[4];

		std::vector<EntityNode*> m_NodeList;
};

#endif
