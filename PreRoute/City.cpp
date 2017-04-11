#include "City.h"
#include "common/MyThreadPool.h"
#include <iostream>
#include "LBSAlgorithm.h"
#include "LYConstData.h"
#include <algorithm>
#include <cstdlib>
#include <time.h>
#include <stdio.h>
#include "define.h"
#define NEED_ORIGINAL_DATA
#define NEED_BIN_DATA
#define RUN_ONE_STARTn
#define PRINT_DEBUG_INFOn
#define DEBUG_AMOUNT_NUMn
#define Tf double
static pthread_mutex_t  file_write_mutex=PTHREAD_MUTEX_INITIALIZER;
class BINPART2{
    public:
        char m_TrafficType;
        float m_Cost;
        int m_TimeCost;
        float m_Dist;
        int lenOfName;
        char* m_Name;
        int m_Step;
        int m_Area1;
        int m_Area2;
        int lenOfEndName;
        char* m_End_Name;
        BINPART2(){
            m_Name=NULL;
            m_End_Name=NULL;
        }
        ~BINPART2(){
            if(m_Name!=NULL)
                delete []m_Name;
            if(m_End_Name!=NULL)
                delete []m_End_Name;
            m_Name=NULL;
            m_End_Name=NULL;

        }
};

class BinInfo{
    public:
        /*
           压缩格式：
           当前一条数据字节数
           %xd

           part1：（线路总体信息）
           :起点坐标x,y/终点坐标x,y/距离/时间/步行距离/线路类型打分/换乘次数
           %xf%xf%xf%xf%xf%xd%xf%xf%xd

           part2：（线路详细信息）
           type/价格/时间/距离/线路文本名字节数/线路名文本/站数/Z1/Z2/车站名字节数/车站名文本
           %xc%xf%xd%xf%xd字符%xd%xd%xd%xd字符
           %xc%xf%xd%xf%xd字符%xd%xd%xd%xd字符
           ……

           part3：（线路坐标）
           坐标数
           %xd
           坐标x,y/标号
           %xf%xf%xd
           %xf%xf%xd
           %xf%xf%xd
           ……
           */    
        //part 1:

        int lenOfInfo;
        double start_X,start_Y,end_X,end_Y;
        float m_Dist;
        int m_TimeCost;
        float m_WalkDist;
        float m_Score;
        int m_TransNum;

        //part2:

        BINPART2 *discriptList;

        //part3:

        int numberOfCoords;
        char *coord_index;
        int *bin_X;
        int *bin_Y;

        BinInfo(){
            discriptList=NULL;
            bin_X=NULL;
            bin_Y=NULL;
            coord_index=NULL;

        }
        ~BinInfo(){
            if(discriptList!=NULL)
                delete []discriptList;
            if(bin_X!=NULL)
                delete []bin_X;
            if(bin_Y!=NULL)
                delete []bin_Y;
            if(coord_index!=NULL)
                delete []coord_index;
        }
        void init(){
#ifdef PRINT_DEBUG_INFO
            printf("start Info Init\t");
#endif
            if(discriptList!=NULL){
                delete []discriptList;
                discriptList=NULL;
            }	
            if(coord_index!=NULL){
                delete []coord_index;
                coord_index=NULL;
            }
            if(bin_X!=NULL){
                delete []bin_X;
                bin_X=NULL;
            }
            if(bin_Y!=NULL){
                delete []bin_Y;
                bin_Y=NULL;
            }
#ifdef PRINT_DEBUG_INFO
            printf("info inited\t");
#endif
        }


        string dumpToBinString(){
            string ret;
            char rettmp[512];	
            int curcnt=0;
#ifdef PRINT_DEBUG_INFO
            printf("Writing ");
#endif
            int d=sizeof(int),c=sizeof(char),f=sizeof(float);

            memcpy(rettmp,&lenOfInfo,d);curcnt+=d;
            memcpy(rettmp+curcnt,&start_X,sizeof(double));curcnt+=sizeof(double);
            memcpy(rettmp+curcnt,&start_Y,sizeof(double));curcnt+=sizeof(double);
            memcpy(rettmp+curcnt,&end_X,sizeof(double));curcnt+=sizeof(double);
            memcpy(rettmp+curcnt,&end_Y,sizeof(double));curcnt+=sizeof(double);
            ret+=string(rettmp,curcnt);curcnt=0;
            memcpy(rettmp+curcnt,&m_Dist,f);curcnt+=f;
            memcpy(rettmp+curcnt,&m_TimeCost,d);curcnt+=d;
            memcpy(rettmp+curcnt,&m_WalkDist,f);curcnt+=f;
            memcpy(rettmp+curcnt,&m_Score,f);curcnt+=f;
            memcpy(rettmp+curcnt,&m_TransNum,d);curcnt+=d;
            ret+=string(rettmp,curcnt);curcnt=0;
#ifdef PRINT_DEBUG_INFO
            printf("Part 1 Wrote  ");
#endif
            for(int i=0;i<m_TransNum;i++){


                memcpy(rettmp+curcnt,&discriptList[i].m_TrafficType,c);curcnt+=c;
                memcpy(rettmp+curcnt,&discriptList[i].m_Cost,f);curcnt+=f;
                memcpy(rettmp+curcnt,&discriptList[i].m_TimeCost,d);curcnt+=d;
                memcpy(rettmp+curcnt,&discriptList[i].m_Dist,f);curcnt+=f;
                memcpy(rettmp+curcnt,&discriptList[i].lenOfName,d);curcnt+=d;
                ret+=string(rettmp,curcnt);curcnt=0;
                ret+=string(discriptList[i].m_Name,discriptList[i].lenOfName);
                memcpy(rettmp+curcnt,&discriptList[i].m_Step,d);curcnt+=d;
                memcpy(rettmp+curcnt,&discriptList[i].m_Area1,d);curcnt+=d;
                memcpy(rettmp+curcnt,&discriptList[i].m_Area2,d);curcnt+=d;
                memcpy(rettmp+curcnt,&discriptList[i].lenOfEndName,d);curcnt+=d;	
                ret+=string(rettmp,curcnt);curcnt=0;
                ret+=string(discriptList[i].m_End_Name,discriptList[i].lenOfEndName);	
            }
#ifdef PRINT_DEBUG_INFO
            printf("part 2 wrote  ");
#endif
            memcpy(rettmp+curcnt,&numberOfCoords,d);curcnt+=d;
            ret+=string(rettmp,curcnt);curcnt=0;
            for(int i=0;i<numberOfCoords;i++){
                memcpy(rettmp+curcnt,bin_X+i,sizeof(int));curcnt+=sizeof(int);
                memcpy(rettmp+curcnt,bin_Y+i,sizeof(int));curcnt+=sizeof(int);
                memcpy(rettmp+curcnt,coord_index+i,c);curcnt+=c;
                ret+=string(rettmp,curcnt);curcnt=0;	
            }
#ifdef PRINT_DEBUG_INFO
            printf("part 3 wrote  ");
#endif

            return ret;
        }




