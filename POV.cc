/*
 * POV.cpp
 *
 *  Created on: 2018年3月16日
 *      Author: blackguess
 */

#include "POV.h"
#include <memory>
#include "constants.h"
#include <vector>
#include "blockchain-network.h"
#include "utils.h"
#include <stdlib.h>
//#include "simulator.h"
#include <fstream>
#include <cstdio>
#include<ctime>
#include "ConfigHelper.h"

#include <functional>
#include <boost/bind.hpp>
#include "boost/function.hpp"
#include <string.h>
//#include <sstream>

POV::POV() {
	// TODO Auto-generated constructor stub

}

POV::~POV() {
	// TODO Auto-generated destructor stub
}
//POV主循环
void POV::run()
{
	//std::cout<<"run:start!\n";
	//处理超时的CallBackInstance
	call_back_mutex.lock();
	for(std::vector<CallBackInstance>::iterator i=CallBackList.begin();i!=CallBackList.end();++i)
	{
		double current_time=getCurrentTime();
		if(i->start_time+i->wait_time<=current_time)
		{
			CallBackList.erase(i);
			i--;
		}
	}
	call_back_mutex.unlock();
	switch(state)
	{
	case sync_block:
	{
		if(sync_block_method==SequenceSync)
//			if(init_commissioner_size<20)
//				syncBlockSequence();
//			else
//				state=generate_first_block;
			syncBlockSequence();
		else
			syncBlockConcurrency();
		break;

	}
	case generate_first_block:
	{
		//std::cout<<network->getNodeId()<<"进入产生创世区块阶段:1\n";
		//在创世区块阶段，系统先获取所有委员的公钥
		bool have_all_commissoner_pubkey=true;
		/*
		std::map<NodeID,NodeInfo>& mapper=network->getNodeList();
		std::cout<<"neighbor nodes:"<<mapper.size()<<"\n";
		for(auto i=mapper.begin();i!=mapper.end();i++)
		{
			std::cout<<network->getNodeId()<<" 邻居节点："<<i->first<<"\n";
		}
		*/
		for(uint32_t i=0;i<init_commissioner_size;i++)
		{
			//uint32_t myid=network->getNodeId();
			//std::cout<<"generate_first_block——collect commissioner pubkey：1\n";
			if(!account_manager.is_commissioner(Initial_Commissioner_Nodes[i]))
			{
				rapidjson::Document &d=msg_manager.make_Request_Commissioner_PublicKey(Initial_Commissioner_Nodes[i]);
				sendMessage(d,true,10,Recall_RequestCommissionerPubkey,Initial_Commissioner_Nodes[i],&POV::handleResponseCommissionerPubkey);
				have_all_commissoner_pubkey=false;
				//delete &d;
				d.SetNull();
				d.GetAllocator().Clear();
				delete &d;
			}
			//std::cout<<"generate_first_block——collect commissioner pubkey：2\n";
		}
		//std::cout<<network->getNodeId()<<"进入产生创世区块阶段:2\n";
		//如果有所有委员的公钥，则进入申请成为委员的流程。
		if(have_all_commissoner_pubkey)
		{
			//std::cout<<"have_all_commissoner_pubkey!\n";
			if(I_am_butler_candidate)
			{
				//申请管家候选流程从这里开始
				//std::cout<<"申请管家候选!\n";
				//std::cout<<network->getNodeId()<<"进入产生创世区块阶段-申请成为管家候选:1\n";
				try
				{
					Initial_ApplyButlerCandidate();

				}
				catch(...)
				{
					std::cout<<"I_am_butler_candidate中Initial_ApplyButlerCandidate()异常\n";
					throw "";
				}
				//std::cout<<network->getNodeId()<<"进入产生创世区块阶段-申请成为管家候选:2\n";
			}/*I_am_butler_candidate*/

			if(I_am_commissioner)
			{
				//std::cout<<"申请委员!\n";
				//std::cout<<network->getNodeId()<<"收集到所有委员公钥!\n";
				//std::cout<<network->getNodeId()<<"进入产生创世区块阶段-申请成为委员:1\n";
				try
				{
					Initial_ApplyCommissioner();
				}
				catch(...)
				{
					std::cout<<"I_am_commissioner中Initial_ApplyCommissioner()异常\n";
					throw "";
				}
				//std::cout<<network->getNodeId()<<"进入产生创世区块阶段-申请成为委员:2\n";
				//当管家候选人数达到或超过预定的管家数，开始生成投票并发送给代理委员。
				//std::cout<<"投票!\n";
				//std::cout<<network->getNodeId()<<"进入产生创世区块阶段-投票:1\n";
				try
				{
					Initial_Vote();

				}
				catch(...)
				{
					std::cout<<"I_am_commissioner中Initial_Vote()异常\n";
					throw "";
				}
				//std::cout<<network->getNodeId()<<"进入产生创世区块阶段-投票:2\n";
				//代理委员执行策略，这里的代理委员用节点ID判断
				if(network->getNodeId()==AgentCommissioner)
				{
					//std::cout<<"生成创世区块!\n";
					//std::cout<<network->getNodeId()<<"进入产生创世区块阶段-产生区块:1\n";
					try
					{
						Initial_GenerateBlock();
					}
					catch(...)
					{
						std::cout<<"AgentCommissioner中Initial_GenerateBlock()异常\n";
						throw "";
					}
					//std::cout<<network->getNodeId()<<"进入产生创世区块阶段-产生区块:2\n";
				}/*值班委员执行策略*/
			}/*I_am_commissioner*/
		}/*have_all_commissoner_pubkey*/
		//std::cout<<network->getNodeId()<<"进入产生创世区块阶段:3\n";
		break;
	}
	case normal:
	{
		//std::cout<<"在"<<network->getNodeId()<<"的Normal状态中：start!\n";
		if(getCurrentTime()-upload_start_time>upload_period)
		{
			collector.setNodeMessage(key_manager.getPublicKey(),
					network->getIP(),
					name,account_manager.is_butler_candidate(),
					account_manager.is_butler(),
					account_manager.is_commissioner());
			collector.sendNodeMessage();
			upload_start_time=getCurrentTime();
		}
		if(getCurrentTime()-check_start_time>check_period)
		{
			for(auto i=login_users.begin();i!=login_users.end();)
			{
				if(getCurrentTime()-i->second>keep_time)
				{
					i=login_users.erase(i);
				}
				else
				{
					i++;
				}
			}
		}
		checkAndAddBlock();
		//testRegistryNDN();
		current_duty_butler_num=((int)((getCurrentTime()-start_time)/Tcut)+init_duty_butler_num)%butler_amount;
		current_duty_butler_pubkey=account_manager.getButlerPubkeyByNum(current_duty_butler_num);
		if(keep_normal_pool_full)
		{
			Normal_PublishTransactionKeepPoolFull();
		}
		else
		{
			Normal_PublishTransaction();
		}
		//std::cout<<"在"<<network->getNodeId()<<"的Normal状态中：2!\n";
		//如果是委员，则在任期结束时投票，否则进入申请委员阶段
		if(account_manager.is_commissioner())
		{
			if(period_generated_block_num==0)
			{
				Normal_Vote();
			}
			checkAndSendSignature();
			Normal_QuitCommissioner();
		}
		else
		{
			Normal_ApplyCommissioner();
		}
		if(account_manager.is_butler_candidate())
		{
			Normal_QuitButlerCandidate();
		}
		else
		{
			Normal_ApplyButlerCandidate();
		}
		//std::cout<<"在"<<network->getNodeId()<<"的Normal状态中：3!\n";
		if(account_manager.is_butler())
		{
			if(account_manager.getMyPubkey()==current_duty_butler_pubkey)
			{
				double passed_time=(getCurrentTime()-start_time)-((int)((getCurrentTime()-start_time)/Tcut))*Tcut;
				//std::cout<<"passed_time:"<<passed_time<<"\n";
				if(passed_time>defer_time)
				{
					//std::cout<<network->getNodeId()<<"节点在值班管家处理中：\n";
					//std::cout<<"start time="<<start_time<<"\n";
					//std::cout<<"getCurrentTime()="<<getCurrentTime()<<"\n";
					//std::cout<<"init_duty_butler_num="<<init_duty_butler_num<<"\n";
					//std::cout<<"current_duty_butler_num="<<current_duty_butler_num<<"\n";
					Normal_GenerateBlock();
				}/*defer time*/
				else
				{
					//GenerateBlock=GenerateBlock_sign;
				}
			}
			else //如果不是值班管家
			{
				GenerateBlock=GenerateBlock_sign;
				//cache_block_mutex.lock();
				if(cache_block!=NULL)
				{

					delete cache_block;
					cache_block=NULL;
				}
				//cache_block_mutex.unlock();

				rbs_mutex.lock();
				raw_block_signatures.clear();
				rbs_mutex.unlock();
			}
		}
		//std::cout<<"在"<<network->getNodeId()<<"的Normal状态中：4!\n";
		break;
	}
	}/*switch(state)*/
	//std::cout<<"run:end!\n";

}
//生成第一个区块
PoVBlock POV::generateFirstBlock()
{
	PoVBlock block;
	//生成初始化交易
	rapidjson::Document system_data;
	system_data.SetObject();
	//把系统常量加入system_data_metadata中，即生成常量交易
	rapidjson::Value system_data_metadata;
	system_data_metadata.SetObject();
	rapidjson::Value butler_num(butler_amount);
	system_data_metadata.AddMember("butler_amout",butler_num,system_data.GetAllocator());
	rapidjson::Value _vote_amount(vote_amount);
	system_data_metadata.AddMember("vote_amount",_vote_amount,system_data.GetAllocator());
	//rapidjson::Value vote_num(vote_amount,system_data.GetAllocator());
	//system_data.AddMember("butler_amout",vote_num,system_data.GetAllocator());
	rapidjson::Value blocks_num(num_blocks);
	system_data_metadata.AddMember("blocks_per_cycle",blocks_num,system_data.GetAllocator());
	rapidjson::Value _Tcut(Tcut);
	system_data_metadata.AddMember("Tcut",_Tcut,system_data.GetAllocator());
	//把system_data_metadata，system_data_metatype等加入system_data中
	system_data.AddMember("metadata",system_data_metadata,system_data.GetAllocator());
	rapidjson::Value system_data_metatype("Constants",system_data.GetAllocator());
	system_data.AddMember("metatype",system_data_metatype,system_data.GetAllocator());
	block.PushBackTransaction(system_data);
	//print_document(system_data);
	//生成申请委员交易
	ac_mutex.lock();
	std::cout<<"生成生区块中——委员交易数量："<<ApplyCommissionerPool.size()<<"\n";
	for(std::vector<DocumentContainer>::iterator i=ApplyCommissionerPool.begin();i!=ApplyCommissionerPool.end();i++)
	{
		rapidjson::Document& temp=i->getDocument();
		block.PushBackTransaction(temp);
		temp.SetNull();
		temp.GetAllocator().Clear();
		delete& temp;
	}
	ac_mutex.unlock();
	//生成申请管家候选交易
	abc_mutex.lock();
	std::cout<<"生成生区块中——管家候选交易数量："<<ApplyButlerCandidatePool.size()<<"\n";
	for(std::vector<DocumentContainer>::iterator i=ApplyButlerCandidatePool.begin();i!=ApplyButlerCandidatePool.end();i++)
	{
		rapidjson::Document& temp=i->getDocument();
		block.PushBackTransaction(temp);
		temp.SetNull();
		temp.GetAllocator().Clear();
		delete& temp;
	}
	abc_mutex.unlock();
	//生成投票交易
	rapidjson::Document &vote_data=*(new rapidjson::Document);
	vote_data.SetObject();
	rapidjson::Document::AllocatorType& vote_data_allocator=vote_data.GetAllocator();
	//打包metatype
	rapidjson::Value &vote_metatype=*(new rapidjson::Value("vote",vote_data_allocator));
	vote_data.AddMember("metatype",vote_metatype,vote_data_allocator);
	//打包metadata
	rapidjson::Value vote_metadata;
	vote_metadata.SetObject();
	rapidjson::Document& result=BallotStatistic(BallotPool);
	rapidjson::Value& ballots=*(new rapidjson::Value(rapidjson::kArrayType));
	b_mutex.lock();
	std::cout<<"生成生区块中——投票数量："<<BallotPool.size()<<"\n";
	for(std::vector<DocumentContainer>::iterator i=BallotPool.begin();i!=BallotPool.end();i++)
	{
		ballots.PushBack(i->getDocument(),vote_data_allocator);
	}
	b_mutex.unlock();
	vote_metadata.AddMember("result",result,vote_data_allocator);
	vote_metadata.AddMember("ballots",ballots,vote_data_allocator);
	vote_data.AddMember("metadata",vote_metadata,vote_data_allocator);
	block.PushBackTransaction(vote_data);
	vote_data.SetNull();
	vote_data.GetAllocator().Clear();
	delete &vote_data;
	//print_document(vote_data);
	//生成区块头
	rapidjson::Document& txs=block.getTransactionsList();
	PoVHeader header;
	header.setHeight(0);	//创世区块高度为0
	header.setCycles(0);
	header.setNumOfTrans(txs.Size());
	header.setGenerator(account_manager.getMyPubkey());
	header.setPreviousHash(""); //创世区块的前一个区块hash为空
	header.setMerkleRoot(key_manager.getHash256(txs));  //这里用hash值代替merkle根

	//print_document(txs);
	block.setHeader(header);
	txs.SetNull();
	txs.GetAllocator().Clear();
	delete &txs;
	return block;
}
//生成区块
PoVBlock POV::generateBlock()
{
	PoVBlock block;
	//生成普通区块
	if(period_generated_block_num!=0)
	{
		//生成申请委员交易
		ac_mutex.lock();
		for(std::vector<DocumentContainer>::iterator i=ApplyCommissionerPool.begin();i!=ApplyCommissionerPool.end();i++)
		{
			rapidjson::Document& temp=i->getDocument();
			block.PushBackTransaction(temp);
			temp.SetNull();
			temp.GetAllocator().Clear();
			delete& temp;
		}
		ac_mutex.unlock();
		//生成申请管家候选交易
		abc_mutex.lock();
		for(std::vector<DocumentContainer>::iterator i=ApplyButlerCandidatePool.begin();i!=ApplyButlerCandidatePool.end();i++)
		{
			rapidjson::Document& temp=i->getDocument();
			block.PushBackTransaction(temp);
			temp.SetNull();
			temp.GetAllocator().Clear();
			delete& temp;
		}
		abc_mutex.unlock();
		//生成退出管家候选交易
		std::cout<<"退出管家候选交易的数量="<<QuitButlerCandidatePool.size()<<"\n";
		qbc_mutex.lock();
		for(auto i=QuitButlerCandidatePool.begin();i!=QuitButlerCandidatePool.end();i++)
		{

			rapidjson::Document& temp=i->getDocument();
			//这里加入限制现任管家不能申请退出管家候选，防止避免管家候选人数少于系统预定人数的情况发生
			std::string pubkey=temp["metadata"]["pubkey"].GetString();
			std::cout<<pubkey<<"打算退出管家候选\n";
			if(!account_manager.is_butler(pubkey))
			{
				block.PushBackTransaction(temp);
				std::cout<<"把"<<pubkey<<"的退出管家候选交易放进生区块中\n";
			}
			temp.SetNull();
			temp.GetAllocator().Clear();
			delete& temp;
		}
		qbc_mutex.unlock();
		//生成退出委员交易
		std::cout<<"退出委员交易的数量="<<QuitCommissionerPool.size()<<"\n";
		qc_mutex.lock();
		for(auto i=QuitCommissionerPool.begin();i!=QuitCommissionerPool.end();i++)
		{
			rapidjson::Document& temp=i->getDocument();
			//这里再确认一次申请退出委员的账号是委员
			std::string pubkey=temp["metadata"]["pubkey"].GetString();
			std::cout<<pubkey<<"打算退出委员\n";
			if(account_manager.is_commissioner(pubkey))
			{
				block.PushBackTransaction(temp);
				std::cout<<"把"<<pubkey<<"的退出委员交易放进生区块中\n";
			}
			temp.SetNull();
			temp.GetAllocator().Clear();
			delete& temp;
		}
		qc_mutex.unlock();
		//生成普通交易
		norm_mutex.lock();
		int normal_trans=lru_NormalPool.size();
		if(normal_trans<=max_normal_trans_per_block)
		{
			for(int i=0;i<normal_trans;i++)
			{
				//rapidjson::Document& temp=lru_NormalPool.getElement(i).getDocument();
				rapidjson::Document& temp=lru_NormalPool.pop().getDocument();
				//std::cout<<"打包的交易：\n";
				print_document(temp);
				block.PushBackTransaction(temp);
				temp.SetNull();
				temp.GetAllocator().Clear();
				delete& temp;
			}
		}
		else
		{
			for(int i=0;i<max_normal_trans_per_block;i++)
			{
				//rapidjson::Document& temp=lru_NormalPool.getElement(i).getDocument();
				rapidjson::Document& temp=lru_NormalPool.pop().getDocument();
				//std::cout<<"打包的交易：\n";
				print_document(temp);
				block.PushBackTransaction(temp);
				temp.SetNull();
				temp.GetAllocator().Clear();
				delete& temp;
			}
		}
		norm_mutex.unlock();

		/*
		std::cout<<"打印交易列表！\n";
		for(int i=0;i<block.getTransactionsAmout();i++)
		{
			rapidjson::Document& temp=block.getTransaction(i);
			print_document(temp);
			delete& temp;
		}
		*/
		/*
		if(NormalPool.size()<max_normal_trans_per_block)
		{
			for(std::vector<DocumentContainer>::iterator i=NormalPool.begin();i!=NormalPool.end();i++)
			{
				rapidjson::Document& temp=i->getDocument();
				block.PushBackTransaction(temp);
				delete& temp;
			}
		}
		else
		{
			for(uint32_t i=0;i<max_normal_trans_per_block;i++)
			{
				uint32_t num=rand()%NormalPool.size();
				rapidjson::Document& temp=NormalPool.at(num).getDocument();
				block.PushBackTransaction(temp);
				NormalPool.erase(NormalPool.begin()+num);
				delete& temp;
			}
		}
		*/
	}
	else //生成特殊区块
	{
		//生成投票交易
		rapidjson::Document &vote_data=*(new rapidjson::Document);
		vote_data.SetObject();
		rapidjson::Document::AllocatorType& vote_data_allocator=vote_data.GetAllocator();
		//打包metatype
		rapidjson::Value &vote_metatype=*(new rapidjson::Value("vote",vote_data_allocator));
		vote_data.AddMember("metatype",vote_metatype,vote_data_allocator);
		//打包metadata
		rapidjson::Value vote_metadata;
		vote_metadata.SetObject();
		rapidjson::Document& result=BallotStatistic(BallotPool);
		rapidjson::Value& ballots=*(new rapidjson::Value(rapidjson::kArrayType));
		b_mutex.lock();
		for(std::vector<DocumentContainer>::iterator i=BallotPool.begin();i!=BallotPool.end();i++)
		{
			ballots.PushBack(i->getDocument(),vote_data_allocator);
		}
		b_mutex.unlock();
		vote_metadata.AddMember("result",result,vote_data_allocator);
		vote_metadata.AddMember("ballots",ballots,vote_data_allocator);
		vote_data.AddMember("metadata",vote_metadata,vote_data_allocator);
		block.PushBackTransaction(vote_data);
		vote_data.SetNull();
		vote_data.GetAllocator().Clear();
		delete &vote_data;
	}

	//print_document(vote_data);
	//生成区块头
	rapidjson::Document& txs=block.getTransactionsList();
	PoVHeader header;
	header.setHeight(blockchain.getHeight()+1);
	std::cout<<"set_height:"<<blockchain.getHeight()<<"\n";
	header.setCycles((int)((getCurrentTime()-start_time)/Tcut));
	header.setNumOfTrans(txs.Size());
	header.setGenerator(account_manager.getMyPubkey());
	PoVBlock pre_block=blockchain.getBlock(blockchain.getHeight());
	rapidjson::Document& h=pre_block.getHeader();
	//std::cout<<"pre_header:\n";
	//print_document(h);
	std::string pre_hash=key_manager.getHash256(h);
	header.setPreviousHash(pre_hash);
	h.SetNull();
	h.GetAllocator().Clear();
	delete &h;
	header.setMerkleRoot(key_manager.getHash256(txs));
	//print_document(txs);
	block.setHeader(header);
	txs.SetNull();
	txs.GetAllocator().Clear();
	delete &txs;
	return block;
}
//设置配置
void POV::setConfig(std::string path)
{
	std::cout<<"flag=f\n";
	//读取配置文件并设置系统变量
    ConfigHelper configSettings(path);
    //设置初始状态的委员账号
    std::string commissioner_ids;
	commissioner_ids=configSettings.Read("委员节点", commissioner_ids);
	std::vector<std::string> com_ids_str=split(commissioner_ids,",");
	delete[] Initial_Commissioner_Nodes;
	init_commissioner_size=com_ids_str.size();
	Initial_Commissioner_Nodes=new NodeID[init_commissioner_size];
	for(uint32_t i=0;i<com_ids_str.size();i++)
	{
		std::vector<std::string> addr=split(com_ids_str[i],":");
		NodeID id=network->getNodeId(addr[0],atoi(addr[1].c_str()));
		//std::stringstream ss(com_ids_str.at(i));
		//ss>>id;
		//std::cout<<"id:"<<id<<std::endl;
		Initial_Commissioner_Nodes[i]=id;
	}
	//设置管家候选节点ID
    std::string butler_candidate_ids;
	butler_candidate_ids=configSettings.Read("管家候选节点", butler_candidate_ids);
	std::vector<std::string> butler_candidate_ids_str=split(butler_candidate_ids,",");
	delete[] Initial_Butler_Candidate_Nodes;
	init_butler_candidate_size=butler_candidate_ids_str.size();
	Initial_Butler_Candidate_Nodes=new NodeID[init_butler_candidate_size];
	for(uint32_t i=0;i<butler_candidate_ids_str.size();i++)
	{
		std::vector<std::string> addr=split(butler_candidate_ids_str[i],":");
		NodeID id=network->getNodeId(addr[0],atoi(addr[1].c_str()));
		//std::stringstream ss(butler_candidate_ids_str.at(i));
		//ss>>id;
		//std::cout<<"id:"<<id<<std::endl;
		Initial_Butler_Candidate_Nodes[i]=id;
	}
	//管家节点数
	butler_amount=configSettings.Read("管家节点数",butler_amount);
	//投票人数
	vote_amount=configSettings.Read("投票节点数",vote_amount);
	//每个任职周期产生区块数
	num_blocks=configSettings.Read("每个周期产生区块数",num_blocks);
	//每个区块产生的截止时间
	Tcut=configSettings.Read("区块产生截止时间",Tcut);
	//正常交易缓存池的容量大小
	NormalPoolCapacity=configSettings.Read("交易缓存池容量",NormalPoolCapacity);
	//每个区块最多存放的普通交易数量
	max_normal_trans_per_block=configSettings.Read("区块最大交易量",max_normal_trans_per_block);
	//每个管家开始打包区块前等待的时间
	defer_time=configSettings.Read("生成区块延迟",defer_time);
	//设置代理委员
	agent_commissioner_num=configSettings.Read("代理委员编号",agent_commissioner_num);
	AgentCommissioner=Initial_Commissioner_Nodes[agent_commissioner_num];
	//设置初始节点身份
	for(uint32_t i=0;i<init_commissioner_size;i++)
	{
		if(Initial_Commissioner_Nodes[i]==network->getNodeId())
		{
			I_am_commissioner=true;
		}
	}
	for(uint32_t i=0;i<init_butler_candidate_size;i++)
	{
		if(Initial_Butler_Candidate_Nodes[i]==network->getNodeId())
		{
			I_am_butler_candidate=true;
		}
	}
	//设置密钥
	std::string pubkey;
	pubkey=configSettings.Read("公钥",pubkey);
	std::string prikey;
	prikey=configSettings.Read("私钥",prikey);
	bool ret1=key_manager.setPrivateKey(prikey);
	bool ret2=key_manager.setPublicKey(pubkey);
	if(ret1 && ret2)
	{
		my_account.setPubKey(pubkey);
		account_manager.setMyPubkey(pubkey);
		msg_manager.setPubkey(pubkey);
		blockchain.setPubkey(pubkey);
	}
	//清空区块链数据库
	std::string is_clear_database;
	is_clear_database=configSettings.Read("清空数据库",is_clear_database);
	if(is_clear_database=="yes")
	{
		blockchain.deleteBlockChainFromDatabase();
	}
	//加载区块链
	blockchain.loadBlockChain();
	//设置查询日志端口
	query_port=configSettings.Read("查询端口",query_port);
	query_thread=new std::thread(std::bind(&POV::handleLogQuery,this));
	query_thread->detach();
	//设置同步区块的开始时间
	fetch_height_wait_time=configSettings.Read("同步区块的等待时间",fetch_height_wait_time);
	fetch_height_wait_time_last_encore=configSettings.Read("发送高度请求的时间间隔",fetch_height_wait_time_last_encore);
	syncer_wait_time=configSettings.Read("同步器失效的等待时间",syncer_wait_time);
	//设置程序结束的区块高度
	end_height=configSettings.Read("结束高度",end_height);
	//设置每个节点产生交易的概率
	prob_generate_normal_tx=configSettings.Read("节点产生交易的概率",prob_generate_normal_tx);
	//设置程序结束的区块高度
	keep_normal_pool_full=configSettings.Read("保持满交易池",keep_normal_pool_full);
}
//设置配置
void POV::setConfig(parameters &parcel)
{
	std::cout<<"flag=f\n";
	std::cout<<"setConfig:1\n";
	std::cout<<"parcel.init_commissioner_size="<<parcel.init_commissioner_size<<"\n";
	std::cout<<"init_commissioner_size="<<init_commissioner_size<<"\n";
	init_commissioner_size=20;
	std::cout<<"init_commissioner_size="<<init_commissioner_size<<"\n";
	init_commissioner_size=parcel.init_commissioner_size;
	std::cout<<"init_commissioner_size="<<init_commissioner_size<<"\n";
	//delete[] Initial_Commissioner_Nodes;
	Initial_Commissioner_Nodes=parcel.Initial_Commissioner_Nodes;
	std::cout<<"parcel.Initial_Commissioner_Nodes.size="<<sizeof(parcel.Initial_Commissioner_Nodes)<<"\n";
	std::cout<<"Initial_Commissioner_Nodes[0]="<<Initial_Commissioner_Nodes[0]<<"\n";
	init_butler_candidate_size=parcel.init_butler_candidate_size;
	std::cout<<"setConfig:1-1\n";
	//delete[] Initial_Butler_Candidate_Nodes;
	Initial_Butler_Candidate_Nodes=parcel.Initial_Butler_Candidate_Nodes;
	std::cout<<"setConfig:1-2\n";
	butler_amount=parcel.butler_amount;
	vote_amount=parcel.vote_amount;
	num_blocks=parcel.num_blocks;
	std::cout<<"setConfig:1-3\n";
	Tcut=parcel.Tcut;
	NormalPoolCapacity=parcel.NormalPoolCapacity;
	lru_NormalPool.init(NormalPoolCapacity);
	max_normal_trans_per_block=parcel.max_normal_trans_per_block;
	defer_time=parcel.defer_time;
	AgentCommissioner=parcel.AgentCommissioner;
	vote_mode=parcel.vote_mode;
	//I_am_commissioner=parcel.I_am_commissioner;
	//I_am_butler_candidate=parcel.I_am_butler_candidate;
	std::cout<<"setConfig:2\n";
	//设置初始节点身份
	for(uint32_t i=0;i<init_commissioner_size;i++)
	{
		if(Initial_Commissioner_Nodes[i]==network->getNodeId())
		{
			I_am_commissioner=true;
		}
	}
	std::cout<<"setConfig:3\n";
	for(uint32_t i=0;i<init_butler_candidate_size;i++)
	{
		if(Initial_Butler_Candidate_Nodes[i]==network->getNodeId())
		{
			I_am_butler_candidate=true;
		}
	}
	std::cout<<"setConfig:4\n";
	//设置密钥和初始化各种组件
	bool ret1=key_manager.setPrivateKey(parcel.prikey);
	bool ret2=key_manager.setPublicKey(parcel.pubkey);
	std::cout<<"setConfig:5\n";
	if(ret1 && ret2)
	{
		my_account.setPubKey(parcel.pubkey);
		account_manager.setMyPubkey(parcel.pubkey);
		msg_manager.setPubkey(parcel.pubkey);
		blockchain.setPubkey(parcel.pubkey);
	}
	std::cout<<"setConfig:6\n";
	std::string is_clear_database=parcel.is_clear_database;
	//清空区块链数据
	if(is_clear_database=="yes")
	{
		blockchain.deleteBlockChainFromDatabase();
	}
	//清空NDN和NDN-USER数据
	blockchain.deleteAllDataFromDatabase("NDN");
	blockchain.deleteAllDataFromDatabase("NDN-USER");
	std::cout<<"setConfig:7\n";
	//加载区块链
	blockchain.loadBlockChain();
	std::cout<<"setConfig:8\n";
	//设置查询日志端口
	query_port=parcel.query_port;
	//开启查询服务线程
	//query_thread=new std::thread(std::bind(&POV::handleLogQuery,this));
	//query_thread->detach();
	service_thread=new std::thread(std::bind(&POV::service,this));
	service_thread->detach();
	std::cout<<"setConfig:9\n";
	fetch_height_wait_time=parcel.fetch_height_wait_time;
	fetch_height_wait_time_last_encore=parcel.fetch_height_wait_time_last_encore;
	syncer_wait_time=parcel.syncer_wait_time;
	//设置程序结束的区块高度
	end_height=parcel.end_height;
	//设置每个节点产生交易的概率
	prob_generate_normal_tx=parcel.prob_generate_normal_tx;
	std::cout<<"setConfig:10\n";
	//保持交易缓存池为满状态，测试用。
	keep_normal_pool_full=parcel.keep_normal_pool_full;

	passed_prob=parcel.passed_prob;
	wait_for_all_sigs_time=parcel.wait_for_all_sigs_time;
	//读取委员过滤列表
	filter_list.clear();
	std::string filter_path=parcel.filter_path+"filter_"+parcel.HOST_IP+"_"+getIntString(parcel.HOST_PORT);
	std::cout<<filter_path<<endl;
    char buffer[256];
    fstream outFile;
    outFile.open(filter_path.c_str(),ios::in);
    //cout<<"inFile.txt"<<"--- all file is as follows:---"<<endl;
    if(outFile)
    {
		while(!outFile.eof())
		{
			memset(buffer,0,256);
			outFile.getline(buffer,256,'\n');//getline(char *,int,char) 表示该行字符达到256个或遇到换行就结束
			cout<<buffer<<endl;
			std::string line(buffer);
			std::vector<std::string> words=split(line,",");
			//filter_list.insert(filter_list.end(),words.begin(),words.end());
			for(auto i=words.begin();i!=words.end();i++)
			{
				if(trim(*i)!="")
				filter_list.push_back(*i);
			}
		}
		outFile.close();
    }
    else
    {
    	std::cout<<"读取过滤列表失败，检查路径和文件名！\n";
    }
    std::cout<<"测试filter列表：\n";
    for(int i=0;i<filter_list.size();i++)
    {
    	std::cout<<filter_list[i]<<"\n";
    }
    //读取管家过滤列表
	butler_filter_list.clear();
	std::string butler_filter_path=parcel.butler_filter_path+"butler_filter_"+parcel.HOST_IP+"_"+getIntString(parcel.HOST_PORT);
	std::cout<<butler_filter_path<<endl;
    //char buffer[256];
    fstream butler_list_File;
    butler_list_File.open(butler_filter_path.c_str(),ios::in);
    //cout<<"inFile.txt"<<"--- all file is as follows:---"<<endl;
    if(butler_list_File)
    {
		while(!butler_list_File.eof())
		{
			memset(buffer,0,256);
			butler_list_File.getline(buffer,256,'\n');//getline(char *,int,char) 表示该行字符达到256个或遇到换行就结束
			cout<<buffer<<endl;
			std::string line(buffer);
			std::vector<std::string> words=split(line,",");
			//filter_list.insert(filter_list.end(),words.begin(),words.end());
			for(auto i=words.begin();i!=words.end();i++)
			{
				if(trim(*i)!="")
					butler_filter_list.push_back(*i);
			}
		}
		butler_list_File.close();
    }
    else
    {
    	std::cout<<"读取管家过滤列表失败，检查路径和文件名！\n";
    }
    std::cout<<"测试butler_filter列表：\n";
    for(int i=0;i<butler_filter_list.size();i++)
    {
    	std::cout<<butler_filter_list[i]<<"\n";
    }
    name=parcel.name;
    //collector.setNodeMessage(key_manager.getPublicKey(),network->getIP(),name);
    collector.setServerAddress(parcel.ServerIP,parcel.ServerPort);

//	testRegistryNDN();
//	testgetUserNDN();
//	testGenNDN();
//	testUpdateNDN();
//	testQueryNDN();
//	testDeleteNDN();
}
//deprecated
void POV::updateNodeId()
{
	account_manager.setMyNodeID(network->getNodeId());
	m_ID=network->getNodeId();

}

