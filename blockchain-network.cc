/*
 * blockchain-network.cpp
 *
 *  Created on: 2018年3月11日
 *      Author: blackguess
 */

#include "blockchain-network.h"
//#include "ns3/udp-socket-factory.h"
//#include "ns3/uinteger.h"
//#include "ns3/log.h"
//#include "ns3/socket.h"
//#include "ns3/address-utils.h"
//#include "ns3/udp-socket.h"
//#include "ns3/tcp-socket-factory.h"
//#include "ns3/simulator.h"
#include <time.h>
//#include "ns3/simulator.h"
#include "POV.h"
#include <memory>
//#include "ns3/ipv4-address.h"
#include "utils.h"
#include "constants.h"
#include "udp.h"
#include <iomanip>
#include "ConfigHelper.h"


blockchain_network::blockchain_network() //: m_oUDPRecv(this), m_oTcpIOThread(this)
{

}

blockchain_network::~blockchain_network()
{

}


void blockchain_network::init(std::string ip,uint32_t port)
{
	//初始化网络参数
	my_nodeinfo.ip=ip;
	std::cout<<"my_nodeinfo.ip："<<my_nodeinfo.ip<<"\n";
	my_nodeinfo.port=port;
	std::cout<<"my_nodeinfo.port"<<my_nodeinfo.port<<"\n";
	my_nodeinfo.id=getNodeId(ip,port);
	std::cout<<"my_nodeinfo.id："<<my_nodeinfo.id<<"\n";

	std::cout<<"init blockchain_network:1\n";
	index=0;
	std::cout<<"init blockchain_network:2\n";

	std::cout<<"init blockchain_network:4\n";
	msgCache=std::vector<cache>();
	lru_cache.init(4000);
	std::cout<<"init blockchain_network:5\n";

	//初始化pov
	pov=new POV();
	pov->init(this);
}

void blockchain_network::StartApplication ()
{
	//初始化网络
	int ret=Init(PRIVATE_IP,PRIVATE_PORT,2);
	if(ret!=0)
	{
		std::cout<<"初始化失败："<<DNS_PORT<<std::endl;
	}
	//运行网络
	RunNetWork();
	program_start_time=getCurrentTime();

	//receive_limiter=new std::thread(std::bind(&blockchain_network::runServer,this));
	//receive_limiter->detach();
	//send_limiter=new std::thread(std::bind(&blockchain_network::runClient,this));
	//send_limiter->detach();
	PoV_runner=new std::thread(std::bind(&blockchain_network::run,this));
	PoV_runner->detach();
}

void blockchain_network::run()
{
	//运行主线程，循环执行节点发现协议以及POV算法
	while(1)
	{
		//cout<<"on run:1\n";
		if(stop_time>0)
		{
			if(getCurrentTime()>stop_time+program_start_time)
			{
				std::cout<<"CurrentTime: "<<getCurrentTime()<<"\n";
				std::cout<<"stop_time: "<<stop_time<<"\n";
				std::cout<<"program_start_time: "<<program_start_time<<"\n";
				exit(0);
			}
		}
		//mutex.lock();
		//cout<<"on run:2\n";
		try
		{
			runDiscoverNode();
		}
		catch(...)
		{
			std::cout<<"运行节点发现协议过程中产生异常\n";
			throw "";
		}
		//cout<<"on run:3\n";
		if(NodeList.size()>=2)
		{
			try
			{
				pov->run();
			}
			catch(...)
			{
				std::cout<<"pov运行过程中产生异常\n";
				throw "";
			}
		}
		else
		{
			//std::cout<<"NodeList.size="<<NodeList.size()<<"\n";
		}
		//cout<<"on run:4\n";
		//mutex.unlock();
		sleep(interval);
	}
}

//运行数据传输服务器
void blockchain_network::startServer()
{
	int ret=Init(my_nodeinfo.ip,my_nodeinfo.port,2);
	if(ret!=0)
	{
		std::cout<<"初始化失败："<<DNS_PORT<<std::endl;
	}
	//运行网络
	RunNetWork();
	PoV_runner=new std::thread(std::bind(&blockchain_network::runServer,this));
	PoV_runner->detach();
}

