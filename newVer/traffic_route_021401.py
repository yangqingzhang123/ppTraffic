#!/usr/bin/python
#coding:utf-8
import CoordDist
import sys
import json
import heapq
import redis
import get_poi
import time
sys.path.append('../script/common/')
sys.path.append('../script/google_transit/')
sys.setdefaultencoding('utf8')
from SSDB import SSDB
from toolLib import formatCoor
#score:烦躁度,客观指标结合主观喜恶而得,很明显,我们要寻找score最低最低最低的路线;烦躁度约等于交通时间
r0 = redis.StrictRedis(host='localhost', port=6379, db=0)
r1 = redis.StrictRedis(host='localhost', port=6379, db=1)
ssdb = SSDB('127.0.0.1',8888)

PATHNUM = 10000
global_num = 0
prefers={}
prefers["time"]={}
prefers["time"]["walk_velo"]=1.2 #正常人1.2m/s
prefers["time"]["transit_as_time"]=3*60
prefers["time"]["trans_limit"]=50
prefers["time"]["no"]=2 #方便打印trans_type字段
prefers["best"]={}
prefers["best"]["walk_velo"]=1.2 #正常人1.2m/s
prefers["best"]["transit_as_time"]=3*60
prefers["best"]["trans_limit"]=50
prefers["best"]["no"]=0
prefers["walk"]={}
prefers["walk"]["walk_velo"]=1.5
prefers["walk"]["transit_as_time"]=3*60 #中转一次,认为旅行时间额外增加了3分钟
prefers["walk"]["trans_limit"]=50
prefers["walk"]["no"]=1
prefers["transit_num"]={}
prefers["transit_num"]["walk_velo"]=1.2
prefers["transit_num"]["transit_as_time"]=5*60
prefers["transit_num"]["trans_limit"]=50 #偏好中转次数时,中转次数上限为50次
prefers["transit_num"]["no"]=3

prefer_type="best" #从prefers定义可看出 best和time一致
public_traffic_types=set(["subu","subway","tram","cablecar","bus"])

#定义公共交通站点之间的交通
class Traffic:
    #交通类型,包括步行和其它5种公共交通
    traffic_types=set(["walk"])|public_traffic_types

    #key为两个站点的sid按字母序排列后的拼接string,value为double类型,单位为m; a到b的距离和b到a的距离都对应key-->"a_b"
    #用于加速dist的计算
    #其存储的是任意两个站点之间的步行直线步行直线步行直线距离!
    station_pair_dists={}

    def __init__(self,start_id,dest_id,run_time=0,wait_time=0,traffic_type="walk",line_name="WALK",station_pass=[]):
        if traffic_type in Traffic.traffic_types:
            self.__ttype=traffic_type
        else:
            raise

        #交通起点
        self.__start_id=start_id
        #交通终点
        self.__dest_id=dest_id
        #经过的站点列表,比如该Traffic 是2号线积水潭到雍和宫段,则station_pass为鼓楼大街和安定门,station_list另添加首尾
        self.__station_list=[]
        self.__station_list+=[self.__start_id]
        self.__station_list+=station_pass
        self.__station_list+=[self.__dest_id]

        #线路名称
        self.__line_name=line_name

        #时间的单位都为秒
        #运行时间
        self.__run_time=int(run_time)
        if self.__run_time<0:
            self.__run_time=2*60*60
            #print(self)

        #等车时间
        self.__wait_time=int(wait_time)

        #总耗时:运行时间+等车时间
        self.__time=self.__run_time+ self.__wait_time

        #站点之间的距离
        self.__dist=sys.maxint
        self.get_dist(True)

        #此段交通的烦躁度
        self.__score=sys.maxint
        self.get_score(True)

    def __str__(self):
        return self.__start_id+" --> "+self.__dest_id+ " @ "+self.__line_name
	
    def get_station_list(self):
        return self.__station_list

    def get_ttype(self):
        return self.__ttype

    def get_line_name(self):
        return self.__line_name

    def get_time(self):
        return self.__time

    def get_run_time(self):
        return self.__run_time

    def get_wait_time(self):
        return self.__wait_time

    #在添加完中转站点后,计算两个站点之间的距离
    def get_dist(self,initial=False):
        #两站点间距离,单位为米; 如果计算鼓楼大街到雍和宫的距离,则其值为
        #鼓楼大街-->安定门 与 安定门-->雍和宫 距离之和
        if initial:
            whole_key='_'.join(sorted([self.__start_id,self.__dest_id]))
            if Traffic.station_pair_dists.has_key(whole_key) and len(self.__station_list)==2: #步行或紧挨的两站
                self.__dist=Traffic.station_pair_dists[whole_key]
                if self.__ttype=="walk": #步行时不认为能直线位移
                    self.__dist*=1.2
            else:
                self.__dist=0
                stations=self.__station_list
                for i in range(len(stations)-1):
                    key='_'.join(sorted([stations[i],stations[i+1]]))
                    if not Traffic.station_pair_dists.has_key(key):
                        start_list = stations[i].split('#')
                        dest_list = stations[i+1].split('#')
                        start_map = start_list[len(start_list) - 1]
                        dest_map = dest_list[len(dest_list) - 1]
                        Traffic.station_pair_dists[key]=CoordDist.getDistance(start_map.split(",")+dest_map.split(","))
                        #Traffic.station_pair_dists[key]=CoordDist.getDistance(stations[i].split("#")[1].split(",")+stations[i+1].split("#")[1].split(","))
                    self.__dist+=Traffic.station_pair_dists[key]
                if len(self.__station_list)==2:
                    Traffic.station_pair_dists[whole_key]=self.__dist

            assert self.__dist>=0

        return self.__dist

    def get_score(self,initial=False):
        if initial:
            if self.__ttype=="walk":
                global prefers, prefer_type
                velo=prefers[prefer_type]["walk_velo"]
                self.__run_time=int(self.get_dist()/velo)
                self.__time=self.__run_time+self.__wait_time
            self.__score=self.__time
            assert self.__score>=0

        return self.__score

