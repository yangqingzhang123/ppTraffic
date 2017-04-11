import random
import json

def parse_path(line):
    route=eval(line)
    route.pop("type")
    route.pop("score")
    route.pop("fast")
    route.pop("price")
    route.pop("trans_type")
    route.pop("coor")
    route.pop("city_id")
    route["time"]=str(float(route["time"])/60)+"min"
    route["wait_time"]=str(float(route["wait_time"])/60)+"min"
    route["dist"]=str(route["dist"])+"m"
    route["walk"]=str(route["walk"])+"m"
    parts=route["info"].split("|")
    count=0
    infos=[]
    for part in parts:
        count+=1
        part_d=dict()
        items=part.split("#")
        part_d["no"]="the "+str(count)+" part"
        part_d["type"]=items[0]
        part_d["time"]=str(float(items[2])/60)+"min"
        part_d["dist"]=items[3]+"m"
        part_d["line_name"]=items[4]
        part_d["pass_no"]=items[5]
        part_d["start"],part_d["end"]=items[7].split("~")
        infos.append(part_d)
        route["info"]=infos
    return json.dumps(route)
