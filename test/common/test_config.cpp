
#include <list>
#include "../../code/common/config.h"
using namespace std;
auto port = myriel::Config::Lookup("system.port",8080,"bind port");
auto vec = myriel::Config::Lookup("system.vec",vector<int>{2,3},"vec");
//auto logss = myriel::Config::Lockup("logs",vector<string>(),"logs");
class Person{
public:
    Person() = default;
    Person(const string& name,int age): m_age(age), m_name(name){}
    int m_age = 0;
    std::string m_name ;
    std::string toString() const {
        std::stringstream ss;
        ss<<"[ name="<<m_name<<", age="<<m_age<<" ]";
        return ss.str();
    }
    bool operator==(const Person& oth){
        return m_age == oth.m_age && m_name == oth.m_name;
    }
};
namespace myriel{
template<>
class LaxicalCast<std::string ,Person>{
public:
    Person operator()(const std::string& str){
        YAML::Node node = YAML::Load(str);
        Person res;
        res.m_name = node["name"].as<std::string>();
        res.m_age = node["age"].as<int>();
        return res;
    }
};

template<>
class LaxicalCast<Person ,std::string >{
public:
    std::string operator()(const Person& v){
        YAML::Node node;
        node["name"] = v.m_name;
        node["age"] = v.m_age;
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};
}
void test1(){
    LOG_INFO(LOG_ROOT()) << "Before\n" << vec->toString();

    //YAML::Node config = YAML::LoadFile("config/log.yaml");
    myriel::Config::LoadFromFile("config/log.yaml");
    myriel::Config::Lookup("system.port","123"s,"vec");
    LOG_INFO(LOG_ROOT()) << "After\n" << vec->toString();
    //cout<<myriel::LaxicalCast<string,vector<int>>()("- 1\n- 2\n- 5")[2];
    //cout<<myriel::LaxicalCast<list<list<vector<int>>>,string>()(list<list<vector<int>>>{{{1,2,4,5},{2,3}}});
    list<float> value{1.2,3.1415};
    cout<<myriel::LaxicalCast<map<string,list<float>>,string>()(map<string,list<float>> {{"myriel",value}});
}
void test2(){
    //auto pm = myriel::Config::Lookup("map",map<string,vector<Person>>(),"");

    //pm->setValue(map<string,vector<Person>>());
    auto a = myriel::Config::Lookup("class.person",Person(),"");
//    a->addListener(1,[](const auto& old_val, const auto& new_val){
//        myriel_LOG_INFO(myriel_LOG_ROOT())<<old_val.toString()<<" -> "<<new_val.toString();
//    });

    a->setValue(Person("aaa",44));
    //myriel_LOG_DEBUG(myriel_LOG_ROOT())<<"Before \n"<<a->toString()<<" "<<a->getValue().toString();
    //myriel::Config::LoadFromFile("config/log.yaml");

    //auto tmp = pm->getValue();
    myriel::Config::Visit([](myriel::ConfigVarBase::ptr config){
        LOG_INFO(LOG_ROOT()) << "" << config->getName() << " " 
			<< config->getDescription() << " " << config->toString();
    });

    //auto b = a->getValue().toString();
}
int main(){
    test2();
}
