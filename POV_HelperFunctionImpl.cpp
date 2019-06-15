#include "POV.h"
#include <fstream>

//收集投票数据发送到服务器
void POV::collectVoteData()
{
	//收集签名和错误消息
	//std::cout<<"collectVoteData:1\n";
	rapidjson::Document& txs=cache_block->getTransactionsList();
	//std::cout<<"collectVoteData:2\n";
	int txs_num=txs.Size();
	int height=cache_block->getHeight();
//	for(int i=0;i<raw_block_signatures.size();i++)
//	{
//		std::string pubkey=raw_block_signatures[i].pubkey;
//		int agreement=1;
//		//int txs_num=cache_block->getTransactionsAmout();
//		std::string type="";
//		if(txs_num>0)
//			type=getNDNType(txs[0]);
//		collector.collectVoteData(pubkey,height,agreement,txs_num,type);
//	}
//	for(int i=0;i<raw_block_error.size();i++)
//	{
//		std::string pubkey=raw_block_error[i].pubkey;
//		int agreement=-1;
//		std::string type="";
//		if(txs_num>0)
//			type=getNDNType(txs[0]);
//		collector.collectVoteData(pubkey,height,agreement,txs_num,type);
//	}
	int com_num=account_manager.getCommissionerAmount();
	for(int i=0;i<com_num;i++)
	{
		account com=account_manager.getCommissioner(i);
		std::string pubkey=com.getPubKey();
		bool is_set=false;
		for(int j=0;j<raw_block_signatures.size();j++)
		{
			if(raw_block_signatures[j].pubkey==pubkey)
			{
				//std::string pubkey=raw_block_signatures[i].pubkey;
				int agreement=1;
				//int txs_num=cache_block->getTransactionsAmout();
				std::string type="";
				if(txs_num>0)
					type=getNDNType(txs[0]);
				collector.collectVoteData(pubkey,height,agreement,txs_num,type);
				is_set=true;
				break;
			}
		}
		if(is_set)
			continue;
		rbe_mutex.lock();
		for(int j=0;j<raw_block_error.size();j++)
		{
			if(raw_block_error[j].pubkey==pubkey)
			{
				//std::string pubkey=raw_block_error[i].pubkey;
				int agreement=-1;
				std::string type="";
				if(txs_num>0)
					type=getNDNType(txs[0]);
				collector.collectVoteData(pubkey,height,agreement,txs_num,type);
				is_set=true;
				break;
			}
		}
		rbe_mutex.unlock();
		if(is_set)
			continue;
		int agreement=0;
		std::string type="";
		if(txs_num>0)
			type=getNDNType(txs[0]);
		collector.collectVoteData(pubkey,height,agreement,txs_num,type);
	}
	delete &txs;
	collector.sendVoteMessage();
}
//统计数据
void POV::statisticData()
{
	//统计数据
	std::cout<<network->getNodeId()<<"节点结束运行!\n";
	StatContainer *staters=new StatContainer[end_height];
	for(uint32_t i=0;i<end_height;i++)
	{
		PoVBlock block=blockchain.getBlock(i);
		//区块高度
		staters[i].block_num=block.getHeight();
		//生成时间
		staters[i].Te=block.getTe();
		//获取区块大小
		rapidjson::Document& json_block=block.getBlock();
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		json_block.Accept(writer);
		staters[i].blocksize=buffer.GetSize();
		delete& json_block;
		//交易总数
		staters[i].tx_amout=block.getTransactionsAmout();
		//普通交易数
		std::vector<PoVTransaction> norm_txs;
		block.getNormalTransactions(norm_txs);
		staters[i].normal_tx_amout=norm_txs.size();
		//所有交易的时间戳
		staters[i].norm_txs_timestamp=new double[norm_txs.size()];
		for(uint32_t j=0;j<norm_txs.size();j++)
		{
			rapidjson::Document& tx=norm_txs.at(j).getData();
			//print_document(tx);
			staters[i].norm_txs_timestamp[j]=tx["timestamp"].GetDouble();
			delete &tx;
		}
		staters[i].cycles=block.getCycles();
	}
	std::cout<<"统计完成\n";
	//区块数据统计
	std::ofstream block_data;
	block_data.open("data/block_data.txt");
	block_data<<"区块高度, ";
	block_data<<"区块大小, ";
	block_data<<"生成时间, ";
	block_data<<"交易数量, ";
	block_data<<"正常交易数量, ";
	block_data<<"周期数, ";
	block_data<<"平均正常交易生成速率, ";
	block_data<<"每个区块交易打包速率;\n";
	double last_Te=0;
	double first_block_Te=staters[0].Te;
	uint32_t time_norm_txs_amout=0;
	for(uint32_t i=0;i<end_height;i++)
	{
		uint32_t num=staters[i].block_num;
		uint32_t size=staters[i].blocksize;
		double Te=staters[i].Te;
		uint32_t tx_amout=staters[i].tx_amout;
		uint32_t normal_tx_amout=staters[i].normal_tx_amout;
		uint32_t cycles=staters[i].cycles;
		block_data<<num<<", ";
		block_data<<size<<", ";
		//std::string Te(staters[i].Te,3);
		block_data<<std::to_string(Te)<<", ";
		block_data<<tx_amout<<", ";
		block_data<<normal_tx_amout<<", ";
		block_data<<cycles<<", ";
		time_norm_txs_amout=time_norm_txs_amout+normal_tx_amout;
		block_data<<std::to_string(time_norm_txs_amout/(Te-first_block_Te))<<", ";
		block_data<<std::to_string(normal_tx_amout/(Te-last_Te))<<";\n";
		last_Te=Te;
	}
	block_data.close();
	double total_defer_time=0;
	uint32_t total_normal_txs=0;
	std::cout<<"区块统计数据写入文件完成\n";
	//交易数据统计
	std::cout<<"locate: 1\n";
	std::ofstream transaction_data;
	std::cout<<"locate: 2\n";
	transaction_data.open("data/transaction_data.txt");
	std::cout<<"locate: 3\n";
	transaction_data<<"交易生成时间, ";
	std::cout<<"locate: 4\n";
	transaction_data<<"所在区块, ";
	std::cout<<"locate: 5\n";
	transaction_data<<"所在区块普通交易数, ";
	std::cout<<"locate: 6\n";
	transaction_data<<"所在区块生成时间;\n";
	std::cout<<"locate: 7\n";
	for(uint32_t i=0;i<end_height;i++)
	{
		//std::string Te(staters[i].Te,3);
		std::cout<<"第"<<i<<"区块交易数："<<staters[i].normal_tx_amout<<"\n";
		total_normal_txs+=staters[i].normal_tx_amout;
		for(uint32_t j=0;j<staters[i].normal_tx_amout;j++)
		{
			double Te=staters[i].Te;
			double norm_tx_timestamp=staters[i].norm_txs_timestamp[j];
			uint32_t num=staters[i].block_num;
			uint32_t amout=staters[i].normal_tx_amout;
			//std::cout<<staters[i].norm_txs_timestamp[j]<<",";
			//std::string tx_Te(staters[i].norm_txs_timestamp[j],3);
			transaction_data<<std::to_string(norm_tx_timestamp)<<", ";
			transaction_data<<num<<", ";
			transaction_data<<amout<<", ";
			transaction_data<<std::to_string(Te)<<";\n";
			total_defer_time=total_defer_time+Te-norm_tx_timestamp;
		}
		//std::cout<<"\n";
	}
	double T_interval=staters[end_height-1].Te-staters[1].Te;
	transaction_data.close();
	std::cout<<"交易统计数据写入文件完成\n";
	//记录系统设置参数
	std::ofstream settings;
	settings.open("data/settings.txt");
	settings<<"程序开始时间："<<network->getStartTime()<<";\n";
	settings<<"程序运行间隔："<<network->getInterval()<<";\n";
	settings<<"正常交易缓存池的容量大小："<<NormalPoolCapacity<<";\n";
	settings<<"区块最大普通交易数量："<<max_normal_trans_per_block<<";\n";
	settings<<"每个节点产生交易的概率："<<prob_generate_normal_tx<<";\n";
	settings<<"每个管家开始生成区块前等待的时间："<<defer_time<<";\n";
	settings<<"代理委员编号："<<agent_commissioner_num<<";\n";
	settings<<"委员数量："<<init_commissioner_size<<";\n";
	settings<<"委员名单：{";
	for(uint32_t i=0;i<init_commissioner_size;i++)
	{
		settings<<Initial_Commissioner_Nodes[i]<<", ";
	}
	settings<<"};\n";
	settings<<"管家候选数量："<<init_butler_candidate_size<<";\n";
	settings<<"管家候选名单：{";
	for(uint32_t i=0;i<init_butler_candidate_size;i++)
	{
		settings<<Initial_Butler_Candidate_Nodes[i]<<", ";
	}
	settings<<"};\n";
	settings<<"管家人数："<<butler_amount<<";\n";
	settings<<"投票人数："<<vote_amount<<";\n";
	settings<<"每个任职周期产生的区块数："<<num_blocks<<";\n";
	settings<<"每个区块产生的截止时间："<<Tcut<<";\n";
	settings<<"普通交易内容长度："<<tx_len<<";\n";
	settings<<"程序结束高度："<<end_height<<";\n";
	settings<<"平均交易时延："<<std::to_string(total_defer_time/total_normal_txs)<<";\n";
	std::cout<<"平均交易时延："<<std::to_string(total_defer_time/total_normal_txs)<<";\n";
	settings<<"TPS："<<std::to_string((total_normal_txs-staters[1].normal_tx_amout)/T_interval)<<";\n";
	std::cout<<"TPS："<<std::to_string((total_normal_txs-staters[1].normal_tx_amout)/T_interval)<<";\n";
	std::time_t now_time=time(NULL);
	std::time_t running_time=now_time-network->getSystemStartTime();
	settings<<"运行时间："<<running_time<<";\n";
	settings.close();
	std::cout<<"设置数据写入文件完成;\n";
	std::vector<int> MSGStatData=msg_manager.getMSGStatData();
	std::cout<<"消息产生统计：\n";
	for(int i=0;i<MSGStatData.size();i++)
	{
		std::cout<<i<<": "<<MSGStatData[i]<<"\n";
	}
}
