#include "POV.h"

//测试生成NDN数据
void POV::testGenNDN()
{
	rapidjson::Document test_generate;
	test_generate.SetObject();
	rapidjson::Document::AllocatorType& allocator=test_generate.GetAllocator();
	rapidjson::Value type("NDN-IP",allocator);
	test_generate.AddMember("type",type,allocator);
	//构造data字段
	rapidjson::Value data;
	data.SetObject();
	rapidjson::Value command("Generate",allocator);
	data.AddMember("command",command,allocator);
	rapidjson::Value NDN("/blackguess/渣渣辉",allocator);
	data.AddMember("NDN",NDN,allocator);

	rapidjson::Value IP("127.0.0.1",allocator);
	data.AddMember("IP",IP,allocator);

	std::string pubkey_str=key_manager.getPublicKey();
	rapidjson::Value pubkey(pubkey_str.c_str(),pubkey_str.size(),allocator);
	data.AddMember("pubkey",pubkey,allocator);

	std::string hash_str="random hash";
	rapidjson::Value hash(hash_str.c_str(),hash_str.size(),allocator);
	data.AddMember("hash",hash,allocator);

	rapidjson::Value time(22.35);
	data.AddMember("timestamp",time,allocator);

	std::string sig_str=key_manager.signDocument(data);
	rapidjson::Value sig(sig_str.c_str(),sig_str.size(),allocator);
	test_generate.AddMember("sig",sig,allocator);

	std::cout<<"生成NDN Data：\n";
	print_document(data);
	test_generate.AddMember("data",data,allocator);


	std::cout<<"生成NDN请求：\n";
	print_document(test_generate);
	ErrorMessage result=handleNDNQuery(test_generate);
	std::cout<<"生成NDN结果：\n";
	std::cout<<"result.code="<<result.errcode<<"\n";
	std::cout<<"result.msg="<<result.msg<<"\n";
	if(validateNDNTransaction(test_generate))
	{
		std::cout<<"验证生成NDN成功\n";
	}
	else
	{
		std::cout<<"验证生成NDN失败\n";
	}
	updateNDNTransaction(test_generate);
}
//测试更新NDN功能
void POV::testUpdateNDN()
{
	rapidjson::Document test_generate;
	test_generate.SetObject();
	rapidjson::Document::AllocatorType& allocator=test_generate.GetAllocator();
	rapidjson::Value type("NDN-IP",allocator);
	test_generate.AddMember("type",type,allocator);
	//构造data字段
	rapidjson::Value data;
	data.SetObject();
	rapidjson::Value command("Update",allocator);
	data.AddMember("command",command,allocator);
	rapidjson::Value NDN("/blackguess/渣渣辉",allocator);
	data.AddMember("NDN",NDN,allocator);

	rapidjson::Value IP("192.168.1.1",allocator);
	data.AddMember("IP",IP,allocator);

	std::string hash_str="random hash";
	rapidjson::Value hash(hash_str.c_str(),hash_str.size(),allocator);
	data.AddMember("hash",hash,allocator);

	rapidjson::Value time(22.35);
	data.AddMember("timestamp",time,allocator);

	std::string sig_str=key_manager.signDocument(data);
	rapidjson::Value sig(sig_str.c_str(),sig_str.size(),allocator);

	test_generate.AddMember("sig",sig,allocator);
	test_generate.AddMember("data",data,allocator);


	ErrorMessage result=handleNDNQuery(test_generate);
	std::cout<<"更新NDN结果：\n";
	std::cout<<"result.code="<<result.errcode<<"\n";
	std::cout<<"result.msg="<<result.msg<<"\n";
	if(validateNDNTransaction(test_generate))
	{
		std::cout<<"验证更新NDN成功\n";
	}
	else
	{
		std::cout<<"验证更新NDN失败\n";
	}
	updateNDNTransaction(test_generate);
}
//测试删除NDN功能
void POV::testDeleteNDN()
{
	rapidjson::Document test_generate;
	test_generate.SetObject();
	rapidjson::Document::AllocatorType& allocator=test_generate.GetAllocator();
	rapidjson::Value type("NDN-IP",allocator);
	test_generate.AddMember("type",type,allocator);
	//构造data字段
	rapidjson::Value data;
	data.SetObject();
	rapidjson::Value command("Delete",allocator);
	data.AddMember("command",command,allocator);
	rapidjson::Value NDN("/blackguess/渣渣辉",allocator);
	data.AddMember("NDN",NDN,allocator);

	rapidjson::Value time(22.35);
	data.AddMember("timestamp",time,allocator);

	std::string sig_str=key_manager.signDocument(data);
	rapidjson::Value _sig(sig_str.c_str(),sig_str.size(),allocator);

	test_generate.AddMember("sig",_sig,allocator);
	test_generate.AddMember("data",data,allocator);


	ErrorMessage result=handleNDNQuery(test_generate);
	std::cout<<"删除NDN结果：\n";
	std::cout<<"result.code="<<result.errcode<<"\n";
	std::cout<<"result.msg="<<result.msg<<"\n";
	if(validateNDNTransaction(test_generate))
	{
		std::cout<<"验证删除NDN成功\n";
	}
	else
	{
		std::cout<<"验证删除NDN失败\n";
	}
	updateNDNTransaction(test_generate);
}
//测试查询NDN功能
void POV::testQueryNDN()
{
	rapidjson::Document test_generate;
	test_generate.SetObject();
	rapidjson::Document::AllocatorType& allocator=test_generate.GetAllocator();
	rapidjson::Value type("NDN-IP",allocator);
	test_generate.AddMember("type",type,allocator);
	//构造data字段
	rapidjson::Value data;
	data.SetObject();
	rapidjson::Value command("Query",allocator);
	data.AddMember("command",command,allocator);
	rapidjson::Value NDN("/blackguess/渣渣辉",allocator);
	data.AddMember("NDN",NDN,allocator);

	rapidjson::Value QueryCode(0);
	data.AddMember("QueryCode",QueryCode,allocator);
	test_generate.AddMember("data",data,allocator);

	ErrorMessage result=handleNDNQuery(test_generate);
	std::cout<<"查询NDN结果：\n";
	std::cout<<"msg.code="<<result.errcode<<"\n";
	std::cout<<"msg.msg="<<result.msg<<"\n";
}
//测试注册NDN功能
void POV::testRegistryNDN()
{
	rapidjson::Document testRegistry;
	testRegistry.SetObject();
	rapidjson::Document::AllocatorType& allocator=testRegistry.GetAllocator();
	rapidjson::Value type("NDN-IP",allocator);
	testRegistry.AddMember("type",type,allocator);
	//构造data字段
	rapidjson::Value data;
	data.SetObject();
	rapidjson::Value command("Registry",allocator);
	data.AddMember("command",command,allocator);

	std::string pubkey_str=key_manager.getPublicKey();
	rapidjson::Value pubkey(pubkey_str.c_str(),pubkey_str.size(),allocator);
	data.AddMember("pubkey",pubkey,allocator);

	std::string prefix_str="/blackguess";
	rapidjson::Value prefix(prefix_str.c_str(),prefix_str.size(),allocator);
	data.AddMember("prefix",prefix,allocator);

	rapidjson::Value level(0);
	data.AddMember("level",level,allocator);

	rapidjson::Value time(22.35);
	data.AddMember("timestamp",time,allocator);

	std::string sig_str=key_manager.signDocument(data);
	rapidjson::Value _sig(sig_str.c_str(),sig_str.size(),allocator);

	testRegistry.AddMember("sig",_sig,allocator);
	testRegistry.AddMember("data",data,allocator);


	ErrorMessage result=handleNDNQuery(testRegistry);
	//validateNDNTransaction(testRegistry);
	//updateNDNTransaction(testRegistry);
	std::cout<<"注册NDN用户结果：\n";
	std::cout<<"result.code="<<result.errcode<<"\n";
	std::cout<<"result.msg="<<result.msg<<"\n";
//	if(validateNDNTransaction(testRegistry))
//	{
//		std::cout<<"验证注册NDN用户成功\n";
//	}
//	else
//	{
//		std::cout<<"验证注册NDN用户失败\n";
//	}
//	updateNDNTransaction(testRegistry);
}
//测试查询用户功能
void POV::testgetUserNDN()
{
	rapidjson::Document testGetUser;
	testGetUser.SetObject();
	rapidjson::Document::AllocatorType& allocator=testGetUser.GetAllocator();
	rapidjson::Value type("NDN-IP",allocator);
	testGetUser.AddMember("type",type,allocator);
	//构造data字段
	rapidjson::Value data;
	data.SetObject();
	rapidjson::Value command("getUser",allocator);
	data.AddMember("command",command,allocator);

	std::string pubkey_str=key_manager.getPublicKey();
	rapidjson::Value pubkey(pubkey_str.c_str(),pubkey_str.size(),allocator);
	data.AddMember("pubkey",pubkey,allocator);
	testGetUser.AddMember("data",data,allocator);

	ErrorMessage result=handleNDNQuery(testGetUser);
	std::cout<<"查询NDN用户信息：\n";
	std::cout<<"msg.code="<<result.errcode<<"\n";
	std::cout<<"msg.msg="<<result.msg<<"\n";
}