//初始化POV模块，注意一些对象的初始化顺序不能打乱
void POV::init(blockchain_network* bn)
{
	//std::cout<<"init POV:1\n";
	network=bn;
	key_manager.init();
	key_manager.setKeyPair();//使用随机生成的一对秘钥，在实际使用中建立存储恢复秘钥功能
	std::cout<<"生成的private key:"<<key_manager.getPrivateKey()<<"\n";
	std::cout<<"生成的public key:"<<key_manager.getPublicKey()<<"\n";
	//std::cout<<"init POV:2\n";
	//设置自己账号的公钥
	my_account.setPubKey(key_manager.getPublicKey());
	account_manager.setMyPubkey(key_manager.getPublicKey());
	account_manager.setMyNodeID(network->getNodeId());
	//std::cout<<"init POV:3\n";
	msg_manager.init(key_manager.getPublicKey(),network,&key_manager); 	//初始化消息管理器
	//std::cout<<"init POV:4\n";
	state=sync_block; //设置系统的初始状态为同步区块状态。
	CallBackList=std::vector<CallBackInstance>();
	m_ID=network->getNodeId();
	//std::cout<<"init POV:5\n";
	//申请成为委员的相关元数据
	Apply_Commissioner_Metadata.SetNull();
	Apply_Commissioner_Signatures=std::vector<signature>();
	Apply_Commissioner_Transaction.SetNull();
	ApplyCommissionerPool=std::vector<DocumentContainer>();
	//std::cout<<"init POV:6\n";
	gegenerate_first_bolck_apply_commissioner_state=ApplyCommissioner_apply_signatures;		//初始化申请委员过程的状态控制
	generate_first_bolck_apply_butler_candidate_state=ApplyButlerCandidate_apply_recommendation_letter;		//初始化申请管家候选过程的控制状态
	Apply_Butler_Candidate_Signatures=std::vector<signature>();
	ApplyButlerCandidatePool=std::vector<DocumentContainer>();
	BallotPool=std::vector<DocumentContainer>();
	Quit_Butler_Candidate_Metadata.SetNull();
	Quit_Commissioner_Metadata.SetNull();
	GenerateFirstBlock_vote=Vote_voting;	//初始化投票过程的控制状态
	GenerateFirstBlock=GenerateBlock_sign;	//初始化生成区块过程的控制状态
	//初始化正常阶段的各个过程控制状态
	normal_apply_commissioner_state=ApplyCommissioner_finish;
	normal_apply_butler_candidate_state=ApplyButlerCandidate_finish;
	normal_quit_butler_candidate_state=QuitButlerCandidate_finish;
	normal_quit_commissioner_state=QuitCommissioner_finish;
	//std::cout<<"init POV:7\n";
	//vote_number=vote_amount;
	cache_block=NULL;
	//std::cout<<"init POV:71\n"
	//setClient(client);
	//blockchain=PoVBlockChain(client);
	blockchain.clear();
	blockchain.setPubkey(key_manager.getPublicKey());
	//blockchain.setCollection(client,my_account.getPubKey());
	//std::cout<<"init POV:72\n";
	AgentCommissioner=0;
	raw_block_signatures=std::vector<signature>();
	NormalPool=std::vector<DocumentContainer>();
	lru_NormalPool.init(NormalPoolCapacity);
	GenerateBlock=GenerateBlock_sign;
	Initial_Commissioner_Nodes=new NodeID[init_commissioner_size];
	Initial_Butler_Candidate_Nodes=new NodeID[init_butler_candidate_size];
	ptr=std::shared_ptr<POV>(this);
	//rapidjson::Document data;
	//data.SetObject();
	//rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	//std::string rand_str="hello world";
	//rapidjson::Value& content=*(new rapidjson::Value(rand_str.c_str(),rand_str.size(),allocator));
	//data.AddMember("content",content,allocator);
	//rapidjson::Document& d=msg_manager.make_Request_Normal(20);
	//std::cout<<"generate data:\n";
	//print_document(data);
	//blockchain.saveDataToDatabase(data,"test",false,"content",rand_str);

	//rapidjson::Document& d=blockchain.getDataFromDatabase("content",rand_str,"test",false);
	//std::cout<<"get data:\n";
	//blockchain.saveDataToDatabase(data,"test",false,"content",rand_str);
	//print_document(d);
	//std::cout<<"init POV:8\n";
	/*
	for(uint32_t i=0;i<init_commissioner_size;i++)
	{
		if(Initial_Commissioner_Nodes[i]==network->getNodeId())
		{
			I_am_commissioner=true;
		}
	}
	for(uint32_t i=0;i<init_butler_candidate_size;i++)
	{
		if(Initial_Butler_Candidate_Nodes[i]==network->getNodeId())
		{
			I_am_butler_candidate=true;
		}
	}
	*/
	//std::cout<<"init POV:9\n";
}
//投票统计
rapidjson::Document& POV::BallotStatistic(std::vector<DocumentContainer> &ballots)
{
	//对投票进行统计
	rapidjson::Document& result=*(new rapidjson::Document);
	//result.SetObject();
	result.SetArray();
	rapidjson::Document::AllocatorType& allocator=result.GetAllocator();
	/*
	rapidjson::Value& result_metatype=*(new rapidjson::Value("result",allocator));
	result.AddMember("metatype",result_metatype,allocator);

	rapidjson::Value& ballot_result=*(new rapidjson::Value(rapidjson::kArrayType));
	*/
	std::map<std::string,uint32_t> rank_list;
	//统计每个账号的得票数，结果用一个map数据结构存储
	b_mutex.lock();
	for(std::vector<DocumentContainer>::iterator i=ballots.begin();i!=ballots.end();i++)
	{
		rapidjson::Document& ballot=i->getDocument();
		rapidjson::Value& metadata=ballot["metadata"];
		//std::string sig=ballot["sig"].GetString();
		//std::string pubkey=metadata["voter"].GetString();
		rapidjson::Value& vote_list=metadata["vote_list"];
		for(uint32_t j=0;j<vote_list.Size();j++)
		{
			std::string pubkey=vote_list[j].GetString();
			if(rank_list.find(pubkey)==rank_list.end())
			{
				rank_list[pubkey]=1;
			}
			else
			{
				rank_list[pubkey]=rank_list[pubkey]+1;
			}
		}
	}
	b_mutex.unlock();
	//取出最高票数的账号作为投票结果生成选举交易
	for(uint32_t i=0;i<vote_amount;i++)
	{
		uint32_t max=0;
		std::string max_pubkey;
		for(std::map<std::string,uint32_t>::iterator j=rank_list.begin();j!=rank_list.end();j++)
		{
			if(j->second>max)
			{
				max=j->second;
				max_pubkey=j->first;
			}
		}
		rapidjson::Value& new_butler=*(new rapidjson::Value(rapidjson::kObjectType));
		new_butler.SetObject();
		new_butler.SetString(max_pubkey.c_str(),max_pubkey.size(),allocator);
		result.PushBack(new_butler,allocator);
		rank_list[max_pubkey]=0;
	}
	//result.AddMember("metadata",ballot_result,allocator);
	return result;
}
//计算出下一个管家编号
uint32_t POV::getNextButler(rapidjson::Value& sigs)
{
	//根据委员的签名计算下一任管家编号
	//std::cout<<"debug: 3\n";
	uint32_t num=sigs.Size();
	uint32_t butler_nums[num];
	std::string max_sig="";
	double max_time=0;
	//print_document(sigs);
	for(uint32_t i=0;i<num;i++)
	{
		double time=sigs[i]["timestamp"].GetDouble();
		std::string sig=sigs[i]["sig"].GetString();
		if(time>max_time)
		{
			max_time=time;
			max_sig=sig;
		}
	}
	//std::cout<<"debug: 4\n";
	//std::string hash=key_manager.getHash256(max_sig);
	unsigned char hex_sig[100];
	size_t len;
	key_manager.Str2Hex(max_sig,hex_sig,&len);

	//std::cout<<"debug: 5\n";
	unsigned char *timestamp=(unsigned char *)(&max_time);
	size_t double_len=sizeof(double);
	char *p;
	size_t p_len;
	//std::cout<<"debug: 6\n";
	if(len>double_len)
	{
		p_len=len;
		p=new char[len];
		for(size_t i=0;i<len;i++)
		{
			if(i<double_len)
			{
				p[i]=timestamp[i]^hex_sig[i];
			}
			else
				p[i]=hex_sig[i];
		}
	}
	else
	{
		p_len=double_len;
		p=new char[double_len];
		for(size_t i=0;i<double_len;i++)
		{
			if(i<len)
			{
				p[i]=timestamp[i]^hex_sig[i];
			}
			else
				p[i]=hex_sig[i];
		}
	}
	//std::cout<<"debug: 7\n";
	uint32_t* butler_num=(uint32_t*)&(p[p_len-4]);
	//std::cout<<"debug: 8\n";
	uint32_t ret_num=(*butler_num)%butler_amount;
	delete[] p;
	return ret_num;
}
//获得时间戳
uint32_t POV::getTe(rapidjson::Value& sigs)
{
	//把签名中最后时间戳最大值作为区块的时间戳
	uint32_t num=sigs.Size();
	double max_time=0;
	for(uint32_t i=0;i<num;i++)
	{
		double time=sigs[i]["timestamp"].GetDouble();
		if(time>max_time)
		{
			max_time=time;
		}
	}
	return max_time;
}