//执行数据传输服务器
void blockchain_network::runServer()
{
	while(1)
	{
		//cout<<"receive_queue is empty?--"<<receive_queue.empty()<<"\n";
		//cout<<"running server!\n";
		receive_queue_mutex.lock();
		if(receive_queue.size()>0)
		{
			//cout<<"receive_queue_mutex.lock()\n";
			string message="";
			try
			{
				message=receive_queue.front();
				receive_queue.pop();
			}
			catch(...)
			{
				cout<<"receive_queue弹出异常\n";
				throw "";
			}
			//cout<<"receive_queue_mutex.unlock()\n";
			if(set_limit==true and receive_speed>0)
			{
				//cout<<"接收延时\n";
				double ideal_time=message.size()*8/(receive_speed*1000000);
				double begin=getCurrentTime();
				//cout<<"receive_ideal_time:"<<ideal_time<<"\n";
				while(getCurrentTime()-begin<ideal_time);
				//cout<<"receive_real_time:"<<getCurrentTime()-begin<<"\n";
			}

			//double time=getCurrentTime();
			//std::cout<<"receiveTime="<<time<<"\n";
			//receive_time=time;
			//receive_num++;

			try
			{
				handleReceiveMessage(message);
			}
			catch(...)
			{
				cout<<"handleReceiveMessage异常\n";
				throw "";
			}
		}
		receive_queue_mutex.unlock();

	}
}

//运行数据传输客户端
void blockchain_network::startClient(std::string ip,int port,int len, int times)
{
	int ret=Init(my_nodeinfo.ip,my_nodeinfo.port,2);
	if(ret!=0)
	{
		std::cout<<"初始化失败："<<DNS_PORT<<std::endl;
	}
	//运行网络
	RunNetWork();
	std::string rand_str=getRandomString(len);
	capsule cap;
	cap.ip=ip;
	cap.port=port;
	cap.message=rand_str;
	double time=getCurrentTime();
	std::cout<<"sendTime="<<time<<"\n";
	send_time=time;
	for(int i=0;i<times;i++)
		//SendMessageTCP(0, ip, port,rand_str);
		send_queue.push(cap);
	PoV_runner=new std::thread(std::bind(&blockchain_network::runClient,this));
	PoV_runner->detach();
	//while(1);
}

//执行数据传输客户端
void blockchain_network::runClient()
{
	while(1)
	{
		//cout<<"send_queue is empty?--"<<send_queue.empty()<<"\n";
		//cout<<"running client!\n";
		send_queue_mutex.lock();
		if(!send_queue.empty())
		{
			//cout<<"send_queue_mutex.lock()\n";
			//
			capsule msg;
			try
			{
				msg=send_queue.front();
				send_queue.pop();
			}
			catch(...)
			{
				cout<<"send_queue弹出异常\n";
				throw "";
			}
			//
			//cout<<"send_queue_mutex.unlock()\n";
			int size=msg.message.size();
			if(size>7000000)
			{
				cout<<"发送消息超过7M:"<<size<<"\n";
			}
			//double a=0;
			//double b=size/a;
			//cout<<"size="<<size<<"\n";
			//cout<<"ideal_time:"<<ideal_time<<"\n";

			double begin=getCurrentTime();
			try
			{
				SendMessageTCP(0, msg.ip, msg.port,msg.message);
			}
			catch(...)
			{
				cout<<"发送消息异常:\n";

				cout<<"message="<<msg.message<<std::endl;
				cout<<"receiver ip="<<msg.ip<<std::endl;
				cout<<"receiver port="<<msg.port<<std::endl;
				cout<<"sender ip="<<my_nodeinfo.ip<<std::endl;
				cout<<"sender port="<<my_nodeinfo.port<<std::endl;
				throw "";
			}
			//cout<<"before_sleep:"<<getCurrentTime()<<"\n";
			if(set_limit==true and send_speed>0)
			{
				double ideal_time=size*8/(send_speed*1000000);
				//std::cout<<"发送延时"<<std::endl;
				while(getCurrentTime()-begin<ideal_time);
			}
			//cout<<"sleep_time:"<<getCurrentTime()-begin<<"\n";

//			double end=getCurrentTime();
//			double real_time=end-begin;
//			unsigned int sleep_time=useconds_t((ideal_time-real_time)*1000);
//			cout<<"sleep_time:"<<sleep_time<<"\n";
//			cout<<"end_1:"<<end<<"\n";
//			usleep(sleep_time);
//			end=getCurrentTime()-end;
//			cout<<"end_2:"<<end<<"\n";
		}
		send_queue_mutex.unlock();
	}
}

