/*
 * MessageManager.cc
 *
 *  Created on: 2018年3月31日
 *      Author: blackguess
 */

#include "MessageManager.h"
#include "blockchain-network.h"


MessageManager::MessageManager() {
	// TODO Auto-generated constructor stub

}

MessageManager::~MessageManager() {
	// TODO Auto-generated destructor stub
}

void MessageManager::init(std::string public_key,blockchain_network *bn,KeyManager *manager)
{
	index=0;
	pubkey=public_key;
	network=bn;
	key_manager=manager;
	for(int i=0;i<29;i++)
	{
		msg_distribution.push_back(0);
	}
}

//获取消息统计数据
std::vector<int> MessageManager::getMSGStatData(){
	return msg_distribution;
}

MetaMessage MessageManager::toMetaMessage(rapidjson::Document& msg)
{
	MetaMessage result;
	if(!msg.IsObject())
	{
		return result;
	}
	if(msg.HasMember("index")&&msg["index"].IsUint())
	{
		result.index=msg["index"].GetUint();
	}
	if(msg.HasMember("sender")&& msg["sender"].IsUint64())
	{
		result.sender=msg["sender"].GetUint64();
	}
	if(msg.HasMember("receiver")&&msg["receiver"].IsUint64())
	{
		result.receiver=msg["receiver"].GetUint64();
	}
	if(msg.HasMember("pubkey")&&msg["pubkey"].IsString())
	{
		result.pubkey=msg["pubkey"].GetString();
	}
	if(msg.HasMember("type")&&msg["type"].IsInt())
	{
		result.type=(MessageType)msg["type"].GetInt();
	}
	if(msg.HasMember("data")&&(msg["data"].IsObject()||msg["data"].IsArray()))
	{
		result.data=getDocumentString(msg["data"]);
	}
	if(msg.HasMember("respond_to")&&msg["respond_to"].IsUint())
	{
		result.respond_to=msg["respond_to"].GetUint();
	}
	if(msg.HasMember("signature")&&msg["signature"].GetString())
	{
		result.signature=msg["signature"].GetString();
	}
	if(msg.HasMember("callback_type")&&msg["callback_type"].IsUint())
	{
		result.callback_type=msg["callback_type"].GetUint();
	}
	if(msg.HasMember("child_type")&&msg["child_type"].IsUint64())
	{
		result.child_type=msg["child_type"].GetUint64();
	}
	return result;
}
rapidjson::Document& MessageManager::toDocumentMessage(MetaMessage msg)
{
	rapidjson::Document data;
	try
	{
		data.Parse(msg.data.c_str(),msg.data.size());
	}
	catch(...)
	{
		data.SetObject();
	}
	rapidjson::Document& result=make_Message(msg.index,msg.sender,msg.receiver,msg.pubkey,msg.type,data,msg.respond_to,msg.signature);
	rapidjson::Document::AllocatorType &allocator=result.GetAllocator();
	rapidjson::Value callback_type(msg.callback_type);
	result.AddMember("callback_type",callback_type,allocator);
	rapidjson::Value child_type(msg.child_type);
	result.AddMember("child_type",child_type,allocator);
	return result;
}

void MessageManager::setPubkey(std::string pubkey)
{
	this->pubkey=pubkey;
}

uint32_t MessageManager::getIndex()
{
	return index;
}

//设置index
void MessageManager::setIndex(int i)
{
	index=i;
}

