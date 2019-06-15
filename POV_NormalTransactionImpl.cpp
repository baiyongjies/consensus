#include "POV.h"

//正常阶段委员投票
void POV::Normal_Vote()
{
	switch(Normal_vote)
	{
		case Vote_voting:
		{
			if(account_manager.getButlerCandidateAmount()>=butler_amount)
			{
				/*
				std::cout<<network->getNodeId()<<"管家候选人数满足要求！\n";
				for(uint32_t i=0;i<butler_amount;i++)
				{
					std::string butler_pubkey=account_manager.getButlerPubkeyByNum(i);
					//std::cout<<"和管家编号对应的管家公钥："<<butler_pubkey<<"\n";
				}
				for(uint32_t i=0;i<butler_amount;i++)
				{
					NodeID butler_id=account_manager.butler_list[i].getNodeId();
					//std::cout<<"管家ID："<<butler_id<<"\n";
				}
				*/
				for(uint32_t i=0;i<butler_amount;i++)
				{
					std::string butler_pubkey=account_manager.getButlerPubkeyByNum(i);
					NodeID butler_id=account_manager.get_Butler_ID(butler_pubkey);
					if(butler_id==0)
					{
						//id为0表示没有id，需要请求ID
						//std::cout<<"发送NodeID请求!\n";
						char* ch=const_cast<char*>(butler_pubkey.c_str());
						uint32_t *map_id=(uint32_t*)ch;
						rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(butler_pubkey);
						sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
						msg.SetNull();
						msg.GetAllocator().Clear();
						delete &msg;
					}
					else
					{
						//std::cout<<account_manager.getMyNodeID()<<"发送投票给"<<butler_id<<"\n";
						std::vector<account>& butler_candidate_list=account_manager.get_Butler_Candidate_List();
						rapidjson::Document &ballot=account_manager.getBallot(vote_amount);
						rapidjson::Document& msg=msg_manager.make_Request_Vote(butler_id,ballot);
						//print_document(msg);
						sendMessage(msg,true,10,Recall_RequestVote,butler_id,&POV::handleResponseVote);
						ballot.SetNull();
						ballot.GetAllocator().Clear();
						delete &ballot;
						msg.SetNull();
						msg.GetAllocator().Clear();
						delete &msg;
					}
				}

			}
			break;
		}
		case Vote_finish:
		{
			break;
		}
	}
}
//正常阶段生成区块
void POV::Normal_GenerateBlock()
{
	switch(GenerateBlock)
	{
		case GenerateBlock_sign:
		{
			//std::cout<<"在区块签名阶段\n";
			//period_generated_block_num为0表示下个生成的区块是特殊区块
			if(period_generated_block_num==0)
			{
				b_mutex.lock();
				if(BallotPool.size()<account_manager.getCommissionerAmount()/2.0)
				{
					//std::cout<<"period_generated_block_num="<<period_generated_block_num<<"\n";
					//std::cout<<"BallotPool.size()="<<BallotPool.size()<<"\n";
					//std::cout<<"account_manager.getCommissionerAmount()/2.0="<<account_manager.getCommissionerAmount()/2.0<<"\n";
					b_mutex.unlock();
					break;
				}
				b_mutex.unlock();
			}
			//std::cout<<"在区块签名阶段——1\n";
			//cache_block_mutex.lock();
			if(cache_block==NULL)
			{
				add_sig_mutex.lock();
				PoVBlock povblock=generateBlock();
				//std::cout<<"generate block:1\n";
				cache_block=new PoVBlock();
				cache_block->copyfrom(povblock);
				//因为更新区块和产生新的生区块之间有一段距离，因此必须在这里把签名池clear一下，不然可能在
				rbs_mutex.lock();
				raw_block_signatures.clear();
				rbs_mutex.unlock();
				rbe_mutex.lock();
				raw_block_error.clear();
				rbe_mutex.unlock();
				wait_for_all_sigs_start=getCurrentTime();
				add_sig_mutex.unlock();
				//print_document(cache_block->getBlock());
			}
			//cache_block_mutex.unlock();
			//std::cout<<"在区块签名阶段——2\n";
			uint32_t commissioners_num=account_manager.getCommissionerAmount();
			//std::cout<<"commissioners_num="<<commissioners_num<<"\n";
			if(raw_block_signatures.size()+raw_block_error.size()>=commissioners_num)
			{
				//std::cout<<"raw_block_signatures.size()+raw_block_error.size()>=commissioners_num：1\n";
				if(raw_block_signatures.size()>commissioners_num/2.0)
				{
					//std::cout<<"raw_block_signatures.size()+raw_block_error.size()>=commissioners_num：2\n";
					GenerateBlock=GenerateBlock_publish;
				}
				else if(raw_block_error.size()>commissioners_num/2.0)
				{
					//std::cout<<"raw_block_signatures.size()+raw_block_error.size()>=commissioners_num：3\n";
					rbs_mutex.lock();
					collectVoteData();
					rbs_mutex.unlock();
					delete cache_block;
					cache_block=NULL;
					break;
				}
				//std::cout<<"raw_block_signatures.size()+raw_block_error.size()>=commissioners_num：4\n";
			}
			else if(getCurrentTime()-wait_for_all_sigs_start>wait_for_all_sigs_time)
			{
				//std::cout<<"getCurrentTime()-start_time>wait_for_all_sigs_time：1\n";
				if(raw_block_signatures.size()>commissioners_num/2.0)
				{
					//std::cout<<"getCurrentTime()-start_time>wait_for_all_sigs_time：2\n";
					GenerateBlock=GenerateBlock_publish;
				}
				else if(raw_block_error.size()>commissioners_num/2.0)
				{
					//std::cout<<"getCurrentTime()-start_time>wait_for_all_sigs_time：3\n";
					rbs_mutex.lock();
					collectVoteData();
					rbs_mutex.unlock();
					delete cache_block;
					cache_block=NULL;
					break;
				}
				//std::cout<<"getCurrentTime()-start_time>wait_for_all_sigs_time：4\n";
			}

			//std::cout<<"在区块签名阶段——3\n";
			for(uint32_t i=0;i<commissioners_num;i++)
			{
				account comm=account_manager.getCommissioner(i);
				//std::cout<<"comm.id="<<comm.getNodeId()<<"\n";
				if(comm.getNodeId()==0)
				{
					//std::cout<<"发送NodeID请求!\n";
					char* ch=const_cast<char*>(comm.getPubKey().c_str());
					uint32_t *map_id=(uint32_t*)ch;
					rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(comm.getPubKey());
					sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
				}
				else
				{
					//std::cout<<"发送生区块给"<<comm.getNodeId()<<"\n";
					//cache_block_mutex.lock();
					//std::cout<<"cache_block->getRawBlock():1\n";
					rapidjson::Document& raw_zero_block=cache_block->getRawBlock();
					//std::cout<<"cache_block->getRawBlock():2\n";
					//cache_block_mutex.unlock();
					bool sig_existed=false;
					rbs_mutex.lock();
					for(int i=0;i<raw_block_signatures.size();i++)
					{
						std::string key=raw_block_signatures[i].pubkey;
						if(comm.getPubKey()==key)
						{
							sig_existed=true;
							break;
						}
					}
					rbs_mutex.unlock();
					if(sig_existed)
					{
						raw_zero_block.SetNull();
						raw_zero_block.GetAllocator().Clear();
						delete &raw_zero_block;
					}
					else
					{
						int height=cache_block->getHeight();
						double time = getCurrentTime();
						rapidjson::Document& msg=msg_manager.make_Request_Block_Signature((NodeID)comm.getNodeId(),raw_zero_block);
						//print_document(msg);
						sendMessage(msg,true,10,Recall_RequestBlockSignature,(NodeID)comm.getNodeId(),&POV::handleResponseBlockSignature);
						raw_zero_block.SetNull();
						raw_zero_block.GetAllocator().Clear();
						delete &raw_zero_block;
						msg.SetNull();
						msg.GetAllocator().Clear();
						delete &msg;
					}
				}
			}
			//std::cout<<"在区块签名阶段——4\n";
			break;
		}
		case GenerateBlock_publish:
		{
			//std::cout<<"在区块发布阶段\n";
			uint32_t commissioners_num=account_manager.getCommissionerAmount();
			//std::cout<<"GenerateBlock_publish：1\n";
			if(raw_block_signatures.size()<=commissioners_num/2.0)
			{
				//std::cout<<"GenerateBlock_publish：2\n";
				if(raw_block_error.size()>commissioners_num/2.0)
				{
					//std::cout<<"GenerateBlock_publish：3\n";
					rbs_mutex.lock();
					collectVoteData();
					rbs_mutex.unlock();
					delete cache_block;
					cache_block=NULL;
					GenerateBlock=GenerateBlock_sign;
					//raw_block_signatures.clear();
				}
				//std::cout<<"GenerateBlock_publish：4\n";
				break;
			}

			//cache_block_mutex.lock();
			if(!cache_block->complete)
			{

				rbs_mutex.lock();
				cache_block->setSigs(raw_block_signatures);
				try{
				collectVoteData();
				}
				catch(...)
				{
					std::cout<<"收集投票数据异常!\n";

				}
				rbs_mutex.unlock();
				rapidjson::Document& txs=cache_block->getPoVHeader().getSignatures();
				uint32_t next_butler_num=getNextButler(txs);
				//std::cout<<"butler_num="<<next_butler_num<<"\n";
				cache_block->setNextButler(next_butler_num);
				cache_block->setTe(getTe(txs));
				txs.SetNull();
				txs.GetAllocator().Clear();
				delete &txs;
				cache_block->complete=true;
			}
			//std::cout<<"cache_block->getBlock():1\n";
			rapidjson::Document& complete_block=cache_block->getBlock();
			//std::cout<<"cache_block->getBlock():2\n";
			//cache_block_mutex.unlock();
			rapidjson::Document& msg=msg_manager.make_Request_Publish_Block(complete_block);
			//sendMessage(msg,false,30,Recall_RequestBlockSignature,0,&POV::handleResponseBlockSignature);
			//sendMessage(msg,false,30,Recall_RequestBlockSignature,0,&POV::handleResponseBlockSignature);
			//unsigned int index=msg["index"].GetUint();
//			for(int i=0;i<account_manager.getCommissionerAmount();i++)
//			{
//				account com=account_manager.getCommissioner(i);
//				NodeID com_id=com.getNodeId();
//				//msg["receiver"].SetUint64(com.getNodeId());
//				//msg["index"].SetUint(index+i);
//				//std::cout<<"publish_block: receiver="<<com.getNodeId()<<"\n";
//				//std::cout<<"publish_block: index="<<index+i<<"\n";
//				//msg_manager.setIndex(index+i);
//				rapidjson::Document& msg=msg_manager.make_Request_Publish_Block(com_id,complete_block);
//
//				sendMessage(msg,false,30,Recall_RequestBlockSignature,0,&POV::handleResponseBlockSignature);
//				delete &msg;
//			}
			sendMessage(msg,false,30,Recall_RequestBlockSignature,0,&POV::handleResponseBlockSignature);
			msg.SetNull();
			msg.GetAllocator().Clear();
			delete &msg;
			GenerateBlock=GenerateBlock_finish;
			complete_block.SetNull();
			complete_block.GetAllocator().Clear();
			delete &complete_block;
			break;
		}
		case GenerateBlock_finish:
		{
			//std::cout<<"在区块生成结束阶段\n";
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
			rbe_mutex.lock();
			raw_block_error.clear();
			rbe_mutex.unlock();
			break;
		}
	}/*switch(Normal_vote)*/
}
//正常阶段发布交易
void POV::Normal_PublishTransaction()
{
	//随机发布正常交易，content字段只有一个自定义长度的随机字符串，测试用
	if(prob_generate_normal_tx<1)
	{
		double r=getRandomNum();
		//std::cout<<"在"<<network->getNodeId()<<"的Normal状态中：1!\n";

		if(r<prob_generate_normal_tx)
		{
			//testRegistryNDN();
			rapidjson::Document& msg=msg_manager.make_Request_Normal(100);
			for(int i=0;i<account_manager.getButlerAmount();i++)
			{
				account& butler=account_manager.getButler(i);
				msg["receiver"].SetUint64(butler.getNodeId());
				sendMessage(msg,false,100,Recall_RequestNormal,0,&POV::handleResponseNormal);
			}
			msg.SetNull();
			msg.GetAllocator().Clear();
			delete &msg;
			/*
			//uint32_t i=rand()%account_manager.getButlerAmount();
			for(uint32_t i=0;i<account_manager.getButlerAmount();i++)
			{
				account& butler=account_manager.getButler(i);
				NodeID id=butler.getNodeId();
				if(id==0)
				{
					std::cout<<"发送NodeID请求!\n";
					char* ch=const_cast<char*>(butler.getPubKey().c_str());
					uint32_t *map_id=(uint32_t*)ch;
					rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(butler.getPubKey());
					sendMessage(msg,true,100,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
					//delete map_id;
					delete &msg;
				}
				else
				{
					std::cout<<"发送Normal交易请求!\n";
					rapidjson::Document& msg=msg_manager.make_Request_Normal(id,tx_len);
					sendMessage(msg,true,100,Recall_RequestNormal,id,&POV::handleResponseNormal);
					delete &msg;
				}
			}
			*/
		}

	}
	else
	{
		for(int i=0;i<prob_generate_normal_tx;i++)
		{
			rapidjson::Document& msg=msg_manager.make_Request_Normal_for_Test(0);
			for(int i=0;i<account_manager.getButlerAmount();i++)
			{
				account& butler=account_manager.getButler(i);
				msg["receiver"].SetUint64(butler.getNodeId());
				sendMessage(msg,false,100,Recall_RequestNormal,0,&POV::handleResponseNormal);
			}
			msg.SetNull();
			msg.GetAllocator().Clear();
			delete &msg;
		}
	}

}