#定义公共交通站点
class Station:
    def __init__ (self,id):
        self.__id = id #!!!此id和数据库中的站点sid并不一致,由于浮点数的精度不同,或经纬度存在细微差异
        self.__name, self.__map_info= id.split("#")
        self.__previous = None
        self.__dest_traffics={}
        #mark是否已经寻得最优路径
        self.__visited=False
        #从源点到此站点的烦躁度,此属性是用作比较的属性
        self.__score=sys.maxint
        #从源点到此站点的换乘次数,此属性有上限,和日常概念一致
        self.__transit_num=0

    def __eq__(self, other):
        return isinstance(other, Station) and self.__id == other.get_id()

    def __str__(self):
        #return str(self.__id) + ' dest_traffics: ' + str([x.get_id() for x in self.__dest_traffics])
        return str(self.__id)

    def __hash__(self):
        return hash(self.__id)

    def get_id(self):
        return self.__id

    def get_name(self):
        return self.__name

    def get_map_info(self):
        return self.__map_info

    def add_traffic(self, dest, traffic=None):
        #步行交通不应超过3公里
        #两站点之间有公共交通直达时，一定不选择步行-->更少出badcase
        if traffic==None:
            traffic=Traffic(self.get_id(),dest.get_id())
        if self!=dest and (traffic.get_ttype()!="walk" or ( not self.__dest_traffics.has_key(dest) and 3000 > CoordDist.getDistance(self.get_id().split("#")[-1].split(",")+dest.get_id().split("#")[-1].split(",")))):
            self.__dest_traffics.setdefault(dest,list())
            self.__dest_traffics[dest].append(traffic)

    def get_dest_stations(self):
        return self.__dest_traffics.keys()

    def get_traffic(self, dest):
        #初始化后,两站点之间可能有多种交通方式,选择最优的一种作为两站点之间的连接
        if self.__dest_traffics.has_key(dest) and len(self.__dest_traffics[dest])>1:
                self.__dest_traffics[dest]=[sorted(self.__dest_traffics[dest],key=lambda x:x.get_score())[0]]

        return self.__dest_traffics[dest][0] if len(self.__dest_traffics[dest])==1 else None

    def set_score(self, score):
        self.__score = score

    #计算从源点,以current为中转到此点的烦躁度
    #返回None值代表此路被剪枝
    #这里并不并不并不set score
    def cal_score(self,current):
        global prefers, prefer_type
        prefer=prefers[prefer_type]
        previous=current.__previous
        if not current.get_traffic(self):
            return None
        if not previous: #此时current一定是源点
            return current.get_traffic(self).get_score()
        #两段同为walk,或者同一条线路时,则肯定不是最优选择
        #从previous到current一定有traffic
        if previous.get_traffic(current).get_line_name()==current.get_traffic(self).get_line_name():
            return None
        elif self.__transit_num+1>prefer["trans_limit"]:
            return None
        else:
            #中转额外惩罚:从步行转到公共交通,或者从公共交通转到其它公共交通时触发
            #注意和日常中转概念的微细差别:walk-->bus-->walk 在日常被认为零次中转,但在这里会触发一次惩罚,且是必要的
            transit_cost_extra=0
            if current.get_traffic(self).get_ttype()!="walk":
                transit_cost_extra=prefer["transit_as_time"]
            return current.get_score() + current.get_traffic(self).get_score() + transit_cost_extra

    def get_score(self):
        return self.__score

    #设置前继站点,参数是站点对象,而非id;同时更新从源点到此点的中转数
    def set_previous(self, prev):
        self.__previous = prev
        self.__update_trans_num()

    def get_previous(self):
        return self.__previous

    def set_visited(self):
        self.__visited = True

    def is_visited(self):
        return self.__visited

    #仅保留交通连接,其它属性重置
    def reset(self):
        self.__visited = False
        self.__previous = None
        self.__score=sys.maxint
        self.__transit_num=0

    #每两个站点之间肯定有交通
    def __update_trans_num(self):
        dept,dest=self.__previous,self
        self.__transit_num,lines_count=0,0
        while dept:
            if dept.get_traffic(dest).get_ttype()!="walk":

                lines_count +=1
            dept,dest=dept.__previous,dept
        if lines_count>0:
            self.__transit_num=lines_count-1

    #在搜索结束后,打印从源点到站点的路径
    def path_dump(self,traffic_net):
        if not self.__previous: #此时,源点即为该点自身
            return None

        time=0
        wait_time=0
        dist=0
        walk=0
        traffics=[]
        dept,dest=self.__previous,self
        while dept:
            #每两个站点之间肯定有交通
            traffic=dept.get_traffic(dest)
            traffics.append(traffic)

            time+=traffic.get_time()
            wait_time+=traffic.get_wait_time()
            dist+=traffic.get_dist()
            if traffic.get_ttype()=="walk":
                walk+=traffic.get_dist()

            dept,dest=dept.__previous,dept

        dept=dest #dept设为源点
        traffics.reverse()
        has_public_traffic=False
        infos=[]
        coords=[]
        public_traffic_begin_count=-2
        public_traffic_end_count=-1
        for i in range(len(traffics)):
            traffic=traffics[i]
            station_list=traffic.get_station_list()
            if traffic.get_ttype()=="walk":
                b_tag="::S" if i==0 else "::0"
                coords.append(traffic_net.get_station(station_list[0]).get_map_info()+b_tag)
                e_tag="::E" if i==len(traffics)-1 else "::0"
                coords.append(traffic_net.get_station(station_list[-1]).get_map_info()+e_tag)
                info="WALK#0#%d#%.5f#WALK#1#-1~-1#%s~%s##"%(traffic.get_run_time(),traffic.get_dist(),traffic_net.get_station(station_list[0]).get_name(),traffic_net.get_station(station_list[-1]).get_name())
            else:
                has_public_traffic=True
                public_traffic_begin_count+=2
                public_traffic_end_count+=2
                for j in range(len(station_list)):
                    if j==0:
                        tag="::S" if i==0 else "::"+str(public_traffic_begin_count)
                        coords.append(traffic_net.get_station(station_list[0]).get_map_info()+tag)
                    elif j== len(station_list)-1:
                        tag="::E" if i==len(traffics)-1 else "::"+str(public_traffic_end_count)
                        coords.append(traffic_net.get_station(station_list[-1]).get_map_info()+tag)
                    else:
                        coords.append(traffic_net.get_station(station_list[j]).get_map_info()+"::0")
                info="%s#0#%d#%.5f#%s#%d#0~0#%s~%s##"%(traffic.get_ttype(),traffic.get_run_time(),traffic.get_dist(),traffic.get_line_name().split('_')[0],len(station_list)-1,traffic_net.get_station(station_list[0]).get_name(),traffic_net.get_station(station_list[-1]).get_name())

            infos.append(info)

        info="|".join(infos)
        coor="|".join(coords)

        path={}
        path["time"]=time
        path["wait_time"]=wait_time
        path["dist"]=dist
        path["walk"]=walk
        path["info"]=info
        path["coor"]=coor
        path["type"]=2 if has_public_traffic else 1 #对于站点交通网络,只要有1段公共交通,就置为公共交通
        path["price"]="NULL"#输出格式中无用,但必须有
        global prefers, prefer_type
        prefer=prefers[prefer_type]
        path["trans_type"]=prefer["no"]
        path["fast"]=1
        path["start_id"]=dept.get_id()
        path["start_map"]=dept.get_map_info()
        path["dest_id"]=self.__id
        path["dest_map"]=self.__map_info
        path["score"]=self.__score
        path["city_id"]=traffic_net.get_city_id()
        path["transit_num"]=self.__transit_num

        #return json.dumps(path,ensure_ascii=False)
	return path


