#include "POV.h"

//处理委员公钥回复
void POV::handleResponseCommissionerPubkey(rapidjson::Document &d)
{
	//std::cout<<"在handleResponseCommissionerPubkey中\n";
	NodeID sender=d["sender"].GetUint64();
	std::string pubkey(d["pubkey"].GetString(),d["pubkey"].GetStringLength());
	//std::cout<<network->getNodeId()<<"节点 received pubkey in handleResponseCommissionerPubkey:"<<pubkey<<" from "<<sender<<"\n";
	account_manager.pushback_commissioner(pubkey,sender);
	//account_manager.set_commissioner_ID(pubkey,sender);
	account_manager.pushback_butler_candidate(pubkey,sender);
	//account_manager.set_butler_candidate_ID(pubkey,sender);
	//std::cout<<network->getNodeId()<<"节点 received pubkey in handleResponseCommissionerPubkey:"<<pubkey<<" from "<<sender<<" end\n";
}
//处理申请委员签名的回复
void POV::handleResponseApplyCommissionerSignature(rapidjson::Document &d)
{
	//std::cout<<network->getNodeId()<<" in handleResponseApplyCommissionerSignature\n";
	//print_document(d);
	NodeID sender=d["sender"].GetUint64();
	if(d.HasMember("data"))
	{
		//std::cout<<network->getNodeId()<<" in handleResponseApplyCommissionerSignature:1\n";
		rapidjson::Value &data=d["data"];
		rapidjson::Document::AllocatorType& a = d.GetAllocator();
		std::string pubkey=data["pubkey"].GetString();
		std::string sig=data["sig"].GetString();
		if(!key_manager.verifyDocument(Apply_Commissioner_Metadata,pubkey,sig))
		{
			std::cout<<network->getNodeId()<<"验证"<<sender<<"签名不通过！\n";
			std::cout<<"pubkey:"<<pubkey<<"\n";
			std::cout<<"sig:"<<sig<<"\n";
			return;
		}
		//std::cout<<network->getNodeId()<<" in handleResponseApplyCommissionerSignature:2\n";
		//std::cout<<"received pubkey:"<<pubkey<<"\n";
		//std::string sig=data["sig"].GetString();
		//判断是否是委员
		if(!account_manager.is_commissioner(pubkey))
		{
			std::cout<<" not commissioner\n";
			return;
		}
		//std::cout<<network->getNodeId()<<" in handleResponseApplyCommissionerSignature:3\n";
		//判断该签名是否已经存在
		acs_mutex.lock();
		//std::cout<<"acs数量："<<Apply_Commissioner_Signatures.size()<<"\n";
		for(std::vector<signature>::iterator i=Apply_Commissioner_Signatures.begin();i!=Apply_Commissioner_Signatures.end();i++)
		{
			//std::cout<<"i->pubkey="<<i->pubkey<<"\n";
			if(i->pubkey==pubkey)
			{
				//std::cout<<" loop in acs 2\n";
				acs_mutex.unlock();
				return;
			}
		}
		acs_mutex.unlock();

		//std::cout<<network->getNodeId()<<" in handleResponseApplyCommissionerSignature:4\n";
		signature _sig;
		_sig.pubkey=pubkey;
		_sig.sig=sig;
		acs_mutex.lock();
		Apply_Commissioner_Signatures.push_back(_sig);
		acs_mutex.unlock();
		//std::cout<<network->getNodeId()<<" in handleResponseApplyCommissionerSignature:5\n";

		if(Apply_Commissioner_Signatures.size()>=account_manager.getCommissionerAmount())
		{
			gegenerate_first_bolck_apply_commissioner_state=ApplyCommissioner_final_apply;
			normal_apply_commissioner_state=ApplyCommissioner_final_apply;
		}
		//std::cout<<network->getNodeId()<<" in handleResponseApplyCommissionerSignature:6\n";

	}
	else
	{
		std::cout<<" not have data member\n";
		print_document(d);
	}

}
//处理申请委员的回复
void POV::handleResponseApplyCommissioner(rapidjson::Document &d)
{
	//为简便起见，这里代理委员或值班管家返回的回复没有加入同意接受或拒绝信息。
	std::cout<<"在处理ResponseApplyCommissioner中\n";
	//print_document(d);
	GenerateFirstBlock_ApplyCommissioner=false;
	gegenerate_first_bolck_apply_commissioner_state=ApplyCommissioner_finish;
	normal_apply_commissioner_state=ApplyCommissioner_finish;
}
//处理请求推荐信的回复
void POV::handleResponseRecommendationLetter(rapidjson::Document &d)
{
	std::cout<<"接收到推荐信：\n";
	//print_document(d);
	if(state==generate_first_block)
	{
		if(generate_first_bolck_apply_butler_candidate_state==ApplyButlerCandidate_apply_recommendation_letter and RecommendationLetter.IsNull())
		{
			//验证签名
			if(!validateRecommendationLetter(d["data"]))
			{
				std::cout<<"验证推荐信不通过！\n";
				return;
			}
			rapidjson::Document::AllocatorType &allocator=d.GetAllocator();
			//RecommendationLetter.SetNull();
			RecommendationLetter.GetAllocator().Clear();
			rapidjson::Value &letter=d["data"];
			RecommendationLetter.CopyFrom(d["data"],RecommendationLetter.GetAllocator());
			//print_document(RecommendationLetter);
			generate_first_bolck_apply_butler_candidate_state=ApplyButlerCandidate_apply_signatures;
		}
	}
	else
	{
		if(normal_apply_butler_candidate_state==ApplyButlerCandidate_apply_recommendation_letter and RecommendationLetter.IsNull())
		{
			//验证签名
			if(!validateRecommendationLetter(d["data"]))
			{
				std::cout<<"验证推荐信不通过！\n";
				return;
			}
			rapidjson::Document::AllocatorType &allocator=d.GetAllocator();
			//RecommendationLetter.SetNull();
			RecommendationLetter.GetAllocator().Clear();
			rapidjson::Value &letter=d["data"];
			RecommendationLetter.CopyFrom(d["data"],RecommendationLetter.GetAllocator());
			//print_document(RecommendationLetter);
			normal_apply_butler_candidate_state=ApplyButlerCandidate_apply_signatures;
		}
	}
}
//处理申请管家候选签名的回复
void POV::handleResponseApplyButlerCandidateSignature(rapidjson::Document &d)
{
	std::cout<<"收到申请管家候选的签名！\n";
	rapidjson::Value &sig=d["data"];
	std::string pubkey=sig["pubkey"].GetString();
	if(!validateApplyButlerCandidateSignature(RecommendationLetter,sig))
	{
		std::cout<<"验证管家候选签名不正确！\n";
	}
	abcs_mutex.lock();
	for(std::vector<signature>::iterator i=Apply_Butler_Candidate_Signatures.begin();i!=Apply_Butler_Candidate_Signatures.end();i++)
	{
		if(i->pubkey==pubkey)
		{
			//std::cout<<" loop in acs 2\n";
			abcs_mutex.unlock();
			return;
		}
	}
	abcs_mutex.unlock();
	signature _sig;
	_sig.pubkey=pubkey;
	_sig.sig=sig["sig"].GetString();
	abcs_mutex.lock();
	Apply_Butler_Candidate_Signatures.push_back(_sig);
	abcs_mutex.unlock();
	if(state==generate_first_block)
	{
		//在生成创世区块阶段，只有获得所有委员的签名才能通过
		if(Apply_Butler_Candidate_Signatures.size()==account_manager.getCommissionerAmount())
		{
			generate_first_bolck_apply_butler_candidate_state=ApplyButlerCandidate_final_apply;
			std::cout<<"进入ApplyButlerCandidate_final_apply阶段！\n";
		}
	}
	else
	{
		//在其他阶段，只要获得超过委员数量一半的签名就能验证通过
		if(Apply_Butler_Candidate_Signatures.size()>=account_manager.getCommissionerAmount()/2)
		{
			//generate_first_bolck_apply_butler_candidate_state=ApplyButlerCandidate_final_apply;
			std::cout<<"进入ApplyButlerCandidate_final_apply阶段！\n";
			normal_apply_butler_candidate_state=ApplyButlerCandidate_final_apply;
		}
	}

}
//处理申请管家候选签名交易的回复
void POV::handleResponseApplyButlerCandidate(rapidjson::Document &d)
{
	std::cout<<account_manager.getMyNodeID()<<"节点接收到最终申请管家候选的响应！\n";
	//清空推荐信和签名

	RecommendationLetter.SetNull();
	RecommendationLetter.GetAllocator().Clear();
	abcs_mutex.lock();
	Apply_Butler_Candidate_Signatures.clear();
	abcs_mutex.unlock();
	//设置申请管家候选阶段为结束阶段。
	if(state==generate_first_block)
		generate_first_bolck_apply_butler_candidate_state=ApplyButlerCandidate_finish;
	else
		normal_apply_butler_candidate_state=ApplyButlerCandidate_finish;
}
//处理投票的回复
void POV::handleResponseVote(rapidjson::Document &d)
{
	//std::cout<<"在投票回复中！\n";
	GenerateFirstBlock_vote=Vote_finish;
	/*假设A节点比B节点先接收到新发布的区块K，进入投票阶段，并向B节点发送投票交易，B节点接收到A发送的投票交易并加入到投票池中并向
	 * A节点返回一个接收到的消息的回复，收到的同时会从CallBackList中把对应的CallBackInstance删除，这样当A再次运行Normal_Vote的时候仍然
	 * 会向B发送投票，如果希望不再发送投票交易，可以把Normal_vote设置为Vote_finish。
	 * 但是这样在极限情况下有可能会导致程序无法进行下去，假设当B接收到A的投票并加入到投票池时开始更新区块K，在更新过程中会将投票池清空掉，
	 * 然后进行产生K+1区块的阶段，但是A已经投过票并且不会再产生新的选票，B则永远无法收集齐所有的委员的投票，因此永远无法产生区块。
	 * 暂时考虑的可行解决办法有：
	 * 1.投票阶段从各个节点主动发送投票交易给管家节点，改成由管家节点向委员节点请求投票，这样可避免管家更新区块较慢的问题。然而管家向委员节点请求投票时
	 * 同样要考虑委员是否已经更新了区块K，处于哪个阶段，委员节点是否要根据当前的阶段决定对不对下一任管家进行投票。但目前看来对委员节点处于哪个阶段对投票
	 * 这件事并不重要，不管在哪个阶段都可以进行投票，因此该方法应该是可行的。
	 * 2.和产生区块过程的解决办法一样，增加一个缓存变量保存不同阶段接收到的投票交易，但由于投票交易本身是没有编号标记的，看起来很麻烦
	 * 3.直接取消掉Normal_vote=Vote_finish，这样会导致在产生特殊区块阶段委员节点会不断发送投票交易给管家节点，从而使通信量增加，但是实现起来最简单方便*/
	//Normal_vote=Vote_finish;
	//Normal_vote=Vote_finish;
}
//处理区块签名的回复
void POV::handleResponseBlockSignature(rapidjson::Document &d)
{
	//GenerateFirstBlock=GenerateBlock_publish;
	//for(std::vector<signature>)
	//发送生区块时，生区块头Te=0，在签名时把Te设置为委员节点的当前时间然后签名，最后把签名、公钥和时间放在一起发送给打包区块的管家，在这里验证的时候也要在区块头中加入签名中的时间再验证。
	//std::cout<<"在处理区块签名的回复中：start!\n";
	NodeID sender=d["sender"].GetUint64();
	if(d.HasMember("data"))
	{
		add_sig_mutex.lock();
		//std::cout<<"在处理区块签名的回复中:"<<1<<"\n";
		if(cache_block==NULL)
		{
			std::cout<<"缓存的生区块错误:cache_block==NULL!\n";
			add_sig_mutex.unlock();
			//cache_block_mutex.unlock();
			return;
		}
		//验证签名正确性
		rapidjson::Document& header=cache_block->getRawHeader();
		rapidjson::Value &data=d["data"];
		if(!validateBlockSignature(header,data))
		{
			std::cout<<"返回的区块签名验证错误！\n";
			std::cout<<"raw_block_signatures.size():"<<raw_block_signatures.size()<<"\n";
			std::cout<<"raw_block_error.size():"<<raw_block_error.size()<<"\n";
			//cache_block_mutex.unlock();
			delete &header;
			add_sig_mutex.unlock();
			//add_sig_mutex.unlock();
			return;
		}
		delete &header;
		//print_document(data);
		//rapidjson::Document::AllocatorType& a = d.GetAllocator();
		int status=data["status"].GetInt();
		if(status!=0)
		{
			std::cout<<"收到验证生区块失败的回复\n";
			ErrorMessage err_msg;
			std::string pubkey=data["pubkey"].GetString();
			err_msg.pubkey=pubkey;

			std::string sig=data["sig"].GetString();
			rbe_mutex.lock();
			//std::cout<<"在接收到错误时检查错误消息池的大小:"<<raw_block_error.size()<<"\n";
			raw_block_error.push_back(err_msg);
			rbe_mutex.unlock();
			add_sig_mutex.unlock();
			return;
		}
		std::string pubkey=data["pubkey"].GetString();
		std::string sig=data["sig"].GetString();
		double timestamp=data["timestamp"].GetDouble();
		std::string name=data["name"].GetString();
		//std::cout<<"在处理区块签名的回复中:"<<2<<"\n";
		//在管家发布新的区块后，就会把cache_block清空，这时如果有接收到迟到的签名回复，就会出错，因此要判断是否为空。
		//cache_block_mutex.lock();

		//std::cout<<"在处理区块签名的回复中:"<<3<<"\n";

		//判断该签名是否已经存在
		rbs_mutex.lock();
		for(std::vector<signature>::iterator i=raw_block_signatures.begin();i!=raw_block_signatures.end();)
		{
			if(i->pubkey==pubkey)
			{
				std::cout<<" 已存在该区块签名\n";
				//rbs_mutex.unlock();
				//return;
				//签名存在则将其替换为新的签名，否则有可能产生错误
				i=raw_block_signatures.erase(i);
				//break;
			}
			else
			{
				i++;
			}
		}
		rbs_mutex.unlock();
		//std::cout<<"在处理区块签名的回复中:"<<5<<"\n";

		signature _sig;
		_sig.pubkey=pubkey;
		_sig.sig=sig;
		_sig.timestamp=timestamp;
		_sig.name=name;
		rbs_mutex.lock();
		raw_block_signatures.push_back(_sig);
		rbs_mutex.unlock();
	}
	else
	{
		std::cout<<" not have data member\n";
		print_document(d);
	}
	add_sig_mutex.unlock();
	//std::cout<<"在处理区块签名的回复中：7!\n";
	//std::cout<<"收到"<<d["sender"].GetUint64()<<"节点区块签名！\n";
	if(state==generate_first_block)
	{
		rbs_mutex.lock();
		if(raw_block_signatures.size()==account_manager.getCommissionerAmount())
		{
			GenerateFirstBlock=GenerateBlock_publish;
		}
		rbs_mutex.unlock();
	}
	else
	{
//		rbs_mutex.lock();
////		if(raw_block_signatures.size()>=account_manager.getCommissionerAmount()/2.0)
////		{
////			GenerateBlock=GenerateBlock_publish;
////		}
//		if(raw_block_signatures.size()+raw_block_error.size()>=account_manager.getCommissionerAmount())
//		{
//			GenerateBlock=GenerateBlock_publish;
//		}
//		rbs_mutex.unlock();
	}

	//std::cout<<"在处理区块签名的回复中：end!\n";
}
//处理请求NodeID的回复
void POV::handleResponseNodeID(rapidjson::Document &d)
{
	//std::cout<<"获得了NodeID！\n";
	NodeID sender=d["sender"].GetUint64();
	std::string pubkey=d["pubkey"].GetString();
	account_manager.set_commissioner_ID(pubkey,sender);
	account_manager.set_butler_candidate_ID(pubkey,sender);
	account_manager.set_butler_ID(pubkey,sender);
}
//处理发送普通交易的回复
void POV::handleResponseNormal(rapidjson::Document &d)
{
	//std::cout<<"接收到了正常交易！\n";
}
//处理退出管家候选的回复
void POV::handleResponseQuitButlerCandidate(rapidjson::Document &d)
{

}
//处理退出委员的回复
void POV::handleResponseQuitCommissioner(rapidjson::Document &d)
{

}