//正常阶段发布普通事务，保证自己的交易缓存池为满
void POV::Normal_PublishTransactionKeepPoolFull()
{
	//std::cout<<"执行保持满的交易缓存池的交易发布算法\n";
	//norm_mutex.lock();
	if(lru_NormalPool.is_full())
		return;
	int nums=lru_NormalPool.getNums();
	int capacity=lru_NormalPool.getCapacity();
	for(int i=0;i<capacity-nums-1;i++)
	{
		rapidjson::Document& msg=msg_manager.make_Request_Normal_for_Test(network->getNodeId());
		//print_document(msg);
		sendMessage(msg,false,100,Recall_RequestNormal,0,&POV::handleResponseNormal);
		msg.SetNull();
		msg.GetAllocator().Clear();
		delete &msg;
	}

	//norm_mutex.unlock();
	//handleMessage(msg);
}
//正常阶段申请成为委员
void POV::Normal_ApplyCommissioner()
{
	switch(normal_apply_commissioner_state)
	{
		case ApplyCommissioner_apply_signatures:
		{
			//把metadata 存放到Apply_Commissioner_Metadata中
			if(Apply_Commissioner_Metadata.IsNull())
			{
				//如果Apply_Commissioner_Metadata为空则把metadata复制到Apply_Commissioner_Metadata中保存起来。
				rapidjson::Document& metadata=msg_manager.get_Apply_Commissioner_Metadata();
				rapidjson::Document::AllocatorType& a=metadata.GetAllocator();
				Apply_Commissioner_Metadata.GetAllocator().Clear();
				Apply_Commissioner_Metadata.CopyFrom(metadata,Apply_Commissioner_Metadata.GetAllocator());
				try
				{
					metadata.SetNull();
					metadata.GetAllocator().Clear();
					delete &metadata;
				}
				catch(...)
				{
					throw "ApplyCommissioner_apply_signatures中删除metadata异常\n";
				}
			}

			uint32_t commissioners_num=account_manager.getCommissionerAmount();
			for(uint32_t i=0;i<commissioners_num;i++)
			{
				account& com=account_manager.getCommissioner(i);
				std::string comm_pubkey=com.getPubKey();
				NodeID comm_id=com.getNodeId();
				if(comm_id==0)
				{
					std::cout<<"发送NodeID请求!\n";
					char* ch=const_cast<char*>(comm_pubkey.c_str());
					uint32_t *map_id=(uint32_t*)ch;
					rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(comm_pubkey);
					sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
				}
				else
				{
					/*
					//把Apply_Commissioner_Metadata复制到新建的metadata中.
					rapidjson::Document metadata;
					rapidjson::Document::AllocatorType& a=Apply_Commissioner_Metadata.GetAllocator();
					metadata.SetNull();
					metadata.GetAllocator().Clear();
					metadata.CopyFrom(Apply_Commissioner_Metadata,metadata.GetAllocator());
					//创建消息请求签名

					rapidjson::Document& d=msg_manager.make_Request_Apply_Commissioner_Signature((NodeID)com.getNodeId(),metadata);
					*/
					rapidjson::Document& d=msg_manager.make_Request_Apply_Commissioner_Signature((NodeID)com.getNodeId(),Apply_Commissioner_Metadata);
					//NodeID id=com.getNodeId();
					sendMessage(d,true,10,Recall_RequestApplyCommissionerSignature,(NodeID)com.getNodeId(),&POV::handleResponseApplyCommissionerSignature);
					d.SetNull();
					d.GetAllocator().Clear();
					delete &d;
				}
			}
			break;
		}
		case ApplyCommissioner_final_apply:
		{
			//std::cout<<network->getNodeId()<<"收集到所有委员的签名\n";
			//设置Apply_Commissioner_Transaction，并发送给代理委员
			if(Apply_Commissioner_Transaction.IsNull())
			{
				Apply_Commissioner_Transaction.GetAllocator().Clear();
				std::cout<<network->getNodeId()<<"设置Apply_Commissioner_Transaction\n";
				Apply_Commissioner_Transaction.SetObject();
				rapidjson::Document::AllocatorType &allocator = Apply_Commissioner_Transaction.GetAllocator();
				rapidjson::Document::AllocatorType &a = Apply_Commissioner_Metadata.GetAllocator();
				//统一每个交易的json数据中的第一层都加上metatype，方便后面代码处理
				rapidjson::Value _metatype("ApplyCommissioner",a);
				Apply_Commissioner_Transaction.AddMember("metatype",_metatype,a);
				rapidjson::Value metadata(Apply_Commissioner_Metadata,allocator);
				Apply_Commissioner_Transaction.AddMember("metadata",metadata,allocator);
				//print_document(Apply_Commissioner_Transaction);
				rapidjson::Value sig_array(rapidjson::kArrayType);
				acs_mutex.lock();
				for(uint32_t i=0;i<Apply_Commissioner_Signatures.size();i++)
				{
					std::string str_pubkey=Apply_Commissioner_Signatures.at(i).pubkey;
					std::string str_sig=Apply_Commissioner_Signatures.at(i).sig;
					rapidjson::Value sig;
					sig.SetObject();
					rapidjson::Value pubkeyValue(str_pubkey.c_str(),str_pubkey.size(),allocator);
					sig.AddMember("pubkey",pubkeyValue,allocator);
					rapidjson::Value sigValue(str_sig.c_str(),str_sig.size(),allocator);
					sig.AddMember("sig",sigValue,allocator);
					sig_array.PushBack(sig,allocator);
				}
				acs_mutex.unlock();
				Apply_Commissioner_Transaction.AddMember("sigs",sig_array,allocator);
				std::cout<<"完成设置Apply_Commissioner_Transaction！\n";
				//print_document(Apply_Commissioner_Transaction);
			}

			//print_document(temp_doc);
			for(uint32_t i=0;i<account_manager.getButlerAmount();i++)
			{
				std::string butler_pubkey=account_manager.getButlerPubkeyByNum(i);
				NodeID butler_id=account_manager.get_Butler_ID(butler_pubkey);
				if(butler_id==0)
				{
					std::cout<<"发送NodeID请求!\n";
					char* ch=const_cast<char*>(butler_pubkey.c_str());
					uint32_t *map_id=(uint32_t*)ch;
					rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(butler_pubkey);
					sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
				}
				else
				{
					//std::cout<<"发送申请委员交易"<<1<<"\n";
					//rapidjson::Document::AllocatorType &allocator = Apply_Commissioner_Transaction.GetAllocator();
					//rapidjson::Document &temp_doc=*(new rapidjson::Document());
					//rapidjson::Document::AllocatorType &allocator =temp_doc.GetAllocator();
					//temp_doc.CopyFrom(Apply_Commissioner_Transaction,allocator);
					rapidjson::Document& msg=msg_manager.make_Request_Apply_Commissioner(butler_id,Apply_Commissioner_Transaction);
					sendMessage(msg,true,10,Recall_RequestApplyCommissioner,butler_id,&POV::handleResponseApplyCommissioner);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
					//std::cout<<"发送申请委员交易"<<2<<"\n";

				}
			}
			break;
		}
		case ApplyCommissioner_finish:
		{
//			double r=getRandomNum();
//			if(r<prob_apply_commissioner)
//			{
//				std::cout<<"in applycommissioner_finish state\n";
//				if(!Apply_Commissioner_Transaction.IsNull())
//				{
//					std::cout<<"in applycommissioner_finish state 1\n";
//					Apply_Commissioner_Transaction.SetNull();
//					std::cout<<"in applycommissioner_finish state 2\n";
//					Apply_Commissioner_Transaction.GetAllocator().Clear();
//				}
//				std::cout<<"in applycommissioner_finish state 3\n";
//				acs_mutex.lock();
//				Apply_Commissioner_Signatures.clear();
//				acs_mutex.unlock();
//				std::cout<<"in applycommissioner_finish state 4\n";
//				normal_apply_commissioner_state=ApplyCommissioner_apply_signatures;
//			}
			break;
		}
	}
	/*Normal_ApplyCommissioner*/
}
//正常阶段申请成为管家候选
void POV::Normal_ApplyButlerCandidate()
{
	switch(normal_apply_butler_candidate_state)
	{
		case ApplyButlerCandidate_apply_recommendation_letter:
		{
			uint32_t comm_num=rand()%account_manager.getCommissionerAmount();
			account comm=account_manager.getCommissioner(comm_num);
			NodeID comm_ID=comm.getNodeId();
			if(comm_ID==0)
			{
				std::string pubkey=comm.getPubKey();
				char* ch=const_cast<char*>(pubkey.c_str());
				uint32_t *map_id=(uint32_t*)ch;
				rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(pubkey);
				sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
				msg.SetNull();
				msg.GetAllocator().Clear();
				delete &msg;
			}
			else
			{
				rapidjson::Document &letter_request=msg_manager.make_Request_Recommend_Letter(comm_ID);
				sendMessage(letter_request,true,10,Recall_RequestApplyRecommendationLetter,comm_ID,&POV::handleResponseRecommendationLetter);
				letter_request.SetNull();
				letter_request.GetAllocator().Clear();
				delete &letter_request;
			}
			break;
		}
		case ApplyButlerCandidate_apply_signatures:
		{
			//std::cout<<"申请管家候选阶段：ApplyButlerCandidate_apply_signatures\n";
			//rapidjson::Value &letter=RecommendationLetter["data"];
			uint32_t commissioners_num=account_manager.getCommissionerAmount();
			for(uint32_t i=0;i<commissioners_num;i++)
			{
				account comm=account_manager.getCommissioner(i);
				NodeID comm_ID=comm.getNodeId();
				if(comm_ID==0)
				{
					std::string pubkey=comm.getPubKey();
					char* ch=const_cast<char*>(pubkey.c_str());
					uint32_t *map_id=(uint32_t*)ch;
					rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(pubkey);
					sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
				}
				else
				{
					//account comm=account_manager.getCommissioner(i);
					rapidjson::Document& msg=msg_manager.make_Request_Apply_Butler_Candidate_Signature((NodeID)comm.getNodeId(),RecommendationLetter);
					//print_document(msg);
					sendMessage(msg,true,10,Recall_RequestApplyButlerCandidateSignature,(NodeID)comm.getNodeId(),&POV::handleResponseApplyButlerCandidateSignature);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
				}
			}

			break;
		}
		case ApplyButlerCandidate_final_apply:
		{
			rapidjson::Document &data=*(new rapidjson::Document);
			data.SetObject();
			rapidjson::Document::AllocatorType &allocator=data.GetAllocator();

			rapidjson::Value& _metatype=*(new rapidjson::Value("ApplyButlerCandidate",allocator));
			data.AddMember("metatype",_metatype,allocator);

			rapidjson::Document &recommendation_letter=*(new rapidjson::Document);
			recommendation_letter.CopyFrom(RecommendationLetter,recommendation_letter.GetAllocator());
			data.AddMember("recommendation_letter",recommendation_letter,allocator);

			rapidjson::Value sigs(rapidjson::kArrayType);
			abcs_mutex.lock();
			for(uint32_t i=0;i<Apply_Butler_Candidate_Signatures.size()/2;i++)
			{
				std::string pubkey=Apply_Butler_Candidate_Signatures[i].pubkey;
				std::string sig=Apply_Butler_Candidate_Signatures[i].sig;
				rapidjson::Value &doc=*(new rapidjson::Value);
				doc.SetObject();
				//rapidjson::Document::AllocatorType &a=doc.GetAllocator();
				rapidjson::Value _pubkey(pubkey.c_str(),pubkey.size(),allocator);
				rapidjson::Value _sig(sig.c_str(),sig.size(),allocator);
				doc.AddMember("pubkey",_pubkey,allocator);
				doc.AddMember("sig",_sig,allocator);
				sigs.PushBack(doc,allocator);
			}
			abcs_mutex.unlock();
			data.AddMember("sigs",sigs,allocator);
			for(uint32_t i=0;i<account_manager.getButlerAmount();i++)
			{
				account butler=account_manager.getButler(i);
				NodeID id=butler.getNodeId();
				if(id==0)
				{
					std::string pubkey=butler.getPubKey();
					char* ch=const_cast<char*>(pubkey.c_str());
					uint32_t *map_id=(uint32_t*)ch;
					rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(pubkey);
					sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
				}
				else
				{
					rapidjson::Document& copy_data=*(new rapidjson::Document);
					copy_data.CopyFrom(data,copy_data.GetAllocator());
					rapidjson::Document& msg=msg_manager.make_Request_Apply_Butler_Candidate(id,copy_data);
					//print_document(msg);
					sendMessage(msg,true,20,Recall_RequestApplyButlerCandidate,id,&POV::handleResponseApplyButlerCandidate);
					copy_data.SetNull();
					copy_data.GetAllocator().Clear();
					delete &copy_data;
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
				}
			}
			data.SetNull();
			data.GetAllocator().Clear();
			delete &data;
			break;
		}
		case ApplyButlerCandidate_finish:
		{
//			double r=getRandomNum();
//			if(r<prob_apply_commissioner)
//			{
//				std::cout<<"applybutlercandidate_finish\n";
//				if(!RecommendationLetter.IsNull())
//				{
//					RecommendationLetter.Clear();
//					RecommendationLetter.SetNull();
//				}
//				abcs_mutex.lock();
//				Apply_Butler_Candidate_Signatures.clear();
//				abcs_mutex.unlock();
//				normal_apply_butler_candidate_state=ApplyButlerCandidate_apply_recommendation_letter;
//			}

			break;
		}
	}/*switch(normal_apply_butler_candidate_state)*/
}
//正常阶段退出委员
void POV::Normal_QuitCommissioner()
{
	//std::cout<<"在退出委员交易中！\n";
	switch(normal_quit_commissioner_state)
	{
		case QuitCommissioner_apply:
		{
			//std::cout<<"Normal_QuitButlerCandidate 1\n";
			if(Quit_Commissioner_Metadata.IsNull())
			{
				//std::cout<<"Normal_QuitButlerCandidate 11\n";
				Quit_Commissioner_Metadata.SetObject();
				//std::cout<<"Normal_QuitButlerCandidate 12\n";
				rapidjson::Document& metadata=msg_manager.get_Quit_Commissioner_Metadata();
				//std::cout<<"Normal_QuitButlerCandidate 13\n";
				Quit_Commissioner_Metadata.CopyFrom(metadata,Quit_Commissioner_Metadata.GetAllocator());
				//std::cout<<"Normal_QuitButlerCandidate 14\n";
				metadata.SetNull();
				metadata.GetAllocator().Clear();
				delete &metadata;
			}
			//std::cout<<"Normal_QuitButlerCandidate 2\n";
			//rapidjson::Document& metadata=*(new rapidjson::Document());
			//metadata.CopyFrom(Quit_Butler_Candidate_Metadata,metadata.GetAllocator());
			//发送给所有管家
			for(uint32_t i=0;i<account_manager.getButlerAmount();i++)
			{
				std::string butler_pubkey=account_manager.getButlerPubkeyByNum(i);
				NodeID butler_id=account_manager.get_Butler_ID(butler_pubkey);
				if(butler_id==0)
				{
					//std::cout<<"在退出委员交易中——请求NodeID！\n";
					//std::cout<<"发送NodeID请求!\n";
					char* ch=const_cast<char*>(butler_pubkey.c_str());
					uint32_t *map_id=(uint32_t*)ch;
					rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(butler_pubkey);
					sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
				}
				else
				{
					//std::cout<<"发送退出管家后选交易"<<1<<"\n";
					//rapidjson::Document::AllocatorType &allocator = Apply_Commissioner_Transaction.GetAllocator();
					/*
					rapidjson::Document &temp_doc=*(new rapidjson::Document());
					rapidjson::Document::AllocatorType &allocator =temp_doc.GetAllocator();
					temp_doc.CopyFrom(Quit_Butler_Candidate_Metadata,allocator);
					*/
					rapidjson::Document& msg=msg_manager.make_Request_Quit_Commissioner(butler_id,Quit_Commissioner_Metadata);
					sendMessage(msg,true,10,Recall_RequestQuitCommissioner,butler_id,&POV::handleResponseQuitCommissioner);
					//std::cout<<"发送退出委员请求\n";
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
					//std::cout<<"发送退出管家候选交易"<<2<<"\n";
				}
				//std::cout<<"Normal_QuitButlerCandidate 3\n";
			}
			break;
		}
		case QuitCommissioner_finish:
		{
//			double r=getRandomNum();
//			if(r<prob_quit_commissioner)
//			{
//				normal_quit_commissioner_state=QuitCommissioner_apply;
//				if(Quit_Commissioner_Metadata.IsObject())
//				{
//					Quit_Commissioner_Metadata.Clear();
//					Quit_Commissioner_Metadata.SetNull();
//				}
//			}
			break;
		}

	}
}
//正常阶段退出管家候选
void POV::Normal_QuitButlerCandidate()
{
	switch(normal_quit_butler_candidate_state)
	{
		case QuitButlerCandidate_apply:
		{
			//std::cout<<"Normal_QuitButlerCandidate 1\n";
			if(Quit_Butler_Candidate_Metadata.IsNull())
			{
				//std::cout<<"Normal_QuitButlerCandidate 11\n";
				Quit_Butler_Candidate_Metadata.SetObject();
				//std::cout<<"Normal_QuitButlerCandidate 12\n";
				rapidjson::Document& metadata=msg_manager.get_Quit_Butler_Candidate_Metadata();
				//std::cout<<"Normal_QuitButlerCandidate 13\n";
				Quit_Butler_Candidate_Metadata.CopyFrom(metadata,Quit_Butler_Candidate_Metadata.GetAllocator());
				//std::cout<<"Normal_QuitButlerCandidate 14\n";
				metadata.SetNull();
				metadata.GetAllocator().Clear();
				delete &metadata;
			}
			//std::cout<<"Normal_QuitButlerCandidate 2\n";
			//rapidjson::Document& metadata=*(new rapidjson::Document());
			//metadata.CopyFrom(Quit_Butler_Candidate_Metadata,metadata.GetAllocator());
			//发送给所有管家
			for(uint32_t i=0;i<account_manager.getButlerAmount();i++)
			{
				std::string butler_pubkey=account_manager.getButlerPubkeyByNum(i);
				NodeID butler_id=account_manager.get_Butler_ID(butler_pubkey);
				if(butler_id==0)
				{
					//std::cout<<"发送NodeID请求!\n";
					char* ch=const_cast<char*>(butler_pubkey.c_str());
					uint32_t *map_id=(uint32_t*)ch;
					rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(butler_pubkey);
					sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
				}
				else
				{
					//std::cout<<"发送退出管家后选交易"<<1<<"\n";
					//rapidjson::Document::AllocatorType &allocator = Apply_Commissioner_Transaction.GetAllocator();
					/*
					rapidjson::Document &temp_doc=*(new rapidjson::Document());
					rapidjson::Document::AllocatorType &allocator =temp_doc.GetAllocator();
					temp_doc.CopyFrom(Quit_Butler_Candidate_Metadata,allocator);
					*/
					rapidjson::Document& msg=msg_manager.make_Request_Quit_Butler_Candidate(butler_id,Quit_Butler_Candidate_Metadata);
					sendMessage(msg,true,10,Recall_RequestQuitButlerCandidate,butler_id,&POV::handleResponseQuitButlerCandidate);
					msg.SetNull();
					msg.GetAllocator().Clear();
					delete &msg;
					//std::cout<<"发送退出管家候选交易"<<2<<"\n";
				}
				//std::cout<<"Normal_QuitButlerCandidate 3\n";
			}
			break;
		}
		case QuitButlerCandidate_finish:
		{
//			double r=getRandomNum();
//			if(r<prob_quit_butler_candidate)
//			{
//				normal_quit_butler_candidate_state=QuitButlerCandidate_apply;
//				if(Quit_Butler_Candidate_Metadata.IsObject())
//				{
//					Quit_Butler_Candidate_Metadata.Clear();
//					Quit_Butler_Candidate_Metadata.SetNull();
//				}
//			}

			break;
		}
	}
}