        int putInFile(char *fileName){
            FILE *fp=fopen(fileName,"ab+");
#ifdef PRINT_DEBUG_INFO
            printf("Writing ");
#endif
            if(fp==NULL){
                printf("Open file: %s ERROR!",fileName);
                return -1;
            }
            int d=sizeof(int),c=sizeof(char),f=sizeof(float);

            fwrite(&lenOfInfo,d,1,fp);
            fwrite(&start_X,sizeof(double),1,fp);
            fwrite(&start_Y,sizeof(double),1,fp);
            fwrite(&end_X,sizeof(double),1,fp);
            fwrite(&end_Y,sizeof(double),1,fp);
            fwrite(&m_Dist,f,1,fp);
            fwrite(&m_TimeCost,d,1,fp);
            fwrite(&m_WalkDist,f,1,fp);
            fwrite(&m_Score,f,1,fp);
            fwrite(&m_TransNum,d,1,fp);
#ifdef PRINT_DEBUG_INFO
            printf("Part 1 Wrote  ");
#endif
            for(int i=0;i<m_TransNum;i++){
                fwrite(&discriptList[i].m_TrafficType,c,1,fp);
                fwrite(&discriptList[i].m_Cost,f,1,fp);
                fwrite(&discriptList[i].m_TimeCost,d,1,fp);
                fwrite(&discriptList[i].m_Dist,f,1,fp);
                fwrite(&discriptList[i].lenOfName,d,1,fp);//cout<<"Name1"<<endl;
                fwrite(discriptList[i].m_Name,c,discriptList[i].lenOfName,fp);//cout<<"Name2"<<endl;
                fwrite(&discriptList[i].m_Step,d,1,fp);
                fwrite(&discriptList[i].m_Area1,d,1,fp);
                fwrite(&discriptList[i].m_Area2,d,1,fp);
                fwrite(&discriptList[i].lenOfEndName,d,1,fp);
                fwrite(discriptList[i].m_End_Name,c,discriptList[i].lenOfEndName,fp);
            }
#ifdef PRINT_DEBUG_INFO
            printf("part 2 wrote  ");
#endif
            fwrite(&numberOfCoords,d,1,fp);
            for(int i=0;i<numberOfCoords;i++){
                fwrite(bin_X+i,sizeof(int),1,fp); 
                fwrite(bin_Y+i,sizeof(int),1,fp);
                fwrite(coord_index+i,c,1,fp);
            }
#ifdef PRINT_DEBUG_INFO
            printf("part 3 wrote  ");
#endif
            fclose(fp);
            return 0;
        }
        int copyStringToChar(char **chr,std::string st){
#ifdef PRINT_DEBUG_INFO
            printf("[%s]",st.c_str());
#endif
            if(chr==NULL) return -1;
            if(*chr!=NULL){
                delete [](*chr);
            }
            *chr=new char[st.size()+1];
            if(*chr==NULL) return -2;

            for(int i=0;i<st.size();i++)
                (*chr)[i]=st[i];
            (*chr)[st.size()]='\0';

            return 0;
        }
        void getInfoByPath(MapNode *_start,MapNode *_end,TrafficPath *curPath,int Cid){        
            init();
            const char *buffer;
            std::string DbgStr="";
            std::string disStr="";
            std::string coordsStr="";
            std::string strtmp="";
            int cnt=0;
            size_t strPos;
            size_t prePos;
            lenOfInfo=0;
            // 		curPath->Normalize();
            start_X   =_start->m_Coords.m_X;
            start_Y   =_start->m_Coords.m_Y;
            end_X     =_end->m_Coords.m_X;
            end_Y     =_end->m_Coords.m_Y;
            m_Dist    = curPath->m_Dist;
            m_TimeCost=curPath->m_TimeCost;
            m_WalkDist=curPath->m_WalkDist;
            m_Score   =curPath->GetPreferScore(REFINE_TYPE);
            m_TransNum =curPath->m_TrafficList.size()-1;
            lenOfInfo+=sizeof(float)*3+sizeof(double)*4+sizeof(int)*2;
            if(m_TransNum>0)
                discriptList=new BINPART2[m_TransNum];
            for(int i=1;i<curPath->m_TrafficList.size();i++){
                TrafficItem *pItem=curPath->m_TrafficList[i];
                discriptList[i-1].m_TrafficType=pItem->m_TrafficType;
                discriptList[i-1].m_Cost=pItem->m_Cost;
                discriptList[i-1].m_TimeCost=pItem->m_TimeCost;
                discriptList[i-1].m_Dist=pItem->m_Dist;
                discriptList[i-1].lenOfName=strlen(pItem->m_Name.c_str());
                copyStringToChar(&discriptList[i-1].m_Name,pItem->m_Name) ;
                discriptList[i-1].m_Step=pItem->m_Step;
                discriptList[i-1].m_Area1=pItem->m_Start->m_Area;
                discriptList[i-1].m_Area2=pItem->m_End->m_Area;
                discriptList[i-1].lenOfEndName=1+strlen(pItem->m_Start->m_Name.c_str())+strlen(pItem->m_End->m_Name.c_str());
                std::string stmp=pItem->m_Start->m_Name+"~"+pItem->m_End->m_Name;
                copyStringToChar(&discriptList[i-1].m_End_Name,stmp);
                lenOfInfo+=sizeof(char)*(discriptList[i-1].lenOfName+discriptList[i-1].lenOfEndName+1)+sizeof(int)*6+sizeof(float)*2;
            }
            curPath->ToString(DbgStr,LYConstData::m_ID2CityMap[Cid]->m_TrafficLines);
            strPos=DbgStr.find("\t");
            disStr=DbgStr.substr(0,strPos);
            coordsStr=DbgStr.substr(strPos+1,DbgStr.size()-strPos-1);
            prePos=0;
            strPos=coordsStr.find("|");
            numberOfCoords=0;
            while(strPos!=std::string::npos){
                numberOfCoords++;
                prePos=strPos+1;
                strPos=coordsStr.find("|",prePos);
            }
            if(coordsStr.size()-prePos>5)
                numberOfCoords++;           
            prePos=0;
            strPos=coordsStr.find("|");
            bin_X=new int[numberOfCoords];
            bin_Y=new int[numberOfCoords];
            coord_index=new char[numberOfCoords];
            while(strPos!=std::string::npos){
                strtmp=coordsStr.substr(prePos,strPos-prePos);

                int x;
                int bx1,bx2,by1,by2;
                size_t dotpos1=strtmp.find(".");
                size_t dotpos2=strtmp.find(".",dotpos1+1);
                buffer=(strtmp.substr(0,dotpos1)).c_str();
                sscanf(buffer,"%d",&bx1);
                buffer=(strtmp.substr(dotpos1+1,dotpos2)).c_str();
                sscanf(buffer,"%d,%d",&bx2,&by1);
                buffer=(strtmp.substr(dotpos2+1,strtmp.size()-dotpos2-1)).c_str();
                sscanf(buffer,"%d::%d",&by2,&x);
                int sign1=1,sign2=1;
                if(strtmp[0]=='-') sign1=-1;
                if(strtmp.find("-",dotpos1)!=std::string::npos) sign2=-1;
                if(bx1<0) bx1=-bx1;
                if(by1<0) by1=-by1;
                bin_X[cnt]=sign1*(bx1*10000000+bx2);
                bin_Y[cnt]=sign2*(by1*10000000+by2);
                coord_index[cnt]=x;                      
                prePos=strPos+1;
                cnt++;
                strPos=coordsStr.find("|",prePos);
#ifdef PRINT_DEBUG_INFO
                printf("getp3  ");
#endif
            }
            if(cnt<numberOfCoords){
                strtmp=coordsStr.substr(prePos,coordsStr.size());
                int x;
                int bx1,bx2,by1,by2;
                size_t dotpos1=strtmp.find(".");
                size_t dotpos2=strtmp.find(".",dotpos1+1);
                buffer=(strtmp.substr(0,dotpos1)).c_str();
                sscanf(buffer,"%d",&bx1);
                buffer=(strtmp.substr(dotpos1+1,dotpos2)).c_str();
                sscanf(buffer,"%d,%d",&bx2,&by1);
                buffer=(strtmp.substr(dotpos2+1,strtmp.size()-dotpos2-1)).c_str();
                sscanf(buffer,"%d::%d",&by2,&x);			
                int sign1=1,sign2=1;
                if(strtmp[0]=='-') sign1=-1;
                if(strtmp.find("-",dotpos1)!=std::string::npos) sign2=-1;
                if(bx1<0) bx1=-bx1;
                if(by1<0) by1=-by1;
                bin_X[cnt]=sign1*(bx1*10000000+bx2);
                bin_Y[cnt]=sign2*(by1*10000000+by2);  
                coord_index[cnt]=x;
                prePos=strPos+1;
                cnt++;
                strPos=coordsStr.find("|",prePos);
            }
            lenOfInfo+=(sizeof(int)*2+sizeof(char))*(numberOfCoords)+sizeof(int);
        }   

};