void blockchain_network::SendPacket(rapidjson::Document& message, std::string ip,uint32_t port)
{
	//发送json数据给指定的地址和端口
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	message.Accept(writer);
	std::string content= buffer.GetString();
	//rapidjson::Document::AllocatorType &allocator=message.GetAllocator();
	/*
	if(message.IsObject() && !message.HasMember("FIND_NODE") && !message.HasMember("log"))
	{
		uint32_t index=message["index"].GetUint();
		NodeID sender=message["sender"].GetUint64();
		NodeID receiver=message["receiver"].GetUint64();
		MessageType type=MessageType(message["type"].GetUint());
		if(receiver==9151314447111820178)
		{
			if(type==ResponseApplyCommissionerSignature)
			{
				std::cout<<getNodeId()<<"发送给"<<receiver<<"申请委员签名回复\n";
			}
			else if(type==ResponseApplyButlerCandidateSignature)
			{
				std::cout<<getNodeId()<<"发送给"<<receiver<<"申请管家候选签名回复\n";
			}
		}
	}
	*/
	//根据数据量大小选择使用TCP还是UDP进行发送
	if(content.size()>5000000)
	{
		cout<<"发送消息大小为:"<<content.size()<<"\n";
	}
	//send_mutex.lock();
	try
	{
		SendMessageTCP(0, ip, port,content);
//		if(!set_limit)
//		{
//			SendMessageTCP(0, ip, port,content);
//			return ;
//		}
//		capsule cap;
//		cap.ip=ip;
//		cap.port=port;
//		cap.message=content;
//		//cout<<"onsendPacket start\n";
//		send_queue_mutex.lock();
//		send_queue.push(cap);
//		send_queue_mutex.unlock();
//		//cout<<"onsendPacket end\n";
	}
	catch(...)
	{
		std::cout<<"发送数据异常\n";
		throw "";
	}
	//send_mutex.unlock();
	//else
	//std::cout<<"sendPacket:1\n";
	//SendMessageUDP(0, ip, port,content);
	//std::cout<<"sendPacket:2\n";
}

int blockchain_network::OnReceiveMessage(const char * pcMessage, const int iMessageLen)
{
	//cout<<"onreceiveMessage start\n";
	string message(pcMessage,iMessageLen);
	handleReceiveMessage(message);
//	try
//	{
//		string message(pcMessage,iMessageLen);
////		if(!set_limit)
////		{
////			handleReceiveMessage(message);
////			return 0;
////		}
//		receive_queue_mutex.lock();
//		receive_queue.push(message);
//		//cout<<"receive_queue.size()="<<receive_queue.size()<<"\n";
//		//handleReceiveMessage(message);
//		receive_queue_mutex.unlock();
//	}
//	catch(...)
//	{
//		cout<<"构造message字符串异常\n";
//		throw "";
//	}
	//cout<<"message="<<message<<"\n";

	//cout<<"onreceiveMessage end\n";
	return 0;
}