#由Station组成的交通网络
class TrafficNet:
    def __init__(self,city_id):
        self.__station_dict = {}
        self.__num_stations = 0
        self.__city_id=city_id
        #self.__route_file=open(city_id+'_routes','w')

    def __iter__(self):
        return iter(self.__station_dict.values())

    def add_station(self, id):
        self.__num_stations += 1
        new_station = Station(id)
        self.__station_dict[id] = new_station
        return new_station

    def get_station(self, id):
        if id in self.__station_dict:
            return self.__station_dict[id]
        else:
            return None

    #通过此接口可以间接地添加站点
    def add_traffic(self, frm, to, traffic):
        if frm not in self.__station_dict:
            self.add_station(frm)
        if to not in self.__station_dict:
            self.add_station(to)

        self.__station_dict[frm].add_traffic(self.__station_dict[to], traffic)

    #给所有点对之间尝试添加walk交通
    def add_walk_traffic(self):
        for dept_s in self:
            for dest_s in self:
                dept_s.add_traffic(dest_s)
                dest_s.add_traffic(dept_s)

    def dump_stations(self):
	out_file = open('%s_station.txt' %self.__city_id,'w')
	for station in self:
	   out_file.write(str(station)+'\n')
        out_file.close()

    def get_city_id(self):
        return self.__city_id

    def __dump_all(self):
        #交通站点插入到redis数据库中
        transit_count = {}
	for s in self:
	    path=s.path_dump(self)
	    if path:
                #统计以某个站点为起点的N转交通的条数
                transit_key = path["transit_num"]
                if transit_key in transit_count:
                   transit_count[transit_key] += 1
                else:
                   transit_count[transit_key] = 1
	        str_key=path["start_map"]
		str_key+="###"
		str_key+=path["dest_map"]
		str_key+="@0@0"
	        r0.set(str_key,path)
        return transit_count
        
    #切换起点时要把交通网络的状态重置
    def __reset(self):
        for station in self:
            station.reset()

    #使用dijkstra 寻找最优路径
    def find_route(self,start):
        self.__reset()
        start.set_score(0)

        while True:
            unvisited_queue = [v for v in self if not v.is_visited()]
            if len(unvisited_queue)==0:
                break
            current=heapq.nsmallest(1,unvisited_queue,key= lambda v: v.get_score())[0]
            current.set_visited()

            for next in current.get_dest_stations():
                if not next.is_visited():
                    new_score=next.cal_score(current)
                        #print 'current = %s next = %s old_score=%s new_score = %s'%(current.get_id(), next.get_id(), next.get_score(), new_score)
                        #注意一定是代价变小时才触发,相等时不无益触发
                    if new_score and new_score < next.get_score():
                       next.set_score(new_score)
                       next.set_previous(current)
        #N转交通路线的条数
        transit_count = self.__dump_all()
        return transit_count