//LBSAlgorithm::Search::ASearch(this,curNode, nodeLmt, maxDepth, DepartTime+21600*add_Time, (DepartTime+21600*add_Time)%86400, pathes,dis_index_array);
class MyWoker: public Worker{
    public:
        MyWoker(City *city,EntityNode *node,int nodelmt,int maxdepth,int departtime,int weekday):_city(city),_node(node),_nodelmt(nodelmt),_maxdepth(maxdepth),_departtime(departtime),_weekday(weekday){}
        virtual int doWork(FILE * file);
        virtual ~MyWoker(){}
    private:
        City *_city;
        EntityNode *_node;
        int _nodelmt;
        int _maxdepth;
        int _departtime;
        int _weekday;
};
int MyWoker::doWork(FILE * file) {
    std::tr1::unordered_map<string,std::tr1::unordered_set<string> >fiter_different_time;
    fiter_different_time.clear();
    std::tr1::unordered_map<int, PathSet > pathes;
    std::tr1::unordered_map<int, PathSet >::iterator iter;
    std::string DbgStr;
    BinInfo bintest;
    char buff[256];
    for(int add_Time=0;add_Time<4;add_Time++)
    {
        LBSAlgorithm::Search::ASearch(_city,_node, _nodelmt, _maxdepth, _departtime+21600*add_Time, (_departtime+21600*add_Time)%86400,pathes);
        if (!pathes.empty()) {
            for(iter=pathes.begin();iter!=pathes.end();iter++){
                if ((*iter).second.m_PathCount==0)
                    continue;
                DbgStr="";
                for (int i=0;i<(*iter).second.m_PathCount;i++){
                    (*iter).second.m_Pathes[i]->Normalize();
                    snprintf(buff, sizeof(buff), "%.7f,%.7f###%.7f,%.7f",_node->m_MapNode->m_Coords.m_X,_node->m_MapNode->m_Coords.m_Y,(* (_city->m_NID2EPMap[NODE_TYPE_STATION][(*iter).first].begin()))->m_MapNode->m_Coords.m_X,(*(_city->m_NID2EPMap[NODE_TYPE_STATION][(*iter).first].begin()))->m_MapNode->m_Coords.m_Y);
                    string mapKey=buff;
                    DbgStr+=buff;
                    snprintf(buff, sizeof(buff), "###%.3f#%d#%.3f#%.2f\t",(*iter).second.m_Pathes[i]->m_Dist,(*iter).second.m_Pathes[i]->m_TimeCost,(*iter).second.m_Pathes[i]->m_WalkDist,(*iter).second.m_Pathes[i]->m_TransNum);
                    DbgStr+= buff;
                    (*iter).second.m_Pathes[i]->ToString(DbgStr, LYConstData::m_ID2CityMap[_city->m_ID]->m_TrafficLines);
                    bintest.getInfoByPath(_node->m_MapNode,(* (_city->m_NID2EPMap[NODE_TYPE_STATION][(*iter).first].begin()))->m_MapNode,(*iter).second.m_Pathes[i],_city->m_ID);
                    string path_bin_string=bintest.dumpToBinString();
                    fiter_different_time[mapKey].insert(path_bin_string);
                    delete (*iter).second.m_Pathes[i];
                }
            }
            pathes.clear();
        }
        std::tr1::unordered_map<string,std::tr1::unordered_set<string> >::iterator mpitem;
        std::tr1::unordered_set<string>::iterator stitem;
        for(mpitem=fiter_different_time.begin();mpitem!=fiter_different_time.end();++mpitem){
            for(stitem=mpitem->second.begin();stitem!=mpitem->second.end();++stitem){
                fwrite((*stitem).data(),sizeof(char),(*stitem).length(),file);
            }
        }
    }
}

struct  path_info{
    double first_coord[2];
    double end_coord[2];
    float path_dis;
    int time_cost;
    float walk_dis;
    float path_type_score;
    vector<struct path_item>info_items;
    vector<struct path_coord>info_coords;
};
struct path_item{
    char  traffic_type[16];
    float price;
    int time_cost;
    float item_dist;
    char  line_name[128];
    int steps;
    int  up_circle;
    int down_circle;
    char  up_station[128];
    char  down_station[128];
};
struct path_coord{
    double station_coord[2];
    int  coord_index;
};



void ptstruct(struct path_info cur){
    printf("pathInfo:\n");
    printf("part1: %f,%f|%f,%f|%f|%d|%f|%f|%f|%d\n",cur.first_coord[0],cur.first_coord[1],cur.end_coord[0],cur.end_coord[1],cur.path_dis,cur.time_cost,cur.walk_dis,cur.path_type_score,cur.info_items.size());
    printf("part2:\n");

    for(int i=0;i<cur.info_items.size();i++){
        cout<<i<<endl;
        struct path_item a=cur.info_items[i];
        printf("%s|%f|%d|%f|%s|%d|%d|%d|%s|%s||\n",a.traffic_type,a.price,a.time_cost,a.item_dist,a.line_name,a.steps,a.up_circle,a.down_circle,a.up_station,a.down_station);
    }
    printf("part3: %d\n",cur.info_coords.size());
    for(int i=0;i<cur.info_coords.size();i++){
        struct path_coord a=cur.info_coords[i];
        printf("%f,%f::%d|",a.station_coord[0],a.station_coord[1],a.coord_index);
    }
    printf("\n\n");


}


int read_file(FILE **fp,std::string &ret){  //return: error -1        full 0      success 1   
    int t=0;
    if(fp==NULL || *fp==NULL) return -1;
    int dataSize;
    if(feof(*fp)) return 0;
    if(fread(&dataSize,sizeof(int),1,*fp)==0) return 0;
    char *str = new char[dataSize+sizeof(int)+1];
    if(fread(str+sizeof(int)/sizeof(char),sizeof(char),dataSize,*fp)==0) return 0;
    memcpy(str,&dataSize,sizeof(int)/sizeof(char));
    ret=string(str,dataSize+sizeof(int));
    return 1;
}

int read_file_key(FILE **fp,std::string &ret){  //return: error -1        full 0      success 1   
    if(fp==NULL || *fp==NULL) return -1;
    float coords[4];
    int dataSize;
    if(feof(*fp)) return 0;
    if(fread(&dataSize,sizeof(int),1,*fp)==0) return 0;
    char *str = new char[dataSize];
    if(fread(str,sizeof(char),dataSize,*fp)==0) return 0;
    memcpy(coords,str,sizeof(float)*4);
    char buffer[256];
    sprintf(buffer,"%.7f,%.7f###%.7f,%.7f",coords[0],coords[1],coords[2],coords[3]);
    ret="";
    ret+=buffer;
    return 1;
}

int read_From_binInfo(const char *binInfo,struct path_info &res){
    int infoSize,TransNum,coordsNum;
    const char *p=binInfo;
    int d=sizeof(int)/sizeof(char),f=sizeof(float)/sizeof(char),c=1;
    memcpy(&infoSize,p,d);
    p+=d;
    memcpy(res.first_coord,p,f*5);
    p+=f*5;
    memcpy(&res.time_cost,p,d);
    p+=d;
    memcpy(&res.walk_dis,p,f*2);
    p+=f*2;
    memcpy(&TransNum,p,d);
    p+=d;
    for(int i=0;i<TransNum;i++){
        struct path_item cur;
        char trafficType;
        int lenOfName;
        char ctmp[256];
        const char *cptmp;
        size_t strpos;
        std::string stmp;
        memcpy(&trafficType,p,c);
        p+=c;
        memcpy(cur.traffic_type,TRAFFIC_TYPE_NAME[trafficType],1+strlen(TRAFFIC_TYPE_NAME[trafficType]));
        memcpy(&cur.price,p,f);
        p+=f;
        memcpy(&cur.time_cost,p,d);
        p+=d;
        memcpy(&cur.item_dist,p,f);
        p+=f;
        memcpy(&lenOfName,p,d);
        p+=d;
        memcpy(cur.line_name,p,lenOfName*c);
        p+=c*lenOfName;
        cur.line_name[lenOfName]='\0';
        memcpy(&cur.steps,p,d*3);
        p+=d*3;
        memcpy(&lenOfName,p,d);
        p+=d;
        memcpy(ctmp,p,lenOfName*c);
        p+=c*lenOfName;
        ctmp[lenOfName]='\0';
        stmp=std::string(ctmp);			
        strpos=stmp.find("~");
        cptmp=(stmp.substr(0,strpos)).c_str();
        memcpy(cur.up_station,cptmp,strlen(cptmp)+1);
        cptmp=(stmp.substr(strpos+1,sizeof(stmp)-strpos-1)).c_str();
        memcpy(cur.down_station,cptmp,strlen(cptmp)+1);
        res.info_items.push_back(cur);			
    }
    //cout<<"HAHAHAHA"<<endl;
    memcpy(&coordsNum,p,d);
    p+=d;
    for(int i=0;i<coordsNum;i++){
        struct path_coord cur;
        char id;
        memcpy(cur.station_coord,p,2*f);
        p+=2*f;
        memcpy(&id,p,c);
        p+=c;
        cur.coord_index=id;
        res.info_coords.push_back(cur);
    }	
    return 1;	
}