int blockchain_network::handleReceiveMessage(string message)
{
		//std::cout<<"UDP Packet:"<<message<<"\n";
		//std::cout<<"OnReceiveMessage:1\n";
		rapidjson::Document d;
		d.Parse(message.c_str(),message.size());
		if(d.HasParseError())
		{
	    	std::cout<<"区块链接收其他节点消息错误——json解析错误\n";
	    	return -1;
		}
		//std::cout<<"OnReceiveMessage:2\n";
		//节点发现协议在网络层处理
	    if(!d.IsObject())
	    {
	    	std::cout<<"d.IsObject()!\n";
	    	return -1;
	    }
	    //std::cout<<"OnReceiveMessage:3\n";
	    if(d.HasMember("FIND_NODE"))
	    {
	    	//std::cout<<"OnReceiveMessage:4\n";
	    	try
	    	{
	        	handleDiscoverNode(d);
	    	}
	    	catch(...)
	    	{
	    		std::cout<<"节点发现协议处理消息异常\n";
	    		throw "";
	    	}
	    }
	    else if(d.HasMember("log"))
	    {
	    	//std::cout<<"OnReceiveMessage:5\n";
	    	//接收到外部发送来的日志由pov的handleLog函数进行处理
	    	std::cout<<"接收到日志信息：\n";
	    	print_document(d);
	    	try
	    	{
	        	pov->handleLog(d);

	    	}
	    	catch(...)
	    	{
	    		std::cout<<"处理日志消息异常\n";
	    		throw "";
	    	}
	    }
	    else
	    {
	    	//std::cout<<"OnReceiveMessage:6\n";
	    	//检查消息合法性
	    	if(!d.HasMember("sender") or !d.HasMember("index") or !d.HasMember("receiver"))
	    	{

	    		std::cout<<"incomplete document!\n";
	    		return -2;
	    	}
	    	NodeID sender=d["sender"].GetUint64();
	    	NodeID index=d["index"].GetUint64();
	    	NodeID receiver=d["receiver"].GetUint64();
	    	MessageType type=MessageType(d["type"].GetUint());
	    	/*
	    	if(getNodeId()==9151314447111820178)
	    	{
	    		if(type==ResponseApplyCommissionerSignature)
	    		{
	    			std::cout<<getNodeId()<<"收到"<<sender<<"发送的申请委员签名回复\n";
	    		}
	    		else if(type==ResponseApplyButlerCandidateSignature)
	    		{
	    			std::cout<<getNodeId()<<"收到"<<sender<<"发送的申请管家候选签名回复\n";
	    		}
	    	}
	    	*/
	    	/*
	    	for(std::vector<cache>::const_iterator msg=msgCache.begin();msg !=msgCache.end();++msg)
	    	{
	    		if(sender==msg->sender and index==msg->index)
	    		{
	    			return -3;
	    		}
	    	}
	    	*/
	    	cache c;
	    	c.index=index;
	    	c.sender=sender;
	    	c.receiver=receiver;
	    	mutex.lock();
	    	bool result=lru_cache.append(c);
	    	mutex.unlock();
	    	if(!result)
	    	{
	    		return -3;
	    	}

	    	/*
	    	if(msgCache.size()>4000)
	    		msgCache.erase(msgCache.begin());
	    	msgCache.push_back(c);
			*/

	    	//转发处理
	    	NodeID _receiver=d["receiver"].GetUint64();
	    	if(_receiver==0)
	    	{
	    		forwarding(d);
	    		pov->handleMessage(d);
	    	}
	    	else
	    	{
	    		if(_receiver!=getNodeId())
	    		{
	    			forwarding(d);
	    			return -4;
	    		}
	    		try
	    		{
	        		pov->handleMessage(d);
	    		}
	    		catch(...)
	    		{
	    			std::cout<<"处理消息异常\n";
	    			MessageType type=MessageType(d["type"].GetUint());
	    			std::cout<<"消息类型为："<<int(type)<<"\n";
	    			throw "handle message aborted";
	    		}
	    	}
	    }
	    //std::cout<<"OnReceiveMessage:7\n";
		return 0;
}
//int blockchain_network::OnReceiveMessage(const char * pcMessage, const int iMessageLen)
//{
//	//处理接受到的消息，如果数pov的消息，转换成json对象后交由pov的消息处理函数进行进一步处理
//	//std::unique_lock<std::mutex> lock(mutex);
//	std::string message(pcMessage,iMessageLen);
//	//std::cout<<"UDP Packet:"<<message<<"\n";
//	//std::cout<<"OnReceiveMessage:1\n";
//	rapidjson::Document d;
//	d.Parse(pcMessage,iMessageLen);
//	//std::cout<<"OnReceiveMessage:2\n";
//	//节点发现协议在网络层处理
//    if(!d.IsObject())
//    {
//    	std::cout<<"d.IsObject()!\n";
//    	return -1;
//    }
//    //std::cout<<"OnReceiveMessage:3\n";
//    if(d.HasMember("FIND_NODE"))
//    {
//    	//std::cout<<"OnReceiveMessage:4\n";
//    	try
//    	{
//        	handleDiscoverNode(d);
//    	}
//    	catch(...)
//    	{
//    		std::cout<<"节点发现协议处理消息异常\n";
//    		throw "";
//    	}
//    }
//    else if(d.HasMember("log"))
//    {
//    	//std::cout<<"OnReceiveMessage:5\n";
//    	//接收到外部发送来的日志由pov的handleLog函数进行处理
//    	std::cout<<"接收到日志信息：\n";
//    	print_document(d);
//    	try
//    	{
//        	pov->handleLog(d);
//
//    	}
//    	catch(...)
//    	{
//    		std::cout<<"处理日志消息异常\n";
//    		throw "";
//    	}
//    }
//    else
//    {
//    	//std::cout<<"OnReceiveMessage:6\n";
//    	//检查消息合法性
//    	if(!d.HasMember("sender") or !d.HasMember("index") or !d.HasMember("receiver"))
//    	{
//
//    		std::cout<<"incomplete document!\n";
//    		return -2;
//    	}
//    	NodeID sender=d["sender"].GetUint64();
//    	NodeID index=d["index"].GetUint64();
//    	NodeID receiver=d["receiver"].GetUint64();
//    	MessageType type=MessageType(d["type"].GetUint());
//    	/*
//    	if(getNodeId()==9151314447111820178)
//    	{
//    		if(type==ResponseApplyCommissionerSignature)
//    		{
//    			std::cout<<getNodeId()<<"收到"<<sender<<"发送的申请委员签名回复\n";
//    		}
//    		else if(type==ResponseApplyButlerCandidateSignature)
//    		{
//    			std::cout<<getNodeId()<<"收到"<<sender<<"发送的申请管家候选签名回复\n";
//    		}
//    	}
//    	*/
//    	/*
//    	for(std::vector<cache>::const_iterator msg=msgCache.begin();msg !=msgCache.end();++msg)
//    	{
//    		if(sender==msg->sender and index==msg->index)
//    		{
//    			return -3;
//    		}
//    	}
//    	*/
//    	cache c;
//    	c.index=index;
//    	c.sender=sender;
//    	c.receiver=receiver;
//    	mutex.lock();
//    	bool result=lru_cache.append(c);
//    	mutex.unlock();
//    	if(!result)
//    	{
//    		return -3;
//    	}
//
//    	/*
//    	if(msgCache.size()>4000)
//    		msgCache.erase(msgCache.begin());
//    	msgCache.push_back(c);
//		*/
//
//    	//转发处理
//    	NodeID _receiver=d["receiver"].GetUint64();
//    	if(_receiver==0)
//    	{
//    		forwarding(d);
//    		pov->handleMessage(d);
//    	}
//    	else
//    	{
//    		if(_receiver!=getNodeId())
//    		{
//    			forwarding(d);
//    			return -4;
//    		}
//    		try
//    		{
//        		pov->handleMessage(d);
//    		}
//    		catch(...)
//    		{
//    			std::cout<<"处理消息异常\n";
//    			MessageType type=MessageType(d["type"].GetUint());
//    			std::cout<<"消息类型为："<<int(type)<<"\n";
//    			throw "handle message aborted";
//    		}
//    	}
//    }
//    //std::cout<<"OnReceiveMessage:7\n";
//	return 0;
//}

