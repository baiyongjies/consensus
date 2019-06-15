#include "POV.h"

//更新普通交易
void POV::updateNormalTransaction(rapidjson::Value& tx)
{
	//std::cout<<"updateNormalTransaction:1\n";
	if (tx.HasMember("content") && tx["content"].IsObject())
	{
		//std::cout<<"updateNormalTransaction:2\n";
		rapidjson::Value& content = tx["content"];
		if (content.HasMember("type") && content["type"].IsString())
		{
			//std::cout<<"updateNormalTransaction:3\n";
			std::string type = content["type"].GetString();
			if (type == "NDN-IP")
			{
				std::cout << "处理NDN-IP交易:\n";
				//print_document(content);
				updateNDNTransaction(content);
			}
		}
	}
}
//更新NDN交易
void POV::updateNDNTransaction(rapidjson::Value& tx)
{
	if (!tx.HasMember("data") || !tx["data"].HasMember("command"))
		return;
	rapidjson::Value& ndn_data = tx["data"];
	std::string command = ndn_data["command"].GetString();
	if (command == "Generate")
	{
		if (!(ndn_data.HasMember("NDN")
			&& ndn_data.HasMember("IP")
			&& ndn_data.HasMember("pubkey")
			&& ndn_data.HasMember("timestamp")
			&& ndn_data.HasMember("hash")
			&& ndn_data.HasMember("other")))
			return;
		std::string ndn = ndn_data["NDN"].GetString();
		if (blockchain.hasData("NDN", ndn, "NDN"))
		{
			std::cout << "updateNDNTransaction中已经存在该NDN标识:\n" << ndn << "\n";
			return;
		}
		std::string IP = ndn_data["IP"].GetString();
		std::string pubkey = ndn_data["pubkey"].GetString();
		std::string hash = ndn_data["hash"].GetString();
		double timestamp = ndn_data["timestamp"].GetDouble();
		std::string other = ndn_data["other"].GetString();
		//构建NDN数据结构并存储到数据库当中
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
		//std::string rand_str="hello world";
		rapidjson::Value ndn_value(ndn.c_str(), ndn.size(), allocator);
		doc.AddMember("NDN", ndn_value, allocator);
		rapidjson::Value IP_value(IP.c_str(), IP.size(), allocator);
		doc.AddMember("IP", IP_value, allocator);
		rapidjson::Value pubkey_value(pubkey.c_str(), pubkey.size(), allocator);
		doc.AddMember("pubkey", pubkey_value, allocator);
		rapidjson::Value hash_value(hash.c_str(), hash.size(), allocator);
		doc.AddMember("hash", hash_value, allocator);
		rapidjson::Value other_value(other.c_str(), other.size(), allocator);
		doc.AddMember("other", other_value, allocator);
		rapidjson::Value time(timestamp);
		doc.AddMember("timestamp", time, allocator);
		std::cout << "将生成的标识存储到区块链中\n";
		int result = blockchain.saveDataToDatabase(doc, "NDN", false, "NDN", ndn);
		if (result >= 0)
		{
			std::cout << "NDN标识" << ndn << "生成成功\n";
		}
		else
		{
			std::cout << "NDN标识" << ndn << "生成失败\n";
		}
		//rapidjson::Document& d=blockchain.getDataFromDatabase("content",rand_str,"test",false);
		//std::cout<<"get data:\n";
		//blockchain.saveDataToDatabase(data,"test",false,"content",rand_str);

		return;
	}
	else if (command == "Update")
	{
		//std::cout<<"Update:1\n";
		if (!tx.HasMember("sig"))
			return;
		//std::cout<<"Update:2\n";
		if (!(ndn_data.HasMember("NDN")
			&& ndn_data.HasMember("IP")
			&& ndn_data.HasMember("timestamp")
			&& ndn_data.HasMember("hash")))
			return;
		//std::cout<<"Update:3\n";
		//从ndn_data数据提取ndn、IP、timestamp，从数据库中查找pubkey
		std::string ndn = ndn_data["NDN"].GetString();
		if (!blockchain.hasData("NDN", ndn, "NDN"))
		{
			std::cout << "updateNDNTransaction中不存在该NDN标识:\n" << ndn << "\n";
			return;
		}
		//std::cout<<"Update:4\n";
		std::string IP = ndn_data["IP"].GetString();
		double timestamp = ndn_data["timestamp"].GetDouble();
		std::string sig = tx["sig"].GetString();
		//std::cout<<"Update:5\n";
		rapidjson::Document& old_data = blockchain.getDataFromDatabase("NDN", ndn, "NDN", false);
		print_document(old_data);
		std::string pubkey = old_data["pubkey"].GetString();
		std::string other = old_data["other"].GetString();
		//std::cout<<"Update:6\n";
		//构建NDN数据结构并更新数据库当中
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
		//std::string rand_str="hello world";
		rapidjson::Value ndn_value(ndn.c_str(), ndn.size(), allocator);
		doc.AddMember("NDN", ndn_value, allocator);
		rapidjson::Value IP_value(IP.c_str(), IP.size(), allocator);
		doc.AddMember("IP", IP_value, allocator);
		rapidjson::Value pubkey_value(pubkey.c_str(), pubkey.size(), allocator);
		doc.AddMember("pubkey", pubkey_value, allocator);
		rapidjson::Value time(timestamp);
		doc.AddMember("timestamp", time, allocator);
		rapidjson::Value other_value(other.c_str(), other.size(), allocator);
		doc.AddMember("other", other_value, allocator);
		//std::cout<<"Update:7\n";
		int result = blockchain.saveDataToDatabase(doc, "NDN", true, "NDN", ndn);
		if (result < 0)
		{
			std::cout << "NDN标识" << ndn << "更新失败\n";
		}
		else if (result == 0)
		{
			std::cout << "插入新NDN:" << ndn << "\n";
		}
		else if (result == 1)
		{
			std::cout << "NDN标识" << ndn << "更新成功\n";
		}
		//验证签名
		return;
	}
	else if (command == "Delete")
	{
		if (!tx.HasMember("sig"))
			return;
		if (!(ndn_data.HasMember("NDN")
			&& ndn_data.HasMember("timestamp")))
			return;
		std::string ndn = ndn_data["NDN"].GetString();
		if (!blockchain.hasData("NDN", ndn, "NDN"))
		{
			std::cout << "updateNDNTransaction中不存在该NDN标识：\n" << ndn << "\n";
			return;
		}
		double timestamp = ndn_data["timestamp"].GetDouble();
		std::string sig = tx["sig"].GetString();
		int ret = blockchain.deleteData("NDN", ndn, "NDN", true);
		std::cout << "删除NDN数量：" << ret;
		//验证签名
		return;
	}
	else if (command == "Registry")
	{
		if (!(ndn_data.HasMember("pubkey")
			&& ndn_data.HasMember("prefix")
			&& ndn_data.HasMember("level")
			&& ndn_data.HasMember("timestamp")
			&& ndn_data.HasMember("real_msg")))
			return;
		std::string pubkey = ndn_data["pubkey"].GetString();
		std::string prefix = ndn_data["prefix"].GetString();
		int level = ndn_data["level"].GetInt();
		double timestamp = ndn_data["timestamp"].GetDouble();
		std::string real_msg = ndn_data["real_msg"].GetString();
		std::string fingerprint = ndn_data["fingerprint"].GetString();
		//std::cout<<"fingerprint数据："<<fingerprint<<"\n";
		std::string avatar = ndn_data["avatar"].GetString();
		//std::cout<<"avatar数据："<<avatar<<"\n";
		std::string iris = ndn_data["iris"].GetString();
		//std::cout<<"iris数据："<<iris<<"\n";
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
		rapidjson::Value pubkey_value(pubkey.c_str(), pubkey.size(), allocator);
		doc.AddMember("pubkey", pubkey_value, allocator);
		rapidjson::Value prefix_value(prefix.c_str(), prefix.size(), allocator);
		doc.AddMember("prefix", prefix_value, allocator);
		rapidjson::Value level_value(level);
		doc.AddMember("level", level_value, allocator);
		rapidjson::Value _real_msg(real_msg.c_str(), real_msg.size(), allocator);
		doc.AddMember("real_msg", _real_msg, allocator);
		rapidjson::Value timestamp_value(timestamp);
		doc.AddMember("timestamp", timestamp_value, allocator);
		rapidjson::Value fingerprint_value(fingerprint.c_str(), fingerprint.size(), allocator);
		doc.AddMember("fingerprint", fingerprint_value, allocator);
		rapidjson::Value avatar_value(avatar.c_str(), avatar.size(), allocator);
		doc.AddMember("avatar", avatar_value, allocator);
		rapidjson::Value iris_value(iris.c_str(), iris.size(), allocator);
		doc.AddMember("iris", iris_value, allocator);
		//std::cout<<"将用户数据存储到数据库中\n";
		int result = blockchain.saveDataToDatabase(doc, "NDN-USER", false, "pubkey", pubkey);
		if (result >= 0)
		{
			std::cout << "用户" << pubkey << "注册成功\n";
		}
		else
		{
			std::cout << "用户" << pubkey << "注册失败\n";
		}
	}
	else if (command == "Log")
	{
		if (!(ndn_data.HasMember("QueryCode")))
			return;
		int QueryCode = ndn_data["QueryCode"].GetInt();
		if (QueryCode == 21)
		{
			if (!(ndn_data.HasMember("name")
				&& ndn_data.HasMember("sig")
				&& ndn_data.HasMember("real_msg")
				&& ndn_data.HasMember("timestamp")))
				return;
			std::string name = ndn_data["name"].GetString();
			std::string sig = ndn_data["sig"].GetString();
			std::string real_msg = ndn_data["real_msg"].GetString();
			std::string timestamp = ndn_data["timestamp"].GetString();
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
			rapidjson::Value name_value(name.c_str(), name.size(), allocator);
			doc.AddMember("name", name_value, allocator);
			rapidjson::Value sig_value(sig.c_str(), sig.size(), allocator);
			doc.AddMember("sig", sig_value, allocator);
			rapidjson::Value real_msg_value(real_msg.c_str(), real_msg.size(), allocator);
			doc.AddMember("real_msg", real_msg_value, allocator);
			rapidjson::Value timestamp_value(timestamp.c_str(), timestamp.size(), allocator);
			doc.AddMember("timestamp", timestamp_value, allocator);
			int result = blockchain.saveDataToDatabase(doc, "event_log", false, "timestamp", timestamp);
			if (result >= 0)
			{
				std::cout << "普通日志" << timestamp << "记录成功\n";
			}
			else
			{
				std::cout << "普通日志" << timestamp << "记录失败\n";
			}
		}
		else if (QueryCode == 22)
		{
			if (!(ndn_data.HasMember("pubkey")
				&& ndn_data.HasMember("real_msg")
				&& ndn_data.HasMember("timestamp")))
				return;
			std::string pubkey = ndn_data["pubkey"].GetString();
			std::string real_msg = ndn_data["real_msg"].GetString();
			std::string timestamp = ndn_data["timestamp"].GetString();
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
			rapidjson::Value pubkey_value(pubkey.c_str(), pubkey.size(), allocator);
			doc.AddMember("pubkey", pubkey_value, allocator);
			rapidjson::Value real_msg_value(real_msg.c_str(), real_msg.size(), allocator);
			doc.AddMember("real_msg", real_msg_value, allocator);
			rapidjson::Value timestamp_value(timestamp.c_str(), timestamp.size(), allocator);
			doc.AddMember("timestamp", timestamp_value, allocator);
			int result = blockchain.saveDataToDatabase(doc, "visa_log", false, "pubkey", pubkey);
			if (result >= 0)
			{
				std::cout << "签证日志" << timestamp << "记录成功\n";
			}
			else
			{
				std::cout << "签证日志" << timestamp << "记录失败\n";
			}
		}
	}
	else
	{
		return;
	}
}
//更新变量
void POV::updateVariables(rapidjson::Value& b)
{
	//std::cout<<"更新变量：1\n";
	PoVBlock block;
	block.setBlock(b);
	//处理普通交易,仅仅是清除缓存池中的相同交易而已
	if (!keep_normal_pool_full)
	{
		std::vector<PoVTransaction> Normals;
		block.getNormalTransactions(Normals);
		//std::cout<<"更新变量：2\n";
		for (std::vector<PoVTransaction>::iterator i = Normals.begin(); i != Normals.end(); i++)
		{
			//std::cout<<"更新变量：21\n";
			rapidjson::Document& tx = i->getData();
			//double timestamp=tx["timestamp"].GetDouble();
			//rapidjson::Value &content=tx["content"];
			DocumentContainer container;
			container.saveDocument(tx);
			norm_mutex.lock();
			if (lru_NormalPool.find(container))
			{
				lru_NormalPool.pop(container);
			}
			norm_mutex.unlock();
			/*
			for(std::vector<DocumentContainer>::iterator i=NormalPool.begin();i!=NormalPool.end();i++)
			{
				rapidjson::Document& cache_tx=i->getDocument();
				double cache_timestamp=cache_tx["timestamp"].GetDouble();
				rapidjson::Value& cache_content=cache_tx["content"];
				//普通交易的content有两种类型，分别是字符串或者json对象
				if(content.IsObject() && cache_content.IsObject()
						&&content.HasMember("log") &&cache_content.HasMember("log")
						&&content["log"].GetInt()==cache_content["log"].GetInt())
				{
					NormalPool.erase(i);
					i--;
				}
				else if(content.IsString())
				{
					if(cache_content.IsString())
					{
						if(std::string(content.GetString())==std::string(cache_content.GetString()))
						{
							NormalPool.erase(i);
							i--;
						}
					}
				}
				delete &cache_tx;

			}
			*/
			//print_document(tx);
			updateNormalTransaction(tx);
			tx.SetNull();
			tx.GetAllocator().Clear();
			delete &tx;
		}
	}

	//std::cout<<"更新变量：3\n";
	//处理常量交易
	std::vector<PoVTransaction> constants;
	block.getConstantsTransactions(constants);
	for (std::vector<PoVTransaction>::iterator i = constants.begin(); i != constants.end(); i++)
	{
		rapidjson::Document& tx = i->getData();
		//print_document(tx);
		butler_amount = tx["metadata"]["butler_amout"].GetInt();
		num_blocks = tx["metadata"]["blocks_per_cycle"].GetInt();
		Tcut = tx["metadata"]["Tcut"].GetDouble();
		vote_amount = tx["metadata"]["vote_amount"].GetInt();
		tx.SetNull();
		tx.GetAllocator().Clear();
		delete &tx;
	}
	//std::cout<<"更新变量：4\n";
	//处理申请委员交易交易
	std::vector<PoVTransaction> ApplyCommissioners;
	block.getApplyCommissionerTransactions(ApplyCommissioners);
	for (std::vector<PoVTransaction>::iterator i = ApplyCommissioners.begin(); i != ApplyCommissioners.end(); i++)
	{
		rapidjson::Document& tx = i->getData();
		std::string applicant = tx["metadata"]["pubkey"].GetString();
		account_manager.pushback_commissioner(applicant);
		ac_mutex.lock();
		for (std::vector<DocumentContainer>::iterator j = ApplyCommissionerPool.begin(); j != ApplyCommissionerPool.end(); j++)
		{
			rapidjson::Document& _tx = j->getDocument();
			if (_tx["metadata"]["pubkey"].GetString() == applicant)
			{
				ApplyCommissionerPool.erase(j);
				_tx.SetNull();
				_tx.GetAllocator().Clear();
				delete &_tx;
				break;
			}
			_tx.SetNull();
			_tx.GetAllocator().Clear();
			delete &_tx;
		}
		ac_mutex.unlock();
		tx.SetNull();
		tx.GetAllocator().Clear();
		delete &tx;
	}
	//std::cout<<"更新变量：5\n";
	//处理申请管家候选交易
	std::vector<PoVTransaction> ApplyButlerCandidates;
	block.getApplyButlerCandidateTransactions(ApplyButlerCandidates);
	for (std::vector<PoVTransaction>::iterator i = ApplyButlerCandidates.begin(); i != ApplyButlerCandidates.end(); i++)
	{
		rapidjson::Document& tx = i->getData();
		std::string applicant = tx["recommendation_letter"]["metadata"]["refferal"].GetString();
		account_manager.pushback_butler_candidate(applicant);
		abc_mutex.lock();
		for (std::vector<DocumentContainer>::iterator j = ApplyButlerCandidatePool.begin(); j != ApplyButlerCandidatePool.end(); j++)
		{
			rapidjson::Document& _tx = j->getDocument();
			if (_tx["recommendation_letter"]["metadata"]["refferal"].GetString() == applicant)
			{
				ApplyButlerCandidatePool.erase(j);
				_tx.SetNull();
				_tx.GetAllocator().Clear();
				delete &_tx;
				break;
			}
			_tx.SetNull();
			_tx.GetAllocator().Clear();
			delete &_tx;
		}
		abc_mutex.unlock();
		tx.SetNull();
		tx.GetAllocator().Clear();
		delete &tx;
	}
	//处理退出委员交易
	std::vector<PoVTransaction> QuitCommissioners;
	block.getQuitCommissionerTransactions(QuitCommissioners);
	for (std::vector<PoVTransaction>::iterator i = QuitCommissioners.begin(); i != QuitCommissioners.end(); i++)
	{
		rapidjson::Document& tx = i->getData();
		std::string applicant = tx["metadata"]["pubkey"].GetString();
		std::cout << applicant << "退出委员\n";
		account_manager.pop_commissioner(applicant);
		for (std::vector<DocumentContainer>::iterator j = QuitCommissionerPool.begin(); j != QuitCommissionerPool.end(); j++)
		{
			rapidjson::Document& _tx = j->getDocument();
			if (_tx["metadata"]["pubkey"].GetString() == applicant)
			{
				QuitCommissionerPool.erase(j);
				_tx.SetNull();
				_tx.GetAllocator().Clear();
				delete &_tx;
				break;
			}
			_tx.SetNull();
			_tx.GetAllocator().Clear();
			delete &_tx;
		}
		tx.SetNull();
		tx.GetAllocator().Clear();
		delete &tx;
	}
	//处理退出管家候选交易
	std::vector<PoVTransaction> QuitButlerCandidates;
	block.getQuitButlerCandateTransactions(QuitButlerCandidates);
	for (std::vector<PoVTransaction>::iterator i = QuitButlerCandidates.begin(); i != QuitButlerCandidates.end(); i++)
	{
		rapidjson::Document& tx = i->getData();
		std::string applicant = tx["metadata"]["pubkey"].GetString();
		std::cout << applicant << "退出管家候选\n";
		account_manager.pop_butler_candidate(applicant);
		qbc_mutex.lock();
		for (std::vector<DocumentContainer>::iterator j = QuitButlerCandidatePool.begin(); j != QuitButlerCandidatePool.end(); j++)
		{
			rapidjson::Document& _tx = j->getDocument();
			if (_tx["metadata"]["pubkey"].GetString() == applicant)
			{
				QuitButlerCandidatePool.erase(j);
				_tx.SetNull();
				_tx.GetAllocator().Clear();
				delete &_tx;
				break;
			}
			_tx.SetNull();
			_tx.GetAllocator().Clear();
			delete &_tx;
		}
		qbc_mutex.unlock();
		tx.SetNull();
		tx.GetAllocator().Clear();
		delete &tx;
	}
	//std::cout<<"更新变量：6\n";
	//处理投票交易
	BallotPool.clear();
	std::vector<PoVTransaction> Votes;
	block.getVoteTransactions(Votes);
	for (std::vector<PoVTransaction>::iterator i = Votes.begin(); i != Votes.end(); i++)
	{
		rapidjson::Document& tx = i->getData();
		rapidjson::Value& result = tx["metadata"]["result"];
		//account_manager.clear_butler();
		for (uint32_t i = 0; i < result.Size(); i++)
		{
			std::string butler = result[i].GetString();
			account_manager.pushback_butler(butler);
			account_manager.setButlerPubkeyByNum(i, butler);
		}
		tx.SetNull();
		tx.GetAllocator().Clear();
		delete &tx;
	}
	//std::cout<<"更新变量：7\n";
	//更新自己的账号身份
	std::string my_pubkey = account_manager.getMyPubkey();
	if (account_manager.is_commissioner(my_pubkey))
	{
		account_manager.setCommissioner();
	}
	else
	{
		account_manager.setNotCommissioner();
	}

	if (account_manager.is_butler_candidate(my_pubkey))
	{
		account_manager.setButlerCandidate();
	}
	else
	{
		account_manager.setNotButlerCandidate();
	}

	if (account_manager.is_butler(my_pubkey))
	{
		account_manager.setButler();
	}
	else
	{
		account_manager.setNotButler();
	}
	//std::cout<<"更新变量：8\n";
	//更新系统变量
	//cache_block_mutex.lock();
	if (cache_block != NULL)
	{
		delete cache_block;
		cache_block = NULL;
	}
	//cache_block_mutex.unlock();
	rbs_mutex.lock();
	raw_block_signatures.clear();
	rbs_mutex.unlock();
	init_duty_butler_num = block.getNextButler();
	start_time = block.getTe();
	period_generated_block_num = (period_generated_block_num + 1) % num_blocks;
	std::cout << "period_generated_block_num=" << period_generated_block_num << "\n";
	std::cout << "num_blocks=" << num_blocks << "\n";
	GenerateBlock = GenerateBlock_sign;
	Normal_vote = Vote_voting;
	//std::cout<<"管家候选人数："<<account_manager.getButlerCandidateAmount()<<"\n";
	//std::cout<<"委员人数："<<account_manager.getCommissionerAmount()<<"\n";
	//std::cout<<"更新变量：9\n";
}
