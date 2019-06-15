#include "POV.h"

//处理接收到的消息
void POV::handleMessage(rapidjson::Document &d)
{
	//处理接收到的消息PoV协议层面的消息在这里进行处理
	//std::cout<<"in pov->handleMessage\n";
	rapidjson::Document::AllocatorType &allocator=d.GetAllocator();
	uint32_t index=d["index"].GetUint();
	//std::cout<<"in pov->handleMessage 1\n";
	NodeID sender=d["sender"].GetUint64();
	//std::cout<<"in pov->handleMessage 2\n";
	NodeID receiver=d["receiver"].GetUint64();
	//std::cout<<"in pov->handleMessage 3\n";
	std::string pubkey(d["pubkey"].GetString(),d["pubkey"].GetStringLength());
	MessageType type=MessageType(d["type"].GetUint());
	rapidjson::Value &data=d["data"];
	uint32_t respond_index=d["respond_to"].GetUint();
	//std::cout<<"in pov->handleMessage 4\n";
	std::string sig(d["signature"].GetString(),d["signature"].GetStringLength());
	callback_type callbacktype=callback_type(d["callback_type"].GetUint());
	//std::cout<<"in pov->handleMessage 5\n";
	NodeID callback_childtype=d["child_type"].GetUint64();
	//std::cout<<"in pov->handleMessage 6\n";
	//如果是对某个请求的回复，则从CallBackList查找该请求并调用相应的回调函数进行处理
	if(respond_index!=0)
	{
		//std::cout<<"CallBackList在handleMessage时的size为"<<CallBackList.size()<<"\n";
		call_back_mutex.lock();
		for(std::vector<CallBackInstance>::iterator i=CallBackList.begin();i!=CallBackList.end();i++)
		{
			//std::cout<<"i->index="<<i->index <<"&& i->type="<<int(i->type)<<" && i->childtype="<<i->childtype<<"\n";
			if(i->index==respond_index and i->type==callbacktype and i->childtype==callback_childtype)
			{
				//std::cout<<"respond_index="<<respond_index <<"&& callbacktype"<<int(callbacktype)<<" && callback_childtype="<<callback_childtype<<"\n";
				//这里的顺序不可更改，否则会出错
				try
				{
					CallBackInstance cbi=*i;
					CallBackList.erase(i);
					(cbi.caller)(d);
				}
				catch(...)
				{
					std::cout<<"处理消息回复错误，消息类型为"<<type<<"\n";
					throw "";
				}

				//std::cout<<"callbackList delete:1\n";
				//std::cout<<"CallBackList在handleMessage的erase后的size为"<<CallBackList.size()<<"\n";
				//std::cout<<"callbackList delete:2\n";
				call_back_mutex.unlock();
				return;
			}
		}
		call_back_mutex.unlock();
	}
	//如果不是某个请求的回复，则根据消息的类型进行相应的处理
	switch(type)
	{
		//处理委员公钥的请求
		case RequestCommissionerPubkey:
		{
			rapidjson::Document &response_pubkey_request=msg_manager.make_Response_Commissioner_PublicKey(sender,index);
			sendMessage(response_pubkey_request,false,30,callbacktype,callback_childtype,NULL);
			try
			{
				delete &response_pubkey_request;

			}
			catch(...)
			{
				throw "RequestCommissionerPubkey中删除response_pubkey_request异常\n";
			}
			break;
		}
		//处理申请委员签名的请求
		case RequestApplyCommissionerSignature:
		{
			std::cout<<network->getNodeId()<<"收到"<<sender<<"的申请委员签名请求\n";
			//std::cout<<"收到委员公钥！:"<<pubkey<<"\n";
			//rapidjson::StringBuffer buffer;
			//rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			//d.Accept(writer);
			//std::cout<<"response文档信息："<<buffer.GetString()<<"\n";
			//if(account_manager.is_commissioner(pubkey))

			rapidjson::Document &response=msg_manager.make_Response_Apply_Commissioner_Signature(sender,index,data);

			/*
			rapidjson::Value &_data=response["data"];
			std::string pk=_data["pubkey"].GetString();
			std::string s=_data["sig"].GetString();
			bool result=key_manager.verifyDocument(data,pk,s);
			std::cout<<"验证签名结果："<<result<<"\n";
			*/
			sendMessage(response,false,10,Recall_RequestApplyCommissionerSignature,callback_childtype,NULL);
			try
			{
				delete &response;

			}
			catch(...)
			{
				std::cout<<"RequestApplyCommissionerSignature中删除response异常\n";
				throw "";
			}
			break;
		}
		//处理申请委员的请求
		case RequestApplyCommissioner:
		{
			//std::cout<<"处理申请委员请求1\n";

			if(validateApplyCommissionerTransaction(data))
			{
				bool existed=false;
				std::string pubkey=data["metadata"]["pubkey"].GetString();
				ac_mutex.lock();
				for(std::vector<DocumentContainer>::iterator i=ApplyCommissionerPool.begin();i!=ApplyCommissionerPool.end();i++)
				{
					try
					{
						rapidjson::Document& doc=i->getDocument();
						if(doc["metadata"]["pubkey"].GetString()==pubkey)
						{
							existed=true;
							delete &doc;
							std::cout<<"RequestApplyCommissioner中委员已存在\n";
							break;
						}
						delete &doc;
					}
					catch(...)
					{
						std::cout<<"RequestApplyCommissioner中删除doc异常\n";
						throw "";
					}
				}
				ac_mutex.unlock();
				if(!existed)
				{
					//在生成创世区块阶段，只接受委员的申请，其他阶段则只接受非委员的申请。
					if(state==generate_first_block)
					{
						if(account_manager.is_commissioner(pubkey))
						{
							DocumentContainer container;
							container.saveDocument(data);
							ac_mutex.lock();
							ApplyCommissionerPool.push_back(container);
							ac_mutex.unlock();
						}
					}
					else
					{
						if(!account_manager.is_commissioner(pubkey))
						{
							DocumentContainer container;
							container.saveDocument(data);
							ac_mutex.lock();
							ApplyCommissionerPool.push_back(container);
							ac_mutex.unlock();
						}
					}
				}
				//std::cout<<"处理申请委员请求2\n";

				//std::cout<<"申请委员消息：\n";
				//print_document(d);
				//简单起见，对于申请成为委员，默认同意
				rapidjson::Document &response=msg_manager.make_Response_Apply_Commissioner(sender,index);
				sendMessage(response,false,20,Recall_RequestApplyCommissioner,callback_childtype,NULL);
				try
				{
					delete &response;

				}
				catch(...)
				{
					std::cout<<"RequestApplyCommissioner中删除response异常\n";
					throw "";
				}
				//std::cout<<"处理申请委员请求3\n";

			}
			else
				std::cout<<"申请委员交易验证不通过!\n";
			break;
		}
		//处理推荐信请求
		case RequestRecommendationLetter:
		{
			std::cout<<"接收到推荐信请求\n";
			rapidjson::Document &msg=msg_manager.make_Response_Recommend_Letter(sender,index,pubkey);
			sendMessage(msg,false,20,Recall_RequestApplyRecommendationLetter,callback_childtype,NULL);
			try
			{
				delete &msg;

			}
			catch(...)
			{
				std::cout<<"RequestRecommendationLetter中删除msg异常\n";
				throw "";
			}
			break;
		}
		//处理申请管家候选签名的请求
		case RequestApplyButlerCandidateSignature:
		{
			std::cout<<network->getNodeId()<<"收到"<<sender<<"的申请管家候选签名请求\n";
			if(!validateRecommendationLetter(data))
			{
				std::cout<<"推荐信验证不通过\n";
				break;
			}
			rapidjson::Document &response=msg_manager.make_Response_Apply_Butler_Candidate_Signature(sender,index,data);
			sendMessage(response,false,20,Recall_RequestApplyButlerCandidateSignature,callback_childtype,NULL);
			try
			{
				delete &response;
			}
			catch(...)
			{
				std::cout<<"RequestApplyButlerCandidateSignature中删除response异常\n";
				throw "";
			}
			break;
		}
		//处理申请管家候选的请求
		case RequestApplyButlerCandidate:
		{
			std::cout<<network->getNodeId()<<"收到"<<sender<<"的申请管家候选请求\n";

			//rapidjson::Document& msg=
			try
			{
				if(!validateApplyButlerCandidateData(data))
				{
					std::cout<<"验证申请管家候选不通过！\n";
					break;
				}
			}
			catch(...)
			{
				std::cout<<"RequestApplyButlerCandidate中验证申请管家候选数据异常\n";
				throw "";
			}
			//判断是否已经有这个申请了，如果没有就加入到ApplyButlerCandidatePool中。
			bool existed=false;
			std::string new_refferal="";
			try
			{
				new_refferal=data["recommendation_letter"]["metadata"]["refferal"].GetString();
			}
			catch(...)
			{
				std::cout<<"RequestApplyButlerCandidate中的的data异常，data：\n";
				print_document(data);
				throw "";
			}
			abc_mutex.lock();
			for(std::vector<DocumentContainer>::iterator i=ApplyButlerCandidatePool.begin();i!=ApplyButlerCandidatePool.end();i++)
			{
				rapidjson::Document& doc=i->getDocument();
				//rapidjson::Value& letter=doc["recommendation_letter"];
				std::string refferal="";
				try
				{
					refferal=doc["recommendation_letter"]["metadata"]["refferal"].GetString();
				}
				catch(...)
				{
					std::cout<<"RequestApplyButlerCandidate中从推荐信获取refferal异常\n";
					throw "";
				}
				try
				{
					delete &doc;

				}
				catch(...)
				{
					std::cout<<"RequestApplyButlerCandidate中删除doc异常\n";
					throw "";
				}
				if(refferal==new_refferal)
				{
					existed=true;
					break;
				}
			}
			abc_mutex.unlock();
			try
			{
				if(!existed)
				{
					rapidjson::Document temp;
					rapidjson::Document::AllocatorType &allocator=temp.GetAllocator();
					temp.CopyFrom(data,allocator);
					DocumentContainer container;
					container.saveDocument(temp);
					abc_mutex.lock();
					ApplyButlerCandidatePool.push_back(container);
					abc_mutex.unlock();
					std::string pubkey=data["recommendation_letter"]["metadata"]["refferal"].GetString();
					if(state==generate_first_block)
					{
						try
						{
							account_manager.pushback_butler_candidate(pubkey,sender);//这里直接把发送方的ID作为申请人的ID会不会有安全问题？这是发送节点ID可篡改带来的问题，可以在底层再加一层签名解决，这里不实现。
						}
						catch(...)
						{
							std::cout<<"RequestApplyButlerCandidate中添加butler_candidate异常\n";
							throw "";
						}
					}
				}
			}
			catch(...)
			{
				std::cout<<"RequestApplyButlerCandidate中存储推荐信异常\n";
				throw "";
			}
			//发送回复表示已经收到
			try
			{
				rapidjson::Document &response=msg_manager.make_Response_Apply_Butler_Candidate(sender,index);
				sendMessage(response,false,10,Recall_RequestApplyButlerCandidate,callback_childtype,NULL);

				try
				{
					delete &response;
				}
				catch(...)
				{
					std::cout<<"RequestApplyButlerCandidate中删除response异常\n";
					throw "";
				}
			}
			catch(...)
			{
				std::cout<<"RequestApplyButlerCandidate中发送回复异常\n";
				throw "";
			}
			break;

		}
		//处理投票请求
		case RequestVote:
		{
			//std::cout<<"接收到Vote请求：start!\n";
			//验证投票正确性
//			try
//			{
//				if(!validateBallot(data))
//				{
//					std::cout<<"验证投票错误！\n";
//					break;
//				}
//			}
//			catch(...)
//			{
//				std::cout<<"验证投票数据异常\n";
//				throw "";
//			}
			//检查投票池中是否已经有该委员的投票
			bool existed=false;
			std::string new_voter=data["metadata"]["voter"].GetString();
			b_mutex.lock();
			for(std::vector<DocumentContainer>::iterator i=BallotPool.begin();i!=BallotPool.end();i++)
			{
				rapidjson::Document& doc=i->getDocument();
				//rapidjson::Value& letter=doc["recommendation_letter"];
				std::string voter=doc["metadata"]["voter"].GetString();
				try{
				delete &doc;
				}
				catch(...)
				{
					std::cout<<"RequestVote中删除doc异常\n";
					throw "";
				}
				if(voter==new_voter)
				{
					existed=true;
					break;
				}
			}
			b_mutex.unlock();
			if(!existed)
			{
				//rapidjson::Document& Ballot=*(new rapidjson::Document);
				//Ballot.CopyFrom(data,Ballot.GetAllocator());
				DocumentContainer container;
				container.saveDocument(data);
				b_mutex.lock();
				BallotPool.push_back(container);
				std::cout<<"BallotPool="<<BallotPool.size()<<"\n";
				b_mutex.unlock();
			}
			//发送回复表示已经收到
			rapidjson::Document &response=msg_manager.make_Response_Vote(sender,index);
			sendMessage(response,false,10,Recall_RequestVote,callback_childtype,NULL);
			delete &response;
			//std::cout<<"接收到Vote请求：END!\n";
			break;
		}
		//处理区块签名请求
		case RequestBlockSignature:
		{
			//std::cout<<network->getNodeId()<<"节点接收到"<<network->getNodeId()<<"节点的区块签名请求：start!\n";
			PoVBlock block;
			block.setRawBlock(data);
			uint32_t height=block.getHeight();
			//std::cout<<"RequestBlockSignature中check：1\n";
			//std::cout<<network->getNodeId()<<"节点收到"<<sender<<"节点发送来的高度为"<<height<<"的生区块\n";
			//验证高度
			if(height<blockchain.getHeight()+1)
			{
					std::cout<<getCurrentTime()<<" "<<network->getNodeId()<<"节点收到"<<sender<<"节点发送来的高度为"<<height<<"的生区块\n";
					std::cout<<"请求区块签名中验证区块头错误——高度错误!\n";
					//std::cout<<"请求区块签名中验证区块头错误——高度错误!\n";
					std::cout<<"height:"<<height<<"\n";
					std::cout<<"my height:"<<blockchain.getHeight()<<"\n";
					//print_document(data);
					break;
			}
			else
			{
				if(height>0)
				{
					MetaMessage msg=msg_manager.toMetaMessage(d);
					future_raw_blocks_mutex.lock();
					future_raw_blocks[height]=msg;
					future_raw_blocks_mutex.unlock();
				}
				else
				{
					try
					{
						if(!validateRawBlock(data))
						{
							std::cout<<"区块验证不通过\n";
							 //rapidjson::Value& header=data["header"];
							//NodeInfo node=network->NodeList[sender];
							rapidjson::Document doc;
							rapidjson::Document &response=msg_manager.make_Response_Block_Signature_Failed(sender,index,1,"validate block wrong",doc,name);
							//print_document(response);
							sendMessage(response,false,10,Recall_RequestBlockSignature,callback_childtype,NULL);
							delete &response;
							break;
						}
						else
						{
							std::cout<<"请求区块签名中生区块验证通过\n";
						}
					}
					catch(...)
					{
						std::cout<<"RequestBlockSignature中验证生区块错误\n";
						throw "";
					}
					//std::cout<<"RequestBlockSignature中check：3\n";
					//blockchain.pushbackBlock(block);
					rapidjson::Value& header=data["header"];
					//NodeInfo node=network->NodeList[sender];
					rapidjson::Document &response=msg_manager.make_Response_Block_Signature(sender,index,header,name);
					//print_document(response);
					sendMessage(response,false,10,Recall_RequestBlockSignature,callback_childtype,NULL);
					delete &response;
				}
			}
			//std::cout<<"RequestBlockSignature中check：2\n";
//			try
//			{
//				if(!validateRawBlock(data))
//				{
//					std::cout<<"区块验证不通过\n";
//					break;
//				}
//				else
//				{
//					std::cout<<"请求区块签名中生区块验证通过\n";
//				}
//			}
//			catch(...)
//			{
//				std::cout<<"RequestBlockSignature中验证生区块错误\n";
//				throw "";
//			}
//			//std::cout<<"RequestBlockSignature中check：3\n";
//			//blockchain.pushbackBlock(block);
//			rapidjson::Value& header=data["header"];
//			//NodeInfo node=network->NodeList[sender];
//			rapidjson::Document &response=msg_manager.make_Response_Block_Signature(sender,index,header);
//			//print_document(response);
//			sendMessage(response,false,10,Recall_RequestBlockSignature,callback_childtype,NULL);
//			try
//			{
//				delete &response;
//			}
//			catch(...)
//			{
//				std::cout<<"RequestBlockSignature中删除response异常\n";
//				throw "";
//			}
			//std::cout<<"接收到区块签名请求：end!\n";
			break;
		}
		//处理发布的区块
		case PublishBlock:
		{
			PoVBlock block;
			block.setBlock(data);
			uint32_t height=block.getHeight();
			std::cout<<network->getNodeId()<<"节点接收到新发布的高度为"<<height<<"的区块！\n";
			if(height==0)
			{
				try
				{
					if(!validateBlock(data))
					{
						break;
					}
	//				else
	//				{
	//					std::cout<<"发布区块消息处理中验证正式区块通过\n";
	//				}
				}
				catch(...)
				{
					std::cout<<"PublishBlock中验证区块错误\n";
					throw "";
				}
				blockchain.pushbackBlockToDatabase(block);
				try
				{
					updateVariables(data);
				}
				catch(...)
				{
					std::cout<<"接收到新区快更新变量异常\n";
					throw "";
				}
				state=normal;
				network->set_limit=true;
			}
			else
			{
				if(height>=blockchain.getHeight()+1)
				{
					future_blocks_mutex.lock();
					future_blocks[height]=block;
//					rapidjson::Document& header=block.getHeader();
//					rapidjson::Document& txs=block.getTransactionsList();
//					std::cout<<"区块头大小="<<getDocumentString(header).size()<<"\n";
//					std::cout<<"交易大小="<<getDocumentString(txs).size()<<"\n";
//					std::cout<<"区块大小="<<getDocumentString(data).size()<<"\n";
					future_blocks_mutex.unlock();
				}
				if(height>=blockchain.getHeight()+5 && state==normal)
				{
					state=sync_block;
					fetch_height_start_time=getCurrentTime();
				}
			}

//			//验证高度
//			if(height!=blockchain.getHeight()+1)
//			{
//				if(height<blockchain.getHeight()+1)
//				{
//					std::cout<<"publish block 中验证区块头错误——高度错误！\n";
//					std::cout<<"接收到的区块高度:"<<height<<"\n";
//					std::cout<<"本地区块链高度:"<<blockchain.getHeight()<<"\n";
//				}
//				else
//				{
//					async_block_queue[height]=block;
//				}
//				break;
//					/*
//					int h=blockchain.getHeight()+1;
//					bool update_to_recent_height=true;
//					while(h<height)
//					{
//						if(async_block_queue.find(h)!=async_block_queue.end())
//						{
//							PoVBlock pb=async_block_queue[h];
//							rapidjson::Document& block_old=pb.getBlock();
//							updateVariables(block_old);
//							blockchain.pushbackBlockToDatabase(pb);
//							async_block_queue.erase(h);
//							h++;
//						}
//						else
//						{
//							update_to_recent_height=false;
//							break;
//						}
//					}
//					if(!update_to_recent_height)
//					{
//						PoVBlock pb;
//						break;
//					}
//				}
//				*/
//				//print_document(data);
//			}
//			//std::cout<<"发布区块:1\n";
//			//先加入到数据库在更新变量，因为更新变量后节点就进入了下一个区块的产生流程，duty_butler就会开始产生区块，Normal_GenerateBlock是根据本地区块链高度+1来产生生区块的，
//			//如果先存储到区块链再更新变量，写入数据库的过程较慢，如果在写入数据库之前主线程就开始产生生区块，就会导致产生的生区块高度不对，请求签名的时候验证无法通过，因此必须先写入
//			//数据库再执行更新变量的操作
//			blockchain.pushbackBlockToDatabase(block);
//			try
//			{
//				updateVariables(data);
//			}
//			catch(...)
//			{
//				std::cout<<"接收到新区快更新变量异常\n";
//				throw "";
//			}
//			//std::cout<<"发布区块:2\n";
//			//blockchain.pushbackBlock(block);
//			std::cout<<network->getNodeId()<<"把高度为"<<height<<"的区块加入到区块链中!\n";
//
//			//print_document(data);
//			if(height==0)
//			{
//				state=normal;
//				network->set_limit=true;
//			}
//			else if(height==end_height+1)
//			{
//				//print_document(data);
//				statisticData();
//				exit(0);
//			}
//
//			else
//			{
//				height++;
//				try
//				{
//					while(async_block_queue.find(height)!=async_block_queue.end())
//					{
//						PoVBlock pb=async_block_queue[height];
//						rapidjson::Document& block_old=pb.getBlock();
//						updateVariables(block_old);
//						blockchain.pushbackBlockToDatabase(pb);
//						async_block_queue.erase(height);
//						height++;
//					}
//				}
//				catch(...)
//				{
//					std::cout<<"PublishBlock中异步区块更新异常\n";
//					throw "";
//				}
//			}
			break;
		}
		//处理NodeID请求
		case RequestNodeID:
		{
			//std::cout<<"收到节点ID请求\n";
			std::string receiver_pubkey=data["pubkey"].GetString();
			if(receiver_pubkey==account_manager.getMyPubkey())
			{
				//std::cout<<"in pubkey equal!\n";
				rapidjson::Document &response=msg_manager.make_Response_NodeID_By_Pubkey(sender,index);
				sendMessage(response,false,10,Recall_RequestNodeID,callback_childtype,NULL);
				try
				{
					delete &response;

				}
				catch(...)
				{
					std::cout<<"请求节点ID异常\n";
					throw "111";
				}
			}
			break;
		}
		//处理普通交易请求
		case RequestNormal:
		{
			/*查找有是否已经接受过该交易，是的话抛弃，为了减低算法复杂度，该部分不使用
			for(std::vector<DocumentContainer>::iterator i=NormalPool.begin();i!=NormalPool.end();i++)
			{
				rapidjson::Document& tx=i->getDocument();
				if(data["content"].GetString()==tx["content"].GetString() and data["timestamp"].GetString()==tx["timestamp"].GetString())
				{
					break;
				}
			}
			*/
			//std::cout<<"接收到正常交易请求：start!\n";
			//std::cout<<"正常交易缓存数量："<<NormalPool.size()<<"\n";
//			if(!validateNDNTransactionButler(data))
//			{
//				break;
//			}
			if(account_manager.is_butler())
			{
				//防止接收相同的交易
				DocumentContainer container;
				container.saveDocument(data);
				norm_mutex.lock();
				if(!lru_NormalPool.find(container))
				{
					lru_NormalPool.append(container);
				}
				norm_mutex.unlock();
				/*
				rapidjson::Value& content=data["content"];
				if(content.IsObject() && content.HasMember("log"))
				{
					int hash=content["log"].GetInt();
					for(uint32_t i=0;i<NormalPool.size();i++)
					{
						rapidjson::Document& tx=NormalPool.at(i).getDocument();
						rapidjson::Value& _content=tx["content"];
						 if(_content.IsObject() && _content.HasMember("log") && _content["log"].GetInt()==hash)
						 {
							 delete &tx;
							 return;
						 }
					}
				}
				DocumentContainer container;
				container.saveDocument(data);
				if(NormalPool.size()>=NormalPoolCapacity)
				{
					NormalPool.erase(NormalPool.begin());
				}
				NormalPool.push_back(container);
				*/
				//if(account_manager.getMyNodeID()==2)
				//{
					//print_document(data);
				//}
			}

			//rapidjson::Document &response=msg_manager.make_Response_Normal(sender,index);
			//sendMessage(response,false,10,Recall_RequestNormal,callback_childtype,NULL);
			//delete &response;
			//std::cout<<"接收到正常交易请求：end!\n";
			break;
		}
		//处理退出管家候选请求
		case RequestQuitButlerCandidate:
		{
			//std::cout<<"收到退出管家候选请求！\n";
			//print_document(data);
			if(!validateQuitButlerCandidateTransaction(data))
			{
				std::cout<<"退出管家候选请求验证失败！";
			}
			std::string applicant=data["metadata"]["pubkey"].GetString();
			if(account_manager.is_butler(applicant))
			{
				std::cout<<"申请退出管家候选的申请者是管家!\n";
				break;
			}
			//判断该交易是否已经存在
			bool exist=false;
			qbc_mutex.lock();
			for(auto i=QuitButlerCandidatePool.begin();i!=QuitButlerCandidatePool.end();i++)
			{
				rapidjson::Document &tx=i->getDocument();
				std::string _app=tx["metadata"]["pubkey"].GetString();
				if(_app==applicant)
				{
					delete &tx;
					exist=true;
					break;
				}
				delete &tx;
			}
			qbc_mutex.unlock();
			if(exist)
			{
				break;
			}
			DocumentContainer container;
			container.saveDocument(data);
			qbc_mutex.lock();
			QuitButlerCandidatePool.push_back(container);
			qbc_mutex.unlock();
			rapidjson::Document &response=msg_manager.make_Response_Quit_Butler_Candidate(sender,index);
			sendMessage(response,false,10,Recall_RequestQuitButlerCandidate,callback_childtype,NULL);
			delete &response;
			break;
		}
		//处理退出委员请求
		case RequestQuitCommissioner:
		{
			std::cout<<"收到退出委员请求！\n";
			if(!validateQuitCommissionerTransaction(data))
			{
				std::cout<<"退出委员请求验证失败！";
			}
			std::string applicant=data["metadata"]["pubkey"].GetString();
			//判断该交易是否已经存在
			bool exist=false;
			for(auto i=QuitCommissionerPool.begin();i!=QuitCommissionerPool.end();i++)
			{
				rapidjson::Document &tx=i->getDocument();
				std::string _app=tx["metadata"]["pubkey"].GetString();
				if(_app==applicant)
				{
					delete &tx;
					exist=true;
					break;
				}
				delete &tx;
			}
			if(exist)
			{
				break;
			}
			std::cout<<"把退出委员请求存放进交易池中！\n";
			DocumentContainer container;
			container.saveDocument(data);
			QuitCommissionerPool.push_back(container);
			rapidjson::Document &response=msg_manager.make_Response_Quit_Commissioner(sender,index);
			sendMessage(response,false,10,Recall_RequestQuitCommissioner,callback_childtype,NULL);
			delete &response;
			break;
		}
		//处理区块链高度请求
		case RequestHeight:
		{
			rapidjson::Document &response=msg_manager.make_Response_Height(sender,blockchain.getHeight());
			sendMessage(response,false,10,Recall_RequestHeight,callback_childtype,NULL);
			delete &response;
			break;
		}
		//处理区块链高度回复
		case ResponseHeight:
		{
			int height=data["height"].GetInt();
			std::cout<<receiver<<"收到高度为"<<height<<"的高度请求回复\n";
			if(sync_block_method==SequenceSync)
			{
				if(height>blockchain.getHeight())
				{
					block_syncer s;
					s.id=sender;
					s.height=height;
					syncer_mutex.lock();
					syncers.push(s);
					syncer_mutex.unlock();
				}
			}
			else
			{
				int current_height=blockchain.getHeight();
				if(height>current_height)
				{
					int start_height=current_height+1;
					if((!con_syncers.empty()) && con_syncers[con_syncers.size()-1].height<height)
					{
						start_height=con_syncers[con_syncers.size()-1].height+1;
					}
					for(int h=start_height;h<=height;h++)
					{
						concurrent_block_syncer s;
						s.height=h;
						s.prepared=false;
						con_syncers.push_back(s);
					}
				}
			}

			break;
		}
		//处理区块请求
		case RequestBlock:
		{
			int height=data["height"].GetInt();
			std::cout<<receiver<<"收到高度为"<<height<<"的区块请求\n";
			if(blockchain.getHeight()<height)
				break;
			std::cout<<account_manager.getMyNodeID()<<"返回高度为"<<height<<"的区块, childtype为"<<callback_childtype<<"\n";
			PoVBlock block=blockchain.getBlock(height);
			rapidjson::Document& doc=block.getBlock();
			rapidjson::Document &response=msg_manager.make_Response_Block(sender,index,doc);
			sendMessage(response,false,10,Recall_RequestBlock,callback_childtype,NULL);
			//print_document(response);
			delete &doc;
			delete &response;
			break;
		}
		/*
		case ResponseBlock:
		{
			print_document(data);
			int height=data["response_height"].GetInt();
			std::cout<<"在handleMessage中接受到ResponseBlock的消息，高度为"<<height<<"\n";
			break;
		}
		*/
		default:
			break;
	}
}