def find_all():
    city_conf={}
    with open('../data/city.conf', 'r') as f:
        for line in f.readlines():
            details=line.strip().split()
            city_id=details[1]
            city_conf[city_id]={}
            city_conf[city_id]["lng_min"]=details[2]
            city_conf[city_id]["lat_min"]=details[3]
            city_conf[city_id]["lng_max"]=details[4]
            city_conf[city_id]["lat_max"]=details[5]

    city_list=[]
    with open('../data/city.list', 'r') as f:
        for line in f.readlines():
            city_list.append(line.strip())

    for city in city_list:
        traffic_net=TrafficNet(city)
        #生产该城市的所有poi点
        get_poi.generate_poi(city)
        for ttype in public_traffic_types:
            file_name=city+"."+ttype
            with open('../data/'+file_name, 'r') as f:
                for line in f.readlines():
                    blocks=line.split("###")
                    line_name=blocks[0]
                    time_table=blocks[3][6:-1]
                    station_desc=blocks[4:]
                    dept_times_strs=time_table.split("][")
                    dept_times=[]
                    for one_day in dept_times_strs:
                        dept_times.append(eval('['+one_day[9:]+']'))
                    sorted(dept_times,key=lambda x:len(x))
                    normal=dept_times[(len(dept_times)-1)/2]
                    wait_time=(normal[-1]-normal[0])*60*60/((len(normal)-1)*2) if len(normal)>1 else 6*60*60 #6个小时
                    stations=[]
                    for i in range(len(station_desc)/8):
                        #站点名#经纬度,到达时间相对首站出发偏移量,出发时间相对首站出发偏移量
                        if station_desc[i*8+1]>=city_conf[city]["lng_min"] and station_desc[i*8+1]<=city_conf[city]["lng_max"] and station_desc[i*8+2]>=city_conf[city]["lat_min"] and station_desc[i*8+2]<=city_conf[city]["lat_max"]:
                            stations.append([station_desc[i*8+0]+"#"+station_desc[i*8+1]+","+station_desc[i*8+2],round(float(station_desc[i*8+4]),2),round(float(station_desc[i*8+5]),2)])

                    for i in range(len(stations)-1):
                        for j in range(i+1,len(stations)):
                            dept_id=stations[i][0]
                            dest_id=stations[j][0]
                            run_time=(stations[j][1]-stations[i][2])*60*60
                            station_pass=[station[0] for station in stations[i+1:j]]
                            traffic_net.add_traffic(dept_id,dest_id,Traffic(dept_id,dest_id,run_time,wait_time,ttype,line_name,station_pass))

        traffic_net.dump_stations()
        traffic_net.add_walk_traffic()
        res = {}
        station_num = 0
        for station in traffic_net:
            station_num += 1
            transit_count = traffic_net.find_route(station)
            #统计所有交通路线N转交通的条数
            for key,val in transit_count.items():
                if key in res:
                   res[key] += val
                else:
                   res[key] = val
        routes_num = station_num * (station_num - 1) 
        for key,val in res.items():
            print("%d 转交通路线条数%d 及其所占的比例：%f"%(key,val,float(val)/routes_num))
        generate_poi_dic(city)
        find_all_path(city)

#生成(key,value),其中key是该城市非站点id，value是字符串"站点id|距离@站点id|距离,..."）并且插入到redis数据库中
def generate_poi_dic(city_id):
     station_list = []
     all_poi_list = []
     not_station  = []
     with open("%s_station.txt"%city_id, 'r') as f:
	for line in f.readlines():
	    station_list.append(line)	    	
     f.close()
     with open("%s_all_poi.txt"%city_id, 'r') as f:
	for line in f.readlines():
            all_poi_list.append(line)
     f.close()
     for item in all_poi_list:
	if item not in station_list:
	     not_station.append(item)    
     dic = dict()
     for not_stationNode in not_station:
	dic[not_stationNode]=""
	for stationNode in station_list:
           #start_list = not_stationNode.split('#')
           #dest_list = stationNode.split('#')
           #start_map = start_list[len(start_list) - 1]
           #dest_map = dest_list[len(dest_list) - 1]
           #dic[not_stationNode] += stationNode.strip() +"|"+str(CoordDist.getDistance(start_map.split(',')+dest_map.split(',')))+"@"
           dic[not_stationNode] += stationNode.strip() +"|"+str(CoordDist.getDistance(not_stationNode.split('#')[-1].split(',')+stationNode.split('#')[-1].split(',')))+"@"
     for k in dic:
        if k:
	   r1.set(k.strip(),str(dic[k]))
     generate_poi_pair(station_list,not_station,city_id)

#生产poi点对信息，写入到cityID.query文件中。startId，destId）startId中不仅有poi点Id还有该poi类型信息，0表示站点，1表示非站点
def generate_poi_pair(station_list,not_station,city_id):
     all_poi_type = []
     for item in station_list:
         item = item.strip() + '|0'
         all_poi_type.append(item.strip())
     for item in not_station:
         item = item.strip() + '|1'
         all_poi_type.append(item.strip())
     #生产所有的poi点对
     out_file = open("%s.query"%city_id,'w')
     for dest in all_poi_type:
         for start in all_poi_type:
             if start.strip() != dest.strip():
		out_file.write(start.strip()+"|"+dest.strip()+'\n')
     out_file.close()

