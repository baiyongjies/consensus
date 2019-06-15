/*
 * blockchain-network.h
 *
 *  Created on: 2018年3月11日
 *      Author: blackguess
 */

#ifndef BLOCKCHAIN_NETWORK_H
#define BLOCKCHAIN_NETWORK_H


//#include "POV.h"
#include "CallBackInstance.h"
#include <memory>
#include "constants.h"
#include "DocumentContainer.h"
#include <queue>
#include <ctime>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "dfnetwork.h"
#include <thread>
#include "udp.h"
#include "tcp.h"
#include <mutex>
//#include <boost/thread/thread.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include "LRU.h"
#include <queue>

//程序配置参数
struct parameters{
	//网络配置参数
	double interval=0.01;	//程序每次运行的时间间隔
	std::string DNS_IP="127.0.0.1";  //用于节点发现协议的IP地址
	uint32_t DNS_PORT;
	std::string PRIVATE_IP;
	uint32_t PRIVATE_PORT;
	std::string HOST_IP;
	uint32_t HOST_PORT;
	double stop_time=0;		//程序停止时间，单位秒
	//PoV配置参数
	uint32_t init_commissioner_size=5; //委员数量
	NodeID *Initial_Commissioner_Nodes; //委员名单
	uint32_t agent_commissioner_num=0;//这里固定代理委员为第一个委员
	uint32_t init_butler_candidate_size=10;	//系统初始阶段管家候选人数
	NodeID *Initial_Butler_Candidate_Nodes; //管家候选名单
	uint32_t butler_amount=10;//管家人数
	uint32_t vote_amount=10; //投票人数
	uint32_t num_blocks=10; //每个任职周期产生的区块数
	double Tcut=20; //每个区块产生的截止时间
	uint32_t NormalPoolCapacity=4000; //正常交易缓存池的容量大小
	uint32_t max_normal_trans_per_block=2000; //每个区块最多存放的普通交易数量
	double defer_time=5; //每个管家开始打包区块前等待的时间
	NodeID AgentCommissioner;
	bool I_am_commissioner=false;		//本账号是否委员
	bool I_am_butler_candidate=false;	//本节点是否管家候选
	std::string pubkey;
	std::string prikey;
	std::string is_clear_database;
	int query_port;		//日志查询端口
	double fetch_height_wait_time=10;	//同步区块的等待时间
	double fetch_height_wait_time_last_encore=1;	//发送高度请求的时间间隔
	double syncer_wait_time=15;		//同步器失效的等待时间
	uint32_t end_height=10; //程序结束高度
	double prob_generate_normal_tx=0.001; //每个节点产生交易的概率
	bool keep_normal_pool_full=false;
	double send_speed=1000; //节点最大发送速率，Mbps
	double receive_speed=1000; //节点最大接受速率，Mbps
	std::string filter_path=""; //委员过滤列表路径
	std::string butler_filter_path=""; //管家过滤列表路径
	std::string name="";
	std::string ServerIP="";
	int ServerPort=3001;
	std::string vote_mode="";
	double passed_prob=0.4;
	double wait_for_all_sigs_time=3;

};

//定义LRU缓存所需的结构体及比较函数
struct cache
{
	uint32_t index;
	NodeID sender;
	NodeID receiver;
};

struct comp{
	bool operator()(const cache &c1,const cache &c2)
	{
		if(c1.sender<c2.sender)
			return true;
		else if(c1.sender>c2.sender)
			return false;
		else
		{
			if(c1.index<c2.index)
				return true;
			else
				return false;
		}
	}
};


struct DocumentSocketContainer
{
	DocumentContainer container;
	//Ptr<Socket> socket;
};

struct NodeInfo{
	std::string ip;
	uint32_t port;
	NodeID id;
};

struct capsule{
	string ip;
	int port;
	string message;
};