//处理高度请求的回复
void POV::handleResponseHeight(rapidjson::Document &d)
{

}
//处理区块请求的回复
void POV::handleResponseBlock(rapidjson::Document &d)
{
	std::cout<<"收到请求回复的区块\n";
	NodeID sender=d["sender"].GetUint64();
	rapidjson::Value& block=d["data"]["block"];
	PoVBlock b;
	b.setBlock(block);
	if(sync_block_method==SequenceSync)
	{
		if(b.getHeight()==blockchain.getHeight()+1)
		{
			std::cout<<account_manager.getMyNodeID()<<"收到高度为"<<b.getHeight()<<"的区块！\n";
			//blockchain.pushbackBlock(b);
			blockchain.pushbackBlockToDatabase(b);

			/*
			else
			{
				std::cout<<"blockchain.getHeight()="<<blockchain.getHeight()<<"\n";
				std::cout<<"current_syncer.height="<<current_syncer.height<<"\n";
			}
			*/
		}
		else
		{
			std::cout<<"b.getHeight()="<<b.getHeight()<<"\n";
			std::cout<<"blockchain.getHeight()"<<blockchain.getHeight()<<"\n";
		}
	}
	else
	{
		int height=b.getHeight();
		for(int i=0;i<con_syncers.size();i++)
		{
			if(con_syncers[i].height==height)
			{
				con_syncers[i].block=b;
				con_syncers[i].prepared=true;
				con_syncers[i].prehash=b.getPreHash();
				//con_syncers[i].hash=key_manager.getHash256(block);
			}
		}
	}

}