rapidjson::Document& MessageManager::make_Message(uint32_t index,NodeID sender,NodeID receiver,
		std::string pubkey,MessageType type,rapidjson::Document &data,uint32_t respond_to,std::string signature)
{
	//经测试，返回Document类型数据或它的引用，都会导致Document中的字符串改变，原因不明，因此这里需要返回指针。
	rapidjson::Document &d=*(new rapidjson::Document());
	//rapidjson::Document &d=*doc;
	d.SetObject();
	rapidjson::Document::AllocatorType &allocator=d.GetAllocator();
	rapidjson::Value _index(index);
	d.AddMember("index",_index,allocator);
	rapidjson::Value _sender(sender);
	d.AddMember("sender",_sender,allocator);
	rapidjson::Value _receiver(receiver);
	d.AddMember("receiver",_receiver,allocator);

	rapidjson::Value *_pubkey=new rapidjson::Value(pubkey.c_str(),pubkey.size(),allocator);
	//_pubkey.SetString(pubkey.c_str(),pubkey.size());
	d.AddMember("pubkey",*_pubkey,allocator);


	rapidjson::Value _type(type);
	d.AddMember("type",_type,allocator);

	//rapidjson::Value _data;
	//rapidjson::Document &dat=*(new rapidjson::Document);
	//dat.CopyFrom(data,dat.GetAllocator());
	//注意！！！这里复制了data，也就是说data从哪里生成的就需要从哪里释放，生成的消息跟参数中的data没有关系
	rapidjson::Value& data_value=*(new rapidjson::Value);
	data_value.CopyFrom(data,allocator);
	//data.GetAllocator().Clear();
	d.AddMember("data",data_value,allocator);

	rapidjson::Value _respond_to(respond_to);
	d.AddMember("respond_to",_respond_to,allocator);
	rapidjson::Value _signature(signature.c_str(),signature.size(),allocator);
	d.AddMember("signature",_signature,allocator);
	msg_distribution[int(type)]++;
	//std::string pk(d["pubkey"].GetString(),d["pubkey"].GetStringLength());
	//std::cout<<"test 1:"<<pk<<"\n";

	/*
	  rapidjson::StringBuffer buffer;
	  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	  d->Accept(writer);
	  std::cout<<buffer.GetString()<<"\n";
	  */
	return d;
}

rapidjson::Document& MessageManager::make_Message(NodeID receiver,
		MessageType type,rapidjson::Document &data,uint32_t respond_to,std::string signature)
{
	//这里自动设置index以及sender字段
	index++;
	uint32_t sender=network->getNodeId();
	rapidjson::Document &d=make_Message(index,network->getNodeId(),receiver,
			pubkey,type,data,respond_to,signature);
	//std::cout<<"index="<<index<<"\n";
	//std::cout<<"index in document:"<<d["index"].GetInt64()<<"\n";
	return d;
}

rapidjson::Document& MessageManager::make_Request_Commissioner_PublicKey(NodeID receiver)
{
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document &d=make_Message(receiver,RequestCommissionerPubkey,data,0,"");
	return d;
}

rapidjson::Document& MessageManager::make_Response_Commissioner_PublicKey(NodeID receiver,uint32_t response_index)
{
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document &d=make_Message(receiver,ResponseCommissionerPubkey,data,response_index,"");

	//std::string pk(d["pubkey"].GetString(),d["pubkey"].GetStringLength());
	//std::cout<<"test 2:"<<pk<<"\n";
	return d;
}

rapidjson::Document& MessageManager::get_Apply_Commissioner_Metadata()
{
	//元数据需要有时间戳，不然每次生成的元数据都一样，签名结果也都一样，那么某节点退出委员后只要拿以前的签名就可以再次申请成为委员。管家在最终接收验证的时候需要对比时间戳是否比以前的时间更新。
	rapidjson::Document *d=new rapidjson::Document();
	rapidjson::Document& data=*d;
	data.SetObject();
	rapidjson::Document::AllocatorType &allocator=data.GetAllocator();
	//std::string metadata="ApplyCommissioner";
	//rapidjson::Value _metadata(metadata.c_str(),metadata.size(),allocator);
	rapidjson::Value _metadata("ApplyCommissioner",allocator);
	data.AddMember("metatype",_metadata,allocator);
	std::string pubkey=key_manager->getPublicKey();
	rapidjson::Value _pubkey(pubkey.c_str(),pubkey.size(),allocator);
	data.AddMember("pubkey",_pubkey,allocator);
	rapidjson::Value time(getCurrentTime());
	data.AddMember("timestamp",time,allocator);
	return data;
}

rapidjson::Document& MessageManager::make_Request_Apply_Commissioner_Signature(NodeID receiver,rapidjson::Document& data)
{
	//rapidjson::Document &data=get_Apply_Commissioner_Metadata();

	rapidjson::Document &d=make_Message(receiver,RequestApplyCommissionerSignature,data,0,"");
	return d;
}

