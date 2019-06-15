#include "POV.h"

//处理区块链查询
ErrorMessage POV::handleBlockchainQuery(rapidjson::Document& doc)
{
	ErrorMessage msg;
	msg.type = "blockchain";
	msg.type_num = 2;
	msg.use_num_type = true;
	if (!doc.HasMember("data"))
	{
		msg.errcode = 98;
		msg.msg = "json中不包含data字段";
		return msg;
	}
	rapidjson::Value& data = doc["data"];
	if (!data.HasMember("command"))
	{
		msg.errcode = 97;
		msg.msg = "data中不包含command字段";
		return msg;
	}
	std::string command = data["command"].GetString();
	if (command == "getHeight")
	{
		if (!data.HasMember("height"))
		{
			msg.errcode = 11;
			msg.msg = "data中不包含height字段";
			return msg;
		}
		if (!data["height"].IsInt())
		{
			msg.errcode = 12;
			msg.msg = "data中height字段不是int类型";
			return msg;
		}
		int height = data["height"].GetInt();
		if (height > blockchain.getHeight())
		{
			msg.errcode = 13;
			msg.msg = "数据库中无此区块";
			return msg;
		}
		msg.errcode = 10;
		PoVBlock block = blockchain.getBlockFromDatabase(height);
		rapidjson::Document& d = block.getBlock();
		std::string block_str = getDocumentString(d);
		msg.msg = block_str;
		msg.is_json = true;
		d.SetNull();
		d.GetAllocator().Clear();
		delete &d;
		return msg;
	}
	else if (command == "getHeightByRange")
	{
		if (!data.HasMember("height_min"))
		{
			msg.errcode = 21;
			msg.msg = "data中不包含height_min字段";
			return msg;
		}
		if (!data.HasMember("height_max"))
		{
			msg.errcode = 22;
			msg.msg = "data中不包含height_max字段";
			return msg;
		}
		if (!data["height_min"].IsInt())
		{
			msg.errcode = 23;
			msg.msg = "data中height_min字段不是int类型";
			return msg;
		}
		if (!data["height_max"].IsInt())
		{
			msg.errcode = 24;
			msg.msg = "data中height_max字段不是int类型";
			return msg;
		}
		int height_max = data["height_max"].GetInt();
		int height_min = data["height_min"].GetInt();
		//		if(height_min>blockchain.getHeight())
		//		{
		//			msg.errcode=13;
		//			msg.msg="数据库中无此区块";
		//			return msg;
		//		}
		msg.errcode = 20;
		rapidjson::Document condition;
		condition.SetObject();
		rapidjson::Document::AllocatorType& allocator = condition.GetAllocator();
		rapidjson::Value _height;
		_height.SetObject();
		rapidjson::Value _height_min(height_min);
		rapidjson::Value _height_max(height_max);
		_height.AddMember("$gte", _height_min, allocator);
		_height.AddMember("$lte", _height_max, allocator);
		condition.AddMember("height", _height, allocator);

		rapidjson::Document& blocks = blockchain.getBlockFromDatabaseByCondition(condition);
		//rapidjson::Document& d=block.getBlock();
		std::string block_str = getDocumentString(blocks);
		msg.msg = block_str;
		msg.is_json = true;
		blocks.SetNull();
		blocks.GetAllocator().Clear();
		delete &blocks;
		return msg;
	}
	else if (command == "getTimeByRange")
	{
		if (!data.HasMember("time_min"))
		{
			msg.errcode = 31;
			msg.msg = "data中不包含time_min字段";
			return msg;
		}
		if (!data.HasMember("time_max"))
		{
			msg.errcode = 32;
			msg.msg = "data中不包含time_max字段";
			return msg;
		}
		if (!data["time_min"].IsDouble())
		{
			msg.errcode = 33;
			msg.msg = "data中time_min字段不是double类型";
			return msg;
		}
		if (!data["time_max"].IsDouble())
		{
			msg.errcode = 34;
			msg.msg = "data中time_max字段不是double类型";
			return msg;
		}
		double time_max = data["time_max"].GetDouble();
		double time_min = data["time_min"].GetDouble();
		//		if(height_min>blockchain.getHeight())
		//		{
		//			msg.errcode=13;
		//			msg.msg="数据库中无此区块";
		//			return msg;
		//		}
		msg.errcode = 30;
		rapidjson::Document condition;
		condition.SetObject();
		rapidjson::Document::AllocatorType& allocator = condition.GetAllocator();
		rapidjson::Value _time;
		_time.SetObject();
		rapidjson::Value _time_min(time_min);
		rapidjson::Value _time_max(time_max);
		_time.AddMember("$gte", _time_min, allocator);
		_time.AddMember("$lte", _time_max, allocator);
		condition.AddMember("block.header.Te", _time, allocator);

		rapidjson::Document& blocks = blockchain.getBlockFromDatabaseByCondition(condition);
		//rapidjson::Document& d=block.getBlock();
		std::string block_str = getDocumentString(blocks);
		msg.msg = block_str;
		msg.is_json = true;
		blocks.SetNull();
		blocks.GetAllocator().Clear();
		delete &blocks;
		return msg;
	}
	else if (command == "ApplyCommissioner")
	{
		msg.errcode = 20;
		msg.msg = "接收申请成功";

		Apply_Commissioner_Signatures.clear();
		//Apply_Commissioner_Metadata.Clear();
		//Apply_Commissioner_Transaction.Clear();
		//Apply_Commissioner_Metadata.SetNull();
		Apply_Commissioner_Metadata.GetAllocator().Clear();
		Apply_Commissioner_Transaction.SetNull();
		Apply_Commissioner_Transaction.GetAllocator().Clear();
		normal_apply_commissioner_state = ApplyCommissioner_apply_signatures;
		return msg;
	}
	else if (command == "ApplyButlerCandidate")
	{
		msg.errcode = 20;
		msg.msg = "接收申请成功";

		Apply_Butler_Candidate_Signatures.clear();
		//Apply_Commissioner_Metadata.Clear();
		//Apply_Commissioner_Transaction.Clear();
		RecommendationLetter.SetNull();
		RecommendationLetter.GetAllocator().Clear();
		//Apply_Commissioner_Transaction.SetNull();
		//Apply_Commissioner_Transaction.GetAllocator().Clear();
		normal_apply_butler_candidate_state = ApplyButlerCandidate_apply_recommendation_letter;
		return msg;
	}
	else if (command == "QuitCommissioner")
	{
		msg.errcode = 20;
		msg.msg = "接收申请成功";

		//Apply_Butler_Candidate_Signatures.clear();
		//Apply_Commissioner_Metadata.Clear();
		//Apply_Commissioner_Transaction.Clear();
		Quit_Commissioner_Metadata.SetNull();
		Quit_Commissioner_Metadata.GetAllocator().Clear();
		//Apply_Commissioner_Transaction.SetNull();
		//Apply_Commissioner_Transaction.GetAllocator().Clear();
		normal_quit_commissioner_state = QuitCommissioner_apply;
		return msg;
	}
	else if (command == "QuitButlerCandidate")
	{
		msg.errcode = 20;
		msg.msg = "接收申请成功";

		//Apply_Butler_Candidate_Signatures.clear();
		//Apply_Commissioner_Metadata.Clear();
		//Apply_Commissioner_Transaction.Clear();
		Quit_Butler_Candidate_Metadata.SetNull();
		Quit_Butler_Candidate_Metadata.GetAllocator().Clear();
		//Apply_Commissioner_Transaction.SetNull();
		//Apply_Commissioner_Transaction.GetAllocator().Clear();
		normal_quit_butler_candidate_state = QuitButlerCandidate_apply;
		return msg;
	}
	else
	{
		msg.errcode = 99;
		msg.msg = "该command无定义";
		return msg;
	}
}
//处理NDN查询
ErrorMessage POV::handleNDNQuery(rapidjson::Document& doc)
{
	ErrorMessage msg;
	msg.type = "NDN";
	if (!doc.HasMember("data"))
	{
		msg.errcode = 98;
		msg.msg = "json中不包含data字段";
		return msg;
	}
	rapidjson::Value& data = doc["data"];
	if (!data.HasMember("command"))
	{
		msg.errcode = 97;
		msg.msg = "data中不包含command字段";
		return msg;
	}
	std::string command = data["command"].GetString();

	if (command == "Generate")
	{
		if (!data.HasMember("NDN"))
		{
			msg.errcode = 105;
			msg.msg = "data中不包含NDN字段";
			return msg;
		}
		if (!data.HasMember("pubkey"))
		{
			msg.errcode = 106;
			msg.msg = "data中不包含pubkey字段";
			return msg;
		}
		if (!data.HasMember("IP"))
		{
			msg.errcode = 107;
			msg.msg = "data中不包含IP字段";
			return msg;
		}
		if (!data.HasMember("hash"))
		{
			msg.errcode = 108;
			msg.msg = "data中不包含hash字段";
			return msg;
		}
		if (!data.HasMember("other"))
		{
			msg.errcode = 109;
			msg.msg = "data中不包含other字段";
			return msg;
		}
		std::string ndn = data["NDN"].GetString();
		if (blockchain.hasData("NDN", ndn, "NDN"))
		{
			msg.errcode = 101;
			msg.msg = "数据库中已经存在该NDN标识";
			return msg;
		}
		std::string pubkey = data["pubkey"].GetString();
		std::string sig = doc["sig"].GetString();
		//			//验证签名
		//			if(!key_manager.verifyDocument(data,pubkey,sig))
		//			{
		//				msg.errcode=102;
		//				msg.msg="验证签名失败";
		//				std::cout<<"data:\n";
		//				print_document(data);
		//				std::cout<<"pubkey="<<pubkey<<"\n";
		//				std::cout<<"sig="<<sig<<"\n";
		//				//delete &old_data;
		//				return msg;
		//			}
					//检查该用户是否已经注册
		if (!blockchain.hasData("pubkey", pubkey, "NDN-USER"))
		{
			msg.errcode = 103;
			msg.msg = "数据库中不存在该用户";
			return msg;
		}
		if (login_users.find(pubkey) == login_users.end())
		{
			msg.errcode = 110;
			msg.msg = "用户未登录";
			return msg;
		}
		//检查ndn前缀是否正确
		rapidjson::Document& old_data = blockchain.getDataFromDatabase("pubkey", pubkey, "NDN-USER", false);
		std::string ndn_prefix = old_data["prefix"].GetString();
		if (ndn_prefix.size() >= ndn.size())
		{
			msg.errcode = 104;
			msg.msg = "ndn前缀不正确";
			return msg;
		}
		int len = ndn_prefix.size();
		for (int i = 0; i < len; i++)
		{
			if (ndn_prefix[i] != ndn[i])
			{
				msg.errcode = 104;
				msg.msg = "ndn前缀不正确";
				return msg;
			}
		}

		rapidjson::Document& ndn_tx = msg_manager.make_Request_Normal(0, doc);
		//			//转发给值班管家和下一个编号的管家
		//			account& butler=account_manager.getButler(current_duty_butler_num);
		//			ndn_tx["receiver"].SetUint64(butler.getNodeId());
		//			sendMessage(ndn_tx,false,100,Recall_RequestNormal,0,&POV::handleResponseNormal);
		//			account& butler2=account_manager.getButler((current_duty_butler_num+1)%butler_amount);
		//			ndn_tx["receiver"].SetUint64(butler.getNodeId());
		//			sendMessage(ndn_tx,false,100,Recall_RequestNormal,0,&POV::handleResponseNormal);
					//转发给所有管家
		for (int i = 0; i < account_manager.getButlerAmount(); i++)
		{
			account& butler = account_manager.getButler(i);
			ndn_tx["receiver"].SetUint64(butler.getNodeId());
			sendMessage(ndn_tx, false, 100, Recall_RequestNormal, 0, &POV::handleResponseNormal);
		}
		ndn_tx.SetNull();
		ndn_tx.GetAllocator().Clear();
		delete &ndn_tx;
		//构造返回信息
		msg.errcode = 100;
		msg.msg = "接收ndn标识成功";
		return msg;
	}
	else if (command == "Update")
	{
		std::string ndn = data["NDN"].GetString();

		if (!blockchain.hasData("NDN", ndn, "NDN"))
		{
			msg.errcode = 201;
			msg.msg = "数据库中不存在该NDN标识，无法进行更新";
			return msg;
		}
		//验证签名
		std::string sig = doc["sig"].GetString();
		rapidjson::Document& old_data = blockchain.getDataFromDatabase("NDN", ndn, "NDN", false);
		std::string pubkey = old_data["pubkey"].GetString();
		if (!key_manager.verifyDocument(data, pubkey, sig))
		{
			msg.errcode = 202;
			msg.msg = "验证签名失败";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return msg;
		}

		//检查该用户是否已经注册,正常情况下不会发生没有该用户的情况
		if (!blockchain.hasData("pubkey", pubkey, "NDN-USER"))
		{
			msg.errcode = 203;
			msg.msg = "数据库中不存在该用户";
			return msg;
		}
		rapidjson::Document& user_data = blockchain.getDataFromDatabase("pubkey", pubkey, "NDN-USER", false);
		std::string ndn_prefix = user_data["prefix"].GetString();
		//检查ndn前缀是否正确

		if (ndn_prefix.size() >= ndn.size())
		{
			msg.errcode = 204;
			msg.msg = "ndn前缀不正确";
			return msg;
		}
		int len = ndn_prefix.size();
		for (int i = 0; i < len; i++)
		{
			if (ndn_prefix[i] != ndn[i])
			{
				msg.errcode = 204;
				msg.msg = "ndn前缀不正确";
				return msg;
			}
		}
		//std::string pubkey=data["NDN"].GetString();
		//std::string sig=doc["sig"].GetString();

		rapidjson::Document& ndn_tx = msg_manager.make_Request_Normal(0, doc);
		//转发给值班管家和下一个编号的管家
		account& butler = account_manager.getButler(current_duty_butler_num);
		ndn_tx["receiver"].SetUint64(butler.getNodeId());
		sendMessage(ndn_tx, false, 100, Recall_RequestNormal, 0, &POV::handleResponseNormal);
		account& butler2 = account_manager.getButler((current_duty_butler_num + 1) % butler_amount);
		ndn_tx["receiver"].SetUint64(butler.getNodeId());
		sendMessage(ndn_tx, false, 100, Recall_RequestNormal, 0, &POV::handleResponseNormal);
		//			//转发给所有管家
		//			for(int i=0;i<account_manager.getButlerAmount();i++)
		//			{
		//				account& butler=account_manager.getButler(i);
		//				ndn_tx["receiver"].SetUint64(butler.getNodeId());
		//				sendMessage(ndn_tx,false,100,Recall_RequestNormal,0,&POV::handleResponseNormal);
		//			}
		ndn_tx.SetNull();
		ndn_tx.GetAllocator().Clear();
		delete &ndn_tx;
		//构造返回信息
		msg.errcode = 200;
		msg.msg = "接收更新ndn标识请求成功";
		old_data.SetNull();
		old_data.GetAllocator().Clear();
		delete &old_data;
		return msg;
	}
	else if (command == "Delete")
	{
		std::string ndn = data["NDN"].GetString();

		if (!blockchain.hasData("NDN", ndn, "NDN"))
		{
			msg.errcode = 301;
			msg.msg = "数据库中不存在该NDN标识，无法进行删除";
			return msg;
		}
		//验证签名
		rapidjson::Document& old_data = blockchain.getDataFromDatabase("NDN", ndn, "NDN", false);
		std::string pubkey = old_data["pubkey"].GetString();
		std::string sig = doc["sig"].GetString();
		if (!key_manager.verifyDocument(data, pubkey, sig))
		{
			msg.errcode = 302;
			msg.msg = "验证签名失败";
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return msg;
		}
		//转发给值班管家和下一个编号的管家
		rapidjson::Document& ndn_tx = msg_manager.make_Request_Normal(0, doc);
		account& butler = account_manager.getButler(current_duty_butler_num);
		ndn_tx["receiver"].SetUint64(butler.getNodeId());
		sendMessage(ndn_tx, false, 100, Recall_RequestNormal, 0, &POV::handleResponseNormal);
		account& butler2 = account_manager.getButler((current_duty_butler_num + 1) % butler_amount);
		ndn_tx["receiver"].SetUint64(butler.getNodeId());
		sendMessage(ndn_tx, false, 100, Recall_RequestNormal, 0, &POV::handleResponseNormal);
		//			//转发给所有管家
		ndn_tx.SetNull();
		ndn_tx.GetAllocator().Clear();
		//			for(int i=0;i<account_manager.getButlerAmount();i++)
		//			{
		//				account& butler=account_manager.getButler(i);
		//				ndn_tx["receiver"].SetUint64(butler.getNodeId());
		//				sendMessage(ndn_tx,false,100,Recall_RequestNormal,0,&POV::handleResponseNormal);
		//			}
		delete &ndn_tx;
		//构造返回信息
		msg.errcode = 300;
		msg.msg = "接收删除ndn标识请求成功";
		old_data.SetNull();
		old_data.GetAllocator().Clear();
		delete &old_data;
		return msg;
	}
	else if (command == "Query" && data.HasMember("QueryCode"))
	{
		int query_code = data["QueryCode"].GetInt();
		if (query_code == 0)
		{
			//				if(!data.HasMember("pubkey"))
			//				{
			//					msg.errcode=403;
			//					msg.msg="该查询请求中没有公钥";
			//					return msg;
			//				}
			//				if(!data["pubkey"].IsString())
			//				{
			//					msg.errcode=404;
			//					msg.msg="该查询请求中pubkey字段的值不是字符串";
			//					return msg;
			//				}
			//				std::string pubkey=data["pubkey"].GetString();
			//				if(!blockchain.hasData("pubkey",pubkey,"NDN-USER"))
			//				{
			//					msg.errcode=405;
			//					msg.msg="未注册用户不允许查询标识";
			//					return msg;
			//				}
			std::string ndn = data["NDN"].GetString();
			if (!blockchain.hasData("NDN", ndn, "NDN"))
			{
				msg.errcode = 401;
				msg.msg = "数据库中不存在该NDN标识";
				return msg;
			}
			//返回查询结果
			rapidjson::Document& old_data = blockchain.getDataFromDatabase("NDN", ndn, "NDN", true);
			std::cout << "查询结果：\n";
			print_document(old_data);
			msg.errcode = 400;
			msg.msg = getDocumentString(old_data);
			msg.is_json = true;
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return msg;
		}
		else if (query_code == 1)
		{
			std::string pubkey = data["pubkey"].GetString();
			if (!blockchain.hasData("pubkey", pubkey, "NDN"))
			{
				msg.errcode = 401;
				msg.msg = "数据库中不存在该pubkey";
				return msg;
			}
			//返回查询结果
			rapidjson::Document& old_data = blockchain.getDataFromDatabase("pubkey", pubkey, "NDN", true);
			msg.errcode = 400;
			msg.msg = getDocumentString(old_data);
			msg.is_json = true;
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return msg;
		}
		else if (query_code == 2)
		{
			std::string prefix = data["prefix"].GetString();
			if (!blockchain.hasData("prefix", prefix, "NDN"))
			{
				msg.errcode = 401;
				msg.msg = "数据库中不存在该prefix";
				return msg;
			}
			//返回查询结果
			rapidjson::Document& old_data = blockchain.getDataFromDatabase("prefix", prefix, "NDN", true);
			msg.errcode = 400;
			msg.msg = getDocumentString(old_data);
			msg.is_json = true;
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return msg;
		}
		else
		{
			msg.errcode = 402;
			msg.msg = "该QueryCode无定义";
			return msg;
		}
	}
	else if (command == "Registry")
	{
		std::string pubkey = data["pubkey"].GetString();
		std::string prefix = data["prefix"].GetString();
		int level = data["level"].GetInt();
		double timestamp = data["timestamp"].GetDouble();
		std::string sig = doc["sig"].GetString();
		if (!data.HasMember("real_msg"))
		{
			msg.errcode = 504;
			msg.msg = "data中缺少real_msg字段";
			return msg;
		}
		if (blockchain.hasData("pubkey", pubkey, "NDN-USER"))
		{
			msg.errcode = 501;
			msg.msg = "数据库中已经存在该用户";
			return msg;
		}
		if (level > 0)
		{
			if (blockchain.hasData("prefix", prefix, "NDN-USER"))
			{
				msg.errcode = 502;
				msg.msg = "数据库中已经存在该前缀";
				return msg;
			}
		}
		//验证签名
		//rapidjson::Document& old_data=blockchain.getDataFromDatabase("pubkey",pubkey,"NDN-USER",false);
		//std::string pubkey=old_data["pubkey"].GetString();
		//std::string sig=doc["sig"].GetString();
//			if(!key_manager.verifyDocument(data,pubkey,sig))
//			{
//				msg.errcode=503;
//				msg.msg="验证签名失败";
//				//delete &old_data;
//				return msg;
//			}
//			//转发给值班管家和下一个编号的管家
//			rapidjson::Document& ndn_tx=msg_manager.make_Request_Normal(0,doc);
//			account& butler=account_manager.getButler(current_duty_butler_num);
//			ndn_tx["receiver"].SetUint64(butler.getNodeId());
//			sendMessage(ndn_tx,false,100,Recall_RequestNormal,0,&POV::handleResponseNormal);
//			account& butler2=account_manager.getButler((current_duty_butler_num+1)%butler_amount);
//			ndn_tx["receiver"].SetUint64(butler.getNodeId());
//			sendMessage(ndn_tx,false,100,Recall_RequestNormal,0,&POV::handleResponseNormal);
			//转发给所有管家
		rapidjson::Document& ndn_tx = msg_manager.make_Request_Normal(0, doc);
		for (int i = 0; i < account_manager.getButlerAmount(); i++)
		{
			account& butler = account_manager.getButler(i);
			ndn_tx["receiver"].SetUint64(butler.getNodeId());
			sendMessage(ndn_tx, false, 100, Recall_RequestNormal, 0, &POV::handleResponseNormal);
		}
		ndn_tx.SetNull();
		ndn_tx.GetAllocator().Clear();
		delete &ndn_tx;
		//构造返回信息
		msg.errcode = 500;
		msg.msg = "接收用户注册请求成功";
		return msg;
	}
	else if (command == "getUser")
	{
		if (!data.HasMember("QueryCode"))
		{
			msg.errcode = 602;
			msg.msg = "没有QueryCode";
			return msg;
		}
		if (!data["QueryCode"].IsInt())
		{
			msg.errcode = 603;
			msg.msg = "QueryCode不是int类型";
			return msg;
		}
		int QueryCode = data["QueryCode"].GetInt();
		if (QueryCode == 0)
		{
			std::string pubkey = data["pubkey"].GetString();
			if (!blockchain.hasData("pubkey", pubkey, "NDN-USER"))
			{
				msg.errcode = 601;
				msg.msg = "数据库中不存在该用户";
				return msg;
			}
			//返回查询结果
			rapidjson::Document& old_data = blockchain.getDataFromDatabase("pubkey", pubkey, "NDN-USER", true);
			msg.errcode = 600;
			msg.msg = getDocumentString(old_data);
			msg.is_json = true;
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return msg;
		}
		else if (QueryCode == 1)
		{
			rapidjson::Document& old_data = blockchain.getAllDataFromDatabase("NDN-USER");
			rapidjson::Document::AllocatorType &allocator = old_data.GetAllocator();
			for (int i = 0; i < old_data.Size(); i++)
			{
				old_data[i]["fingerprint"].SetString("", allocator);
				old_data[i]["avatar"].SetString("", allocator);
				old_data[i]["iris"].SetString("", allocator);
			}
			std::cout << "返回数据:\n";
			print_document(old_data);
			msg.errcode = 600;
			msg.msg = getDocumentString(old_data);
			msg.is_json = true;
			old_data.SetNull();
			old_data.GetAllocator().Clear();
			delete &old_data;
			return msg;
		}
		else
		{
			msg.errcode = 604;
			msg.msg = "该用户查询码无定义";
			return msg;
		}
	}
	else if (command == "login")
	{
		std::string pubkey = data["pubkey"].GetString();
		bool result = data["result"].GetBool();
		if (!blockchain.hasData("pubkey", pubkey, "NDN-USER"))
		{
			msg.errcode = 701;
			msg.msg = "数据库中不存在该用户";
			return msg;
		}
		if (result)
		{
			login_users[pubkey] = getCurrentTime();
			msg.errcode = 700;
			msg.msg = "用户登录成功";
			return msg;
		}
		else
		{
			msg.errcode = 702;
			msg.msg = "登录验证失败";
			return msg;
		}
	}
	else if (command == "Log")
	{
		//转发给所有管家
		rapidjson::Document& ndn_tx = msg_manager.make_Request_Normal(0, doc);
		for (int i = 0; i < account_manager.getButlerAmount(); i++)
		{
			account& butler = account_manager.getButler(i);
			ndn_tx["receiver"].SetUint64(butler.getNodeId());
			sendMessage(ndn_tx, false, 100, Recall_RequestNormal, 0, &POV::handleResponseNormal);
		}
		ndn_tx.SetNull();
		ndn_tx.GetAllocator().Clear();
		delete &ndn_tx;
		//构造返回信息
		msg.errcode = 1000;
		msg.msg = "日志记录成功";
		return msg;
	}
	else
	{
		msg.errcode = 99;
		msg.msg = "该command无定义";
		return msg;
	}

}
//从NDN交易中获取NDN命令类型
std::string POV::getNDNType(rapidjson::Value& tx)
{
	std::string result = "";
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
				if (content.HasMember("data") && content["data"].IsObject())
				{
					rapidjson::Value& data = content["data"];
					if (data.HasMember("command") && data["command"].IsString())
					{
						result = data["command"].GetString();
					}
				}
			}
		}
	}
	return result;
}