#路径信息写入到ctiy_id_num.case文件中，每个文件中含有num条路径数据
def write_file(path_list,city_id,file_num):
    file_name = city_id+"_"+str(file_num)
    out_file = open("%s.case"%file_name,'w')
    for path in path_list:
        out_file.write(path+'\n')
    out_file.close()
     
#找到该城市所有poi点对的最优路径
def find_all_path(city_id):
    path_num = 0
    path_list = []
    file_num = 0
    with open("%s.query01"%city_id,'r') as f:
	for line in f.readlines():
            #start起始poi start_type起始poi类型，0表示交通站点，1表示非站点
            start,start_type,dest,dest_type = line.split('|')
            #两个poi点都是交通站点，直接从redis中拿到数据
            strKey = start.strip().split('#')[-1]+"###"+dest.strip().split('#')[-1]+"@0@0"
            dis = CoordDist.getDistance(start.split('#')[-1].split(',')+dest.split('#')[-1].split(','))
            if start_type.strip() == "0" and dest_type.strip() == "0":
                path = r0.get(strKey)
                if path:
                   path_dic = eval(path)
                   if path_dic["time"] > 3*60*60:
                      path1 = google_driving_path(start,dest)
                      if path1 != "":
                         path_list.append(path1)
                         path_num += 1
                   path_list.append(str(path))
                   path_num += 1
                   #当路径条数等于PATHNUM时，写入文件
                   if path_num >= PATHNUM:
                      write_file(path_list,city_id,file_num)
                      path_list = []
                      path_num = 0
                      file_num += 1
                else:
                   path = google_walking_driving_path(start,dest,city_id,dis)  
                   path_num += 1
                   if path_num >= PATHNUM:
                      write_file(path_list,city_id,file_num)
                      path_list = []
                      path_num = 0
                      file_num += 1
            #只要有一个poi点是非交通站点，则需要拼接路线信息
	    else:
                path = ""
                #两个poi点之间的距离大于1000m
                if dis > 1000:
                   paths = find_path(start.strip(),start_type.strip(),dest.strip(),dest_type.strip(),city_id)
                   for path in paths:
                      path_num += 1
                      path_list.append(path)
                #两个poi点之间的距离小于1000m，获得Google步行数据
                else:
                   path = google_walk_path(start.strip(),dest.strip())      
                   #googleb步行数据为空时，拉一条直线距离
                   pathStr = str(path)
                   if pathStr == "d None None" or pathStr == "not_found None None":
                       pathStr = line_walk_path(start.strip(),dest.strip(),city_id,dis)
                   else:
                       pathStr = pathStr[8:]
                   path_list.append(pathStr)
                   path_num += 1
                if path_num >= PATHNUM:
                   write_file(path_list,city_id,file_num)
                   path_list = []
                   path_num = 0
                   file_num += 1 
    write_file(path_list,city_id,file_num)
    f.close()

def google_walk_path(start,dest):
    #start_list = start.split('#')
    #dest_list = dest.split('#')
    #real_start_map = formatCoor(start_list[len(start_list) - 1])
    #real_dest_map = formatCoor(dest_list[len(dest_list) - 1])
    real_start_map = formatCoor(start.split('#')[-1])
    real_dest_map = formatCoor(dest.split('#')[-1])
    ssdb_key = "###".join(['walking',real_start_map,real_dest_map])
    path = ssdb.request('get',[ssdb_key])
    return path


def line_walk_path(start,dest,city_id,dist):
    global prefers,prefer_type
    start_list = start.split('#')
    dest_list = dest.split('#')
    start_name = start_list[0]
    #start_map = start_list[len(start_list) - 1]
    start_map = start_list[-1]
    dest_name = dest_list[0]
    dest_map = dest_list[-1]
    #dest_map = dest_list[len(dest_list) - 1]
        
    time = dist / prefers["time"]["walk_velo"]
    info = "WALK#0#"+str(time)+"#"+str(dist)+"#WALK#-1~-1#"+start_name+"~"+dest_name+"##"
    coor = start_map+"::S|"+dest_map+"::E"
    path={}
    path["time"] = time
    path["wait_time"] = 0
    path["dist"] = dist
    path["walk"] = dist
    path["info"] = info
    path["type"] = 2
    path["coor"] = coor
    path["price"] = "NULL"
    prefer=prefers[prefer_type]
    path["trans_type"]=prefer["no"]
    path["fast"] = 1
    path["start_id"] = start
    path["start_map"] = start_map
    path["dest_id"] = dest
    path["dest_map"] = dest_map
    path["score"] = time
    path["city_id"] = city_id
    path["transit_num"] = 0 
    return str(path)

#剪枝掉poi点附近的不合理的交通站点
def pruning_nearest_station(nearest_station):
    station_count = len(nearest_station)
    for index1 in range(station_count):
        if index1 == (len(nearest_station)):
           break
        start_station = nearest_station[index1][0]
        start_map = start_station.split('#')[-1]
        tmp = nearest_station[0:index1+1]
        for index2 in range(index1+1,len(nearest_station)):
            dest_station = nearest_station[index2][0]
            dest_map = dest_station.split('#')[-1]
            dis = CoordDist.getDistance(start_station.split('#')[-1].split(',')+dest_station.split('#')[-1].split(','))
            if dis >= 500:
               tmp.append(nearest_station[index2])
        tmp.sort(key=lambda x:x[1])
        nearest_station = tmp
    return nearest_station
    