class blockchain_network: public phxpaxos::DFNetWork {
public:
	//static TypeId GetTypeId (void);
	blockchain_network();
	virtual ~blockchain_network();
	//正式启动网络线程和PoV线程
	void StartApplication ();
	//发送json对象给指定IP和端口
	void SendPacket(rapidjson::Document& message, std::string ip,uint32_t port);
	//作为回调函数处理接收到的PoV协议信息及日志采集信息
	int OnReceiveMessage(const char * pcMessage, const int iMessageLen);
	int handleReceiveMessage(string message);
	//工具函数，把字符串IP转换为32位的整数
	static uint32_t IPToValue(const string& strIP);
	//把消息发送给网络中的所有节点
	void SendToAll(rapidjson::Document &d);
	//把消息发送给除自己以外的所有节点
	void SendToNeighbor(rapidjson::Document &d);
	//把消息发送给指定NodeID的节点
	void SendToOne(rapidjson::Document &d,NodeID receiver);
	//把消息转发给除自己及发送方以外的所有节点
	void forwarding(rapidjson::Document &d);
	//提供给PoV调用的接口函数，根据json中的信息来决定发送目标
	void SendMessage(rapidjson::Document &d);
	//获取本节点ID
	NodeID getNodeId();
	//把IP和端口转换为一个64位的NodeID
	static NodeID getNodeId(std::string ip,uint32_t port);
	//执行节点发现协议及PoV逻辑
	void run();
	//运行数据传输服务器
	void startServer();
	//执行数据传输服务器
	void runServer();
	//运行数据传输客户端
	void startClient(std::string ip,int port,int len,int num);
	//执行数据传输客户端
	void runClient();
	//节点发现协议
	void runDiscoverNode();
	//发送节点发现协议消息
	void sendDiscoverNode(std::string receiver_ip,uint32_t receiver_port,bool is_request);
	//处理接收到的节点发现协议消息
	void handleDiscoverNode(rapidjson::Document& d);
	//系统初始化
	void init(std::string ip,uint32_t port);
	//根据配置文件设置POV相关参数
	void setPoVConfig(std::string file);
	void setPoVConfig(parameters &parcel);
	//根据配置文件设置网络模块相关参数
	void setNetworkConfig(std::string file);
	void setNetworkConfig(parameters &parcel);
	//获取相邻节点信息
	std::map<NodeID,NodeInfo>& getNodeList();
	//统计工具
	double getStartTime();
	double getInterval();
	double getSystemStartTime();
	std::string getIP(){return my_nodeinfo.ip;};
	int getPort(){return my_nodeinfo.port;};
private:
	std::mutex mutex;  //线程锁
	std::mutex send_mutex;
	POV *pov;   //PoV对象
	std::thread *PoV_runner;   //PoV逻辑线程
	std::thread *send_limiter; //限制发送速率线程
	std::thread *receive_limiter; //限制接受速率线程
	std::vector<cache> msgCache;  //缓存接收到的消息
	LRU<cache,comp> lru_cache;
	uint32_t index;		//从程序开始发送的消息编号
	//bool temp_switch;
	double interval=0.1;	//程序每次运行的时间间隔
	double start_time=2;    //程序开始运行的时间
	double last_discover_node_time=-1;	//上次进行节点发现的时间
	double program_start_time;   //程序开始的时间
	std::map<NodeID,NodeInfo> NodeList;  //把NodeID映射到具体的IP地址和端口
	NodeInfo my_nodeinfo;	//本节点的地址和端口
	std::string DNS_IP="127.0.0.1";  //用于节点发现协议的IP地址
	uint32_t DNS_PORT=5010;		//用于节点发现协议的端口
	double stop_time=0;		//程序停止时间，单位秒
	double discover_node_period=60;		//发节点发现协议运行周期
	std::string PRIVATE_IP;
	uint32_t PRIVATE_PORT;
	double send_begin=0;
	//double send_end=0;
	queue<capsule> send_queue;
	std::mutex send_queue_mutex;
	double send_speed=1000; //Mbps
	queue<string> receive_queue;
	std::mutex receive_queue_mutex;
	double receive_speed=1000; //Mbps



public:
	double send_time=0;
	double receive_time=0;
	double receive_num=0;
	bool set_limit=false;
};


#endif /* SRC_APPLICATIONS_MODEL_CONSENSUS_BLOCKCHAINNETWORK_H_ */
