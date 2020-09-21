#include <ledger/ledger_manager.h>
#include "contract_manager.h"

namespace CEG {
	void V8Contract::CallBackLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
		if (args.Length() < 1) {
			args.GetReturnValue().Set(false);
			return;
		}

		v8::HandleScope scope(args.GetIsolate());
		V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
		if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
			LOG_TRACE("Failed to find contract object by isolate id");
			return;
		}
		std::string this_contract = v8_contract->parameter_.this_address_;
		v8::String::Utf8Value str1(args[0]);
		const char* cstr = ToCString(str1);
		LOG_TRACE("V8contract log[%s:%s]\n%s", this_contract.c_str(), v8_contract->parameter_.msg_.sender_.c_str(), cstr);
		//v8_contract->AddLog(cstr);

		return;
	}

	void V8Contract::CallBackGetAccountAsset(const v8::FunctionCallbackInfo<v8::Value>& args) {
		if (args.Length() != 2) {
			LOG_TRACE("parameter error");
			args.GetReturnValue().Set(false);
			return;
		}

		do {
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_TRACE("Contract execution error, CallBackGetAccountAsset, parameter 1 should be a String");
				break;
			}
			std::string address = ToCString(v8::String::Utf8Value(args[0]));

			if (!args[1]->IsObject()) {
				LOG_TRACE("Contract execution error, CallBackGetAccountAsset parameter 2 should be a object");
				break;
			}

			protocol::AssetKey asset_key = GetAssetFromJsObject(args.GetIsolate(), args[1]->ToObject());
			CEG::AccountFrm::pointer account_frm = nullptr;
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				LOG_TRACE("Failed to find account %s.", address.c_str());
				break;
			}

			protocol::AssetStore asset;
			if (!account_frm->GetAsset(asset_key, asset)) {
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), utils::String::ToString(asset.amount()).c_str()));
			return;
		} while (false);

		args.GetReturnValue().Set(false);
	}

	void V8Contract::CallBackGetContractProperty(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 1) {
				error_desc = "parameter error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				error_desc = "contract execution error, parameter 0 should be a string";
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			std::string address = ToCString(v8::String::Utf8Value(args[0]));
			AccountFrm::pointer account_frm = NULL;

			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				error_desc = utils::String::Format("Failed to find account %s.", address.c_str());
				break;
			}

			double type = -1;
			double length = 0;
			const protocol::Account &proto_account = account_frm->GetProtoAccount();
			if (proto_account.has_contract()) {
				const protocol::Contract &proto_contract = proto_account.contract();
				type = (double)proto_contract.type();
				length = (double)proto_contract.payload().size();
			}

			v8::Local<v8::Object> obj = v8::Object::New(args.GetIsolate());
			v8::Local<v8::Number> number_type = v8::Number::New(args.GetIsolate(), type);
			obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "type"), number_type);

			v8::Local<v8::Number> number_length = v8::Number::New(args.GetIsolate(), length);
			obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "length"), number_length);

			args.GetReturnValue().Set(obj);
			return;
		} while (false);

		LOG_TRACE("%s", error_desc.c_str());
		args.GetReturnValue().Set(false);
	}

	void V8Contract::CallBackGetAccountMetadata(const v8::FunctionCallbackInfo<v8::Value>& args) {
		if (args.Length() != 2) {
			LOG_TRACE("parameter error");
			args.GetReturnValue().Set(false);
			return;
		}

		do {
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString() || !args[1]->IsString()) {
				LOG_TRACE("Contract execution error, CallBackGetAccountMetadata, parameter should be string");
				break;
			}

			std::string address = ToCString(v8::String::Utf8Value(args[0]));
			std::string key = ToCString(v8::String::Utf8Value(args[1]));

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			CEG::AccountFrm::pointer account_frm = nullptr;
			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				LOG_TRACE("Failed to find account %s.", address.c_str());
				break;
			}

			protocol::KeyPair key_pair;
			if (!account_frm->GetMetaData(key, key_pair)) {
				LOG_ERROR("Failed to find account %s 's metadata key:%s", address.c_str(), key.c_str());
				break;
			}
			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), key_pair.value().c_str(), v8::NewStringType::kNormal).ToLocalChecked());

			return;
		} while (false);

		args.GetReturnValue().Set(false);
	}

	void V8Contract::CallBackContractQueryGt1001(const v8::FunctionCallbackInfo<v8::Value>& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		v8::Local<v8::Object> obj = v8::Object::New(args.GetIsolate());
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "parameter error";
				break;
			}

			if (!args[0]->IsString()) { //the called contract address
				error_desc = "contract execution error,CallBackContractQuery, parameter 0 should be a String";
				break;
			}

			if (!args[1]->IsString()) {
				error_desc = "contract execution error,CallBackContractQuery, parameter 1 should be a String";
				break;
			}

			std::string address = ToCString(v8::String::Utf8Value(args[0]));
			std::string input = ToCString(v8::String::Utf8Value(args[1]));

			CEG::AccountFrm::pointer account_frm = nullptr;
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				error_desc = utils::String::Format("Failed to find account %s.", address.c_str());
				break;
			}

			if (!account_frm->GetProtoAccount().has_contract()) {
				error_desc = utils::String::Format("The account(%s) has no contract.", address.c_str());
				break;
			}

			protocol::Contract contract = account_frm->GetProtoAccount().contract();
			if (contract.payload().size() == 0) {
				error_desc = utils::String::Format("The account(%s) has no contract.", address.c_str());
				break;
			}

			ledger_context->transaction_stack_.push_back(ledger_context->GetTopTx());

			ContractParameter parameter = v8_contract->GetParameter();
			//contract
			parameter.code_ = contract.payload();
			parameter.init_ = false;
			parameter.input_ = input;
			parameter.this_address_ = address;

			//msg
			parameter.msg_.Reset();
			parameter.msg_.coin_amount_ = 0;
			parameter.msg_.asset_.Clear();
			parameter.msg_.initiator_ = v8_contract->GetParameter().msg_.initiator_;
			parameter.msg_.nonce_ = 0;
			parameter.msg_.operation_index_ = 0;
			parameter.msg_.sender_ = v8_contract->GetParameter().this_address_;

			Json::Value query_result;
			bool ret = ContractManager::Instance().Query(contract.type(), parameter, query_result);
			ledger_context->transaction_stack_.pop_back();

			//Just like this, { "result": "abcde"} or {"error" : true}
			if (!ret) {
				error_desc = "Failed to query";
				break;
			}

			Json::Value js_object = query_result["result"];
			v8::Local<v8::Value> v8_result;
			CppJsonToJsValue(args.GetIsolate(), js_object, v8_result);
			obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "result"), v8_result);
			args.GetReturnValue().Set(obj);

			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		v8::Local<v8::Boolean> flag_false = v8::Boolean::New(args.GetIsolate(), true);
		obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "error"), flag_false);
		args.GetReturnValue().Set(obj);
	}

	void V8Contract::CallBackContractQuery(const v8::FunctionCallbackInfo<v8::Value>& args) {
		if (CHECK_VERSION_GT_1001) {
			CallBackContractQueryGt1001(args);
			return;
		} 

		v8::HandleScope handle_scope(args.GetIsolate());
		v8::Local<v8::Object> obj = v8::Object::New(args.GetIsolate());
		v8::Local<v8::Boolean> flag_false = v8::Boolean::New(args.GetIsolate(), false);
		obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "success"), flag_false);

		do {
			if (args.Length() != 2) {
				LOG_TRACE("parameter error");
				args.GetReturnValue().Set(false);
				break;
			}

			if (!args[0]->IsString()) { //the called contract address
				LOG_TRACE("contract execution error,CallBackContractQuery, parameter 0 should be a String");
				break;
			}

			if (!args[1]->IsString()) {
				LOG_TRACE("contract execution error,CallBackContractQuery, parameter 1 should be a String");
				break;
			}

			std::string address = ToCString(v8::String::Utf8Value(args[0]));
			std::string input = ToCString(v8::String::Utf8Value(args[1]));

			CEG::AccountFrm::pointer account_frm = nullptr;
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(1000);

			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				LOG_TRACE("Failed to find account %s.", address.c_str());
				break;
			}

			if (!account_frm->GetProtoAccount().has_contract()) {
				LOG_TRACE("The account(%s) has no contract.", address.c_str());
				break;
			}

			protocol::Contract contract = account_frm->GetProtoAccount().contract();
			if (contract.payload().size() == 0) {
				LOG_TRACE("The account(%s) has no contract.", address.c_str());
				break;
			}

			ContractParameter parameter = v8_contract->GetParameter();
			//contract
			parameter.code_ = contract.payload();
			
			parameter.init_ = false;
			parameter.this_address_ = address;
			parameter.input_ = input;
			

			//msg
			parameter.msg_.Reset();
			parameter.msg_.coin_amount_ = 0;
			parameter.msg_.asset_.Clear();
			parameter.msg_.initiator_ = v8_contract->GetParameter().msg_.initiator_;
			parameter.msg_.nonce_ = 0;
			parameter.msg_.operation_index_ = 0;
			parameter.msg_.sender_ = v8_contract->GetParameter().this_address_;

			//Query
			Json::Value query_result;
			bool ret = ContractManager::Instance().Query(contract.type(), parameter, query_result);

			//Just like this, {"success": true, "result": "abcde"}
			if (!ret) {
				v8::Local<v8::Boolean> flag = v8::Boolean::New(args.GetIsolate(), true);
				obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "error"), flag);
			}
			else {
				Json::Value js_object = query_result["result"];
				v8::Local<v8::Value> v8_result;
				CppJsonToJsValue(args.GetIsolate(), js_object, v8_result);
				obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "result"), v8_result);
			}


		} while (false);

		args.GetReturnValue().Set(obj);
	}

	void V8Contract::CallBackDelegateQuery(const v8::FunctionCallbackInfo<v8::Value>& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		v8::Local<v8::Object> obj = v8::Object::New(args.GetIsolate());
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "parameter error";
				break;
			}

			if (!args[0]->IsString()) { //the called contract address
				error_desc = "contract execution error, CallBackDelegateQuery, parameter 0 should be a String";
				break;
			}

			if (!args[1]->IsString()) {
				error_desc = "contract execution error, CallBackDelegateQuery, parameter 1 should be a String";
				break;
			}

			std::string address = ToCString(v8::String::Utf8Value(args[0]));
			std::string input = ToCString(v8::String::Utf8Value(args[1]));

			CEG::AccountFrm::pointer account_frm = nullptr;
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				error_desc = utils::String::Format("Failed to find account %s.", address.c_str());
				break;
			}

			if (!account_frm->GetProtoAccount().has_contract()) {
				error_desc = utils::String::Format("The account(%s) has no contract.", address.c_str());
				break;
			}

			protocol::Contract contract = account_frm->GetProtoAccount().contract();
			if (contract.payload().size() == 0) {
				error_desc = utils::String::Format("The account(%s) has no contract.", address.c_str());
				break;
			}

			ledger_context->transaction_stack_.push_back(ledger_context->GetTopTx());

			ContractParameter parameter = v8_contract->GetParameter();
			//contract
			parameter.code_ = contract.payload();
			parameter.init_ = false;
			parameter.input_ = input;
			parameter.this_address_ = v8_contract->GetParameter().this_address_;

			Json::Value query_result;
			bool ret = ContractManager::Instance().Query(contract.type(), parameter, query_result);
			ledger_context->transaction_stack_.pop_back();

			//Just like this, { "result": "abcde"} or {"error" : true}
			if (!ret) {
				error_desc = "Failed to query";
				break;
			}

			Json::Value js_object = query_result["result"];
			v8::Local<v8::Value> v8_result;
			CppJsonToJsValue(args.GetIsolate(), js_object, v8_result);
			obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "result"), v8_result);
			args.GetReturnValue().Set(obj);

			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		v8::Local<v8::Boolean> flag_false = v8::Boolean::New(args.GetIsolate(), true);
		obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "error"), flag_false);
		args.GetReturnValue().Set(obj);
	}
	
	void V8Contract::CallBackGetValidators(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 0) {
				LOG_TRACE("parameter error");
				args.GetReturnValue().Set(false);
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());

			Json::Value jsonValidators;
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			jsonValidators = ledger_context->GetTopTx()->environment_->GetValidators();

			std::string strvalue = jsonValidators.toFastString();
			v8::Local<v8::String> returnvalue = v8::String::NewFromUtf8(args.GetIsolate(), strvalue.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
			args.GetReturnValue().Set(v8::JSON::Parse(returnvalue));

			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	void V8Contract::CallBackAddressValidCheck(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 1) {
				error_desc = "parameter number error";
				break;
			}

			if (!args[0]->IsString()) {
				error_desc = "arg0 should be string";
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			v8::String::Utf8Value utf8(args[0]);
			std::string address = std::string(ToCString(utf8));
			bool ret = PublicKey::IsAddressValid(address);

			args.GetReturnValue().Set(ret);
			return;
		} while (false);

		args.GetReturnValue().Set(false);
	}
	//Get the balance of the given account 
	void V8Contract::CallBackGetBalance(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1) {
				LOG_TRACE("parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_TRACE("contract execution error, parameter 0 should be a string");
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				LOG_TRACE("Failed to find contract object by isolate id");
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			std::string address = ToCString(v8::String::Utf8Value(args[0]));
			AccountFrm::pointer account_frm = NULL;

			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				Environment::AccountFromDB(address, account_frm);
			}

			std::string balance = "0";
			if (account_frm) {
				balance = utils::String::ToString(account_frm->GetAccountBalance());
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), balance.c_str(),
				v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);

		args.GetReturnValue().Set(false);
	}

	// 	//Get the hash value of one of the 1024 most recent complete blocks
	void V8Contract::CallBackGetBlockHash(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1) {
				LOG_TRACE("parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsNumber()) {
				LOG_TRACE("contract execution error, parameter 0 should be a string");
				break;
			}
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				LOG_TRACE("Failed to find contract object by isolate id");
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			protocol::LedgerHeader lcl = LedgerManager::Instance().GetLastClosedLedger();
			int64_t seq = lcl.seq() - (int64_t)args[0]->NumberValue();
			if (seq <= lcl.seq() - 1024 || seq > lcl.seq()) {
				LOG_TRACE("The parameter seq(" FMT_I64 ") <= " FMT_I64 " or > " FMT_I64, seq, lcl.seq() - 1024, lcl.seq());
				break;
			}

			LedgerFrm lfrm;
			if (lfrm.LoadFromDb(seq)) {
				args.GetReturnValue().Set(v8::String::NewFromUtf8(
					args.GetIsolate(), utils::String::BinToHexString(lfrm.GetProtoHeader().hash()).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			}
			else {
				break;
			}

			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	void V8Contract::CallBackSha256(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1 && args.Length() != 2) {
				LOG_TRACE("Parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_TRACE("Contract execution error, parameter 0 should be a string.");
				break;
			}
			DataEncodeType encode_type = BASE16;
			if (args.Length() == 2) {
				if (!TransEncodeType(args[1], encode_type)) {
					LOG_TRACE("Contract execution error, trans data encode type wrong.");
					break;
				}
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				LOG_TRACE("Failed to find contract object by isolate id");
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);
			std::string sha256_data;
			if (!TransEncodeData(args[0], encode_type, sha256_data)) {
				LOG_TRACE("Contract execution error, trans data wrong.");
				break;
			}

			std::string output = utils::Sha256::Crypto(sha256_data);
			if (output.empty()) {
				LOG_TRACE("Sha256 result empty");
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), utils::String::BinToHexString(output).c_str(),
				v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	void V8Contract::CallBackVerify(const v8::FunctionCallbackInfo<v8::Value>& args) {
		bool result = false;
		do {
			if (args.Length() != 3 && args.Length() != 4) {
				LOG_TRACE("Parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString()) {
				LOG_TRACE("Parameters should be string");
				break;
			}

			DataEncodeType encode_type = BASE16;
			if (args.Length() == 4) {
				if (!TransEncodeType(args[3], encode_type)) {
					LOG_TRACE("Contract execution error, trans data encode type wrong.");
					break;
				}
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				LOG_TRACE("Failed to find contract object by isolate id");
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			std::string signed_data = ToCString(v8::String::Utf8Value(args[0]));
			std::string public_key = ToCString(v8::String::Utf8Value(args[1]));
			std::string blob_data;
			if (!TransEncodeData(args[2], encode_type, blob_data)) {
				LOG_TRACE("Contract execution error, trans data wrong.");
				break;
			}

			if (blob_data.empty() || signed_data.empty() || public_key.empty()) {
				LOG_TRACE("Parameter are empty");
				break;
			}

			result = PublicKey::Verify(blob_data, utils::String::HexStringToBin(signed_data), public_key);
		} while (false);
		args.GetReturnValue().Set(result);
	}

	void V8Contract::CallBackToAddress(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1) {
				LOG_TRACE("Parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_TRACE("Contract execution error, parameter 0 should be a string");
				break;
			}
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				LOG_TRACE("Failed to find contract object by isolate id");
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			std::string pub_key_str = ToCString(v8::String::Utf8Value(args[0]));
			if (pub_key_str.empty()) {
				LOG_TRACE("To address parameter empty");
				break;
			}

			CEG::PublicKey pub_key(pub_key_str);
			if (!pub_key.IsValid()) {
				LOG_TRACE("ConvertPublicKey public key invalid.%s", pub_key_str.c_str());
				break;
			}
			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), pub_key.GetEncAddress().c_str(), v8::NewStringType::kNormal).ToLocalChecked());

			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}
	void V8Contract::CallBackStorageLoad(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1) {
				LOG_TRACE("parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				LOG_TRACE("contract execution error,Storage load, parameter 0 should be a String");
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				LOG_TRACE("Failed to find contract object by isolate id");
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetTopTx()->ContractStepInc(100);

			std::string key = ToCString(v8::String::Utf8Value(args[0]));
			CEG::AccountFrm::pointer account_frm = nullptr;
			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(v8_contract->parameter_.this_address_, account_frm)) {
				LOG_ERROR("Failed to find account %s.", v8_contract->parameter_.this_address_.c_str());
				break;
			}

			protocol::KeyPair kp;
			if (!account_frm->GetMetaData(key, kp)) {
				break;
			}
			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), kp.value().c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	//str to int64 check
	void V8Contract::CallBackStoI64Check(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 1) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, stoI64Check, parameter 0 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));

			int64_t iarg0 = 0;
			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, stoI64Check, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			args.GetReturnValue().Set(true);
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	// 	//Int64 add
	void V8Contract::CallBackInt64Add(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Add, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Add, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Add, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Add, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::SafeIntAdd(iarg0, iarg1, iarg0)) {
				error_desc = "Contract execution error, int64Add, parameter 0 + parameter 1 overflowed";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Int64 sub
	void V8Contract::CallBackInt64Sub(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Sub, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Sub, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Sub, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Sub, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::SafeIntSub(iarg0, iarg1, iarg0)) {
				error_desc = "Contract execution error, int64Sub, parameter 0 - parameter 1 overflowed";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	// 	//Int64 compare
	void V8Contract::CallBackInt64Compare(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = " Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Compare, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Compare, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Compare, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Compare, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			int32_t compare1 = 0;
			if (iarg0 > iarg1) compare1 = 1;
			else if (iarg0 == iarg1) {
				compare1 = 0;
			}
			else {
				compare1 = -1;
			}

			args.GetReturnValue().Set(v8::Int32::New(args.GetIsolate(), compare1));
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackInt64Div(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Div, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Div, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Div, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Div, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (iarg1 <= 0 || iarg0 < 0) {
				error_desc = "Parameter arg <= 0";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0 / iarg1).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}
	//Int64 mod
	void V8Contract::CallBackInt64Mod(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Mod, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Mod, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Mod, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Mod, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (iarg1 <= 0 || iarg0 < 0) {
				error_desc = "Parameter arg <= 0";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0 % iarg1).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Int64 mod
	void V8Contract::CallBackInt64Mul(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Mul, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Mul, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)) {
				error_desc = "Contract execution error, int64Mul, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)) {
				error_desc = "Contract execution error, int64Mul, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::SafeIntMul(iarg0, iarg1, iarg0)) {
				error_desc = "Contract execution error, int64Mul, parameter 0 * parameter 1 overflowed";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackToBaseUnit(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 1) {
				error_desc = utils::String::Format("Parameter number error:%d", args.Length());
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "contract execution error, toBaseUnit, parameter 0 should be a String";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			if (!utils::String::IsDecNumber(arg0, General::CEG_DECIMALS)) {
				error_desc = utils::String::Format("Not a decimal number:%s", arg0.c_str());
				break;
			}

			std::string multi_result = utils::String::MultiplyDecimal(arg0, General::CEG_DECIMALS).c_str();
			int64_t multi_i64 = 0;
			if (!utils::String::SafeStoi64(multi_result, multi_i64)) {
				error_desc = utils::String::Format("CallBackToBaseUnit error, int64 overflow:%s", multi_result.c_str());
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), multi_result.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("Failed to convert string to base unit, %s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

}