#找两个poi点之间的最优路径
def find_path(start,start_type,dest,dest_type,city_id):    
     start_nearest_station = []
     dest_nearest_station = []
     res_path = ""
     #起始和终止poi点都是非站点，找到当前poi点的最近的十个站点，组合求最优路径
     if start_type == "1" and dest_type == "1":
        start_station_dis = r1.get(start).split('@')
        for item in start_station_dis:
           if item:
              arr = item.split('|')
              start_nearest_station.append((arr[0],float(arr[1])))
        start_nearest_station = heapq.nsmallest(10,start_nearest_station,key=lambda x:x[1])
        start_nearest_station = pruning_nearest_station(start_nearest_station)
        dest_station_dis = r1.get(dest).split('@')
        for item in dest_station_dis:
           if item:
              arr = item.split('|')
              dest_nearest_station.append((arr[0],float(arr[1])))
        dest_nearest_station = heapq.nsmallest(10,dest_nearest_station,key=lambda x:x[1])
        dest_nearest_station = pruning_nearest_station(dest_nearest_station)
        res_path = find_path_sub(start,dest,start_type,dest_type,start_nearest_station,dest_nearest_station,city_id)
     #起始poi是非交通站点，得到该poi点附近的十个站点
     elif start_type == "1" and dest_type == "0":
        start_station_dis = r1.get(start).split('@')
        for item in start_station_dis:
           if item:
              arr = item.split('|')
              start_nearest_station.append((arr[0],float(arr[1])))
        start_nearest_station = heapq.nsmallest(10,start_nearest_station,key=lambda x:x[1])
        start_nearest_station = pruning_nearest_station(start_nearest_station)
        dest_nearest_station = [(dest,0.0)]
        res_path = find_path_sub(start,dest,start_type,dest_type,start_nearest_station,dest_nearest_station,city_id)
     #终止poi是非交通站点，得到该poi点附近的十个站点
     elif start_type == "0" and dest_type == "1":
        dest_station_dis = r1.get(dest).split('@')
        for item in dest_station_dis:
           if item:
              arr = item.split('|')
              dest_nearest_station.append((arr[0],float(arr[1])))
        dest_nearest_station = heapq.nsmallest(10,dest_nearest_station,key=lambda x:x[1])
        dest_nearest_station = pruning_nearest_station(dest_nearest_station)
        start_nearest_station = [(start,0.0)]
        res_path = find_path_sub(start,dest,start_type,dest_type,start_nearest_station,dest_nearest_station,city_id)
     elif start_type == "0" and dest_type == "0":
          print "0------>>>>0"
     res_path_list = [res_path]
     res_path_dic = eval(res_path)
     if res_path_dic["time"] >= 3*60*60:
        res_path = google_driving_path(start,dest)
        if res_path != "":
           res_path_list.append(res_path)
     return res_path_list
        

#当当两点之间的时间花费超过3小时，获取Google驾车数据
def google_driving_path(start,dest):
    start_map = start.split('#')[-1]
    dest_map = dest.split('#')[-1]
    real_start_map = formatCoor(start_map)
    real_dest_map = formatCoor(dest_map)
    ssdb_key_driving = "###".join(["driving",real_start_map,real_dest_map])
    path_driving = ssdb.request("get",[ssdb_key_driving])
    path_driving_str = str(path_driving)
    if path_driving_str == "d None None" or path_driving_str == "not_found None None":
       return ""
    return path_driving_str[8:]

