#include "POV.h"
#include <cstring>

//判断消息是否读取完整
size_t POV::read_complete(char * buff, const boost::system::error_code & err, size_t bytes)
{
    if ( err) return 0;
    bool found = std::find(buff, buff + bytes, '\n') < buff + bytes;
    // 我们一个一个读取直到读到回车，不缓存
    return found ? 0 : 1;
}
//返回消息给客户端
void POV::sendResponseToClient(boost::asio::ip::tcp::socket& sock,std::string& msg)
{
	//std::cout<<"send msg:"<<msg<<"\n";

    int len=msg.size();

    std::string len_str((char*)(&len),sizeof(int));
    std::string data=len_str+msg;
    const char* chs=data.c_str();
    boost::system::error_code ec;
    std::size_t size=boost::asio::write(sock, boost::asio::buffer(chs, data.size()),ec);
    std::cout<<"已发送数据大小"<<size<<"\n";
    if(ec)
    {
		std::cout << "status: " << ec.message() << "\n";
		//sock.close();
    }
    //sock.send(buffer(len_str+msg));
    //sock.send(buffer(msg));
}
//对外服务函数
void POV::service()
{
	//std::cout<<"handleLogQuery: 1\n";
	boost::asio::io_service service;
	//std::cout<<"handleLogQuery: 2\n";
	boost::asio::ip::tcp::acceptor acceptor(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),query_port));

    while ( true) {
        //char buff[65535];
        //memset(buff,0,65535);
    	//std::cout<<"处理日志查询任务中...\n";
    	boost::asio::ip::tcp::socket sock(service);
    	//std::cout<<"handleLogQuery: 4\n";
        acceptor.accept(sock);
    	//std::cout<<"handleLogQuery: 5\n";

		//sock.read_some(boost::asio::buffer(buff));
        //读取数据长度
		boost::asio::streambuf sb(sizeof(int));
		boost::system::error_code ec;
		boost::asio::read(sock, sb, ec);
		if (ec) {
			std::cout << "status: " << ec.message() << "\n";
			sock.close();
			continue;
		}
		boost::asio::streambuf::const_buffers_type bufs = sb.data();
		std::string line(boost::asio::buffers_begin(bufs),boost::asio::buffers_begin(bufs) + sizeof(int));
		const char *ch=line.data();
		printf("%d\n",ch[0]);
		printf("%d\n",ch[1]);
		printf("%d\n",ch[2]);
		printf("%d\n",ch[3]);
		//int len;
		//memcpy(&len,ch,4);
		int len=*((int*)ch);
		std::cout << "received size: '" << len << "'\n";
		//读取数据内容
		boost::asio::streambuf sb1(len);
		boost::system::error_code ec1;
		std::size_t n=boost::asio::read(sock, sb1, ec1);
		if (ec1) {
			std::cout << "status: " << ec1.message() << "\n";
			sock.close();
			continue;
		}
		boost::asio::streambuf::const_buffers_type buf1 = sb1.data();
		std::string msg(boost::asio::buffers_begin(buf1),boost::asio::buffers_begin(buf1) + len);
		std::cout<<"msg.size="<<msg.size()<<"\n";
        //int bytes = boost::asio::read(sock, boost::asio::buffer(buff), boost::bind(&POV::read_complete,buff,_1,_2));
    	//std::cout<<"handleLogQuery: 6\n";

    	//std::string msg(buff);
    	rapidjson::Document doc;
		ErrorMessage response;
    	try
		{

    		//std::cout<<"received msg:"<<msg<<"\n";
    		doc.Parse(msg.c_str(),msg.size());
    		if(doc.HasParseError())
    		{
        		std::cout<<"接收的数据不是json数据:\n";
        		std::cout<<"接收到的信息："<<msg<<"\n";
        		std::cout<<"msg.size="<<msg.size()<<"\n";
        		std::cout<<"接收的数据不是json数据:\n";
        		std::cout << "received size: '" << len << "'\n";
        		sock.close();
        		continue;
    		}
		}
    	catch(...)
    	{
    		response.errcode=1;
    		response.msg="接收的数据不是json数据";
			rapidjson::Document resp;
			resp.SetObject();
			rapidjson::Document::AllocatorType& allocator=resp.GetAllocator();
			rapidjson::Value errcode(response.errcode);
			resp.AddMember("StatusCode",errcode,allocator);
			if(response.is_json)
			{
				rapidjson::Document msg;
				msg.Parse(response.msg.c_str(),response.msg.size());
				resp.AddMember("msg",msg,allocator);
				rapidjson::Value isJson("json",allocator);
				resp.AddMember("MsgType",isJson,allocator);
				if(response.use_num_type)
				{
					rapidjson::Value _type(response.type_num);
					resp.AddMember("type",_type,allocator);
				}
				try
				{
					std::string resp_msg=getDocumentString(resp);
					sendResponseToClient(sock,resp_msg);
					//sock.write_some(boost::asio::buffer(getDocumentString(resp)));
				}
				catch(...)
				{
					std::cout<<"写数据异常：0\n";
				}
			}
			else
			{
				rapidjson::Value msg(response.msg.c_str(),response.msg.size(),allocator);
				resp.AddMember("msg",msg,allocator);
				rapidjson::Value isJson("string",allocator);
				resp.AddMember("MsgType",isJson,allocator);
				if(response.use_num_type)
				{
					rapidjson::Value _type(response.type_num);
					resp.AddMember("type",_type,allocator);
				}
				try
				{
					std::string resp_msg=getDocumentString(resp);
					sendResponseToClient(sock,resp_msg);
					//sock.write_some(boost::asio::buffer(getDocumentString(resp)));
				}
				catch(...)
				{
					std::cout<<"写数据异常：1\n";
				}
			}
    		std::cout<<"接收的数据不是json数据:\n";
    		std::cout<<"接收到的信息："<<msg<<"\n";
    		sock.close();
    		continue;
    	}

    	if(doc.IsObject()&&doc.HasMember("type"))
    	{
    		std::string type=doc["type"].GetString();
    		if(type=="NDN-IP")
    		{
    			response=handleNDNQuery(doc);

    		}
    		else if(type=="blockchain")
    		{
    			response=handleBlockchainQuery(doc);
    		}

    	}
    	else if(!doc.IsObject())
    	{
    		response.errcode=2;
    		response.msg="json字符串不是json对象";
//    		std::cout<<"不是object错误\n";
//    		rapidjson::Document d;
//    		d.SetObject();
//    		rapidjson::Document::AllocatorType& allocator=d.GetAllocator();
//    		std::string error_msg="字符串不是json文档";
//    		rapidjson::Value _error_msg(error_msg.c_str(),error_msg.size(),allocator);
//    		d.AddMember("error",_error_msg,allocator);
//	        sock.write_some(boost::asio::buffer(getDocumentString(d)));
    	}
    	else
    	{
    		response.errcode=3;
    		response.msg="json对象不包含type类型";
//    		std::cout<<"没有type错误\n";
//    		rapidjson::Document d;
//    		d.SetObject();
//    		rapidjson::Document::AllocatorType& allocator=d.GetAllocator();
//    		std::string error_msg="不含type类型";
//    		rapidjson::Value _error_msg(error_msg.c_str(),error_msg.size(),allocator);
//    		d.AddMember("error",_error_msg,allocator);
//	        sock.write_some(boost::asio::buffer(getDocumentString(d)));
    	}
		rapidjson::Document resp;
		resp.SetObject();
		rapidjson::Document::AllocatorType& allocator=resp.GetAllocator();
		rapidjson::Value errcode(response.errcode);
		resp.AddMember("StatusCode",errcode,allocator);
		if(response.is_json)
		{
			rapidjson::Document msg;
			msg.Parse(response.msg.c_str(),response.msg.size());
			resp.AddMember("msg",msg,allocator);
			rapidjson::Value isJson("json",allocator);
			resp.AddMember("MsgType",isJson,allocator);
			if(response.use_num_type)
			{
				rapidjson::Value _type(response.type_num);
				resp.AddMember("type",_type,allocator);
			}
			try
			{
				std::string resp_msg=getDocumentString(resp);
				std::cout<<"返回数据:"<<resp_msg<<"\n";
				std::cout<<"数据大小:"<<resp_msg.size()<<"\n";
				sendResponseToClient(sock,resp_msg);
				//sock.write_some(boost::asio::buffer(getDocumentString(resp)));
			}
			catch(...)
			{
				std::cout<<"写数据异常：2\n";
			}
		}
		else
		{
			rapidjson::Value msg(response.msg.c_str(),response.msg.size(),allocator);
			resp.AddMember("msg",msg,allocator);
			rapidjson::Value isJson("string",allocator);
			resp.AddMember("MsgType",isJson,allocator);
			if(response.use_num_type)
			{
				rapidjson::Value _type(response.type_num);
				resp.AddMember("type",_type,allocator);
			}
			try
			{
				std::string resp_msg=getDocumentString(resp);
				sendResponseToClient(sock,resp_msg);
				//sock.write_some(boost::asio::buffer(getDocumentString(resp)));
			}
			catch(...)
			{
				std::cout<<"写数据异常：3\n";
			}
		}
        sock.close();
    }
}
