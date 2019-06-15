#include "POV.h"


//验证区块
bool POV::validateBlock(rapidjson::Value &data)
{

	//验证区块是否合法
//	if(!validateRawBlock(data))
//	{
//		std::cout<<"区块验证不通过\n";
//		return false;
//	}
//	else
//	{
//		std::cout<<"正式区块验证通过\n";
//	}

	rapidjson::Value& header=data["header"];
	rapidjson::Value& transactions=data["transactions"];
	std::string merkle_root=header["merkle_root"].GetString();
	std::string root=key_manager.getHash256(transactions);
	if(merkle_root!=root)
	{
		std::cout<<"验证区块头错误——交易哈希错误\n";
		return false;
	}
	PoVBlock block;
	block.setBlock(data);
	uint32_t height=block.getHeight();

	//验证区块头签名
	rapidjson::Document& sigs=block.getSignatures();
	rapidjson::Document& raw_header=block.getRawHeader();
	if(sigs.Size()<=account_manager.getCommissionerAmount()/2.0)
	{
		std::cout<<"验证区块头错误——区块签名没有超过委员数量的一半！\n";
		sigs.SetNull();
		sigs.GetAllocator().Clear();
		delete &sigs;
		raw_header.SetNull();
		raw_header.GetAllocator().Clear();
		delete &raw_header;
		return false;
	}
	for(uint32_t i=0;i<sigs.Size();i++)
	{
		rapidjson::Value& sig=sigs[i];
		if(!validateBlockSignature(raw_header,sig))
		{
			std::cout<<"验证区块头错误——签名错误!\n";
			sigs.SetNull();
			sigs.GetAllocator().Clear();
			delete &sigs;
			raw_header.SetNull();
			raw_header.GetAllocator().Clear();
			delete &raw_header;
			return false;
		}
	}
	//验证下一任管家
	uint32_t next_butler=getNextButler(sigs);
	if(block.getNextButler()!=next_butler)
	{
		std::cout<<"验证区块头错误——下一任管家错误!\n";
		sigs.SetNull();
		sigs.GetAllocator().Clear();
		delete &sigs;
		raw_header.SetNull();
		raw_header.GetAllocator().Clear();
		delete &raw_header;
		return false;
	}
	//验证区块截止时间
	double Te=getTe(sigs);
	sigs.SetNull();
	sigs.GetAllocator().Clear();
	delete &sigs;
	raw_header.SetNull();
	raw_header.GetAllocator().Clear();
	delete &raw_header;
	if(block.getTe()!=Te)
	{
		std::cout<<"验证区块头错误——Te错误!\n";
		return false;
	}
	return true;
}
//验证区块签名
bool POV::validateBlockSignature(rapidjson::Document& header,rapidjson::Value &data)
{
	//std::cout<<"header info:\n";
	//print_document(header);
	//std::cout<<"data info:\n";
	//print_document(data);
	std::string pubkey=data["pubkey"].GetString();
	std::string sig=data["sig"].GetString();
	//double timestamp=data["timestamp"].GetDouble();
	//header["Te"].SetDouble(timestamp);
	if(!key_manager.verifyDocument(header,pubkey,sig))
	{
		std::cout<<network->getNodeId()<<"验证签名不通过！\n";
		std::cout<<"pubkey:"<<pubkey<<"\n";
		std::cout<<"sig:"<<sig<<"\n";
		std::cout<<"raw header:\n";
		print_document(header);
		return false;
	}
	//std::cout<<"received pubkey:"<<pubkey<<"\n";
	//std::string sig=data["sig"].GetString();
	//判断是否是委员
	if(!account_manager.is_commissioner(pubkey))
	{
		std::cout<<" not commissioner\n";
		return false;
	}
	return true;
}
//验证预区块
bool POV::validateRawBlock(rapidjson::Value &data)
{
	if(state==generate_first_block)
		return true;
	if(vote_mode=="prob")
	{
		double rand=getRandomNum();
		if(rand>passed_prob)
		{
			return false;
		}
	}
	//print_document(data);
	//std::cout<<"validateRawBlock中：1\n";
	rapidjson::Value& header=data["header"];
	rapidjson::Value& transactions=data["transactions"];
	if(!transactions.IsArray())
	{
		std::cout<<"验证交易错误——transactions不是array！\n";
		print_document(transactions);
	}
	//std::cout<<"validateRawBlock中：2\n";
	//验证交易是否正确
	//std::cout<<"交易数量："<<transactions.Size()<<"\n";
	for(uint32_t i=0;i<transactions.Size();i++)
	{
		rapidjson::Value& tx=transactions[i];
		std::string metatype=tx["metatype"].GetString();
		if(metatype=="Constants")
		{

		}
		else if(metatype=="ApplyCommissioner")
		{
			//std::cout<<"validateRawBlock中：21\n";
			if(!validateApplyCommissionerTransaction(tx))
			{
				std::cout<<"验证交易错误——申请委员交易错误！\n";
				return false;
			}
		}
		else if(metatype=="vote")
		{
			//std::cout<<"validateRawBlock中：22\n";
			if(!validateVoteTransaction(tx))
			{
				std::cout<<"验证交易错误——投票交易错误！\n";
				return false;
			}
		}
		else if(metatype=="ApplyButlerCandidate")
		{
			//std::cout<<"validateRawBlock中：23\n";
			if(!validateApplyButlerCandidateData(tx))
			{
				std::cout<<"验证交易错误——申请管家候选交易错误！\n";
				return false;
			}
		}
		else if(metatype=="Normal")
		{
			//do some validation by commissioner
			if(!validateNormalTransaction(tx))
				return false;
		}
	}
	//std::cout<<"validateRawBlock中：3\n";
	//验证区块头信息是否正确
	uint32_t height=header["height"].GetInt();
	uint32_t num_of_trans=header["num_of_trans"].GetInt();
	std::string generator=header["generator"].GetString();
	uint32_t cycles=header["cycles"].GetInt();
	std::string merkle_root=header["merkle_root"].GetString();
	std::string hash=header["previous_hash"].GetString();
	int myheight=blockchain.getHeight();
	//std::cout<<"validateRawBlock中：4\n";
//	if(height!=myheight+1)
//	{
//		std::cout<<"验证区块头错误——高度错误！\n";
//		return false;
//	}
	//std::cout<<"验证区块头:my height="<<myheight<<"\n";
	if(myheight>0)
	{
		PoVBlock b=blockchain.getBlock(myheight);
		rapidjson::Document& h=b.getHeader();
		std::string true_hash=key_manager.getHash256(h);
		h.SetNull();
		h.GetAllocator().Clear();
		delete &h;
		if(true_hash!=hash)
		{
			std::cout<<"验证区块头错误——前一个区块的hash错误！\n";
			std::cout<<"myheight="<<myheight<<"\n";
			std::cout<<"true_hash="<<true_hash<<"\n";
			std::cout<<"hash="<<hash<<"\n";

			return false;
		}
	}
	//std::cout<<"validateRawBlock中：5\n";
	if(num_of_trans!=transactions.Size())
	{
		std::cout<<"验证区块头错误——交易数错误！\n";
		return false;
	}
	//std::cout<<"validateRawBlock中：6\n";
	if(state==generate_first_block)
	{
		//NodeID AgentCommissioner=Initial_Commissioner_Nodes[agent_commissioner_num];
		if(generator!=account_manager.getCommissionerByNodeID(AgentCommissioner).getPubKey())
		{
			std::cout<<"验证区块头错误——公钥错误！\n";
			return false;
		}
		if(cycles!=0)
		{
			std::cout<<"验证区块头错误——周期数错误！\n";
			return false;
		}
	}
	else
	{

	}
	//std::cout<<"validateRawBlock中：7\n";
	std::string root=key_manager.getHash256(transactions);
	if(merkle_root!=root)
	{
		return false;
	}
	//std::cout<<"validateRawBlock中：8\n";
	return true;
}
//验证投票交易
bool POV::validateVoteTransaction(rapidjson::Value &data)
{
	std::string metatype=data["metatype"].GetString();
	rapidjson::Value& metadata=data["metadata"];
	rapidjson::Value& result=metadata["result"];
	rapidjson::Value& ballots=metadata["ballots"];
	if(state==generate_first_block)
	{
		if(ballots.Size()!=account_manager.getCommissionerAmount())
		{
			std::cout<<"验证投票交易错误——创世区块阶段投票数错误！\n";
		}
	}
	else
	{
		if(ballots.Size()<account_manager.getCommissionerAmount()/2)
		{
			std::cout<<"验证投票交易错误——创世区块阶段投票数错误！\n";
		}
	}
	//验证ballot是否正确
	std::vector<DocumentContainer> ballot_containers;
	for(uint32_t i=0;i<ballots.Size();i++)
	{
		rapidjson::Value& ballot=ballots[i];
		if(!validateBallot(ballot))
		{
			std::cout<<"验证投票交易错误——投票错误！\n";
			return false;
		}
		DocumentContainer container;
		container.saveDocument(ballot);
		ballot_containers.push_back(container);
	}
	rapidjson::Document& stat_result=BallotStatistic(ballot_containers);
	for(uint32_t i=0;i<result.Size();i++)
	{
		bool equal=false;
		for(uint32_t j=0;j<stat_result.Size();j++)
		{
			if(result[i].GetString()!=stat_result[i].GetString())
			{
				equal=true;
				break;
			}
		}

		if(!equal)
		{
			std::cout<<"验证投票交易错误——投票结果错误！\n";
			stat_result.SetNull();
			stat_result.GetAllocator().Clear();
			delete &stat_result;
			return false;
		}
	}
	stat_result.SetNull();
	stat_result.GetAllocator().Clear();
	delete &stat_result;
	return true;
}
//验证投票结果
bool POV::validateBallot(rapidjson::Value &data)
{
	std::string metatype=data["metatype"].GetString();
	rapidjson::Value& metadata=data["metadata"];
	std::string sig=data["sig"].GetString();
	std::string pubkey=metadata["voter"].GetString();
	rapidjson::Value& vote_list=metadata["vote_list"];
	//检查metatype是否Ballot
	if(metatype!="Ballot")
	{
		std::cout<<"验证投票——metatype错误！\n";
		return false;
	}
	//检查投票人是否委员
	if(!account_manager.is_commissioner(pubkey))
	{
		std::cout<<"验证投票——投票人不是委员错误！\n";
		return false;
	}
	//检查投票人数是否正确
	if(vote_list.Size()!=vote_amount)
	{
		std::cout<<"验证投票——投票人数不等于规定的人数错误！\n";
		return false;
	}
	//检查被投票的公钥是否管家候选
	if(state!=generate_first_block)
	{
		for(rapidjson::SizeType i=0;i<vote_list.Size();i++)
		{
			std::string voted_pubkey=vote_list[i].GetString();
			if(!account_manager.is_butler_candidate(voted_pubkey))
			{
				std::cout<<"验证投票——被投票的公钥不是管家候选错误！\n";
				return false;
			}
		}
	}

	//检查签名是否正确
	if(!key_manager.verifyDocument(metadata,pubkey,sig))
	{
		std::cout<<"验证投票——签名错误！\n";
		return false;
	}
	return true;
}
//验证申请成为委员的交易
bool POV::validateApplyCommissionerTransaction(rapidjson::Value &data)
{
	//rapidjson::Value &data=d["data"];
	if(!data.IsObject())
	{
		std::cout<<"验证委员交易——data不是object\n";
		print_document(data);
	}
	rapidjson::Value &metadata=data["metadata"];
	rapidjson::Value &sigs=data["sigs"];
	//检查metatype是否正确
	std::string first_metatype=data["metatype"].GetString();
	if(first_metatype!="ApplyCommissioner")
	{
		std::cout<<"验证委员交易——第一个metatype错误!\n";
		return false;
	}
	if(!metadata.IsObject())
	{
		std::cout<<"验证委员交易——metadata不是object\n";
		print_document(metadata);
	}
	std::string metatype=metadata["metatype"].GetString();
	if(metatype!="ApplyCommissioner")
	{
		std::cout<<"验证委员交易——第二个metatype错误!\n";
		return false;
	}
	//检验是否pubkey是否委员,在生成创世区块的阶段，只有委员才能申请成为委员，在其他阶段委员不能申请成为委员
	if(state==generate_first_block)
	{
		std::string com_pubkey=metadata["pubkey"].GetString();
		if(!account_manager.is_commissioner(com_pubkey))
		{
			std::cout<<"验证委员交易——创世区块阶段不是委员错误!\n";
			return false;
		}
	}
	else
	{
		std::string com_pubkey=metadata["pubkey"].GetString();
		if(account_manager.is_commissioner(com_pubkey))
		{
			std::cout<<"验证委员交易——正常阶段已经是委员错误!\n";
			return false;
		}
	}
	//检验时间——仿真不检验
	//校验签名是否正确
	if(!sigs.IsArray())
	{
		std::cout<<"验证委员交易——签名不是数组错误!\n";
		return false;
	}
	for(rapidjson::SizeType i=0;i<sigs.Size();i++)
	{
		rapidjson::Value &sig=sigs[i];
		if(sig.IsObject() and sig.HasMember("pubkey") and sig.HasMember("sig"))
		{
			std::string pubkey=sig["pubkey"].GetString();
			std::string str_sig=sig["sig"].GetString();
			if(!account_manager.is_commissioner(pubkey))
			{
				std::cout<<"验证委员交易——签名人不是委员错误!\n";
				return false;
			}
			if(!key_manager.verifyDocument(metadata,pubkey,str_sig))
			{
				std::cout<<"验证委员交易——签名不正确错误!\n";
				return false;
			}
		}
		else
		{
			std::cout<<"验证委员交易——缺少pubkey或者sig错误\n";
			return false;
		}
	}
	return true;
}
//验证推荐信
bool POV::validateRecommendationLetter(rapidjson::Value &data)
{
	//rapidjson::Value &data=d["data"];
	rapidjson::Value &metadata=data["metadata"];
	std::string sig=data["sig"].GetString();
	std::string metatype=metadata["metatype"].GetString();
	std::string refferal=metadata["refferal"].GetString();
	std::string refferer=metadata["refferer"].GetString();
	double timestamp=metadata["timestamp"].GetDouble();
	//检查metatype是否正确
	if(metatype!="RecommendationLetter")
	{
		std::cout<<"验证推荐信错误——推荐信类型错误！\n";
		return false;
	}
	//判断refferal是否已经是管家
	if(state!=generate_first_block)
	{
		if(account_manager.is_butler_candidate(refferal))
		{
			std::cout<<"验证推荐信错误——被推荐人已经是管家候选错误！\n";
			return false;
		}
	}

	//判断refferer是否委员
	if(!account_manager.is_commissioner(refferer))
	{
		std::cout<<"验证推荐信错误——推荐人不是委员！\n";
		return false;
	}
	//判断时间是否合法，这个暂时不写
	//校验签名是否正确
	if(!key_manager.verifyDocument(metadata,refferer,sig))
	{
		std::cout<<"验证推荐信错误——签名错误！\n";
		return false;
	}
	return true;
}
//验证申请成为管家候选的签名
bool POV::validateApplyButlerCandidateSignature(rapidjson::Value &letter,rapidjson::Value &data)
{
	//rapidjson::Value &data=d["data"];
	//print_document(data);
	std::string pubkey=data["pubkey"].GetString();
	std::string sig=data["sig"].GetString();
	if(!account_manager.is_commissioner(pubkey))
	{
		std::cout<<"验证申请管家候选的签名——不是委员！\n";
		return false;
	}
	if(!key_manager.verifyDocument(letter,pubkey,sig))
	{
		std::cout<<"验证申请管家候选的签名——签名错误！\n";
		std::cout<<"pubkey="<<pubkey<<"\n";
		std::cout<<"sig="<<sig<<"\n";
		std::cout<<"letter:\n";
		print_document(letter);
		return false;
	}
	return true;
}
//验证申请成为管家候选数据
bool POV::validateApplyButlerCandidateData(rapidjson::Value &data)
{
	if(!data.IsObject())
	{
		std::cout<<"验证申请管家候选交易——data不是Object错误！\n";
		print_document(data);
	}
	std::string metatype=data["metatype"].GetString();
	rapidjson::Value& letter=data["recommendation_letter"];
	rapidjson::Value& sigs=data["sigs"];
	if(metatype!="ApplyButlerCandidate")
	{
		std::cout<<"验证申请管家候选交易——消息类型错误！\n";
		return false;
	}
	if(!letter.IsObject())
	{
		std::cout<<"验证申请管家候选交易——letter不是Object错误！\n";
		print_document(letter);
		return false;
	}
	if(!validateRecommendationLetter(letter))
	{
		std::cout<<"验证申请管家候选交易——验证推荐信错误！\n";
		return false;
	}
	if(!sigs.IsArray() or sigs.Empty())
	{
		std::cout<<"验证申请管家候选交易——sigs不是数组或者为空！\n";
		return false;
	}
	for(rapidjson::SizeType i=0;i<sigs.Size();i++)
	{
		rapidjson::Value& sig=sigs[i];
		if(!validateApplyButlerCandidateSignature(letter,sig))
		{
			std::cout<<"验证申请管家候选交易——验证签名错误！\n";
			return false;
		}
	}
	return true;
}
//验证退出管家候选交易
bool POV::validateQuitButlerCandidateTransaction(rapidjson::Value &data)
{
	std::string metatype=data["metatype"].GetString();
	rapidjson::Value& metadata=data["metadata"];
	std::string sig=data["sig"].GetString();
	std::string type=metadata["type"].GetString();
	std::string pubkey=metadata["pubkey"].GetString();
	double timestamp=metadata["timestamp"].GetDouble();

	if(metatype!="QuitButlerCandidate")
	{
		std::cout<<"验证退出管家候选交易错误——metatype错误！\n";
		return false;
	}
	if(type!="QuitButlerCandidateMetadata")
	{
		std::cout<<"验证退出管家候选交易错误——type错误！\n";
		return false;
	}
	if(!account_manager.is_butler_candidate(pubkey))
	{
		std::cout<<"验证退出管家候选交易错误——pubkey错误！\n";
		return false;
	}
	if(!key_manager.verifyDocument(metadata,pubkey,sig))
	{
		std::cout<<"验证退出管家候选交易错误——签名错误！\n";
		return false;
	}
	return true;
}
//验证退出委员交易
bool POV::validateQuitCommissionerTransaction(rapidjson::Value &data)
{
	return true;
}
//验证普通交易
bool POV::validateNormalTransaction(rapidjson::Value &tx)
{
	if(tx.HasMember("content")&&tx["content"].IsObject())
	{
		rapidjson::Value& content=tx["content"];
		if(content.HasMember("type")&& content["type"].IsString())
		{
			std::string type=content["type"].GetString();
			if(type=="NDN-IP")
			{
				std::cout<<"验证NDN-IP数据:\n";
				//print_document(content);
				if(!validateNDNTransaction(content))
				{
					std::cout<<"验证NDN交易失败\n";
					return false;
				}
			}
		}
	}
	return true;
}
//验证NDN交易
bool POV::validateNDNTransaction(rapidjson::Value &data)
{
	if(!data.HasMember("data") || !data["data"].HasMember("command"))
		return false;
	rapidjson::Value& ndn_data=data["data"];
	std::string command=ndn_data["command"].GetString();

	if(command=="Generate")
	{
		if(!data.HasMember("sig"))
		{
			std::cout<<"生成标识请求数据不完整，缺少sig\n";
			return false;
		}
		if(!(ndn_data.HasMember("NDN")
				&&ndn_data.HasMember("IP")
				&&ndn_data.HasMember("pubkey")
				&&ndn_data.HasMember("timestamp")
				&&ndn_data.HasMember("hash")
				&&ndn_data.HasMember("other")))
		{
			std::cout<<"生成标识请求数据不完整\n";
			return false;
		}
		std::string sig=data["sig"].GetString();
		std::string ndn=ndn_data["NDN"].GetString();
		std::string IP=ndn_data["IP"].GetString();
		std::string pubkey=ndn_data["pubkey"].GetString();
		double timestamp=ndn_data["timestamp"].GetDouble();
//		//验证签名
//		if(!key_manager.verifyDocument(ndn_data,pubkey,sig))
//		{
//			std::cout<<"生成标识请求验证签名失败:\n";
//			print_document(ndn_data);
//			return false;
//		}
		//检查该用户是否已经注册
		if(!blockchain.hasData("pubkey",pubkey,"NDN-USER"))
		{
			std::cout<<"生成标识请求没有该用户\n";
			return false;
		}
		//检查ndn前缀是否正确
		rapidjson::Document& old_data=blockchain.getDataFromDatabase("pubkey",pubkey,"NDN-USER",false);
		std::string ndn_prefix=old_data["prefix"].GetString();
		if(ndn_prefix.size()>=ndn.size())
		{
			std::cout<<"生成标识请求ndn前缀不正确\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		int len=ndn_prefix.size();
		for(int i=0;i<len;i++)
		{
			if(ndn_prefix[i]!=ndn[i])
			{
				std::cout<<"生成标识请求ndn前缀不正确\n";
				old_data.SetNull();
				old_data.GetAllocator().Clear();
				delete &old_data;
				return false;
			}
		}
		//检查是否已存在该NDN标识
		if(blockchain.hasData("NDN",ndn,"NDN"))
		{
			std::cout<<"validateNDNTransaction中已经存在该NDN标识:\n"<<ndn<<"\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		//关键字检查
		int filters_len=filter_list.size();
		for(int i=0;i<filters_len;i++)
		{
			if(ndn.find(filter_list[i])!=std::string::npos)
			{
				std::cout<<"生成标识错误：ndn标识中包含不合法关键词"<<filter_list[i]<<"\n";
				old_data.SetNull();
				old_data.GetAllocator().Clear();
				delete &old_data;
				return false;
			}
		}
		old_data.SetNull();
		old_data.GetAllocator().Clear();
		delete &old_data;
		return true;
	}
	else if(command=="Update")
	{
		if(!data.HasMember("sig"))
		{
			std::cout<<"更新标识请求数据不完整，缺少sig\n";
			return false;
		}
		if(!(ndn_data.HasMember("NDN")
				&&ndn_data.HasMember("IP")
				&&ndn_data.HasMember("timestamp")
				&&ndn_data.HasMember("hash")))
		{
			std::cout<<"更新标识请求数据不完整\n";
			return false;
		}

		std::string ndn=ndn_data["NDN"].GetString();
		std::string IP=ndn_data["IP"].GetString();
		double timestamp=ndn_data["timestamp"].GetDouble();
		std::string sig=data["sig"].GetString();
		//检查是否已存在该NDN标识
		if(!blockchain.hasData("NDN",ndn,"NDN"))
		{
			std::cout<<"validateNDNTransaction中不存在该NDN标识:\n"<<ndn<<"\n";
			return false;
		}
		//验证签名
		rapidjson::Document& old_data=blockchain.getDataFromDatabase("NDN",ndn,"NDN",false);
		std::string pubkey=old_data["pubkey"].GetString();
		if(!key_manager.verifyDocument(ndn_data,pubkey,sig))
		{
			std::cout<<"更新标识请求验证签名失败\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		//检查该用户是否已经注册,正常情况下不会发生没有该用户的情况
		if(!blockchain.hasData("pubkey",pubkey,"NDN-USER"))
		{
			std::cout<<"更新标识请求没有该用户\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		rapidjson::Document& user_data=blockchain.getDataFromDatabase("pubkey",pubkey,"NDN-USER",false);
		std::string ndn_prefix=user_data["prefix"].GetString();
		//检查ndn前缀是否正确

		if(ndn_prefix.size()>=ndn.size())
		{
			std::cout<<"更新标识请求ndn前缀不正确\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			user_data.SetNull();
			user_data.GetAllocator().Clear();
			delete &user_data;
			return false;
		}
		int len=ndn_prefix.size();
		for(int i=0;i<len;i++)
		{
			if(ndn_prefix[i]!=ndn[i])
			{
				std::cout<<"更新标识请求ndn前缀不正确\n";
				old_data.SetNull();
				old_data.GetAllocator().Clear();
				delete &old_data;
				user_data.SetNull();
				user_data.GetAllocator().Clear();
				delete &user_data;
				return false;
			}
		}

		old_data.SetNull();
		old_data.GetAllocator().Clear();
		delete &old_data;
		user_data.SetNull();
		user_data.GetAllocator().Clear();
		delete &user_data;
		return true;
	}
	else if(command=="Delete")
	{
		if(!data.HasMember("sig"))
		{
			std::cout<<"删除标识请求数据不完整，缺少sig\n";
			return false;
		}
		if(!(ndn_data.HasMember("NDN")
				&&ndn_data.HasMember("timestamp")))
		{
			std::cout<<"删除标识请求数据不完整\n";
			return false;
		}
		std::string ndn=ndn_data["NDN"].GetString();
		double timestamp=ndn_data["timestamp"].GetDouble();
		std::string sig=data["sig"].GetString();
		//检查是否已存在该NDN标识
		if(!blockchain.hasData("NDN",ndn,"NDN"))
		{
			std::cout<<"validateNDNTransaction中不存在该NDN标识：\n"<<ndn<<"\n";
			return false;
		}
		//验证签名
		rapidjson::Document& old_data=blockchain.getDataFromDatabase("NDN",ndn,"NDN",false);
		std::string pubkey=old_data["pubkey"].GetString();
		if(!key_manager.verifyDocument(ndn_data,pubkey,sig))
		{
			std::cout<<"删除标识请求验证签名失败\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		old_data.SetNull();
		old_data.GetAllocator().Clear();
		delete &old_data;
		return true;
	}
	else if(command=="Registry")
	{
		if(!(ndn_data.HasMember("pubkey")
				&&ndn_data.HasMember("prefix")
				&&ndn_data.HasMember("level")
				&&ndn_data.HasMember("timestamp")
				&&ndn_data.HasMember("real_msg")))
		{
			std::cout<<"注册用户请求数据不完整\n";
			return false;
		}
		std::string sig=data["sig"].GetString();
		std::string pubkey=ndn_data["pubkey"].GetString();
		std::string prefix=ndn_data["prefix"].GetString();
		int level=ndn_data["level"].GetInt();
		double timestamp=ndn_data["timestamp"].GetDouble();
		//检查是否已存在该公钥的用户
		if(blockchain.hasData("pubkey",pubkey,"NDN-USER"))
		{
			std::cout<<"validateNDNTransaction——Registry中已经存在该pubkey:\n"<<pubkey<<"\n";
			return false;
		}
		//检查是否有用户已使用该前缀
		if(level>0)
		{
			if(blockchain.hasData("prefix",prefix,"NDN-USER"))
			{
				std::cout<<"validateNDNTransaction——Registry中已经存在该prefix:\n"<<prefix<<"\n";
				return false;
			}
		}
		//暂时对用户等级不做检查
		//验证签名
		//rapidjson::Document& old_data=blockchain.getDataFromDatabase("pubkey",pubkey,"NDN-USER",false);
		//std::string pubkey=old_data["pubkey"].GetString();
//		if(!key_manager.verifyDocument(ndn_data,pubkey,sig))
//		{
//			std::cout<<"注册用户请求验证签名失败\n";
//			return false;
//		}
		//关键字检查
		int len=filter_list.size();
		for(int i=0;i<len;i++)
		{
			if(prefix.find(filter_list[i])!=std::string::npos)
			{
				std::cout<<"注册用户错误：prefix标识中包含不合法关键词"<<filter_list[i]<<"\n";
				return false;
			}
		}
		//delete &old_data;
		return true;
	}
	else if(command=="Log")
	{
		if(!ndn_data.HasMember("QueryCode"))
			return false;
		return true;
	}
	else
	{
		return false;
	}
}
//管家验证NDN交易
bool POV::validateNDNTransactionButler(rapidjson::Value &data)
{
	if(!data.HasMember("data") || !data["data"].HasMember("command"))
		return false;
	rapidjson::Value& ndn_data=data["data"];
	std::string command=ndn_data["command"].GetString();

	if(command=="Generate")
	{
		if(!data.HasMember("sig"))
		{
			std::cout<<"生成标识请求数据不完整，缺少sig\n";
			return false;
		}
		if(!(ndn_data.HasMember("NDN")
				&&ndn_data.HasMember("IP")
				&&ndn_data.HasMember("pubkey")
				&&ndn_data.HasMember("timestamp")
				&&ndn_data.HasMember("hash")))
		{
			std::cout<<"生成标识请求数据不完整\n";
			return false;
		}
		std::string sig=data["sig"].GetString();
		std::string ndn=ndn_data["NDN"].GetString();
		std::string IP=ndn_data["IP"].GetString();
		std::string pubkey=ndn_data["pubkey"].GetString();
		double timestamp=ndn_data["timestamp"].GetDouble();
		//验证签名
		if(!key_manager.verifyDocument(ndn_data,pubkey,sig))
		{
			std::cout<<"生成标识请求验证签名失败:\n";
			print_document(ndn_data);
			return false;
		}
		//检查该用户是否已经注册
		if(!blockchain.hasData("pubkey",pubkey,"NDN-USER"))
		{
			std::cout<<"生成标识请求没有该用户\n";
			return false;
		}
		//检查ndn前缀是否正确
		rapidjson::Document& old_data=blockchain.getDataFromDatabase("pubkey",pubkey,"NDN-USER",false);
		std::string ndn_prefix=old_data["prefix"].GetString();
		if(ndn_prefix.size()>=ndn.size())
		{
			std::cout<<"生成标识请求ndn前缀不正确\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		int len=ndn_prefix.size();
		for(int i=0;i<len;i++)
		{
			if(ndn_prefix[i]!=ndn[i])
			{
				std::cout<<"生成标识请求ndn前缀不正确\n";
				old_data.SetNull();
				old_data.GetAllocator().Clear();
				delete &old_data;
				return false;
			}
		}
		//检查是否已存在该NDN标识
		if(blockchain.hasData("NDN",ndn,"NDN"))
		{
			std::cout<<"validateNDNTransaction中已经存在该NDN标识:\n"<<ndn<<"\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		//关键字检查
		int filters_len=butler_filter_list.size();
		for(int i=0;i<filters_len;i++)
		{
			if(ndn.find(butler_filter_list[i])!=std::string::npos)
			{
				std::cout<<"生成标识错误：ndn标识中包含不合法关键词"<<butler_filter_list[i]<<"\n";
				old_data.SetNull();
				old_data.GetAllocator().Clear();
				delete &old_data;
				return false;
			}
		}
		old_data.SetNull();
		old_data.GetAllocator().Clear();
		delete &old_data;
		return true;
	}
	else if(command=="Update")
	{
		if(!data.HasMember("sig"))
		{
			std::cout<<"更新标识请求数据不完整，缺少sig\n";
			return false;
		}
		if(!(ndn_data.HasMember("NDN")
				&&ndn_data.HasMember("IP")
				&&ndn_data.HasMember("timestamp")
				&&ndn_data.HasMember("hash")))
		{
			std::cout<<"更新标识请求数据不完整\n";
			return false;
		}

		std::string ndn=ndn_data["NDN"].GetString();
		std::string IP=ndn_data["IP"].GetString();
		double timestamp=ndn_data["timestamp"].GetDouble();
		std::string sig=data["sig"].GetString();
		//检查是否已存在该NDN标识
		if(!blockchain.hasData("NDN",ndn,"NDN"))
		{
			std::cout<<"validateNDNTransaction中不存在该NDN标识:\n"<<ndn<<"\n";
			return false;
		}
		//验证签名
		rapidjson::Document& old_data=blockchain.getDataFromDatabase("NDN",ndn,"NDN",false);
		std::string pubkey=old_data["pubkey"].GetString();
		if(!key_manager.verifyDocument(ndn_data,pubkey,sig))
		{
			std::cout<<"更新标识请求验证签名失败\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		//检查该用户是否已经注册,正常情况下不会发生没有该用户的情况
		if(!blockchain.hasData("pubkey",pubkey,"NDN-USER"))
		{
			std::cout<<"更新标识请求没有该用户\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		rapidjson::Document& user_data=blockchain.getDataFromDatabase("pubkey",pubkey,"NDN-USER",false);
		std::string ndn_prefix=user_data["prefix"].GetString();
		//检查ndn前缀是否正确

		if(ndn_prefix.size()>=ndn.size())
		{
			std::cout<<"更新标识请求ndn前缀不正确\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			user_data.SetNull();
			user_data.GetAllocator().Clear();
			delete &user_data;
			return false;
		}
		int len=ndn_prefix.size();
		for(int i=0;i<len;i++)
		{
			if(ndn_prefix[i]!=ndn[i])
			{
				std::cout<<"更新标识请求ndn前缀不正确\n";
				old_data.SetNull();
				old_data.GetAllocator().Clear();
				delete &old_data;
				user_data.SetNull();
				user_data.GetAllocator().Clear();
				delete &user_data;
				return false;
			}
		}
		old_data.SetNull();
		old_data.GetAllocator().Clear();
		delete &old_data;
		user_data.SetNull();
		user_data.GetAllocator().Clear();
		delete &user_data;
		return true;
	}
	else if(command=="Delete")
	{
		if(!data.HasMember("sig"))
		{
			std::cout<<"删除标识请求数据不完整，缺少sig\n";
			return false;
		}
		if(!(ndn_data.HasMember("NDN")
				&&ndn_data.HasMember("timestamp")))
		{
			std::cout<<"删除标识请求数据不完整\n";
			return false;
		}
		std::string ndn=ndn_data["NDN"].GetString();
		double timestamp=ndn_data["timestamp"].GetDouble();
		std::string sig=data["sig"].GetString();
		//检查是否已存在该NDN标识
		if(!blockchain.hasData("NDN",ndn,"NDN"))
		{
			std::cout<<"validateNDNTransaction中不存在该NDN标识：\n"<<ndn<<"\n";
			return false;
		}
		//验证签名
		rapidjson::Document& old_data=blockchain.getDataFromDatabase("NDN",ndn,"NDN",false);
		std::string pubkey=old_data["pubkey"].GetString();
		if(!key_manager.verifyDocument(ndn_data,pubkey,sig))
		{
			std::cout<<"删除标识请求验证签名失败\n";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return false;
		}
		old_data.SetNull();
		old_data.GetAllocator().Clear();
		delete &old_data;
		return true;
	}
	else if(command=="Registry")
	{
		if(!(ndn_data.HasMember("pubkey")
				&&ndn_data.HasMember("prefix")
				&&ndn_data.HasMember("level")
				&&ndn_data.HasMember("timestamp")))
		{
			std::cout<<"注册用户请求数据不完整\n";
			return false;
		}
		std::string sig=data["sig"].GetString();
		std::string pubkey=ndn_data["pubkey"].GetString();
		std::string prefix=ndn_data["prefix"].GetString();
		int level=ndn_data["level"].GetInt();
		double timestamp=ndn_data["timestamp"].GetDouble();
		//检查是否已存在该公钥的用户
		if(blockchain.hasData("pubkey",pubkey,"NDN-USER"))
		{
			std::cout<<"validateNDNTransaction——Registry中已经存在该pubkey:\n"<<pubkey<<"\n";
			return false;
		}
		//检查是否有用户已使用该前缀
		if(level>0)
		{
			if(blockchain.hasData("prefix",prefix,"NDN-USER"))
			{
				std::cout<<"validateNDNTransaction——Registry中已经存在该prefix:\n"<<prefix<<"\n";
				return false;
			}
		}
		//暂时对用户等级不做检查
		//验证签名
		//rapidjson::Document& old_data=blockchain.getDataFromDatabase("pubkey",pubkey,"NDN-USER",false);
		//std::string pubkey=old_data["pubkey"].GetString();
		if(!key_manager.verifyDocument(ndn_data,pubkey,sig))
		{
			std::cout<<"注册用户请求验证签名失败\n";
			return false;
		}
		//关键字检查
		int len=butler_filter_list.size();
		for(int i=0;i<len;i++)
		{
			if(prefix.find(butler_filter_list[i])!=std::string::npos)
			{
				std::cout<<"注册用户错误：prefix标识中包含不合法关键词"<<butler_filter_list[i]<<"\n";
				return false;
			}
		}
		//delete &old_data;
		return true;
	}
//	else if(command=="Query")
//	{
//		if(!ndn_data.HasMember("QueryCode"))
//			return false;
//		return true;
//	}
	else
	{
		return false;
	}
}