NodeID blockchain_network::getNodeId()
{
	return my_nodeinfo.id;
}

NodeID blockchain_network::getNodeId(std::string ip,uint32_t port)
{
	NodeID id=IPToValue(ip);
	id=id<<32;
	id+=port;
	return id;
}

uint32_t blockchain_network::IPToValue(const string& strIP)
{
//IP转化为数值
//没有格式检查
//返回值就是结果

    int a[4];
    string IP = strIP;
    string strTemp;
    size_t pos;
    size_t i=3;

    do
    {
        pos = IP.find(".");

        if(pos != string::npos)
        {
            strTemp = IP.substr(0,pos);
            a[i] = atoi(strTemp.c_str());
            i--;
            IP.erase(0,pos+1);
        }
        else
        {
            strTemp = IP;
            a[i] = atoi(strTemp.c_str());
            break;
        }

    }while(1);

    int nResult = (a[3]<<24) + (a[2]<<16)+ (a[1]<<8) + a[0];
    return nResult;
}

//节点发现协议，由sendDiscoverNode和handleDiscoverNode两个函数组成，每个节点维护一个节点列表，该列表
//不包含本地节点。发送方先查找列表，如果列表中没有节点，则向DNS节点发送节点发现请求，否则随机从列表中随机挑
//选一个节点发送节点发送请求。接受到请求的节点则把自己节点列表中的所有节点发送给请求方，然后把请求节点加入到
//节点列表中。请求方把被请求方加入自己的节点列表，随后对比本地节点列表和接收到的节点列表，把本地列表中没有的
//节点添加到列表中，并向这些节点发送节点发现请求。
void blockchain_network::runDiscoverNode()
{
	double current_time=getCurrentTime();
	if(current_time-last_discover_node_time<discover_node_period)
	{
		return;
	}
	last_discover_node_time=current_time;
	if(NodeList.empty())
	{
		sendDiscoverNode(DNS_IP,DNS_PORT,true);
	}
	else
	{
		for(std::map<NodeID,NodeInfo>::const_iterator i=NodeList.begin();i!=NodeList.end();i++)
		{
			NodeInfo node=i->second;
			sendDiscoverNode(node.ip,node.port,true);
		}
	}
}

