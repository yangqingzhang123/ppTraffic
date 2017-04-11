#! /usr/bin/env python
# -*- coding:utf-8 -*-

import sys
sys.path.append('../script')
from common import db_online
from common import utils
import json
reload(sys)
sys.setdefaultencoding("utf-8")


def load_train_station(city_id):
    query_sql = 'select * from station where city_id = "%s";' % city_id
    query_result = db_online.QueryBySQL(query_sql)

    station_dict = dict()
    for each in query_result:
        station_dict[each['station_id']] = {'id': each['station_id'],
                'map': each['map_info'],
                'name': each['station']
                }

    return station_dict


def load_attr_data(city_id):
    query_sql = 'select * from attraction where city_id = "%s"' % city_id
    query_result = db_online.QueryBySQL(query_sql)

    attr_dict = dict()
    for each in query_result:
        attr_dict[each['id']] = {'id': each['id'],
                'name': each['name'],
                'map': each['map_info']
                }

    return attr_dict


def load_hotel_data(city_id):
    query_sql = 'select * from hotel where city_mid = "%s"' % city_id
    query_result = db_online.QueryBySQL(query_sql)

    hotel_dict = dict()
    for each in query_result:
        hotel_dict[each['uid']] = {'id': each['uid'],
                'name': each['hotel_name_en'],
                'map': each['map_info']
                }

    return hotel_dict


def load_airport_data(city_id):
    query_sql = 'select * from airport where city_id = "%s"' % city_id
    query_result = db_online.QueryBySQL(query_sql)

    airport_dict = dict()
    for each in query_result:
        airport_dict[each['iata_code']] = {'id': each['iata_code'],
                'name': each['name_en'],
                'map': each['map_info']
                }

    return airport_dict


def load_bus_data(city_id):
    query_sql = 'select * from bus where city_id = "%s"' % city_id
    query_result = db_online.QueryBySQL(query_sql)

    bus_station_dict = dict()
    for each in query_result:
        bus_station_dict[each['station_id']] = {'id': each['station_id'],
                'name': each['name_en'],
                'map': each['map_info']
                }

    return bus_station_dict


def load_shopping_data(city_id):
    query_sql = 'select * from shopping where city_id = "%s";' % city_id
    query_result = db_online.QueryBySQL(query_sql)

    shopping_dict = dict()
    for each in query_result:
        shopping_dict[each['id']] = {'id': each['id'],
                'name': each['name_en'],
                'map': each['map_info']
                }

    return shopping_dict


def generate_poi(city_id):

    station_data = load_train_station(city_id)
    attr_data = load_attr_data(city_id)
    hotel_data = load_hotel_data(city_id)
    airport_data = load_airport_data(city_id)
    bus_data = load_bus_data(city_id)
    shopping_data = load_shopping_data(city_id)
    

    para_dict = {'station': station_data,
            'attr': attr_data,
            'hotel': hotel_data,
            'airport': airport_data,
            'bus': bus_data,
            'shopping': shopping_data
            }
    out_file = open("%s_all_poi.txt" % city_id,'w')
    for i in para_dict.keys():
	for j in para_dict[i]:
	    out_file.write(para_dict[i][j]["name"]+"#"+para_dict[i][j]["map"]+'\n')
    out_file.close()
    return
if __name__ == '__main__':
    generate_poi('20140')