rapidjson::Document& MessageManager::make_Response_Apply_Commissioner_Signature(NodeID receiver,uint32_t response_index,rapidjson::Value& metadata)
{
	std::string sig=key_manager->signDocument(metadata);

	//rapidjson::Document &data=*(new rapidjson::Document);
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document::AllocatorType &allocator=data.GetAllocator();

	rapidjson::Value _sig(sig.c_str(),sig.size(),allocator);
	data.AddMember("sig",_sig,allocator);
	rapidjson::Value _pubkey(pubkey.c_str(),pubkey.size(),allocator);
	data.AddMember("pubkey",_pubkey,allocator);
	rapidjson::Document &d=make_Message(receiver,ResponseApplyCommissionerSignature,data,response_index,"");
	return d;
}

rapidjson::Document& MessageManager::make_Request_Apply_Commissioner(NodeID receiver,rapidjson::Document& data)
{
	rapidjson::Document &d=make_Message(receiver,RequestApplyCommissioner,data,0,"");
	return d;
}

rapidjson::Document& MessageManager::make_Response_Apply_Commissioner(NodeID receiver,uint32_t response_index)
{
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document &d=make_Message(receiver,ResponseApplyCommissionerSignature,data,response_index,"");
	return d;
}


rapidjson::Document& MessageManager::make_Request_Recommend_Letter(NodeID receiver)
{
	rapidjson::Document data;
	data.SetObject();
	//rapidjson::Document::AllocatorType &allocator=d.GetAllocator();
	//rapidjson::Value _index(index);
	//d.AddMember("index",_index,allocator);
	rapidjson::Document &d=make_Message(receiver,RequestRecommendationLetter,data,0,"");
	return d;
}

rapidjson::Document& MessageManager::get_Recommend_Letter_Metadata(std::string pubkey)
{
	//元数据需要有时间戳，不然每次生成的元数据都一样，签名结果也都一样，那么某节点退出委员后只要拿以前的签名就可以再次申请成为委员。管家在最终接收验证的时候需要对比时间戳是否比以前的时间更新。
	rapidjson::Document *d=new rapidjson::Document();
	rapidjson::Document& data=*d;
	data.SetObject();
	rapidjson::Document::AllocatorType &allocator=data.GetAllocator();
	//std::string metadata="ApplyCommissioner";
	//rapidjson::Value _metadata(metadata.c_str(),metadata.size(),allocator);
	rapidjson::Value _metadata("RecommendationLetter",allocator);
	data.AddMember("metatype",_metadata,allocator);

	std::string refferer_pubkey=key_manager->getPublicKey();
	rapidjson::Value _refferer_pubkey(refferer_pubkey.c_str(),refferer_pubkey.size(),allocator);
	data.AddMember("refferer",_refferer_pubkey,allocator);

	//std::string refferal_pubkey=key_manager->getPublicKey();
	rapidjson::Value _refferal_pubkey(pubkey.c_str(),pubkey.size(),allocator);
	data.AddMember("refferal",_refferal_pubkey,allocator);

	rapidjson::Value time(getCurrentTime());
	data.AddMember("timestamp",time,allocator);
	return data;
}

rapidjson::Document& MessageManager::make_Response_Recommend_Letter(NodeID receiver,uint32_t response_index,std::string refferal_pubkey)
{
	rapidjson::Document& metadata=get_Recommend_Letter_Metadata(refferal_pubkey);
	std::string sig=key_manager->signDocument(metadata);
	rapidjson::Document &data=*(new rapidjson::Document);
	data.SetObject();
	rapidjson::Document::AllocatorType &allocator=data.GetAllocator();
	data.AddMember("metadata",metadata,allocator);
	rapidjson::Value _sig(sig.c_str(),sig.size(),allocator);
	data.AddMember("sig",_sig,allocator);
	rapidjson::Document &d=make_Message(receiver,ResponseRecommendationLetter,data,response_index,"");
	metadata.GetAllocator().Clear();
	delete &data;
	return d;
}

rapidjson::Document& MessageManager::make_Request_Apply_Butler_Candidate_Signature(NodeID receiver,rapidjson::Document& d)
{
	//rapidjson::Document &letter=*(new rapidjson::Document);
	//rapidjson::Document::AllocatorType& allocator=letter.GetAllocator();
	//letter.CopyFrom(d,allocator);
	rapidjson::Document &msg=make_Message(receiver,RequestApplyButlerCandidateSignature,d,0,"");
	return msg;
}