void blockchain_network::sendDiscoverNode(std::string receiver_ip,uint32_t receiver_port,bool is_request)
{
	//有两种发送情况，第一种是发送请求获取其他节点的节点列表，第二种是回复请求，把节点列表发送给请求节点
	if(is_request)
	{
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &allocator=d.GetAllocator();
		rapidjson::Value IntValue(0);
		rapidjson::Value receiver(getNodeId(receiver_ip,receiver_port));
		std::string myip=my_nodeinfo.ip;
		rapidjson::Value sender_ip(myip.c_str(),myip.size(),d.GetAllocator());
		rapidjson::Value sender_port(my_nodeinfo.port);
		rapidjson::Value sender_id(my_nodeinfo.id);
		d.AddMember("FIND_NODE",IntValue,allocator);
		d.AddMember("receiver",receiver,allocator);
		d.AddMember("sender_ip",sender_ip,allocator);
		d.AddMember("sender_port",sender_port,allocator);
		d.AddMember("sender_id",sender_id,allocator);
		SendPacket(d,receiver_ip,receiver_port);
		//cout<<"发送请求\n";
	}
	else
	{
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &allocator=d.GetAllocator();
		rapidjson::Value IntValue(1);
		rapidjson::Value receiver(getNodeId(receiver_ip,receiver_port));
		std::string myip=my_nodeinfo.ip;
		rapidjson::Value sender_ip(myip.c_str(),myip.size(),d.GetAllocator());
		rapidjson::Value sender_port(my_nodeinfo.port);
		rapidjson::Value sender_id(my_nodeinfo.id);
		d.AddMember("FIND_NODE",IntValue,allocator);
		d.AddMember("receiver",receiver,allocator);
		d.AddMember("sender_ip",sender_ip,allocator);
		d.AddMember("sender_port",sender_port,allocator);
		d.AddMember("sender_id",sender_id,allocator);
		//发送节点列表
		rapidjson::Value node_list;
		node_list.SetArray();
		for(std::map<NodeID,NodeInfo>::const_iterator i=NodeList.begin();i!=NodeList.end();i++)
		{
			NodeInfo node=i->second;
			rapidjson::Value nodeinfo;
			nodeinfo.SetObject();
			rapidjson::Value node_ip(node.ip.c_str(),node.ip.size(),allocator);
			rapidjson::Value node_port(node.port);
			rapidjson::Value node_id(node.id);
			nodeinfo.AddMember("node_ip",node_ip,allocator);
			nodeinfo.AddMember("node_port",node_port,allocator);
			nodeinfo.AddMember("node_id",node_id,allocator);
			node_list.PushBack(nodeinfo,allocator);
		}
		d.AddMember("node_list",node_list,allocator);
		SendPacket(d,receiver_ip,receiver_port);
		//cout<<"发送回复\n";
	}
}