int bintoinfo(const char *infile,const char *outfile){
    FILE *fp=fopen(infile,"rb");
    std::string st="";
    while(!read_file(&fp,st)){
        //	cout<<"Hello\n";
        cout<<st.size()<<endl;
        struct path_info res;
        read_From_binInfo(st.c_str(),res);
        FILE *fout=fopen(outfile,"w+");
        //		cout<<res.first_coord[0]<<endl;
        fprintf(fout,"%.7f,%.7f###%.7f,%.7f",res.first_coord[0],res.first_coord[1],res.end_coord[0],res.end_coord[1]);
        fprintf(fout,"###%f#%d#%f#%.3f#\t",res.path_dis,res.time_cost,res.walk_dis,res.path_type_score);	
        for(int i=0;i<res.info_items.size();i++){
            path_item t=res.info_items[i];
            fprintf(fout,"%s#%.2f#%d#%.5f#%s#%d#%d~%d#%s~%s|",t.traffic_type,t.price,t.time_cost,t.item_dist,t.line_name,t.steps,t.up_circle,t.down_circle,t.up_station,t.down_station);
        }
        fprintf(fout,"/t");
        for(int i=0;i<res.info_coords.size();i++)
            fprintf(fout,"%.7f,%.7f::%d|",res.info_coords[i].station_coord[0],res.info_coords[i].station_coord[1],res.info_coords[i].coord_index);
        fprintf(fout,"\n");
        fclose(fout);		
    }
    fclose(fp);
    return 0;
}


int City::LoadTrafficLineData(const char* FileName, int TrafficType) {
    MJ::PrintInfo::PrintLog("City::LoadTrafficLineData, city:[%s], type:[%s], Loading file:[%s]", m_Name.c_str(), TRAFFICLINE_TYPE_NAME[TrafficType], FileName);
    std::ifstream fin;
    fin.open(FileName);
    if (!fin){
        MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, file:[%s] not exist", FileName);
        return RUN_OK;
    }
    std::string line;
    std::string intervalRule;
    int lineCount = 0;
    long long  nodeCount = 0;
    while(!fin.eof()){
        getline(fin,line);
        if (line.length()==0)
            continue;
        std::vector<std::string> items;
        boost::split_regex(items, line, boost::regex("###"));

        if (items.size() < 2) {
            MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, format error: %s", line.c_str());
            continue;
        }
        //	intervalRule="";
        StationNodeLine* trafficLine = new StationNodeLine;
        trafficLine->m_ID = m_TrafficLines[TrafficType].size();
        trafficLine->m_Name = items[0];
        trafficLine->m_LineType = NODELINE_TYPE_TRAFFIC;
        trafficLine->m_StationType = TrafficType;
        //trafficLine->m_Cost = atof(items[1].c_str());
        intervalRule = items[3];
        if (intervalRule.length() <= 8) {
            MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, format error: %s", line.c_str());
            delete trafficLine;
            continue;
        }

        //		MJ::PrintInfo::PrintDbg("City::LoadTraffic(), LineName:%s, type:%s", trafficLine->m_Name.c_str(), intervalRule.c_str());	
        //		for (int i = 0; i < trafficLine->m_FirstSendList.size(); i ++) {
        //			MJ::PrintInfo::PrintLog("City::LoadTrafficLineData, Send time: %d->%d", trafficLine->m_FirstSendList[i]/3600%24,trafficLine->m_FirstSendList[i]%3600/60);
        //		}
        if (intervalRule.find("TABLE") == 0) {
            std::vector<std::string> intervalItems;
            intervalRule = intervalRule.substr(6, intervalRule.length() - 7);
            boost::split_regex(intervalItems, intervalRule, boost::regex("\\]\\["));
            std::vector<SendInterval> intervalList;
            intervalList.resize(intervalItems.size());
            for (int i = 0; i < intervalItems.size(); i ++) {
                std::vector<string> date_clocks;
                boost::split(date_clocks, intervalItems[i], boost::is_any_of(":"));
                char time_format[64];
                snprintf(time_format,sizeof(time_format),"%s000000",date_clocks[0].c_str());
                time_t curTime = MJ::MyTime::toTime(time_format, "%Y%m%d%H%M%S", 0);
                intervalList[i].SetRule(intervalItems[i], curTime, m_TimeZone, TIME_RULE_TYPE_TIMETABLE);
            }
            for(int j=0;j<intervalList.size();j++){
                unsigned int time_tab_key = (intervalList[j].m_TimeTable.begin()->first);
                time_tab_key = time_tab_key - (time_tab_key % 86400);
                for (int k = 0; k < intervalList[j].m_TimeTable[time_tab_key].size(); k ++){ 
                    //cerr << "debug info :: insert key :: " << time_tab_key << endl;
                    trafficLine->m_FirstSendList[time_tab_key].push_back(intervalList[j].m_TimeTable[time_tab_key][k]);
                }
            }
        } //TABLE

        int isCoordsValid = RUN_OK;
        int insertValid = RUN_OK;
        double openTime;
        double closeTime;
        char buff[100];
        std::vector<std::string> next_lines;
        boost::split(next_lines, items[1], boost::is_any_of("|"));
        std::vector<std::string> next_lines_item_pre;
        std::vector<std::string> next_lines_item_aft;
        std::vector<std::string> next_lines_item_temp;
        boost::split(next_lines_item_pre, next_lines[0], boost::is_any_of("@"));
        boost::split(next_lines_item_aft, next_lines[1], boost::is_any_of("@"));
        for (int i = 4; i < items.size(); i+=8 ){			// load station info
            StationNode* stationNode = new StationNode;
            if ( i == 4){
                for(int temp=0;temp<next_lines_item_pre.size();temp++){
                    stationNode->m_NextLines_aft[temp].push_back(next_lines_item_pre[temp]);
                }
            }
            if ( (i+8) == items.size()){
                for(int temp = 0;temp<next_lines_item_aft.size();temp++){
                    boost::split(next_lines_item_temp, next_lines_item_aft[temp], boost::is_any_of(","));
                    for (int tmp=0;tmp<next_lines_item_temp.size();tmp++)
                        stationNode->m_NextLines_aft[temp].push_back(next_lines_item_temp[tmp]);
                }
            }
            MapNode* mapNode = new MapNode;
            stationNode->m_Name = items[i];
            snprintf(buff, sizeof(buff), "%s-%s-%d", items[0].c_str(), items[i].c_str(), (i-2)/5);
            stationNode->m_GID = buff;
            isCoordsValid = RUN_OK;	
            if (MyCommon::ParseDouble(items[i+1], mapNode->m_Coords.m_X) != RUN_OK) isCoordsValid = RUN_ERR;
            if (MyCommon::ParseDouble(items[i+2], mapNode->m_Coords.m_Y) != RUN_OK) isCoordsValid = RUN_ERR;
            MyCommon::ParseDouble(items[i+4], stationNode->m_OpenTime);
            MyCommon::ParseDouble(items[i+5], stationNode->m_CloseTime);
            if (items[i+3]!="NULL")
                MyCommon::ParseInt(items[i+3], stationNode->m_Area);
            std::vector<std::string> station_ids;
            boost::split(station_ids, items[i+6], boost::is_any_of(","));
            for (int ii=0;ii<station_ids.size();ii++){
                if (station_ids[ii]!="")
                    stationNode->m_StationId.push_back(atoi(station_ids[ii].c_str()));
            }
            stationNode->m_Type = NODE_TYPE_STATION;
            stationNode->m_EntityType = TrafficType;
            mapNode->m_ID = m_ID2Node.size();
            mapNode->m_Type = (1<<NODE_TYPE_STATION);
            mapNode->m_EntityType = (1<<TrafficType);
            stationNode->m_MapNode = mapNode;
            stationNode->m_CityID = m_ID;

            insertValid = RUN_ERR;
            Coords xyCoords;
            Coordinate::LatLon2XY(mapNode->m_Coords, xyCoords);
            stationNode->m_ID=nodeCount;
            nodeCount ++;
            if (isCoordsValid == RUN_OK) {
                insertValid = m_QuadTree->InsertNode(stationNode, xyCoords);
                MJ::PrintInfo::PrintDbg("City::LoadTrafficLineData, Inserting node, %d(%s), %d", stationNode->m_ID, stationNode->m_Name.c_str(), insertValid);
            } else {
                MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, Invalid coords, %d, %s, %s, (%s,%s), %s", m_ID, FileName, 
                        trafficLine->m_Name.c_str(), items[i+1].c_str(), items[i+2].c_str(), stationNode->m_Name.c_str());
            }
            if (insertValid == RUN_ERR) {
                MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, error when inserting node to quadtree, %d(%s)", 
                        mapNode->m_ID, stationNode->m_Name.c_str());
                delete mapNode;
                delete stationNode;
            } else {
                if (insertValid != RUN_OK) {
                    m_ID2Node[insertValid]->m_Type |= mapNode->m_Type;
                    m_ID2Node[insertValid]->m_EntityType |= mapNode->m_EntityType;
                    m_NID2EPMap[NODE_TYPE_STATION][insertValid].insert(stationNode);
                    stationNode->m_MapNode = m_ID2Node[insertValid];
                    if ((m_NID2TLIDMap[TrafficType][insertValid].size()== 0) || (trafficLine->m_ID != m_NID2TLIDMap[TrafficType][insertValid].back())) {
                        m_NID2TLIDMap[TrafficType][insertValid].push_back(trafficLine->m_ID);
                    }
                    delete mapNode;
                } else {
                    stationNode->m_MapNode = mapNode;
                    m_NID2EPMap[NODE_TYPE_STATION][mapNode->m_ID].insert(stationNode);
                    m_NID2TLIDMap[TrafficType][mapNode->m_ID].push_back(trafficLine->m_ID);
                    m_ID2Node.push_back(stationNode->m_MapNode);
                }
                trafficLine->m_NodeList.push_back(stationNode);
            }
        }

        int startTime = 0;
        trafficLine->m_TimeList_Open.resize(trafficLine->m_NodeList.size());
        trafficLine->m_TimeList_Close.resize(trafficLine->m_NodeList.size());
        for (int i = 0; i < trafficLine->m_NodeList.size(); i ++) {		// add time for following stations
            if (i > 0) {
                if ((trafficLine->m_NodeList[i]->m_OpenTime >= ERR_THRES) && (trafficLine->m_NodeList[i]->m_CloseTime >= ERR_THRES)) {
                    trafficLine->m_TimeList_Open[i] = MyCommon::double2Seconds(trafficLine->m_NodeList[i]->m_OpenTime) - startTime; 
                    trafficLine->m_TimeList_Close[i] = MyCommon::double2Seconds(trafficLine->m_NodeList[i]->m_CloseTime) - startTime; 
                } else {
                    trafficLine->m_TimeList_Open[i] = trafficLine->m_TimeList_Open[i - 1] + TrafficCost::GetTime(trafficLine->m_NodeList[i], 
                            trafficLine->m_NodeList[i - 1], TRAFFICLINE_TYPE_MAP[TrafficType], 0);
                    trafficLine->m_TimeList_Close[i] = trafficLine->m_TimeList_Close[i - 1] + TrafficCost::GetTime(trafficLine->m_NodeList[i], 
                            trafficLine->m_NodeList[i - 1], TRAFFICLINE_TYPE_MAP[TrafficType], 0);
                }
            } else {
                if ((trafficLine->m_NodeList[i]->m_OpenTime >= 0) && (trafficLine->m_NodeList[i]->m_CloseTime >= 0)) {
                    trafficLine->m_TimeList_Open[i] = 0;
                    startTime = MyCommon::double2Seconds(trafficLine->m_NodeList[i]->m_OpenTime);
                } else {
                    trafficLine->m_TimeList_Close[i] = 0;
                }
            }
            /*			MJ::PrintInfo::PrintLog("City::LoadTrafficLineData, station:%d, arrive time:%d(%d.%d.%.2d)", i, trafficLine->m_TimeList[i], 
                        (trafficLine->m_TimeList[i]/3600)/24, (trafficLine->m_TimeList[i]/3600)%24, (trafficLine->m_TimeList[i]%3600)/60);*/
            for (int j = i + 1; j < trafficLine->m_NodeList.size(); j ++) {
                SetNodeLinkable(trafficLine->m_NodeList[i]->m_MapNode->m_ID, trafficLine->m_NodeList[j]->m_MapNode->m_ID);
            }
        }

        m_TrafficLines[TrafficType].push_back(trafficLine);
    }
    MJ::PrintInfo::PrintDbg("City::LoadTrafficLineData, Successfully loaded trafficline: %d, station:%d", m_TrafficLines[TrafficType].size(), nodeCount);
    return RUN_OK;
}