rapidjson::Document& MessageManager::make_Response_Apply_Butler_Candidate_Signature(NodeID receiver,uint32_t response_index,rapidjson::Value& metadata)
{
	std::string sig=key_manager->signDocument(metadata);

	//rapidjson::Document &data=*(new rapidjson::Document);
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document::AllocatorType &allocator=data.GetAllocator();

	rapidjson::Value &_sig=*(new rapidjson::Value(sig.c_str(),sig.size(),allocator));
	data.AddMember("sig",_sig,allocator);
	rapidjson::Value &_pubkey=*(new rapidjson::Value(pubkey.c_str(),pubkey.size(),allocator));
	data.AddMember("pubkey",_pubkey,allocator);
	rapidjson::Document &d=make_Message(receiver,ResponseApplyButlerCandidateSignature,data,response_index,"");
	return d;
}

rapidjson::Document& MessageManager::make_Request_Apply_Butler_Candidate(NodeID receiver,rapidjson::Document& data)
{
	rapidjson::Document &msg=make_Message(receiver,RequestApplyButlerCandidate,data,0,"");
	return msg;
}

rapidjson::Document& MessageManager::make_Response_Apply_Butler_Candidate(NodeID receiver,uint32_t response_index)
{
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document &d=make_Message(receiver,ResponseApplyButlerCandidate,data,response_index,"");
	return d;
}

rapidjson::Document& MessageManager::make_Request_Vote(NodeID receiver,rapidjson::Document& ballot)
{
	std::string sig=key_manager->signDocument(ballot);
	rapidjson::Document& data=*(new rapidjson::Document);
	//rapidjson::Document data;
	data.SetObject();
	rapidjson::Value temp_ballot;
	temp_ballot.CopyFrom(ballot,data.GetAllocator());
	data.AddMember("metadata",temp_ballot,data.GetAllocator());
	rapidjson::Value& _sig=*(new rapidjson::Value(sig.c_str(),sig.size(),data.GetAllocator()));

	rapidjson::Value _metatype("Ballot",data.GetAllocator());
	data.AddMember("metatype",_metatype,data.GetAllocator());

	data.AddMember("sig",_sig,data.GetAllocator());
	rapidjson::Document &msg=make_Message(receiver,RequestVote,data,0,"");
	delete &data;
	return msg;
}

rapidjson::Document& MessageManager::make_Response_Vote(NodeID receiver,uint32_t response_index)
{
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document &d=make_Message(receiver,ResponseVote,data,response_index,"");
	return d;
}

rapidjson::Document& MessageManager::make_Request_Block_Signature(NodeID receiver,rapidjson::Document& block)
{
	rapidjson::Document &msg=make_Message(receiver,RequestBlockSignature,block,0,"");
	return msg;
}

rapidjson::Document& MessageManager::make_Response_Block_Signature(NodeID receiver,uint32_t response_index,rapidjson::Value& header,std::string name)
{
	double time=getCurrentTime();
	//header["Te"].SetDouble(time);
	std::string sig=key_manager->signDocument(header);
	//std::cout<<"对去块头进行签名:\n";
	//std::cout<<"pubkey:"<<pubkey<<"\n";
	//std::cout<<"sig:"<<sig<<"\n";
	//std::cout<<"raw header:\n";
	//print_document(header);
	//rapidjson::Document &data=*(new rapidjson::Document);
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document::AllocatorType &allocator=data.GetAllocator();
	rapidjson::Value status(0);
	data.AddMember("status",status,allocator);
	rapidjson::Value _name(name.c_str(),name.size(),allocator);
	data.AddMember("name",_name,allocator);
	rapidjson::Value _sig(sig.c_str(),sig.size(),allocator);
	data.AddMember("sig",_sig,allocator);
	rapidjson::Value _pubkey(pubkey.c_str(),pubkey.size(),allocator);
	data.AddMember("pubkey",_pubkey,allocator);
	rapidjson::Value _Te(time);
	data.AddMember("timestamp",_Te,allocator);

	rapidjson::Document &d=make_Message(receiver,ResponseBlockSignature,data,response_index,"");
	return d;
}