#找两个poi点之间的最优路径的子程序
def find_path_sub(start,dest,start_type,dest_type,start_nearest_station,dest_nearest_station,city_id):
     #start_list = start.split('#')
     #dest_list = dest.split('#')
     #start_poi_map = start_list[len(start_list) - 1]
     #dest_poi_map = dest_list[len(dest_list) - 1]
     start_poi_map = start.split('#')[-1]
     dest_poi_map = dest.split('#')[-1]
     strKey_list = []
     #起始和终止poi点附近所有交通站点的组合，方便从redis数据库中批量读取数据
     for start_station in start_nearest_station:
         start_map = start_station[0].split('#')[-1]
         for dest_station in dest_nearest_station:
             dest_map = dest_station[0].split('#')[-1]
             if start_map != dest_map:
                strKey = start_map.strip()+"###"+dest_map.strip()+"@0@0"
                strKey_list.append(strKey)
     #批量读取交通路线数据
     strValue_list = r0.mget(strKey_list)
     path_list = {}
     #读取的数据拼接成字典形式
     for index in range(len(strValue_list)):
         path_list[strKey_list[index]] = strValue_list[index]
     min_score = sys.maxint
     res_path = {}
     start_path_str_res = ""
     dest_path_str_res = ""
     path_str_res = ""
     start_t = time.time() 
     #批量获取起始poi点到附近交通站点的Google交通数据（步行数据和驾车数据）
     start_walking_ssdbkey_list = []
     start_driving_ssdbkey_list = []
     for start_station in start_nearest_station:
         start_map = start_station[0].split('#')[-1]
         real_start_map = formatCoor(start_poi_map)
         real_dest_map = formatCoor(start_map)
         ssdb_key_walking = "###".join(["walking",real_start_map,real_dest_map])
         ssdb_key_driving = "###".join(["driving",real_start_map,real_dest_map])
         start_driving_ssdbkey_list.append(ssdb_key_driving)
         start_walking_ssdbkey_list.append(ssdb_key_walking)
     start_walking_path_str = str(ssdb.request('multi_get',start_walking_ssdbkey_list))         
     start_driving_path_str = str(ssdb.request('multi_get',start_driving_ssdbkey_list))
     start_walking_path_str = start_walking_path_str[8:]
     start_driving_path_str = start_driving_path_str[8:]
     start_walking_path_dic = eval(start_walking_path_str)
     start_driving_path_dic = eval(start_driving_path_str)
     start_driving_path_dic = start_driving_path_dic["items"]
     start_walking_path_dic = start_walking_path_dic["items"]

     #批量获取终止poi点到附近交通站点的Google交通数据（步行数据和驾车数据）
     dest_walking_ssdbkey_list = []
     dest_driving_ssdbkey_list = []
     for dest_station in dest_nearest_station:
         dest_map = dest_station[0].split('#')[-1]
         real_start_map = formatCoor(dest_map)
         real_dest_map = formatCoor(dest_poi_map)
         ssdb_key_walking = "###".join(["walking",real_start_map,real_dest_map])
         ssdb_key_driving = "###".join(["driving",real_start_map,real_dest_map])
         dest_walking_ssdbkey_list.append(ssdb_key_walking)
         dest_driving_ssdbkey_list.append(ssdb_key_driving)
     dest_walking_path_str = str(ssdb.request('multi_get',dest_walking_ssdbkey_list))
     dest_driving_path_str = str(ssdb.request('multi_get',dest_driving_ssdbkey_list))
     dest_walking_path_str = dest_walking_path_str[8:]
     dest_driving_path_str = dest_driving_path_str[8:]
     dest_walking_path_dic = eval(dest_walking_path_str)
     dest_driving_path_dic = eval(dest_driving_path_str)
     dest_walking_path_dic = dest_walking_path_dic["items"]
     dest_driving_path_dic = dest_driving_path_dic["items"]
     #print "ssdb mutli_get : ",end_t - start_t
     #拼接Google获取的交通数据和交通站点的交通数据进行拼接 
     for start_station in start_nearest_station:
        start_map = start_station[0].split('#')[-1]
        start_real_start_map = formatCoor(start_poi_map)
        start_real_dest_map = formatCoor(start_map)
        start_dis = CoordDist.getDistance(start_poi_map.split(',')+start_map.split(','))
        start_path = ""
        #两点之间的直线距离小于3000m时，优先选择Google步行数据，如果Google步行数据为空，则选择驾车数据，如果驾车数据也为空，则在两点之间拉一条直线
        if start_dis <= 3000:
           ssdb_key_walking = "###".join(["walking",start_real_start_map,start_real_dest_map])
           start_path = start_walking_path_dic.get(ssdb_key_walking)
           if start_path is None:
              ssdb_key_driving = "###".join(["driving",start_real_start_map,start_real_dest_map])
              start_path = start_driving_path_dic.get(ssdb_key_driving)
              if start_path is None:
                 start_path = line_walk_path(start,start_station[0],city_id,start_dis)
        else:
           ssdb_key_driving = "###".join(["driving",start_real_start_map,start_real_dest_map])
           start_path = start_driving_path_dic.get(ssdb_key_driving)
           if start_path is None:
              ssdb_key_walking = "###".join(["walking",start_real_start_map,start_real_dest_map])
              start_path = start_walking_path_dic.get(ssdb_key_walking)
              if start_path is None:
                 start_path = line_walk_path(start,start_station[0],city_id,start_dis)
        #start_path = google_walking_driving_path(start_poi_map,start_map,start_dis)
        #google返回的数据为空，则在两个poi点之间拉一条直线
        #if start_path == "":
         #  start_path = line_walk_path(start,start_station[0],city_id,start_dis)
        for dest_station in dest_nearest_station:
            dest_map = dest_station[0].split('#')[-1]
            dest_real_start_map = formatCoor(dest_map) 
            dest_real_dest_map = formatCoor(dest_poi_map)
            dest_dis = CoordDist.getDistance(dest_poi_map.split(',')+dest_map.split(','))
            dest_path = ""
            if dest_dis <= 3000:
               ssdb_key_walking = "###".join(["walking",dest_real_start_map,dest_real_dest_map])
               dest_path = dest_walking_path_dic.get(ssdb_key_walking)
               if dest_path is None:
                  ssdb_key_driving = "###".join(["driving",dest_real_start_map,dest_real_dest_map])
                  dest_path = dest_driving_path_dic.get(ssdb_key_driving)
                  if dest_path is None:
                     dest_path = line_walk_path(dest_station[0],dest,city_id,dest_dis)
            else:
               ssdb_key_driving = "###".join(["driving",dest_real_start_map,dest_real_dest_map])
               dest_path = dest_driving_path_dic.get(ssdb_key_driving)
               if dest_path is None:
                  ssdb_key_walking = "###".join(["walking",dest_real_start_map,dest_real_dest_map])
                  dest_path = dest_walking_path_dic.get(ssdb_key_walking)
                  if dest_path is None:
                     dest_path = line_walk_path(dest_station[0],dest,city_id,dest_dis)
            #dest_path = google_walking_driving_path(dest_map,dest_poi_map,dest_dis)
            #google返回的数据为空，则在两个poi点之间拉一条直线
            #if dest_path == "":           
            #   dest_path = line_walk_path(dest_station[0],dest,city_id,dest_dis)
            if start_map != dest_map:
               strKey = start_map.strip()+"###"+dest_map.strip()+"@0@0"
               #pathStr = r0.get(strKey)
               pathStr = path_list[strKey]
               #两个交通站点之间没有计算出路径信息，则选择google步行数据
               if pathStr is None:
                  path_dis = CoordDist.getDistance(start_map.split(',')+dest_map.split(','))
                  pathStr = google_walking_driving_path(start_station[0],dest_station[0],city_id,path_dis) 
               start_path_dic = eval(start_path)
               dest_path_dic = eval(dest_path)
               try:
                  path_dic = eval(pathStr)
               except:
                  print strKey
               tmp_score = int(path_dic["score"])
               if start_type.strip() == "1":
                  tmp_score += int(start_path_dic["time"])
               if dest_type.strip() == "1":
                  tmp_score += int(dest_path_dic["time"])
               #path_dic = combine_path(start_path,dest_path,start_type,dest_type,pathStr)
               if tmp_score < min_score:
                  start_path_str_res = start_path
                  dest_path_str_res = dest_path
                  path_str_res = pathStr
                  min_score = tmp_score
     res_path = combine_path(start_path_str_res,dest_path_str_res,start_type,dest_type,path_str_res)
     res_path["start_id"] = start
     res_path["dest_id"] = dest
     res_path["start_map"] = start.split('#')[-1]
     res_path["dest_map"] = dest.split("#")[-1]
      
     #print res_path
     return str(res_path)