int City::GetPath(int timeblock,int Cid,int TrafficPrefer, int DepartTime, std::tr1::unordered_map<int, PathSet >& pathes,int weekday) {	// user time
    int maxDepth = 3;
    int nodeLmt = 50;
    struct timeval begin_time_t ;
    struct timeval end_time_t;
    gettimeofday(&begin_time_t,NULL);
    std::string path_uniq_key;
    printf("Start searching %d...\n",Cid);
    MJ::MyTimer t;
    //初始化固态数据
    {
        Coords xyCoords;
        std::string name;
        Coordinate::LatLon2XY(this->m_Center, xyCoords);
        std::priority_queue<NodePair<EntityNode> > query;
        this->m_QuadTree->GetNodeList(this->m_Center, xyCoords, name,100000.0,0,query,1<<NODE_TYPE_STATION, TRAFFICLINE_TYPE_ALL, LBSSORT_TYPE_DIST);
        std::cerr << "Query.size() = " << query.size() << endl;
        std::vector<NodePair<EntityNode> > list;
        while(!query.empty()) {
            list.push_back(query.top());
            query.pop();
        }
        ConstData::init(list);
    }
    int cnt=0;
    std::tr1::unordered_map<int , PathSet >::iterator iter;
    std::vector<MapNode*>::iterator iter1;
    MyThreadPool my_thread_pool;
    FILE * open_binfiles[THREAD_NUM];
    for (int file_idx=0;file_idx<THREAD_NUM;file_idx++){
        char bin_infile[64];
        snprintf(bin_infile,sizeof(bin_infile),"../data/%d_%d_bin_data%d",Cid,weekday,file_idx);
        cerr << "debug info :: file name " << bin_infile << endl;
        open_binfiles[file_idx]=fopen(bin_infile,"ab+");
    }
    my_thread_pool.open(THREAD_NUM,8*1024*1024,&open_binfiles);
    std::vector<Worker*> jobs;
    my_thread_pool.activate();
    int index=0;
    for (iter1=m_ID2Node.begin()+1;iter1!=m_ID2Node.end() && ((*iter1)->m_Type & (1<<NODE_TYPE_STATION)) ;iter1++)
    {	
        EntityNode *curNode = *(m_NID2EPMap[NODE_TYPE_STATION][(*(iter1))->m_ID].begin());
        //MJ::PrintInfo::PrintDbg("id :%d, name : %s", curNode->m_ID, curNode->m_Name);
        MyWoker *job = new MyWoker(this,curNode,nodeLmt,maxDepth,DepartTime,weekday);
        jobs.push_back(job);
        my_thread_pool.add_worker((Worker*)job);
        index++;
    }
    cerr << "debug info :: " << index << " jobs in stack" << endl;
    my_thread_pool.wait_worker_done(jobs);
    for (int file_idx=0;file_idx<THREAD_NUM;file_idx++)
        fclose(open_binfiles[file_idx]);
    for (int i = 0; i < jobs.size(); i ++) {
        Worker* worker = (Worker*)jobs[i];
        delete worker;
    }
    jobs.clear();
    pthread_mutex_destroy(&map_dist_mutex);  
    gettimeofday(&end_time_t,NULL);
    cerr << "time.cost() = " << t.cost() << endl;
    MJ::PrintInfo::PrintErr("ASearch algorithm use time = %d",1000000*(end_time_t.tv_sec - begin_time_t.tv_sec) + (end_time_t.tv_usec - begin_time_t.tv_usec));
    return RUN_OK;
}

