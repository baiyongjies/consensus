#include "POV.h"

//创世区块阶段申请成为委员
void POV::Initial_ApplyCommissioner()
{
	switch(gegenerate_first_bolck_apply_commissioner_state)
	{
		case ApplyCommissioner_apply_signatures:
		{
			//std::cout<<"申请委员：1!\n";
			//把metadata 存放到Apply_Commissioner_Metadata中
			if(Apply_Commissioner_Metadata.IsNull())
			{
				//如果Apply_Commissioner_Metadata为空则把metadata复制到Apply_Commissioner_Metadata中保存起来。
				rapidjson::Document& metadata=msg_manager.get_Apply_Commissioner_Metadata();
				//rapidjson::Document::AllocatorType& a=metadata.GetAllocator();
				Apply_Commissioner_Metadata.GetAllocator().Clear();
				Apply_Commissioner_Metadata.CopyFrom(metadata,Apply_Commissioner_Metadata.GetAllocator());
				delete &metadata;
			}

			uint32_t commissioners_num=account_manager.getCommissionerAmount();

			//std::cout<<"commissioners_num:"<<commissioners_num<<"\n";
			for(uint32_t i=0;i<commissioners_num;i++)
			{
				account& com=account_manager.getCommissioner(i);
				//std::cout<<"com.id="<<com.getNodeId()<<" com.pubkey="<<com.getPubKey()<<"\n";
				bool existed=false;
				acs_mutex.lock();
				for(auto j=Apply_Commissioner_Signatures.begin();j!=Apply_Commissioner_Signatures.end();j++)
				{
					//std::cout<<"pubkey 1="<<j->pubkey<<"\n";
					//std::cout<<"pubkey 2="<<com.getPubKey()<<"\n";
					if(j->pubkey==com.getPubKey())
					{
						existed=true;
						break;
					}
				}
				acs_mutex.unlock();
				if(!existed)
				{
					rapidjson::Document& d=msg_manager.make_Request_Apply_Commissioner_Signature((NodeID)com.getNodeId(),Apply_Commissioner_Metadata);
					//NodeID id=com.getNodeId();
					sendMessage(d,true,20,Recall_RequestApplyCommissionerSignature,(NodeID)com.getNodeId(),&POV::handleResponseApplyCommissionerSignature);
					delete &d;
					//std::cout<<(NodeID)network->getNodeId()<<"向"<<(NodeID)com.getNodeId()<<"请求申请委员签名\n";
				}

			}
			break;
		}
		case ApplyCommissioner_final_apply:
		{
			//std::cout<<"申请委员：2!\n";
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
				rapidjson::Value metadata(Apply_Commissioner_Metadata,a);
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
			//rapidjson::Document::AllocatorType &allocator = Apply_Commissioner_Transaction.GetAllocator();
			//rapidjson::Document temp_doc;
			//temp_doc.CopyFrom(Apply_Commissioner_Transaction,allocator);
			//print_document(temp_doc);
			rapidjson::Document& msg=msg_manager.make_Request_Apply_Commissioner(AgentCommissioner,Apply_Commissioner_Transaction);
			sendMessage(msg,true,10,Recall_RequestApplyCommissioner,AgentCommissioner,&POV::handleResponseApplyCommissioner);
			delete &msg;
			//gegenerate_first_bolck_apply_commissioner_state=ApplyCommissioner_finish;
			break;
		}
		case ApplyCommissioner_finish:
		{
			//std::cout<<"申请委员：3!\n";
			break;
		}
	}
	/*GenerateFirstBlock_ApplyCommissioner*/
}
//创世区块阶段申请管家候选
void POV::Initial_ApplyButlerCandidate()
{
	switch(generate_first_bolck_apply_butler_candidate_state)
	{
		std::cout<<network->getNodeId()<<"申请管家候选：1!\n";
		case ApplyButlerCandidate_apply_recommendation_letter:
		{
			//生成创世区块阶段的请求推荐信是发送给随机的一个委员
			uint32_t comm_num=rand()%init_commissioner_size;
			NodeID comm_ID=Initial_Commissioner_Nodes[comm_num];

			rapidjson::Document &letter_request=msg_manager.make_Request_Recommend_Letter(comm_ID);
			sendMessage(letter_request,true,20,Recall_RequestApplyRecommendationLetter,comm_ID,&POV::handleResponseRecommendationLetter);
			try
			{
				delete &letter_request;
			}catch(...)
			{
				throw "ApplyButlerCandidate_apply_recommendation_letter删除letter_request错误\n";
			}
			break;
		}
		case ApplyButlerCandidate_apply_signatures:
		{
			//std::cout<<network->getNodeId()<<"申请管家候选：2!\n";
			//std::cout<<"申请管家候选阶段：ApplyButlerCandidate_apply_signatures\n";
			//rapidjson::Value &letter=RecommendationLetter["data"];
			//申请管家候选签名发送给所有委员
			uint32_t commissioners_num=account_manager.getCommissionerAmount();
			for(uint32_t i=0;i<commissioners_num;i++)
			{
				account comm=account_manager.getCommissioner(i);
				bool existed=false;
				abcs_mutex.lock();
				for(auto i=Apply_Butler_Candidate_Signatures.begin();i!=Apply_Butler_Candidate_Signatures.end();i++)
				{
					if(i->pubkey==comm.getPubKey())
					{
						existed=true;
						break;
					}
				}
				abcs_mutex.unlock();
				if(!existed)
				{
					//std::cout<<network->getNodeId()<<" 委员"<<(NodeID)comm.getNodeId()<<"请求申请委员签名\n";
					rapidjson::Document& msg=msg_manager.make_Request_Apply_Butler_Candidate_Signature((NodeID)comm.getNodeId(),RecommendationLetter);
					//print_document(msg);
					sendMessage(msg,true,20,Recall_RequestApplyButlerCandidateSignature,(NodeID)comm.getNodeId(),&POV::handleResponseApplyButlerCandidateSignature);

					try
					{
						delete &msg;
					}catch(...)
					{
						throw "ApplyButlerCandidate_apply_signatures删除msg错误\n";
					}
				}
			}
			break;
		}
		case ApplyButlerCandidate_final_apply:
		{
			//最终的管家候选申请交易包含一个推荐信，以及超过一半委员的签名，最终发送给所有管家
			//std::cout<<"申请管家候选：3!\n";
			rapidjson::Document &data=*(new rapidjson::Document);
			data.SetObject();
			rapidjson::Document::AllocatorType &allocator=data.GetAllocator();

			rapidjson::Value& _metatype=*(new rapidjson::Value("ApplyButlerCandidate",allocator));
			data.AddMember("metatype",_metatype,allocator);

			rapidjson::Value &recommendation_letter=*(new rapidjson::Value);
			recommendation_letter.CopyFrom(RecommendationLetter,allocator);
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
			//rapidjson::Document& msg=msg_manager.make_Request_Apply_Butler_Candidate(AgentCommissioner,data);
			//print_document(msg);
			//sendMessage(msg,true,10,Recall_RequestApplyButlerCandidate,AgentCommissioner,&POV::handleResponseApplyButlerCandidate);

			for(uint32_t i=0;i<account_manager.getCommissionerAmount();i++)
			{
				NodeID id=account_manager.getCommissioner(i).getNodeId();
				//rapidjson::Document& copy_data=*(new rapidjson::Document);
				//copy_data.CopyFrom(data,copy_data.GetAllocator());
				rapidjson::Document& msg=msg_manager.make_Request_Apply_Butler_Candidate(id,data);
				//print_document(msg);
				sendMessage(msg,true,10,Recall_RequestApplyButlerCandidate,id,&POV::handleResponseApplyButlerCandidate);
				try{
					delete &msg;
				}catch(...)
				{
					throw "ApplyButlerCandidate_final_apply删除msg错误\n";
				}
			}
			try
			{
				delete &data;
			}catch(...)
			{
				throw "ApplyButlerCandidate_final_apply删除data错误\n";
			}
			//generate_first_bolck_apply_butler_candidate_state=ApplyButlerCandidate_finish;
			break;
		}
		case ApplyButlerCandidate_finish:
		{
			//std::cout<<"申请管家候选：4!\n";
			break;
		}
	}/*switch(generate_first_bolck_apply_butler_candidate_state)*/
}
//创世区块阶段投票选举管家
void POV::Initial_Vote()
{
	switch(GenerateFirstBlock_vote)
	{
		case Vote_voting:
		{
			//std::cout<<network->getNodeId()<<"投票：1!\n";
			//只有管家候选人数等于预设的管家人数才可以进行投票
			if(account_manager.getButlerCandidateAmount()==init_butler_candidate_size)
			{
				//std::cout<<network->getNodeId()<<"节点的委员人数满足要求，管家候选人数满足要求！\n";
				std::vector<account>& butler_candidate_list=account_manager.get_Butler_Candidate_List();
				rapidjson::Document &ballot=account_manager.getBallot(vote_amount);
				rapidjson::Document& msg=msg_manager.make_Request_Vote(AgentCommissioner,ballot);
				//print_document(msg);
				sendMessage(msg,true,20,Recall_RequestVote,AgentCommissioner,&POV::handleResponseVote);
				try
				{
					delete &ballot;
					delete &msg;
				}
				catch(...)
				{
					throw "Vote_voting发生删除ballot和msg异常\n";
				}
			}
			break;
		}
		case Vote_finish:
		{
			//std::cout<<"投票：2!\n";
			break;
		}
	}
}
//创世区块阶段生成区块
void POV::Initial_GenerateBlock()
{
	switch(GenerateFirstBlock)
	{
		case GenerateBlock_sign:
		{
			//生成区块并获取委员的签名
			//std::cout<<"生成创世区块：1!\n";
			//std::cout<<"init_commissioner_size="<<init_commissioner_size<<"\n";
			//std::cout<<"ApplyCommissionerPool.size="<<ApplyCommissionerPool.size()<<"\n";
			//std::cout<<"init_butler_candidate_size="<<init_butler_candidate_size<<"\n";
			//std::cout<<"ApplyButlerCandidatePool.size="<<ApplyButlerCandidatePool.size()<<"\n";
			if(ApplyCommissionerPool.size()<init_commissioner_size)
			{
				break;
			}
			if(ApplyButlerCandidatePool.size()<init_butler_candidate_size)
			{
				break;
			}
			b_mutex.lock();
			if(BallotPool.size()<init_commissioner_size)
			{
				b_mutex.unlock();
				break;
			}
			b_mutex.unlock();
			//cache_block_mutex.lock();
			if(cache_block==NULL)
			{
				PoVBlock povblock=generateFirstBlock();
				cache_block=new PoVBlock();
				cache_block->copyfrom(povblock);
			}
			else if(cache_block->getHeight()!=blockchain.getHeight()+1)
			{
				delete cache_block;
				PoVBlock povblock=generateFirstBlock();
				cache_block=new PoVBlock();
				cache_block->copyfrom(povblock);
			}
			//cache_block_mutex.unlock();
			//print_document(cache_block->getBlock());
			uint32_t commissioners_num=account_manager.getCommissionerAmount();
			//std::cout<<"委员数量为："<<commissioners_num<<"!\n";
			for(uint32_t i=0;i<commissioners_num;i++)
			{
				//这里倒序发送是因为顺序发送时发现最后一个节点收不到消息，倒序发送又可以，原因不明
				//std::cout<<"check lock:1"<<std::endl;
				account comm=account_manager.getCommissioner(i);
				NodeID comm_ID=comm.getNodeId();
				if(comm_ID==0)
				{
					std::string pubkey=comm.getPubKey();
					char* ch=const_cast<char*>(pubkey.c_str());
					uint32_t *map_id=(uint32_t*)ch;
					rapidjson::Document& msg=msg_manager.make_Request_NodeID_By_Pubkey(pubkey);
					sendMessage(msg,true,10,Recall_RequestNodeID,*map_id,&POV::handleResponseNodeID);
					delete &msg;
					continue;
				}
				//std::cout<<"check lock:2"<<std::endl;
				bool existed=false;
				rbs_mutex.lock();
				for(auto j=raw_block_signatures.begin();j!=raw_block_signatures.end();j++)
				{
					if(j->pubkey==comm.getPubKey())
					{
						existed=true;
						break;
					}
				}
				rbs_mutex.unlock();
				//std::cout<<"check lock:3"<<std::endl;
				//cache_block_mutex.lock();
				if(!existed)
				{
					//std::cout<<"发送生区块给"<<comm.getNodeId()<<"\n";
					rapidjson::Document& raw_zero_block=cache_block->getRawBlock();
					rapidjson::Document& msg=msg_manager.make_Request_Block_Signature((NodeID)comm.getNodeId(),raw_zero_block);
					sendMessage(msg,true,10,Recall_RequestBlockSignature,(NodeID)comm.getNodeId(),&POV::handleResponseBlockSignature);
					delete &raw_zero_block;
					delete &msg;
//					try
//					{
//						delete &raw_zero_block;
//						delete &msg;
//					}
//					catch(...)
//					{
//						throw "GenerateBlock_sign中删除raw_zero_block和msg错误\n";
//					}
				}
				//cache_block_mutex.unlock();
				//std::cout<<"check lock:4"<<std::endl;
			}
			break;
		}
		case GenerateBlock_publish:
		{
			//发布生成的区块
			std::cout<<"生成创世区块：2!\n";
			if(raw_block_signatures.size()<account_manager.getCommissionerAmount())
			{
				break;
			}
			rbs_mutex.lock();
			//cache_block_mutex.lock();
			cache_block->setSigs(raw_block_signatures);

			rbs_mutex.unlock();
			uint32_t next_butler_num=getNextButler(cache_block->getPoVHeader().getSignatures());
			//std::cout<<"butler_num="<<next_butler_num<<"\n";
			cache_block->setNextButler(next_butler_num);
			cache_block->setTe(getTe(cache_block->getPoVHeader().getSignatures()));
			rapidjson::Document& complete_block=cache_block->getBlock();
			//cache_block_mutex.unlock();
			rapidjson::Document& msg=msg_manager.make_Request_Publish_Block(complete_block);
			sendMessage(msg,false,30,Recall_RequestBlockSignature,0,&POV::handleResponseBlockSignature);
			GenerateFirstBlock=GenerateBlock_finish;
			try
			{
				delete &complete_block;
				delete &msg;
			}
			catch(...)
			{
				throw "GenerateBlock_publish中删除complete_block和msg异常\n";
			}
			break;
		}
		case GenerateBlock_finish:
		{
			//std::cout<<"生成创世区块：3!\n";
			//cache_block_mutex.lock();
			delete cache_block;
			cache_block=NULL;
			//cache_block_mutex.unlock();
			rbs_mutex.lock();
			raw_block_signatures.clear();
			rbs_mutex.unlock();
			break;
		}
	}
}
