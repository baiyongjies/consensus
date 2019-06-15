#include "POV.h"

//顺序同步
void POV::syncBlockSequence()
{
	//同步区块，该算法的区块高度等待时间必须比区块产生周期短，否则一个节点永远无法加入区块链网络
	//向所有节点发送高度请求，这个请求只发送一次
	double now=getCurrentTime();
	if(fetch_height_start_time<=0)
	{
		//std::cout<<"同步区块：1"<<"\n";
		fetch_height_start_time=now;
		//rapidjson::Document& msg=msg_manager.make_Request_Height();
		//sendMessage(msg,false,10,Recall_RequestCommissionerPubkey,0,&POV::handleResponseCommissionerPubkey);
		//delete &msg;
		//break;
	}

	//根据时间判断当前的syncer是否有效，即正在进行区块同步
	bool valid=false;
	if(now-current_syncer.start_time<current_syncer.wait_time and current_syncer.height>blockchain.getHeight())
	{
		//std::cout<<"同步区块：0"<<"\n";
		valid=true;
	}
	//在syncers为空且current_syncer无效的情况下，若等待区块头时间超过设定的等待时间，则跳转到生成创世区块或者普通阶段
	if(syncers.empty() && !valid)
	{
		//std::cout<<"同步区块：2"<<"\n";
		//std::cout<<"sync block: 1\n";
		if(now-fetch_height_start_time>fetch_height_wait_time)
		{
			//std::cout<<"sync block: 2\n";
			int height= blockchain.getHeight();
			if(height==-1)
			{
				std::cout<<"同步区块：go to genesis"<<"\n";
				state=generate_first_block;
			}
			else
			{
				for(int i=0;i<height;i++)
				{
					PoVBlock block=blockchain.getBlock(i);
					rapidjson::Document& d=block.getBlock();
					updateVariables(d);
					delete &d;
				}
				std::cout<<"同步区块：go to normal"<<"\n";
				state=normal;
			}
			return;
		}
	}

	if(syncers.empty())
	{
		//std::cout<<"同步区块：3"<<"\n";
		rapidjson::Document& msg=msg_manager.make_Request_Height();
		//std::cout<<"cbi数量："<<CallBackList.size()<<"\n";
		sendMessage(msg,true,fetch_height_wait_time_last_encore,Recall_RequestHeight,0,&POV::handleResponseHeight);
		//std::cout<<"cbi数量："<<CallBackList.size()<<"\n";
		delete &msg;
	}
	while(!syncers.empty() && !valid)
	{//如果syncer不为空且当前syncer无效，就从syncers队列中取出一个有效的syncer来进行同步。
		//std::cout<<"sync block: 3\n";
		//std::cout<<"同步区块：4"<<"\n";
		syncer_mutex.lock();
		current_syncer=syncers.front();
		syncers.pop();
		syncer_mutex.unlock();
		//std::cout<<"同步区块：5"<<"\n";
		if(current_syncer.height>blockchain.getHeight())
		{
			valid=true;
			current_syncer.start_time=now;
			current_syncer.wait_time=syncer_wait_time;
			NodeID receiver=current_syncer.id;
			rapidjson::Document& msg=msg_manager.make_Request_Block(receiver,blockchain.getHeight()+1);
			sendMessage(msg,true,5,Recall_RequestBlock,blockchain.getHeight()+1,&POV::handleResponseBlock);
			delete &msg;
		}
		//std::cout<<"同步区块：6"<<"\n";
		/*
		if(syncers.empty())
		{
			rapidjson::Document& msg=msg_manager.make_Request_Height();
			std::cout<<"cbi数量："<<CallBackList.size()<<"\n";
			sendMessage(msg,true,fetch_height_wait_time_last_encore,Recall_RequestCommissionerPubkey,0,&POV::handleResponseCommissionerPubkey);
			std::cout<<"cbi数量："<<CallBackList.size()<<"\n";
			delete &msg;
		}
		*/
	}
	if(valid)
	{
		if(blockchain.getHeight()<current_syncer.height)
		{
			NodeID receiver=current_syncer.id;
			std::cout<<"向"<<receiver<<"请求高度为"<<blockchain.getHeight()+1<<"的区块\n";
			rapidjson::Document& msg=msg_manager.make_Request_Block(receiver,blockchain.getHeight()+1);
			//std::cout<<"CallBackList.size()="<<CallBackList.size()<<"\n";
			sendMessage(msg,true,10,Recall_RequestBlock,blockchain.getHeight()+1,&POV::handleResponseBlock);
			//std::cout<<"CallBackList.size()="<<CallBackList.size()<<"\n";
			//print_document(msg);
			delete &msg;
			current_syncer.start_time=getCurrentTime();
		}
	}
}
//并发同步
void POV::syncBlockConcurrency()
{
	double now=getCurrentTime();
	if(fetch_height_start_time<=0)
	{
		fetch_height_start_time=now;
		//rapidjson::Document& msg=msg_manager.make_Request_Height();
		//sendMessage(msg,false,10,Recall_RequestCommissionerPubkey,0,&POV::handleResponseCommissionerPubkey);
		//delete &msg;
		//break;
	}
	int height= blockchain.getHeight();
	if(con_syncers.empty() && now-fetch_height_start_time>fetch_height_wait_time)
	{
		//std::cout<<"sync block: 2\n";
		if(height==-1)
		{
			state=generate_first_block;
		}
		else
		{
			for(int i=0;i<height;i++)
			{
				PoVBlock block=blockchain.getBlock(i);
				rapidjson::Document& d=block.getBlock();
				updateVariables(d);
				delete &d;
			}
			state=normal;
		}
		return;
	}
	else
	{
		rapidjson::Document& msg=msg_manager.make_Request_Height();
		//std::cout<<"cbi数量："<<CallBackList.size()<<"\n";
		sendMessage(msg,true,fetch_height_wait_time_last_encore,Recall_RequestCommissionerPubkey,0,&POV::handleResponseCommissionerPubkey);
		//std::cout<<"cbi数量："<<CallBackList.size()<<"\n";
		delete &msg;
	}
	std::map<NodeID,NodeInfo>& nodelist=network->getNodeList();
	int nums=nodelist.size();
	std::vector<NodeID> list;
	for(auto i=nodelist.begin();i!=nodelist.end();i++)
	{
		list.push_back(i->first);
	}
	for(int i=0;i<con_syncers.size();i++)
	{
		if(!con_syncers[i].prepared)
		{
			int request_height=con_syncers[i].height;
			NodeID receiver=list[request_height%nums];
			rapidjson::Document& msg=msg_manager.make_Request_Block(receiver,request_height);
			sendMessage(msg,true,5,Recall_RequestBlock,request_height,&POV::handleResponseBlock);
			delete &msg;
		}
	}
	while((!con_syncers.empty()) && con_syncers[0].prepared)
	{
		int next_height=con_syncers[0].height;
		if(next_height>0)
		{
			std::string prehash=con_syncers[0].prehash;
			PoVBlock b=blockchain.getBlock(height);
			std::string true_hash=key_manager.getHash256(b.getHeader());
			if(true_hash!=prehash)
			{
				std::cout<<"验证区块头错误——前一个区块的hash错误！\n";
				std::cout<<"我的区块链高度="<<height<<"\n";
				std::cout<<"我的hash="<<true_hash<<"\n";
				std::cout<<"同步器的区块链高度="<<next_height<<"\n";
				std::cout<<"同步器的prehash="<<prehash<<"\n";
				//验证区块不通过的情况下随机选取节点进行区块同步
				NodeID receiver=list[rand()%nums];
				rapidjson::Document& msg=msg_manager.make_Request_Block(receiver,next_height);
				sendMessage(msg,true,5,Recall_RequestBlock,next_height,&POV::handleResponseBlock);
				con_syncers[0].prepared=false;
				delete &msg;
				break;
			}
			blockchain.pushbackBlockToDatabase(con_syncers[0].block);
			con_syncers.erase(con_syncers.begin());
		}
		else
		{
			blockchain.pushbackBlockToDatabase(con_syncers[0].block);
			con_syncers.erase(con_syncers.begin());
		}
	}
}
