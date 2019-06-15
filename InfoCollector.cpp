/*
 * InfoCollector.cpp
 *
 *  Created on: 2019年3月6日
 *      Author: blackguess
 */

#include "InfoCollector.h"

InfoCollector::InfoCollector() {
	// TODO Auto-generated constructor stub

}

InfoCollector::~InfoCollector() {
	// TODO Auto-generated destructor stub
}

void InfoCollector::sendMessage(string msg)
{
	   io_service iosev;
	    // socket对象
	    ip::tcp::socket socket(iosev);
	    // 连接端点，这里使用了本机连接，可以修改IP地址测试远程连接
	    ip::tcp::endpoint ep(ip::address_v4::from_string(ServerIP), ServerPort);
	    // 连接服务器
	    boost::system::error_code ec;
	    socket.connect(ep,ec);
	    // 如果出错，打印出错信息
	    if(ec)
	    {
	        std::cout << boost::system::system_error(ec).what() << std::endl;
	        return;
	        //throw "";
	    }
	    try{
	        int len=msg.size();

	        std::string len_str((char*)(&len),sizeof(int));
	        std::string data=len_str+msg;
	        const char* chs=data.c_str();
	        boost::system::error_code ec;
	        std::size_t size=boost::asio::write(socket, boost::asio::buffer(chs, data.size()),ec);
	        std::cout<<"已发送数据大小"<<size<<"\n";
	        if(ec)
	        {
	    		std::cout << "status: " << ec.message() << "\n";
	    		//sock.close();
	        }
	    	//socket.write_some(buffer(msg));
	    }
	    catch(...)
	    {
	    	std::cout<<"写数据异常：infocollector\n";
	    }
	    socket.close();
	    // 接收数据
//	    char buf[100];
//	    size_t len=socket.read_some(buffer(buf), ec);
//	    std::string received_msg(buf);
//	    std::cout<<received_msg<<"\n";
}

void InfoCollector::sendVoteMessage()
{
	rapidjson::Document voteDataArray;
	voteDataArray.SetArray();
	rapidjson::Document::AllocatorType& allocator=voteDataArray.GetAllocator();
	for(auto i=voteData.begin();i!=voteData.end();)
	{
		VoteData data=i->second;
		rapidjson::Value doc;
		doc.SetObject();
		rapidjson::Value _pubkey(data.pubkey.c_str(),data.pubkey.size(),allocator);
		doc.AddMember("pubkey",_pubkey,allocator);
		rapidjson::Value height(data.height);
		doc.AddMember("height",height,allocator);
		rapidjson::Value agreement(data.agreement);
		doc.AddMember("agreement",agreement,allocator);
		rapidjson::Value txs_num(data.txs_num);
		doc.AddMember("txs_num",txs_num,allocator);
		rapidjson::Value _tx_type(data.tx_type.c_str(),data.tx_type.size(),allocator);
		doc.AddMember("tx_type",_tx_type,allocator);
		voteDataArray.PushBack(doc,allocator);
		i=voteData.erase(i);
	}
	std::string msg=getDocumentString(voteDataArray);
	//std::cout<<"voteData="<<msg<<"\n";
	sendMessage(msg);
}

void InfoCollector::sendNodeMessage()
{
	rapidjson::Document doc;
	doc.SetObject();
	rapidjson::Document::AllocatorType& allocator=doc.GetAllocator();
	rapidjson::Value _type(1);
	doc.AddMember("type",_type,allocator);
	rapidjson::Value _pubkey(pubkey.c_str(),pubkey.size(),allocator);
	doc.AddMember("pubkey",_pubkey,allocator);
	rapidjson::Value _IP(IP.c_str(),IP.size(),allocator);
	doc.AddMember("IP",_IP,allocator);
	rapidjson::Value _name(name.c_str(),name.size(),allocator);
	doc.AddMember("name",_name,allocator);
	rapidjson::Value _is_butler_candidate(is_butler_candidate);
	doc.AddMember("is_butler_candidate",_is_butler_candidate,allocator);
	rapidjson::Value _is_butler(is_butler);
	doc.AddMember("is_butler",_is_butler,allocator);
	rapidjson::Value _is_commissioner(is_commissioner);
	doc.AddMember("is_commissioner",_is_commissioner,allocator);
	std::string msg=getDocumentString(doc);
	//std::cout<<"voteData="<<msg<<"\n";
	sendMessage(msg);
}

void InfoCollector::setServerAddress(string IP,int port)
{
	ServerIP=IP;
	ServerPort=port;
}
void InfoCollector::collectVoteData(string pubkey,int height,int agreement,int txs_num,string tx_type)
{
	VoteData data;
	data.pubkey=pubkey;
	data.height=height;
	data.agreement=agreement;
	data.txs_num=txs_num;
	data.tx_type=tx_type;
	voteData[pubkey]=data;
}

void InfoCollector::setNodeMessage(string pubkey,string IP,string name,bool is_butler_candidate,bool is_butler,bool is_commissioner)
{
	this->pubkey=pubkey;
	this->IP=IP;
	this->name=name;
	this->is_butler_candidate=is_butler_candidate;
	this->is_butler=is_butler;
	this->is_commissioner=is_commissioner;
}