//生成申请区块签名回复失败
rapidjson::Document& MessageManager::make_Response_Block_Signature_Failed(NodeID receiver,uint32_t response_index,int status,std::string err_msg,rapidjson::Value& header,std::string name)
{
	double time=getCurrentTime();
	//header["Te"].SetDouble(time);
	//std::cout<<"对去块头进行签名:\n";
	//std::cout<<"pubkey:"<<pubkey<<"\n";
	//std::cout<<"sig:"<<sig<<"\n";
	//std::cout<<"raw header:\n";
	//print_document(header);
	//rapidjson::Document &data=*(new rapidjson::Document);
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document::AllocatorType &allocator=data.GetAllocator();
	rapidjson::Value _pubkey(pubkey.c_str(),pubkey.size(),allocator);
	data.AddMember("pubkey",_pubkey,allocator);
	rapidjson::Value _status(status);
	data.AddMember("status",_status,allocator);
	rapidjson::Value _name(name.c_str(),name.size(),allocator);
	data.AddMember("name",_name,allocator);
	rapidjson::Value _msg(err_msg.c_str(),err_msg.size(),allocator);
	data.AddMember("err_msg",_msg,allocator);

	//因为预区块在验证失败后，值班管家会重新生成预区块，因此要验证返回的错误消息是不是最新的预区块的错误消息。
	std::string sig=key_manager->signDocument(header);
	rapidjson::Value _sig(sig.c_str(),sig.size(),allocator);
	data.AddMember("sig",_sig,allocator);
	//data.AddMember("data",resp_data,allocator);

	rapidjson::Value _Te(time);
	data.AddMember("timestamp",_Te,allocator);

	rapidjson::Document &d=make_Message(receiver,ResponseBlockSignature,data,response_index,"");
	return d;
}

rapidjson::Document& MessageManager::make_Request_Publish_Block(rapidjson::Document& block)
{
	rapidjson::Document &msg=make_Message(0,PublishBlock,block,0,"");
	return msg;
}

rapidjson::Document& MessageManager::make_Request_Publish_Block(NodeID receiver,rapidjson::Document& block)
{
	rapidjson::Document &msg=make_Message(receiver,PublishBlock,block,0,"");
	return msg;
}

rapidjson::Document& MessageManager::make_Request_NodeID_By_Pubkey(std::string pubkey)
{
	rapidjson::Document& data=*(new rapidjson::Document);
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	rapidjson::Value& content=*(new rapidjson::Value(pubkey.c_str(),pubkey.size(),allocator));
	data.AddMember("pubkey",content,allocator);
	rapidjson::Value& data_value=*(new rapidjson::Value);
	rapidjson::Document &msg=make_Message(0,RequestNodeID,data,0,"");
	//allocator.Clear();
	//delete &data;
	//print_document(msg);
	delete &data;
	return msg;
}

rapidjson::Document& MessageManager::make_Response_NodeID_By_Pubkey(NodeID receiver,uint32_t response_index)
{
	//rapidjson::Document& data=*(new rapidjson::Document);
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document &msg=make_Message(receiver,ResponseNodeID,data,response_index,"");
	return msg;
}

rapidjson::Document& MessageManager::make_Request_Normal(uint32_t len)
{
	rapidjson::Document& data=*(new rapidjson::Document);
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	rapidjson::Value _metatype("Normal",data.GetAllocator());
	data.AddMember("metatype",_metatype,data.GetAllocator());
	std::string rand_str=getRandomString(len);
	rapidjson::Value& content=*(new rapidjson::Value(rand_str.c_str(),rand_str.size(),allocator));
	data.AddMember("content",content,allocator);
	//rapidjson::Value& timestamp=*(new rapidjson::Value(rand_str.c_str(),rand_str.size(),allocator));
	data.AddMember("timestamp",getCurrentTime(),allocator);
	rapidjson::Document &msg=make_Message(0,RequestNormal,data,0,"");
	delete &data;
	return msg;
}

