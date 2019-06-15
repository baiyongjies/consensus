#include "POV.h"

//处理接收到的日志
void POV::handleLog(rapidjson::Document &d)
{
	//std::cout<<"接受到日志:\n";
	//print_document(d);

	for(uint32_t i=0;i<account_manager.getButlerAmount();i++)
	{
		account butler=account_manager.getButler(i);
		NodeID id=butler.getNodeId();
		//std::cout<<"发送的管家id为："<<id<<"\n";
		if(id==0)
		{
			std::string pubkey=butler.getPubKey();
			char* ch=const_cast<char*>(pubkey.c_str());
			uint32_t *map_id=(uint32_t*)ch;
			rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(pubkey);
			sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
			delete &msg;
		}
		else
		{
			rapidjson::Document& msg=msg_manager.make_Request_Normal(id,d);
			//print_document(msg);
			sendMessage(msg,false,20,Recall_RequestNormal,id,&POV::handleResponseNormal);
			delete &msg;
		}
	}
}

//处理日志查询请求
void POV::handleLogQuery()
{
	//std::cout<<"handleLogQuery: 1\n";
	boost::asio::io_service service;
	//std::cout<<"handleLogQuery: 2\n";
	boost::asio::ip::tcp::acceptor acceptor(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),query_port));
	//std::cout<<"查询端口为："<<query_port<<"\n";
	//std::cout<<"handleLogQuery: 3\n";
	//service.run();

    while ( true) {
        char buff[65535];
        memset(buff,0,65535);
    	std::cout<<"处理日志查询任务中...\n";
    	boost::asio::ip::tcp::socket sock(service);
    	//std::cout<<"handleLogQuery: 4\n";
        acceptor.accept(sock);
    	//std::cout<<"handleLogQuery: 5\n";
    	sock.read_some(boost::asio::buffer(buff));
        //int bytes = boost::asio::read(sock, boost::asio::buffer(buff), boost::bind(&POV::read_complete,buff,_1,_2));
    	//std::cout<<"handleLogQuery: 6\n";

    	std::string msg(buff);
    	rapidjson::Document doc;
    	doc.Parse(msg.c_str(),msg.size());
        std::cout<<"接收到的信息："<<msg<<"\n";

    	if(doc.IsObject()&&doc.HasMember("log"))
    	{
    		std::string command=doc["log"].GetString();
    		if(command=="fetch_height")
    		{
    			std::cout<<"请求区块链高度\n";
    			int height=blockchain.queryHeight();
        		rapidjson::Document d;
        		d.SetObject();
        		rapidjson::Document::AllocatorType& allocator=d.GetAllocator();
        		rapidjson::Value _height(height);
        		d.AddMember("height",_height,allocator);
    	        sock.write_some(boost::asio::buffer(getDocumentString(d)));
    		}
    		else if(command=="fetch_block")
    		{
    			std::cout<<"请求根据高度获取区块\n";
    			if(doc.HasMember("height"))
    			{
    				int height=doc["height"].GetInt();
    				PoVBlock block=blockchain.getBlockFromDatabase(height);
    				rapidjson::Document& d=block.getBlock();
        	        sock.write_some(boost::asio::buffer(getDocumentString(d)));
        	        delete &d;
    			}
    		}
    		else if(command=="fetch_log_time")
    		{
    			std::cout<<"请求根据时间范围获取日志\n";
    			if(doc.HasMember("start_time"))
    			{
    				//std::cout<<"接收到日志请求:1\n";
    				long start_time=doc["start_time"].GetInt64();
    				//std::cout<<"接收到日志请求:2\n";
    				long end_time=9999999999;
    				if(doc.HasMember("end_time"))
    				{
    					end_time=doc["end_time"].GetInt64();
    				}
    				//std::cout<<"接收到日志请求:3\n";
    				rapidjson::Document& d=blockchain.queryLogByTime(start_time,end_time);
    				//std::cout<<"接收到日志请求:4\n";
    				std::string buff=getDocumentString(d);
    				//std::cout<<"接收到日志请求:5\n";
        	        sock.write_some(boost::asio::buffer(buff));
    				//std::cout<<"接收到日志请求:6\n";
        	        delete &d;
    			}
    		}
    		else if(command=="fetch_log_errorCode" && doc.HasMember("errorCode"))
    		{
    			std::cout<<"请求根据错误码获取日志\n";
				//std::cout<<"接收到fetch_log_errorCode请求:1\n";
    			std::string err_code=doc["errorCode"].GetString();
				//std::cout<<"接收到fetch_log_errorCode请求:2\n";
				rapidjson::Document& d=blockchain.queryLogByErrorCode(err_code);
				//std::cout<<"接收到fetch_log_errorCode请求:3\n";
				std::string buff=getDocumentString(d);
				//std::cout<<"接收到fetch_log_errorCode请求:4\n";
    	        sock.write_some(boost::asio::buffer(buff));
				//std::cout<<"接收到fetch_log_errorCode请求:5\n";
    		}
    	}
    	else if(!doc.IsObject())
    	{
    		std::cout<<"不是object错误\n";
    		rapidjson::Document d;
    		d.SetObject();
    		rapidjson::Document::AllocatorType& allocator=d.GetAllocator();
    		std::string error_msg="字符串不是json文档";
    		rapidjson::Value _error_msg(error_msg.c_str(),error_msg.size(),allocator);
    		d.AddMember("error",_error_msg,allocator);
	        sock.write_some(boost::asio::buffer(getDocumentString(d)));
    	}
    	else
    	{
    		std::cout<<"没有log错误\n";
    		rapidjson::Document d;
    		d.SetObject();
    		rapidjson::Document::AllocatorType& allocator=d.GetAllocator();
    		std::string error_msg="不含log类型";
    		rapidjson::Value _error_msg(error_msg.c_str(),error_msg.size(),allocator);
    		d.AddMember("error",_error_msg,allocator);
	        sock.write_some(boost::asio::buffer(getDocumentString(d)));
    	}
        //std::string msg(buff, bytes);
        sock.close();
    }
}