void blockchain_network::handleDiscoverNode(rapidjson::Document& d)
{	//节点发送协议的消息处理
	if(!(d.HasMember("FIND_NODE")&&d.HasMember("sender_ip")&&d.HasMember("sender_port")&&d.HasMember("sender_id")))
	{
		return;
	}
	uint32_t type=d["FIND_NODE"].GetUint();
	std::string sender_ip=d["sender_ip"].GetString();
	uint32_t sender_port=d["sender_port"].GetUint();
	if(!d["sender_id"].IsUint64())
	{
		std::cout<<"sender_id!=uint64!\n";
		return;
	}
	NodeID sender_id=d["sender_id"].GetUint64();
	if(sender_id==my_nodeinfo.id)
		return;
	NodeInfo node;
	node.ip=sender_ip;
	node.port=sender_port;
	node.id=sender_id;
	NodeList[sender_id]=node;
	//相应的接受到的消息也有两种，FIND_NODE的value用来区分这两种消息，type=0表示接受到节点列表请求，否则表示接收到回复
	if(type==0)
	{
		sendDiscoverNode(sender_ip,sender_port,false);
	}
	else
	{
		rapidjson::Value& node_list=d["node_list"];
		for(uint32_t i=0;i<node_list.Size();i++)
		{
			rapidjson::Value& node=node_list[i];
			if(!node["node_id"].IsUint64())
			{
				std::cout<<"node_id!=uint64!\n";
				return;
			}
			NodeID node_id=node["node_id"].GetUint64();
			if(node_id!=my_nodeinfo.id && NodeList.find(node_id)==NodeList.end())
			{
				NodeInfo nodeinfo;
				nodeinfo.ip=node["node_ip"].GetString();
				nodeinfo.port=node["node_port"].GetUint();
				nodeinfo.id=node_id;
				NodeList[node_id]=nodeinfo;
				sendDiscoverNode(nodeinfo.ip,nodeinfo.port,true);
			}
		}
		//std::cout<<"节点列表中的节点：\n";
		//for(std::map<NodeID,NodeInfo>::iterator i=NodeList.begin();i!=NodeList.end();i++)
		//{
		//	std::cout<<i->first<<"——ip="<<i->second.ip<<" && port="<<i->second.port<<"\n";
		//}
	}
}

void blockchain_network::SendToAll(rapidjson::Document &d)
{
	//发送给相邻节点
	for(std::map<NodeID,NodeInfo>::const_iterator i=NodeList.begin();i!=NodeList.end();i++)
	{
		NodeInfo nodeinfo=i->second;
		SendPacket(d,nodeinfo.ip,nodeinfo.port);
	}
	//发送给自己
	//SendPacket(d,m_peersSockets[*m_ipv4]);
	SendPacket(d,my_nodeinfo.ip,my_nodeinfo.port);
}

void blockchain_network::SendToNeighbor(rapidjson::Document &d)
{
	//发送给相邻节点
	for (std::map<NodeID,NodeInfo>::const_iterator i=NodeList.begin();i!=NodeList.end();i++)
	{
		NodeInfo nodeinfo=i->second;
		SendPacket(d,nodeinfo.ip,nodeinfo.port);
	}
}

void blockchain_network::forwarding(rapidjson::Document &d)
{	//消息转发
	if(!d.HasMember("receiver") or !d.HasMember("sender") or !d.HasMember("last_sender"))
	{
		std::cout<<"error: document is not complete!";
		return;
	}
	NodeID label=d["receiver"].GetUint64();
	NodeID sender=d["sender"].GetUint64();
	NodeID last_sender=d["last_sender"].GetUint64();
	if(label==0)
	{
		//如果是发送给所有人，则发送给除了发送者和上个发送者以外的所有人
		for (std::map<NodeID,NodeInfo>::const_iterator i=NodeList.begin();i!=NodeList.end();i++)
		{
			if(i->first==sender or i->first==last_sender)
			{
				continue;
			}
			SendPacket(d,i->second.ip,i->second.port);
		}
	}
	else
	{
		//如果发送给某个节点，就先查找相邻节点有没有该节点,有就直接发送，没有发送给除了发送者和上个发送者以外的所有人
		NodeID receiver=d["receiver"].GetUint64();
		for (std::map<NodeID,NodeInfo>::const_iterator i=NodeList.begin();i!=NodeList.end();i++)
		{
			if(i->first==receiver)
			{
				SendPacket(d,i->second.ip,i->second.port);
				//std::cout<<"find=true\n";
				return;
			}
		}
		for (std::map<NodeID,NodeInfo>::const_iterator i=NodeList.begin();i!=NodeList.end();i++)
		{
			if(i->first==sender or i->first==last_sender)
			{
				continue;
			}
			SendPacket(d,i->second.ip,i->second.port);
		}
	}
}

void blockchain_network::SendToOne(rapidjson::Document &d,NodeID receiver)
{	//发送给指定节点
	if(!d.HasMember("receiver") or !d.HasMember("sender") or !d.HasMember("last_sender"))
	{
		std::cout<<"error: document is not complete!";
		return;
	}
	//先查找相邻节点有没有目标节点，没有就发送给所有相邻节点。
	if(receiver==my_nodeinfo.id)
	{
		SendPacket(d,my_nodeinfo.ip,my_nodeinfo.port);
		return;
	}
	for (std::map<NodeID,NodeInfo>::const_iterator i=NodeList.begin();i!=NodeList.end();i++)
	{
		if(i->first==receiver)
		{
			MessageType type=MessageType(d["type"].GetUint());
			if(type==PublishBlock)
			{
				std::cout<<"发送新区块给特定的节点\n";
			}
			SendPacket(d,i->second.ip,i->second.port);
			return;
		}
	}
	SendToNeighbor(d);
}