//发送消息
void POV::sendMessage(rapidjson::Document &d,bool setCallBack=false,double wait_time=100,callback_type type=Test,NodeID childtype=0,handler_ptr caller=NULL)
{

	//设置回调字段
	rapidjson::Document::AllocatorType &allocator=d.GetAllocator();
	if(d.HasMember("callback_type"))
	{
		d["callback_type"].SetUint(type);
	}
	else
	{
		rapidjson::Value _type(type);
		d.AddMember("callback_type",_type,allocator);
	}

	if(d.HasMember("child_type"))
	{
		d["child_type"].SetUint64(childtype);
	}
	else
	{
		rapidjson::Value _childtype(childtype);
		d.AddMember("child_type",_childtype,allocator);
	}
	uint32_t index=d["index"].GetUint();
	//设置CallBack

	if(setCallBack)
	{
		//std::cout<<"in setCallBack:1\n";
		bool existed=false;
		call_back_mutex.lock();
		if(CallBackList.size()>0)
		{
			//std::cout<<"回调instance数量："<<CallBackList.size()<<"!\n";
			for(std::vector<CallBackInstance>::const_iterator i=CallBackList.begin();i!=CallBackList.end();++i)
			{
				if(i->type==type and i->childtype==childtype)
				{
					//std::cout<<"该消息已发送！\n";
					if(type==Recall_RequestBlock)
					{
						//std::cout<<"type="<<int(Recall_RequestBlock)<<" && childtype="<<childtype<<"have existed!\n";
					}
					existed=true;
					break;
				}
				/*
				else
				{
					std::cout<<"i->type="<<i->type<<"\n";
					std::cout<<"type="<<type<<"\n";
					std::cout<<"i->childtype="<<i->childtype<<"\n";
					std::cout<<"childtype="<<childtype<<"\n";
				}
				*/
			}
		}
		call_back_mutex.unlock();
		//std::cout<<"in setCallBack:2\n";
		if(!existed){
			//std::cout<<"CallBackList size="<<CallBackList.size()<<" 1\n";
			//std::shared_ptr<POV> ptr(this);
			Fun f=std::bind(caller,ptr,std::placeholders::_1);
			CallBackInstance cbi(index,type,childtype,wait_time,f);
			call_back_mutex.lock();
			CallBackList.push_back(cbi);
			call_back_mutex.unlock();
			//std::cout<<"CallBackList size="<<CallBackList.size()<<" 2\n";
		}
		else
		{
			//std::cout<<"in setCallBack:4\n";
			return;

		}
		//std::cout<<"in setCallBack:3\n";

	}
	NodeID receiver=d["receiver"].GetUint64();
//	if(receiver==network->getNodeId())
//	{
//		//std::cout<<"发送给自己则直接使用handleMessage函数处理\n";
//		handleMessage(d);
//	}
//	else
	MessageType msg_type=(MessageType)d["type"].GetInt();
	if(msg_type==RequestBlockSignature)
	{
		NodeID receiver=d["receiver"].GetUint64();
		//std::cout<<getCurrentTime()<<" "<<network->getNodeId()<<"发送生区块给"<<receiver<<"\n";
	}

	network->SendMessage(d);
		//std::cout<<"in setCallBack:5\n";

}
//委员检查预区块池并发送签名给管家
void POV::checkAndSendSignature()
{
	future_raw_blocks_mutex.lock();
	int height=blockchain.getHeight()+1;
	auto iter=future_raw_blocks.find(height);
	if(iter!=future_raw_blocks.end())
	{
		MetaMessage meta_msg=iter->second;
		future_raw_blocks.erase(iter);
		rapidjson::Document d;
		d.Parse(meta_msg.data.c_str(),meta_msg.data.size());
		//PoVBlock block;
		//block.setRawBlock(d);
		try
		{
			if(validateRawBlock(d))
			{
				std::cout<<"请求区块签名中生区块验证通过\n";
				rapidjson::Value& header=d["header"];
				//NodeInfo node=network->NodeList[sender];
				rapidjson::Document &response=msg_manager.make_Response_Block_Signature(meta_msg.sender,meta_msg.index,header,name);
				//print_document(response);
				sendMessage(response,false,10,Recall_RequestBlockSignature,meta_msg.child_type,NULL);
				response.SetNull();
				response.GetAllocator().Clear();
				delete &response;
			}
			else
			{
				std::cout<<getCurrentTime()<<" "<<network->getNodeId()<<"验证高度为"<<height<<"的生区块不通过\n";
				std::cout<<"区块验证不通过\n";
				 //rapidjson::Value& header=data["header"];
				//NodeInfo node=network->NodeList[sender];
				rapidjson::Value& header=d["header"];
				rapidjson::Document &response=msg_manager.make_Response_Block_Signature_Failed(meta_msg.sender,meta_msg.index,1,"validate block wrong",header,name);
				print_document(response);
				sendMessage(response,false,10,Recall_RequestBlockSignature,meta_msg.child_type,NULL);
				response.SetNull();
				response.GetAllocator().Clear();
				delete &response;
//				//查找不正确的交易
//				rapidjson::Document &response=msg_manager.make_Response_Block_Signature_Failed(meta_msg.sender,meta_msg.index,header);
//				//print_document(response);
//				sendMessage(response,false,10,Recall_RequestBlockSignature,meta_msg.child_type,NULL);
//				delete &response;
				//std::cout<<"区块验证不通过\n";
			}
		}
		catch(...)
		{
			std::cout<<"checkAndSendSignature中验证生区块异常\n";
			throw "";
		}
		//std::cout<<"RequestBlockSignature中check：3\n";
		//blockchain.pushbackBlock(block);
//
//		try
//		{
//			delete &response;
//		}
//		catch(...)
//		{
//			std::cout<<"RequestBlockSignature中删除response异常\n";
//			throw "";
//		}
	}
	future_raw_blocks_mutex.unlock();
}
//委员检查并把区块添加进数据库当中
void POV::checkAndAddBlock()
{
	future_blocks_mutex.lock();

	while(future_blocks.find(blockchain.getHeight()+1)!=future_blocks.end())
	{
		int height=blockchain.getHeight()+1;
		auto iter=future_blocks.find(height);
		PoVBlock block=iter->second;

		future_blocks.erase(iter);
		rapidjson::Document& data=block.getBlock();
		try
		{
			if(!validateBlock(data))
			{
				data.SetNull();
				data.GetAllocator().Clear();
				delete &data;
				future_blocks_mutex.unlock();
				return ;
			}
//				else
//				{
//					std::cout<<"发布区块消息处理中验证正式区块通过\n";
//				}
		}
		catch(...)
		{
			std::cout<<"PublishBlock中验证区块异常n";
			data.SetNull();
			data.GetAllocator().Clear();
			delete &data;
			future_blocks_mutex.unlock();
			throw "";
		}
		std::cout<<network->getNodeId()<<"把高度为"<<height<<"的区块加入到区块链中!\n";
		//输出hash
		rapidjson::Document& h=block.getHeader();
		std::string true_hash=key_manager.getHash256(h);
		//std::cout<<"hash="<<true_hash<<"\n";
		//std::cout<<"高度为"<<height<<"的区块头为：\n";
		//print_document(h);
		h.SetNull();
		h.GetAllocator().Clear();
		delete &h;

		blockchain.pushbackBlockToDatabase(block);
		try
		{
			updateVariables(data);
		}
		catch(...)
		{
			std::cout<<"接收到新区快更新变量异常\n";
			data.SetNull();
			data.GetAllocator().Clear();
			delete &data;
			future_blocks_mutex.unlock();
			throw "";
		}
		data.SetNull();
		data.GetAllocator().Clear();
		delete &data;
		if(height==end_height+1)
		{
			statisticData();
			exit(0);
		}
	}
//	if(iter!=future_blocks.end())
//	{
//
//
//	}
	future_blocks_mutex.unlock();
}
/*
void POV::setDatabase(mongocxx::client &client)
{
	this->client=client;
}
*/
/*
rapidjson::Document& POV::generateBallot(std::vector<account> &butler_candidate_list)
{
	rapidjson::Document& ballot=*(new rapidjson::Document);
	ballot.SetObject();
	rapidjson::Document::AllocatorType& allocator=ballot.GetAllocator();
	//排序
	std::sort(butler_candidate_list.begin(),butler_candidate_list.end(),[](account x,account y){
		return x.getScore()>y.getScore();
	});
	//把最高分的成员pubkey加入选票列表中
	rapidjson::Value &selected_member=*(new rapidjson::Value(rapidjson::kArrayType));
	for(uint32_t i=0;i<vote_amout;i++)
	{
		std::string pubkey=butler_candidate_list.at(i).getPubKey();
		rapidjson::Value &member=*(new rapidjson::Value(rapidjson::kObjectType));
		member.SetObject();
		member.SetString(pubkey.c_str(),pubkey.size());
		selected_member.PushBack(member,allocator);
	}
	ballot.AddMember("list",selected_member,allocator);
	//这里的pubkey是从key_manager中获取的，可以获取pubkey的方法太多，以后需要统一改为使用某一固定方法。
	std::string str_voter=key_manager.getPublicKey();
	rapidjson::Value &voter=*(new rapidjson::Value(str_voter.c_str(),str_voter.size(),allocator));
	ballot.AddMember("voter",voter,allocator);
	return ballot;
}
*/