// id|name|x|y|openTime|price|score|time
int City::LoadViewData(const char* FileName, int ViewType) {
    MJ::PrintInfo::PrintLog("City::LoadViewData, city:[%s], ViewType:%d, Loading file:[%s]", m_Name.c_str(), ViewType, FileName);
    std::ifstream fin;
    fin.open(FileName);
    if (!fin){
        MJ::PrintInfo::PrintErr("City::LoadViewData, file:[%s] not exist", FileName);
        return RUN_OK;
    }

    struct timeval begin_time_t;
    struct timeval end_time_t;
    gettimeofday(&begin_time_t,NULL);
    std::string line;
    int lineCount = 0;
    while(!fin.eof()){
        getline(fin,line);
        //		getline(fin,line);
        if (line.length()==0)
            continue;

        std::vector<std::string> items;
        boost::split_regex(items, line, boost::regex("###"));
        if (items.size() < 8) {
            MJ::PrintInfo::PrintErr("City::LoadViewLineData, format error: %s", line.c_str());
            continue;
        }

        ViewNode* viewNode = new ViewNode;
        MapNode* mapNode = new MapNode;
        std::string id = items[0];
        viewNode->m_ID = ++lineCount;
        viewNode->m_Name = items[1];
        viewNode->m_GID = id;

        int isCoordsValid = RUN_OK;	
        if (MyCommon::ParseDouble(items[2], mapNode->m_Coords.m_X) != RUN_OK) isCoordsValid = RUN_ERR;
        if (MyCommon::ParseDouble(items[3], mapNode->m_Coords.m_Y) != RUN_OK) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_X < m_LeftTop.m_X) || (mapNode->m_Coords.m_X > m_RightBot.m_X)) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_Y < m_LeftTop.m_Y) || (mapNode->m_Coords.m_Y > m_RightBot.m_Y)) isCoordsValid = RUN_ERR;
        viewNode->m_OpenTime = items[4];
        viewNode->m_Price = items[5];
        viewNode->m_Score = atof(items[6].c_str());
        viewNode->m_Time = atoi(items[7].c_str());
        viewNode->m_Type = NODE_TYPE_VIEW;
        viewNode->m_EntityType = ViewType;
        mapNode->m_ID = m_ID2Node.size();
        mapNode->m_Type = (1<<NODE_TYPE_VIEW);
        mapNode->m_EntityType = (1<<ViewType);
        viewNode->m_MapNode = mapNode;
        viewNode->m_CityID = m_ID;

        Coords xyCoords;
        Coordinate::LatLon2XY(mapNode->m_Coords, xyCoords);
        int insertValid = RUN_ERR;
        if (isCoordsValid == RUN_OK) {
            //			MJ::PrintInfo::PrintDbg("City::LoadTrafficLineData, Inserting node, %d(%s)", viewNode->m_ID, viewNode->m_Name.c_str());
            //			viewNode->Dump();
            insertValid = m_QuadTree->InsertNode(viewNode, xyCoords);
        } else {
            MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, Invalid coords, %d, %s, %s, (%s,%s)", m_ID, FileName, viewNode->m_Name.c_str(),
                    items[2].c_str(), items[3].c_str());
        }
        if (insertValid == RUN_ERR) {
            MJ::PrintInfo::PrintErr("City::LoadViewData, error when inserting node to quadtree, %d(%s)", 
                    mapNode->m_ID, viewNode->m_Name.c_str());
            delete mapNode;
            delete viewNode;
        } else {
            if (insertValid != RUN_OK) {
                m_ID2Node[insertValid]->m_Type |= mapNode->m_Type;
                m_ID2Node[insertValid]->m_EntityType |= mapNode->m_EntityType;
                m_NID2EPMap[NODE_TYPE_VIEW][insertValid].insert(viewNode);
                viewNode->m_MapNode = m_ID2Node[insertValid];
                delete mapNode;
            } else {
                viewNode->m_MapNode = mapNode;
                m_NID2EPMap[NODE_TYPE_VIEW][mapNode->m_ID].insert(viewNode);
                m_ID2Node.push_back(viewNode->m_MapNode);
            }
            m_ViewMap[id] = viewNode;
            //			viewNode->Dump();
        }
    }

    MJ::PrintInfo::PrintLog("City::LoadViewData, Successfully loaded view: %d", lineCount);
    gettimeofday(&end_time_t,NULL);
    MJ::PrintInfo::PrintLog("LoadViewData use time = %d", 1000000*(end_time_t.tv_sec - begin_time_t.tv_sec) + end_time_t.tv_usec - begin_time_t.tv_usec );
    return RUN_OK;
}

// id|name|x|y|openTime|price|score|mealtype
int City::LoadRestaurantData(const char* FileName, int RestaurantType) {
    MJ::PrintInfo::PrintLog("City::RestaurantType, city:[%s], RestaurantType:%d, Loading file:[%s]", m_Name.c_str(), RestaurantType, FileName);
    std::ifstream fin;
    fin.open(FileName);
    if (!fin){
        MJ::PrintInfo::PrintErr("City::RestaurantType, file:[%s] not exist", FileName);
        return RUN_OK;
    }

    std::string line;
    int lineCount = 0;
    while(!fin.eof()){
        getline(fin,line);
        if (line.length()==0)
            continue;

        std::vector<std::string> items;
        boost::split_regex(items, line, boost::regex("###"));
        if (items.size() < 8) {
            MJ::PrintInfo::PrintErr("City::RestaurantType, format error: %s", line.c_str());
            continue;
        }

        RestaurantNode* restaurantNode = new RestaurantNode;
        MapNode* mapNode = new MapNode;
        std::string id = items[0];
        restaurantNode->m_ID = ++lineCount;
        restaurantNode->m_Name = items[1];
        restaurantNode->m_GID = id;

        int isCoordsValid = RUN_OK;	
        if (MyCommon::ParseDouble(items[2], mapNode->m_Coords.m_X) != RUN_OK) isCoordsValid = RUN_ERR;
        if (MyCommon::ParseDouble(items[3], mapNode->m_Coords.m_Y) != RUN_OK) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_X < m_LeftTop.m_X) || (mapNode->m_Coords.m_X > m_RightBot.m_X)) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_Y < m_LeftTop.m_Y) || (mapNode->m_Coords.m_Y > m_RightBot.m_Y)) isCoordsValid = RUN_ERR;
        restaurantNode->m_OpenTime = items[4];
        restaurantNode->m_Price = atof(items[5].c_str());
        restaurantNode->m_Score = atof(items[6].c_str());
        restaurantNode->m_MealType = atoi(items[7].c_str());
        restaurantNode->m_Type = NODE_TYPE_RESTAURANT;
        restaurantNode->m_EntityType = RestaurantType;
        mapNode->m_ID = m_ID2Node.size();
        mapNode->m_Type = (1<<NODE_TYPE_RESTAURANT);
        mapNode->m_EntityType = (1<<RestaurantType);
        restaurantNode->m_MapNode = mapNode;
        restaurantNode->m_CityID = m_ID;

        Coords xyCoords;
        Coordinate::LatLon2XY(mapNode->m_Coords, xyCoords);
        int insertValid = RUN_ERR;
        if (isCoordsValid == RUN_OK) {
            //			MJ::PrintInfo::PrintDbg("City::LoadTrafficLineData, Inserting node, %d(%s)", restaurantNode->m_ID, restaurantNode->m_Name.c_str());
            //			->Dump();
            insertValid = m_QuadTree->InsertNode(restaurantNode, xyCoords);
        } else {
            MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, Invalid coords, %d, %s, %s, (%s,%s)", m_ID, FileName, 
                    restaurantNode->m_Name.c_str(), items[2].c_str(), items[3].c_str());
        }
        if (insertValid == RUN_ERR) {
            MJ::PrintInfo::PrintErr("City::LoadRestaurantData, error when inserting node to quadtree, %d(%s)", 
                    mapNode->m_ID, restaurantNode->m_Name.c_str());
            delete mapNode;
            delete restaurantNode;
        } else {
            if (insertValid != RUN_OK) {
                m_ID2Node[insertValid]->m_Type |= mapNode->m_Type;
                m_ID2Node[insertValid]->m_EntityType |= mapNode->m_EntityType;
                m_NID2EPMap[NODE_TYPE_RESTAURANT][insertValid].insert(restaurantNode);
                restaurantNode->m_MapNode = m_ID2Node[insertValid];
                delete mapNode;
            } else {
                restaurantNode->m_MapNode = mapNode;
                m_NID2EPMap[NODE_TYPE_RESTAURANT][mapNode->m_ID].insert(restaurantNode);
                m_ID2Node.push_back(restaurantNode->m_MapNode);
            }
            m_RestMap[id] = restaurantNode;
            //			restaurantNode->Dump();
        }
    }

    MJ::PrintInfo::PrintLog("City::LoadRestaurantData, Successfully loaded restaurant: %d", lineCount);
    return RUN_OK;
}