def combine_path(start_path,dest_path,start_type,dest_type,pathStr):
    start_path_dic = eval(start_path)
    dest_path_dic = eval(dest_path)
    path_dic = eval(pathStr)
    max_stop = 0
    if start_type == "1" and start_path_dic["dist"] > 0.0:
        path_dic["time"] = int(start_path_dic["time"])+int(path_dic["time"])
        path_dic["dist"] = int(start_path_dic["dist"])+int(path_dic["dist"])
        path_dic["score"] = int(start_path_dic["time"])+int(path_dic["score"])
        start_list = start_path_dic["coor"].split('|')
        del start_list[len(start_list)-1]
        strpath = "|".join(start_list)
        path_list=path_dic["coor"].split('|')
        for index in range(len(path_list)):
            arr = path_list[index].split('::')
            if arr[1] == 'S':
               path_list[index] = arr[0]+"::1"
            elif arr[1] != 'E' and int(arr[1]) > 0:
               max_stop = int(arr[1])
               path_list[index] = arr[0]+"::"+str(int(arr[1])+1)
        path_dic["coor"] = "|".join(path_list)
        path_dic["coor"] = strpath+"|"+path_dic["coor"]
        path_dic["info"] = start_path_dic["info"]+path_dic["info"]
    if dest_type == "1" and dest_path_dic["dist"] > 0.0:
        path_dic["time"] = int(dest_path_dic["time"])+int(path_dic["time"])
        path_dic["dist"] = int(dest_path_dic["dist"])+int(path_dic["dist"])
        path_dic["score"] = int(dest_path_dic["time"])+int(path_dic["score"])
        #path_dic["coor"] = path_dic["coor"]+"|"+dest_path_dic["coor"] 
        dest_list = dest_path_dic["coor"].split("|")
        del dest_list[0]
        strpath = "|".join(dest_list)
        path_list = path_dic["coor"].split('|')
        for item in path_list:
            arr = item.split('::')
            if arr[1] != 'S' and arr[1] != 'E' and int(arr[1]) > max_stop:
               max_stop = int(arr[1])
        path_list[len(path_list)-1] = path_list[len(path_list)-1].split('::')[0]+"::"+str(max_stop+1)
        path_dic["coor"] = "|".join(path_list)
        path_dic["coor"] = path_dic["coor"]+"|"+strpath
               
        path_dic["info"] = path_dic["info"]+dest_path_dic["info"]
    return path_dic
            
#获取两点之间的google步行数据和驾车数据，如果两者都没有，则拉一条直线距离
def google_walking_driving_path(start,dest,city_id,dist):
    start_map = start.split('#')[-1]
    dest_map = dest.split('#')[-1]
    real_start_map = formatCoor(start_map)
    real_dest_map = formatCoor(dest_map)
    ssdb_key_walking = "###".join(["walking",real_start_map,real_dest_map])
    ssdb_key_driving = "###".join(["driving",real_start_map,real_dest_map])
    if dist < 3000:
       path_walking = ssdb.request("get",[ssdb_key_walking])
       path_walking_str = str(path_walking)
       if path_walking_str == "d None None" or path_walking_str == "not_found None None":
          path_driving = ssdb.request("get",[ssdb_key_driving])
          path_driving_str = str(path_driving)
          #google步行和驾车数据都为空，拉一条直线路径
          if path_driving_str == "d None None" or path_driving_str == "not_found None None":
             return line_walk_path(start,dest,city_id,dist)
          else:
             return path_driving_str[8:]
       else:
          return path_walking_str[8:]
    else:
       path_driving = ssdb.request("get",[ssdb_key_driving])
       path_driving_str = str(path_driving)
       if path_driving_str == "d None None" or path_driving_str == "not_found None None":
          path_walking = ssdb.request("get",[ssdb_key_walking])
          path_walking_str = str(path_walking)
          #google步行和驾车数据都为空时，拉一条直线距离
          if path_walking_str == "d None None" or path_walking_str == "not_found None None":
             return line_walk_path(start,dest,city_id,dist)
          else:
             return path_walking_str[8:]
       else:
          return path_driving_str[8:]
       

if __name__ == '__main__':
    find_all()

