# -*- coding:utf-8 -*-

import math
import sys

EARTH_RADIUS = 6378137
PI = 3.1415927

def rad(d):
    return d * PI / 180.0

# 返回值单位：米
def getDist(lng1, lat1, lng2, lat2):
    radLat1 = rad(lat1)
    radLat2 = rad(lat2)
    a = radLat1 - radLat2
    b = rad(lng1) - rad(lng2)

    s = 2 * math.asin(math.sqrt(math.pow(math.sin(a/2), 2) + math.cos(radLat1) * math.cos(radLat2) * math.pow(math.sin(b/2),2)))

    s = s * EARTH_RADIUS
    s = round(s * 10000) / 10000

    return s

def getDistance(args):
    lng1=float(args[0])
    lat1=float(args[1])
    lng2=float(args[2])
    lat2=float(args[3])
    return getDist(lng1, lat1, lng2, lat2)

if __name__ == '__main__':
    #print getDist(float(sys.argv[1]), float(sys.argv[2]), float(sys.argv[3]), float(sys.argv[4]))
    print getDist(116.39985, 39.914004, 117.236131, 31.853693)
    print getDistance((116.39985, 39.914004, 117.236131, 31.853693))
    print getDistance((u'2.351492339147967', u'48.85746107178952', u'4.835658999999964', u'45.764043'))
    #print getDist(2.309327, 48.872395, 2.3092, 48.872299)
    #print getDist(6.13334, 49.60053, 6.13335, 49.60054)