rapidjson::Document& MessageManager::make_Request_Normal_for_Test(NodeID receiver)
{
	rapidjson::Document& data=*(new rapidjson::Document);
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	rapidjson::Value _metatype("Normal",data.GetAllocator());
	data.AddMember("metatype",_metatype,data.GetAllocator());
	//std::string rand_str=getRandomString(len);
	rapidjson::Value content;
	content.SetObject();
	string prikey=getRandomString(64);
	rapidjson::Value Prikey(prikey.c_str(),prikey.size(),allocator);
	content.AddMember("prikey",Prikey,allocator);
	string account="0x"+getRandomString(32);
	rapidjson::Value Account(account.c_str(),account.size(),allocator);
	content.AddMember("Account",Account,allocator);
	string http_provider="http://0.0.0.0:8445";
	rapidjson::Value HttpProvider(http_provider.c_str(),http_provider.size(),allocator);
	content.AddMember("http_provider",HttpProvider,allocator);
	string ouputpath="./output/";
	rapidjson::Value Outputpath(ouputpath.c_str(),ouputpath.size(),allocator);
	content.AddMember("ouputpath",Outputpath,allocator);
	int EncryptType=0;
	content.AddMember("EncryptType",EncryptType,allocator);
	data.AddMember("content",content,allocator);
	//rapidjson::Value& timestamp=*(new rapidjson::Value(rand_str.c_str(),rand_str.size(),allocator));
	data.AddMember("timestamp",getCurrentTime(),allocator);
	//rapidjson::StringBuffer buffer;
	//rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	//data.Accept(writer);
	//std::string cont=buffer.GetString();
	//std::cout<<"交易大小："<<cont.size()<<"\n";
	rapidjson::Document &msg=make_Message(receiver,RequestNormal,data,0,"");
	delete &data;
	return msg;
}
rapidjson::Document& MessageManager::make_Request_Normal(NodeID receiver,rapidjson::Document& content)
{
	rapidjson::Document& data=*(new rapidjson::Document);
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	rapidjson::Value _metatype("Normal",data.GetAllocator());
	data.AddMember("metatype",_metatype,data.GetAllocator());
	rapidjson::Value _content;
	_content.CopyFrom(content,allocator);
	data.AddMember("content",_content,allocator);
	//rapidjson::Value& timestamp=*(new rapidjson::Value(rand_str.c_str(),rand_str.size(),allocator));
	data.AddMember("timestamp",getCurrentTime(),allocator);
	rapidjson::Document &msg=make_Message(receiver,RequestNormal,data,0,"");
	delete &data;
	return msg;
}

rapidjson::Document& MessageManager::make_Response_Normal(NodeID receiver,uint32_t response_index)
{
	//rapidjson::Document& data=*(new rapidjson::Document);
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document &msg=make_Message(receiver,ResponseNormal,data,response_index,"");
	data.GetAllocator().Clear();
	return msg;
}

rapidjson::Document& MessageManager::get_Quit_Butler_Candidate_Metadata()
{
	rapidjson::Document& data=*(new rapidjson::Document);
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	//构建metatype
	rapidjson::Value _metatype("QuitButlerCandidate",allocator);
	data.AddMember("metatype",_metatype,data.GetAllocator());
	//构建metadata
	rapidjson::Value &_metadata=*(new rapidjson::Value());
	_metadata.SetObject();
	rapidjson::Value submetatype("QuitButlerCandidateMetadata",allocator);
	_metadata.AddMember("type",submetatype,allocator);
	rapidjson::Value _pubkey(pubkey.c_str(),pubkey.size(),allocator);
	_metadata.AddMember("pubkey",_pubkey,allocator);
	_metadata.AddMember("timestamp",getCurrentTime(),allocator);
	data.AddMember("metadata",_metadata,allocator);
	//构建sig
	rapidjson::Value &token=data["metadata"];
	std::string sig=key_manager->signDocument(token);
	//std::cout<<"生成Quit_Butler_Candidate_Metadata时的打印结果:\n";
	//print_document(token);
	//std::cout<<"pubkey="<<pubkey<<"\n";
	//std::cout<<"sig="<<sig<<"\n";
	rapidjson::Value _sig(sig.c_str(),sig.size(),allocator);
	data.AddMember("sig",_sig,allocator);
	return data;
}

//rapidjson::Document& get_Quit_Commissioner_Metadata();

rapidjson::Document& MessageManager::make_Request_Quit_Butler_Candidate(NodeID receiver,rapidjson::Document& data)
{
	rapidjson::Document &msg=make_Message(receiver,RequestQuitButlerCandidate,data,0,"");
	return msg;
}