// id|name|x|y|level|score
int City::LoadHotelData(const char* FileName, int HotelType) {
    MJ::PrintInfo::PrintLog("City::LoadHotelData, city:[%s], HotelType:%d, Loading file:[%s]", m_Name.c_str(), HotelType, FileName);
    std::ifstream fin;
    fin.open(FileName);
    if (!fin){
        MJ::PrintInfo::PrintErr("City::LoadHotelData, file:[%s] not exist", FileName);
        return RUN_OK;
    }

    struct timeval begin_time_t;
    struct timeval end_time_t;
    gettimeofday(&begin_time_t,NULL);

    std::string line;
    int lineCount = 0;
    while(!fin.eof()){
        getline(fin,line);
        if (line.length()==0)
            continue;

        std::vector<std::string> items;
        boost::split_regex(items, line, boost::regex("###"));
        if (items.size() < 6) {
            MJ::PrintInfo::PrintErr("City::LoadHotelData, format error: %s", line.c_str());
            continue;
        }

        HotelNode* hotelNode = new HotelNode;
        MapNode* mapNode = new MapNode;
        std::string id = items[0];
        hotelNode->m_ID = ++lineCount;
        hotelNode->m_Name = items[1];
        hotelNode->m_GID = id;

        int isCoordsValid = RUN_OK;	
        if (MyCommon::ParseDouble(items[2], mapNode->m_Coords.m_X) != RUN_OK) isCoordsValid = RUN_ERR;
        if (MyCommon::ParseDouble(items[3], mapNode->m_Coords.m_Y) != RUN_OK) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_X < m_LeftTop.m_X) || (mapNode->m_Coords.m_X > m_RightBot.m_X)) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_Y < m_LeftTop.m_Y) || (mapNode->m_Coords.m_Y > m_RightBot.m_Y)) isCoordsValid = RUN_ERR;
        hotelNode->m_Level = atoi(items[4].c_str());
        hotelNode->m_Score = atof(items[5].c_str());

        hotelNode->m_Type = NODE_TYPE_HOTEL;
        hotelNode->m_EntityType = HotelType;
        mapNode->m_ID = m_ID2Node.size();
        mapNode->m_Type = (1<<NODE_TYPE_HOTEL);
        mapNode->m_EntityType = (1<<HotelType);
        hotelNode->m_MapNode = mapNode;
        hotelNode->m_CityID = m_ID;

        Coords xyCoords;
        Coordinate::LatLon2XY(mapNode->m_Coords, xyCoords);
        int insertValid = RUN_ERR;
        if (isCoordsValid == RUN_OK) {
            //			MJ::PrintInfo::PrintDbg("City::LoadHotelData, Inserting node, %d(%s)", hotelNode->m_ID, hotelNode->m_Name.c_str());
            //			hotelNode->Dump();
            insertValid = m_QuadTree->InsertNode(hotelNode, xyCoords);
        } else {
            MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, Invalid coords, %d, %s, %s, (%s,%s)", m_ID, FileName, hotelNode->m_Name.c_str(),
                    items[2].c_str(), items[3].c_str());
        }
        if (insertValid == RUN_ERR) {
            MJ::PrintInfo::PrintErr("City::LoadHotelData, error when inserting node to quadtree, %d(%s)", 
                    mapNode->m_ID, hotelNode->m_Name.c_str());
            delete mapNode;
            delete hotelNode;
        } else {
            if (insertValid != RUN_OK) {
                m_ID2Node[insertValid]->m_Type |= mapNode->m_Type;
                m_ID2Node[insertValid]->m_EntityType |= mapNode->m_EntityType;
                m_NID2EPMap[NODE_TYPE_HOTEL][insertValid].insert(hotelNode);
                hotelNode->m_MapNode = m_ID2Node[insertValid];
                delete mapNode;
            } else {
                hotelNode->m_MapNode = mapNode;
                m_NID2EPMap[NODE_TYPE_HOTEL][mapNode->m_ID].insert(hotelNode);
                m_ID2Node.push_back(hotelNode->m_MapNode);
            }
            m_HotelMap[id] = hotelNode;
            //			hotelNode->Dump();
        }
    }

    MJ::PrintInfo::PrintLog("City::LoadHotelData, Successfully loaded hotel: %d", lineCount);
    gettimeofday(&end_time_t,NULL);
    MJ::PrintInfo::PrintLog("LoadHotelData use time = %d",1000000 * ( end_time_t.tv_sec - begin_time_t.tv_sec ) + (end_time_t.tv_usec - begin_time_t.tv_usec)  );

    return RUN_OK;
}

int City::LoadAirportData(const char* FileName, int AirportType) {
    MJ::PrintInfo::PrintLog("City::LoadHotelData, city:[%s], HotelType:%d, Loading file:[%s]",m_Name.c_str(),AirportType, FileName);
    std::ifstream fin;
    fin.open(FileName);
    if( !fin){
        MJ::PrintInfo::PrintErr("City::LoadAirportData, file:[%s] not exist",FileName);
        return RUN_OK;
    }

    std::string line;
    int lineCount = 0;
    while(!fin.eof()){
        getline(fin,line);
        if (line.length() == 0 )
            continue;
        std::vector<std::string> items;
        boost::split_regex(items, line, boost::regex("###"));
        MJ::PrintInfo::PrintErr("airport data's size =%d ",items.size() );
        if(items.size() < 4 ){
            MJ::PrintInfo::PrintErr("City::LoadAirportData, format error: %s",line.c_str());
            continue;
        }

        AirportNode* airportNode = new AirportNode;
        MapNode* mapNode = new MapNode;
        std::string id = items[0];
        airportNode->m_ID = ++lineCount;
        airportNode->m_Name = items[1];
        airportNode->radius = atoi(items[4].c_str());

        int isCoordsValid = RUN_OK;
        if (MyCommon::ParseDouble(items[2], mapNode->m_Coords.m_X) != RUN_OK) isCoordsValid = RUN_ERR;
        if (MyCommon::ParseDouble(items[3], mapNode->m_Coords.m_Y) != RUN_OK) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_X < m_LeftTop.m_X) || (mapNode->m_Coords.m_X > m_RightBot.m_X)) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_Y < m_LeftTop.m_Y) || (mapNode->m_Coords.m_Y > m_RightBot.m_Y)) isCoordsValid = RUN_ERR;

        airportNode->m_Type = NODE_TYPE_AIRPORT;
        airportNode->m_EntityType = AirportType;
        mapNode->m_Type = (1<<NODE_TYPE_AIRPORT);
        mapNode->m_EntityType = (1<<AirportType);
        airportNode->m_MapNode = mapNode;
        airportNode->m_CityID = m_ID;

        //此处不把机场的店插入到四叉树里面，只放到m_HotelMap里面，然后计算时候遍历一下这个就OK
        m_AirportMap[id] = airportNode;
    }
    return RUN_OK;
}