void blockchain_network::SendMessage(rapidjson::Document &d)
{
	MessageType msg_type=MessageType(d["type"].GetUint());
	//如果一个消息需要有receiver、sender、last_sender用来做路由，这几个字段都用nodeid来来表示一个节点，包括这个函数在内的所有上层函数都使用nodeid而不是用address。
	if(!d.HasMember("receiver"))
	{
		return;
	}

	rapidjson::Document::AllocatorType &allocator=d.GetAllocator();
	//设置"sender"字段
	if(!d.HasMember("sender"))
	{
		rapidjson::Value sender(getNodeId());
		d.AddMember("sender",sender,allocator);
	}
	else
	{
		d["sender"].SetUint64(getNodeId());
	}
	//设置"last_sender"字段
	if(!d.HasMember("last_sender"))
	{
		rapidjson::Value lastsender(getNodeId());
		d.AddMember("last_sender",lastsender,allocator);
	}
	else
	{
		d["last_sender"].SetUint64(getNodeId());
	}
	//发送策略
	NodeID receiver=d["receiver"].GetUint64();
	if(receiver==0)
	{
		SendToAll(d);
		return;
	}
	else
	{
		NodeID true_receiver=d["receiver"].GetUint64();
		SendToOne(d,true_receiver);
	}
}

void blockchain_network::setPoVConfig(std::string file)
{
	pov->setConfig(file);
}

void blockchain_network::setPoVConfig(parameters &parcel)
{
	pov->setConfig(parcel);
}

void blockchain_network::setNetworkConfig(std::string file)
{	//从配置文件中读取网络配置
	std::cout<<"配置网络参数：\n";
    ConfigHelper configSettings(file);
	std::cout<<"配置网络参数：start\n";
    interval=configSettings.Read("运行时间间隔", interval);
	std::cout<<"interval："<<interval<<"\n";
    DNS_IP=configSettings.Read("默认节点地址", DNS_IP);
	std::cout<<"DNS_IP："<<DNS_IP<<"\n";
    DNS_PORT=configSettings.Read("默认节点端口", DNS_PORT);
	std::cout<<"DNS_PORT："<<DNS_PORT<<"\n";
	PRIVATE_IP=configSettings.Read("私网地址", PRIVATE_IP);
	std::cout<<"PRIVATE_IP："<<PRIVATE_IP<<"\n";
	PRIVATE_PORT=configSettings.Read("私网端口", PRIVATE_PORT);
	std::cout<<"PRIVATE_PORT："<<PRIVATE_PORT<<"\n";
    stop_time=configSettings.Read("停止时间", stop_time);
	std::cout<<"stop_time："<<stop_time<<"\n";
	send_speed=configSettings.Read("最大发送速度", send_speed);
	std::cout<<"send_speed："<<send_speed<<"\n";
	receive_speed=configSettings.Read("最大接收速度", receive_speed);
	std::cout<<"receive_speed："<<receive_speed<<"\n";
}

void blockchain_network::setNetworkConfig(parameters &parcel)
{
	interval=parcel.interval;
	DNS_IP=parcel.DNS_IP;
	DNS_PORT=parcel.DNS_PORT;
	PRIVATE_IP=parcel.PRIVATE_IP;
	PRIVATE_PORT=parcel.PRIVATE_PORT;
	stop_time=parcel.stop_time;
	send_speed=parcel.send_speed;
	std::cout<<"send_speed："<<send_speed<<"\n";
	receive_speed=parcel.receive_speed;
	std::cout<<"receive_speed："<<receive_speed<<"\n";

}

std::map<NodeID,NodeInfo>& blockchain_network::getNodeList()
{
	return NodeList;
}

double blockchain_network::getStartTime()
{
	return start_time;
}

double blockchain_network::getInterval()
{
	return stop_time;
}

double blockchain_network::getSystemStartTime()
{
	return program_start_time;
}