rapidjson::Document& MessageManager::get_Quit_Commissioner_Metadata()
{
	rapidjson::Document& data=*(new rapidjson::Document);
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	//构建metatype
	rapidjson::Value _metatype("QuitCommissioner",allocator);
	data.AddMember("metatype",_metatype,data.GetAllocator());
	//构建metadata
	rapidjson::Value &_metadata=*(new rapidjson::Value());
	_metadata.SetObject();
	rapidjson::Value submetatype("QuitCommissionerMetadata",allocator);
	_metadata.AddMember("type",submetatype,allocator);
	rapidjson::Value _pubkey(pubkey.c_str(),pubkey.size(),allocator);
	_metadata.AddMember("pubkey",_pubkey,allocator);
	_metadata.AddMember("timestamp",getCurrentTime(),allocator);
	data.AddMember("metadata",_metadata,allocator);
	//构建sig
	rapidjson::Value &token=data["metadata"];
	std::string sig=key_manager->signDocument(token);
	//std::cout<<"生成Quit_Butler_Candidate_Metadata时的打印结果:\n";
	//print_document(token);
	//std::cout<<"pubkey="<<pubkey<<"\n";
	//std::cout<<"sig="<<sig<<"\n";
	rapidjson::Value _sig(sig.c_str(),sig.size(),allocator);
	data.AddMember("sig",_sig,allocator);
	return data;
}
rapidjson::Document& MessageManager::make_Response_Quit_Butler_Candidate(NodeID receiver,uint32_t response_index)
{
	//rapidjson::Document& data=*(new rapidjson::Document);
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document &msg=make_Message(receiver,ResponseQuitButlerCandidate,data,response_index,"");
	data.GetAllocator().Clear();
	return msg;
}
rapidjson::Document& MessageManager::make_Request_Quit_Commissioner(NodeID receiver,rapidjson::Document& data)
{
	rapidjson::Document &msg=make_Message(receiver,RequestQuitCommissioner,data,0,"");
	return msg;
}
rapidjson::Document& MessageManager::make_Response_Quit_Commissioner(NodeID receiver,uint32_t response_index)
{
	//rapidjson::Document& data=*(new rapidjson::Document);
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document &msg=make_Message(receiver,ResponseQuitCommissioner,data,response_index,"");
	data.GetAllocator().Clear();
	return msg;
}
rapidjson::Document& MessageManager::make_Request_Height()
{
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	rapidjson::Value _metatype("request_height",data.GetAllocator());
	data.AddMember("metatype",_metatype,data.GetAllocator());
	rapidjson::Document &msg=make_Message(0,RequestHeight,data,0,"");
	return msg;
}
rapidjson::Document& MessageManager::make_Response_Height(NodeID receiver,int height)
{
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	rapidjson::Value _metatype("response_height",data.GetAllocator());
	data.AddMember("metatype",_metatype,data.GetAllocator());
	rapidjson::Value _height(height);
	data.AddMember("height",_height,data.GetAllocator());
	rapidjson::Document &msg=make_Message(receiver,ResponseHeight,data,0,"");
	return msg;
}
rapidjson::Document& MessageManager::make_Request_Block(NodeID receiver,int height)
{
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	rapidjson::Value _metatype("request_block",data.GetAllocator());
	data.AddMember("metatype",_metatype,data.GetAllocator());
	rapidjson::Value _height(height);
	data.AddMember("height",_height,data.GetAllocator());
	rapidjson::Document &msg=make_Message(receiver,RequestBlock,data,0,"");
	return msg;
}
rapidjson::Document& MessageManager::make_Response_Block(NodeID receiver,uint32_t response_index,rapidjson::Value& block)
{
	rapidjson::Document data;
	data.SetObject();
	rapidjson::Document::AllocatorType& allocator=data.GetAllocator();
	rapidjson::Value _metatype("response_height",data.GetAllocator());
	data.AddMember("metatype",_metatype,data.GetAllocator());
	data.AddMember("block",block,data.GetAllocator());
	rapidjson::Document &msg=make_Message(receiver,ResponseBlock,data,response_index,"");
	return msg;
}


//rapidjson::Document& MessageManager::make_Request_Quit_Commissioner(NodeID receiver);
//rapidjson::Document& MessageManager::make_Response_Quit_Commissioner(uint32_t receiver,uint32_t response_index);

