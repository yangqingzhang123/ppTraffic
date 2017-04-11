#ifndef __MY_CONFIG_H__
#define __MY_CONFIG_H__

#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <boost/lexical_cast.hpp>

class MyConfig{
public:
    MyConfig();
    ~MyConfig();
    static bool init(const std::string& path);
    static int getInt(const std::string& key);
    static std::string getString(const std::string& key);
    static double getDouble(const std::string& key);
public:
    static std::tr1::unordered_map<std::string,std::string> _kv;
};


#endif  //__MY_CONFIG_H__