// id|name|x|y|score
int City::LoadShopData(const char* FileName, int ShopType) {
    MJ::PrintInfo::PrintLog("City::LoadShopData, city:[%s], ShopType:%d, Loading file:[%s]", m_Name.c_str(), ShopType, FileName);
    std::ifstream fin;
    fin.open(FileName);
    if (!fin){
        MJ::PrintInfo::PrintErr("City::LoadShopData, file:[%s] not exist", FileName);
        return RUN_OK;
    }

    std::string line;
    int lineCount = 0;
    while(!fin.eof()){
        getline(fin,line);
        if (line.length()==0)
            continue;

        std::vector<std::string> items;
        boost::split_regex(items, line, boost::regex("###"));
        if (items.size() < 6) {
            MJ::PrintInfo::PrintErr("City::LoadShopData, format error: %s", line.c_str());
            continue;
        }

        ShopNode* shopNode = new ShopNode;
        MapNode* mapNode = new MapNode;
        std::string id = items[0];
        shopNode->m_ID = ++lineCount;
        shopNode->m_Name = items[1];
        shopNode->m_GID = id;

        int isCoordsValid = RUN_OK;	
        if (MyCommon::ParseDouble(items[2], mapNode->m_Coords.m_X) != RUN_OK) isCoordsValid = RUN_ERR;
        if (MyCommon::ParseDouble(items[3], mapNode->m_Coords.m_Y) != RUN_OK) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_X < m_LeftTop.m_X) || (mapNode->m_Coords.m_X > m_RightBot.m_X)) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_Y < m_LeftTop.m_Y) || (mapNode->m_Coords.m_Y > m_RightBot.m_Y)) isCoordsValid = RUN_ERR;
        shopNode->m_Score = atof(items[4].c_str());
        //		shopNode->m_Price = atof(items[5].c_str());
        //		shopNode->m_Time = atoi(items[6].c_str());

        shopNode->m_Type = NODE_TYPE_SHOP;
        shopNode->m_EntityType = ShopType;
        mapNode->m_ID = m_ID2Node.size();
        mapNode->m_Type = (1<<NODE_TYPE_SHOP);
        mapNode->m_EntityType = (1<<ShopType);
        shopNode->m_MapNode = mapNode;
        shopNode->m_CityID = m_ID;

        Coords xyCoords;
        Coordinate::LatLon2XY(mapNode->m_Coords, xyCoords);	
        int insertValid = RUN_ERR;
        if (isCoordsValid == RUN_OK) {
            //			MJ::PrintInfo::PrintDbg("City::LoadHotelData, Inserting node, %d(%s)", hotelNode->m_ID, hotelNode->m_Name.c_str());
            //			hotelNode->Dump();
            insertValid = m_QuadTree->InsertNode(shopNode, xyCoords);
        } else {
            MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, Invalid coords, %d, %s, %s, (%s,%s)", m_ID, FileName, shopNode->m_Name.c_str(),
                    items[2].c_str(), items[3].c_str());
        }
        if (insertValid == RUN_ERR) {
            MJ::PrintInfo::PrintErr("City::LoadShopData, error when inserting node to quadtree, %d(%s)", 
                    mapNode->m_ID, shopNode->m_Name.c_str());
            delete mapNode;
            delete shopNode;
        } else {
            if (insertValid != RUN_OK) {
                m_ID2Node[insertValid]->m_Type |= mapNode->m_Type;
                m_ID2Node[insertValid]->m_EntityType |= mapNode->m_EntityType;
                m_NID2EPMap[NODE_TYPE_SHOP][insertValid].insert(shopNode);
                shopNode->m_MapNode = m_ID2Node[insertValid];
                delete mapNode;
            } else {
                shopNode->m_MapNode = mapNode;
                m_NID2EPMap[NODE_TYPE_SHOP][mapNode->m_ID].insert(shopNode);
                m_ID2Node.push_back(shopNode->m_MapNode);
            }
            m_ShopMap[id] = shopNode;
            //			hotelNode->Dump();
        }
    }

    MJ::PrintInfo::PrintLog("City::LoadShopData, Successfully loaded hotel: %d", lineCount);
    return RUN_OK;
}

// id|name|x|y|score
int City::LoadActivityData(const char* FileName, int ActivityType) {
    MJ::PrintInfo::PrintLog("City::LoadActivityData, city:[%s], ActivityType:%d, Loading file:[%s]", m_Name.c_str(), ActivityType, FileName);
    std::ifstream fin;
    fin.open(FileName);
    if (!fin){
        MJ::PrintInfo::PrintErr("City::LoadActivityData, file:[%s] not exist", FileName);
        return RUN_OK;
    }

    std::string line;
    int lineCount = 0;
    while(!fin.eof()){
        getline(fin,line);
        if (line.length()==0)
            continue;

        std::vector<std::string> items;
        boost::split_regex(items, line, boost::regex("###"));
        if (items.size() < 6) {
            MJ::PrintInfo::PrintErr("City::LoadActivityData, format error: %s", line.c_str());
            continue;
        }

        ActivityNode* activityNode = new ActivityNode;
        MapNode* mapNode = new MapNode;
        std::string id = items[0];
        activityNode->m_ID = ++lineCount;
        activityNode->m_Name = items[1];
        activityNode->m_GID = id;

        int isCoordsValid = RUN_OK;	
        if (MyCommon::ParseDouble(items[2], mapNode->m_Coords.m_X) != RUN_OK) isCoordsValid = RUN_ERR;
        if (MyCommon::ParseDouble(items[3], mapNode->m_Coords.m_Y) != RUN_OK) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_X < m_LeftTop.m_X) || (mapNode->m_Coords.m_X > m_RightBot.m_X)) isCoordsValid = RUN_ERR;
        if ((mapNode->m_Coords.m_Y < m_LeftTop.m_Y) || (mapNode->m_Coords.m_Y > m_RightBot.m_Y)) isCoordsValid = RUN_ERR;
        activityNode->m_Score = atof(items[4].c_str());

        activityNode->m_Type = NODE_TYPE_ACTIVITY;
        activityNode->m_EntityType = ActivityType;
        mapNode->m_ID = m_ID2Node.size();
        mapNode->m_Type = (1<<NODE_TYPE_ACTIVITY);
        mapNode->m_EntityType = (1<<ActivityType);
        activityNode->m_MapNode = mapNode;
        activityNode->m_CityID = m_ID;

        Coords xyCoords;
        Coordinate::LatLon2XY(mapNode->m_Coords, xyCoords);	
        int insertValid = RUN_ERR;
        if (isCoordsValid == RUN_OK) {
            //			MJ::PrintInfo::PrintDbg("City::LoadActivityData, Inserting node, %d(%s)", activityNode->m_ID, activityNode->m_Name.c_str());
            //			activityNode->Dump();
            insertValid = m_QuadTree->InsertNode(activityNode, xyCoords);
        } else {
            MJ::PrintInfo::PrintErr("City::LoadTrafficLineData, Invalid coords, %d, %s, %s, (%s,%s)", m_ID, FileName, activityNode->m_Name.c_str(),
                    items[2].c_str(), items[3].c_str());
        }
        if (insertValid == RUN_ERR) {
            MJ::PrintInfo::PrintErr("City::LoadActivityData, error when inserting node to quadtree, %d(%s)", 
                    mapNode->m_ID, activityNode->m_Name.c_str());
            delete mapNode;
            delete activityNode;
        } else {
            if (insertValid != RUN_OK) {
                m_ID2Node[insertValid]->m_Type |= mapNode->m_Type;
                m_ID2Node[insertValid]->m_EntityType |= mapNode->m_EntityType;
                m_NID2EPMap[NODE_TYPE_ACTIVITY][insertValid].insert(activityNode);
                activityNode->m_MapNode = m_ID2Node[insertValid];
                delete mapNode;
            } else {
                activityNode->m_MapNode = mapNode;
                m_NID2EPMap[NODE_TYPE_ACTIVITY][mapNode->m_ID].insert(activityNode);
                m_ID2Node.push_back(activityNode->m_MapNode);
            }
            m_ActivityMap[id] = activityNode;
            //			hotelNode->Dump();
        }
    }

    MJ::PrintInfo::PrintLog("City::LoadActivityData, Successfully loaded activity: %d", lineCount);
    return RUN_OK;
}
double ConstData::GetDist(Coords& ACoords, Coords& BCoords) {
    double value;
    double radAX = ACoords.m_X*DEG2RAD;
    double radAY = ACoords.m_Y*DEG2RAD;
    double radBX = BCoords.m_X*DEG2RAD;
    double radBY = BCoords.m_Y*DEG2RAD;
    value= cos(radAY)*cos(radBY)*cos(radBX - radAX) + sin(radAY)*sin(radBY);
    if(value > 1) {
        value = 1.0;
    }
    if(value < -1) {
        value = -1.0;
    }
    value = EARTH_RADIUS*acos(value);
    if(value < 0) {
        value = 0;
    }
    return value;
}
std::tr1::unordered_map<MAP_KEY, double,KeyHash,cmp_fun> ConstData::Map_Dist;
