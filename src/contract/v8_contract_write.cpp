#include <ledger/ledger_manager.h>
#include "contract_manager.h"

namespace CEG {
	void V8Contract::CallBackConfigFee(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() < 1) {
				error_desc = "parameter number error";
				break;
			}
			if (!args[0]->IsString()) {
				error_desc = "parameter should be a string";
				break;
			}

			v8::HandleScope scope(args.GetIsolate());
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}

			if (v8_contract->parameter_.this_address_ != GET_CONTRACT_FEE_ADDRESS) {
				error_desc = "This address has no priority.";
				break;
			}

			v8::String::Utf8Value  utf8(args[0]);
			Json::Value json;
			if (!json.fromCString(ToCString(utf8))) {
				error_desc = "Failed to execute fromCString function, fatal error.";
				break;
			}

			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			LOG_INFO("UpdateFee: bottom tx(%s), top tx(%s), result(%s)", utils::String::BinToHexString(ledger_context->GetBottomTx()->GetContentHash()).c_str(), utils::String::BinToHexString(ledger_context->GetTopTx()->GetContentHash()).c_str(), json.toFastString().c_str());
			ledger_context->GetTopTx()->environment_->UpdateFeeConfig(json);
			args.GetReturnValue().Set(true);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackAssert(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc = "Assertion exception occurred";
		do {
			if (args.Length() < 1 || args.Length() > 2) {
				error_desc.append(",parameter nums error");
				break;
			}
			if (!args[0]->IsBoolean()) {
				error_desc.append(",parameter 0 should be boolean");
				break;
			}

			v8::HandleScope scope(args.GetIsolate());
			if (args.Length() == 2) {
				if (!args[1]->IsString()) {
					error_desc.append(",parameter 1 should be string");
					break;
				}
				else {
					v8::String::Utf8Value str1(args[1]);
					error_desc = ToCString(str1);
				}
			}
			if (args[0]->BooleanValue() == false) {
				break;
			}
			args.GetReturnValue().Set(true);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}


	void V8Contract::CallBackTopicLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() < 2 || args.Length() > 6) {
				error_desc = "tlog parameter number error";
				break;
			}

			if (!args[0]->IsString()) {
				error_desc = "tlog parameter 0 should be a String";
				break;
			}

			for (int i = 1; i < args.Length(); i++) {
				if (!(args[i]->IsString() || args[i]->IsNumber() || args[i]->IsBoolean())) {
					error_desc = utils::String::Format("tlog parameter %d should be a String , Number or Boolean", i);
					break;
				}
			}

			if (!error_desc.empty()) {
				break;
			}

			std::string topic = ToCString(v8::String::Utf8Value(args[0]));

			v8::HandleScope scope(args.GetIsolate());
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "tlog couldn't find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->parameter_.ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);
			std::string this_contract = v8_contract->parameter_.this_address_;

			//Add to transaction
			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(this_contract);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_LOG);
			ope->mutable_log()->set_topic(topic);
			for (int i = 1; i < args.Length(); i++) {
				std::string data;
				if (args[i]->IsString()) {
					data = ToCString(v8::String::Utf8Value(args[i]));
				}
				else {
					data = ToCString(v8::String::Utf8Value(args[i]->ToString()));
				}
				*ope->mutable_log()->add_datas() = data;
			}

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);

		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	protocol::AssetKey V8Contract::GetAssetFromJsObject(v8::Isolate* isolate, v8::Local<v8::Object> js_object) {
		protocol::AssetKey asset_key;
		v8::Local<v8::Value> v8_issue = js_object->Get(v8::String::NewFromUtf8(isolate, "issuer"));
		v8::Local<v8::Value> v8_code = js_object->Get(v8::String::NewFromUtf8(isolate, "code"));
		asset_key.set_issuer(ToCString(v8::String::Utf8Value(v8_issue)));
		asset_key.set_code(ToCString(v8::String::Utf8Value(v8_code)));
		return asset_key;
	}

	void V8Contract::CallBackDelegateCall(const v8::FunctionCallbackInfo<v8::Value>& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		v8::Local<v8::Object> obj = v8::Object::New(args.GetIsolate());
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "parameter error";
				break;
			}

			if (!args[0]->IsString()) { //the called contract address
				error_desc = "contract execution error,CallBackDelegateCall, parameter 0 should be a String";
				break;
			}

			if (!args[1]->IsString()) {
				error_desc = "contract execution error,CallBackDelegateCall, parameter 1 should be a String";
				break;
			}

			std::string address = ToCString(v8::String::Utf8Value(args[0]));
			std::string input = ToCString(v8::String::Utf8Value(args[1]));

			CEG::AccountFrm::pointer account_frm = nullptr;
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly";
				break;
			}

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

			Result tmp_result = ContractManager::Instance().Execute(contract.type(), parameter);
			ledger_context->transaction_stack_.pop_back();
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			v8::Local<v8::Value> v8_result;
			CppJsonToJsValue(args.GetIsolate(), tmp_result.contract_result(), v8_result);
			args.GetReturnValue().Set(v8_result);
			return;

		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackSetValidators(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Can't find contract object by isolate id";
				break;
			}

			if (v8_contract->parameter_.this_address_ != GET_CONTRACT_VALIDATOR_ADDRESS) {
				error_desc = utils::String::Format("contract(%s) has no permission to call callBackSetValidators interface.", v8_contract->parameter_.this_address_.c_str());
				break;
			}

			v8::String::Utf8Value  utf8(args[0]);
			Json::Value json;
			if (!json.fromCString(ToCString(utf8))) {
				error_desc = "Failed to execute fromCString function, fatal error.";
				break;
			}

			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetTopTx()->environment_->UpdateNewValidators(json);
			args.GetReturnValue().Set(true);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackCall(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		bool bcoin = false;
		protocol::AssetKey asset_key;
		std::string input;
		do {
			if (args.Length() < 4) {
				error_desc = "Parameter number error";
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());
			//call(address, true, 10, input); 
			// or call(address, {asset}, 10, input )
			if (!args[0]->IsString()) {
				error_desc = "Contract execution error,contractCall parameter 0 should be a string";
				break;
			}

			if (args[1]->IsBoolean() && args[1]->BooleanValue()) {
				bcoin = true;
			}
			else if (args[1]->IsObject()) {
				bcoin = false;
				asset_key = GetAssetFromJsObject(args.GetIsolate(), args[1]->ToObject());
			}
			else {
				error_desc = "Contract execution error, contractCall parameter 1 not valid";
				break;
			}

			if (!args[2]->IsString()) {
				error_desc = "Contract execution error, contractCall parameter 2 should be a string";
				break;
			}

			if (args.Length() > 3) {
				input = ToCString(v8::String::Utf8Value(args[3]));
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly.";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;

			std::string dest_address = std::string(ToCString(v8::String::Utf8Value(args[0])));
			std::string arg_2 = std::string(ToCString(v8::String::Utf8Value(args[2])));
			int64_t pay_amount = 0;
			if (!utils::String::SafeStoi64(arg_2, pay_amount) || pay_amount < 0) {
				error_desc = utils::String::Format("Failed to execute call function in contract, dest_address:%s, amount:%s.", dest_address.c_str(), arg_2.c_str());
				break;
			}

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();
			if (bcoin) {
				ope->set_type(protocol::Operation_Type_PAY_COIN);
				ope->mutable_pay_coin()->set_dest_address(dest_address);
				ope->mutable_pay_coin()->set_amount(pay_amount);
				ope->mutable_pay_coin()->set_input(input);
			}
			else {
				ope->set_type(protocol::Operation_Type_PAY_ASSET);
				ope->mutable_pay_asset()->set_dest_address(dest_address);
				ope->mutable_pay_asset()->mutable_asset()->mutable_key()->set_issuer(asset_key.issuer());
				ope->mutable_pay_asset()->mutable_asset()->mutable_key()->set_code(asset_key.code());
				ope->mutable_pay_asset()->mutable_asset()->set_amount(pay_amount);
				ope->mutable_pay_asset()->set_input(input);
			}

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			if (tmp_result.contract_result().isNull()) { // if the destination is not a contract account
				args.GetReturnValue().Set(true);

			}
			else {
				v8::Local<v8::Value> v8_result;
				CppJsonToJsValue(args.GetIsolate(), tmp_result.contract_result(), v8_result);
				args.GetReturnValue().Set(v8_result);
			}
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackCreate(const v8::FunctionCallbackInfo<v8::Value>& args) {
		//contractCreate(balance, type, code, input);
		std::string error_desc;
		do {
			if (args.Length() != 4) {
				error_desc = "Parameter number error";
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, contractCreate parameter 0 should be a string";
				break;
			}

			if (!args[1]->IsNumber()) {
				error_desc = "Contract execution error, contractCreate parameter 1 should be a number";
				break;
			}

			int32_t arg_1 = (int32_t)args[1]->NumberValue();
			if (arg_1 != Contract::TYPE_V8){
				error_desc = "Contract execution error, contractCreate parameter 1 should be 0 for javascript";
				break;
			}

			if (!args[2]->IsString()) {
				error_desc = "Contract execution error, contractCreate parameter 2 should be a string";
				break;
			}

			if (!args[3]->IsString()) {
				error_desc = "Contract execution error, contractCreate parameter 3 should be a string";
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly.";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;

			std::string arg_0 = std::string(ToCString(v8::String::Utf8Value(args[0])));
			int64_t int_balance = 0;
			if (!utils::String::SafeStoi64(arg_0, int_balance) || int_balance < 0) {
				error_desc = utils::String::Format("Failed to execute contractCreate function in contract, arg_0:%s.", arg_0.c_str());
				break;
			}

			std::string arg_code = std::string(ToCString(v8::String::Utf8Value(args[2])));
			std::string arg_input = std::string(ToCString(v8::String::Utf8Value(args[3])));

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_CREATE_ACCOUNT);
			ope->mutable_create_account()->set_init_balance(int_balance);
			ope->mutable_create_account()->set_init_input(arg_input);
			
			protocol::Contract *contract_proto = ope->mutable_create_account()->mutable_contract();
			contract_proto->set_type((protocol::Contract_ContractType)arg_1);
			contract_proto->set_payload(arg_code);

			protocol::AccountPrivilege *priv = ope->mutable_create_account()->mutable_priv();
			protocol::AccountThreshold *threshold = priv->mutable_thresholds();
			threshold->set_tx_threshold(1);

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), tmp_result.contract_result()["value"].asCString(),
				v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackPayCoin(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() < 2) {
				error_desc = "Parameter number error";
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, payCoin parameter 0 should be a string";
				break;
			}

			if (!args[1]->IsString()) {
				error_desc = "Contract execution error, payCoin parameter 1 should be a string";
				break;
			}

			std::string input;
			if (args.Length() > 2) {
				input = ToCString(v8::String::Utf8Value(args[2]));
			}

			std::string remark;
			if (args.Length() > 3 && CHECK_VERSION_GT_1002){
				remark = ToCString(v8::String::Utf8Value(args[3]));
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly.";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;

			std::string dest_address = std::string(ToCString(v8::String::Utf8Value(args[0])));
			std::string arg_1 = std::string(ToCString(v8::String::Utf8Value(args[1])));
			int64_t pay_amount = 0;
			if (!utils::String::SafeStoi64(arg_1, pay_amount) || pay_amount < 0) {
				error_desc = utils::String::Format("Failed to execute paycoin function in contract, dest_address:%s, amount:%s.", dest_address.c_str(), arg_1.c_str());
				break;
			}

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_PAY_COIN);
			ope->set_metadata(remark);
			ope->mutable_pay_coin()->set_dest_address(dest_address);
			ope->mutable_pay_coin()->set_amount(pay_amount);
			ope->mutable_pay_coin()->set_input(input);

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackIssueAsset(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() < 2) {
				error_desc = "Parameter number error";
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, issueAsset parameter 0 should be a string";
				break;
			}

			if (!args[1]->IsString()) {
				error_desc = "Contract execution error, issueAsset parameter 1 should be a string";
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}

			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;

			std::string assetCode = std::string(ToCString(v8::String::Utf8Value(args[0])));
			std::string amount = std::string(ToCString(v8::String::Utf8Value(args[1])));
			int64_t issueAmount = 0;
			if (!utils::String::SafeStoi64(amount, issueAmount) || issueAmount < 0) {
				error_desc = utils::String::Format("Failed to execute issueAsset function in contract, asset code:%s, asset amount:%s.", assetCode.c_str(), amount.c_str());
				break;
			}

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_ISSUE_ASSET);
			ope->mutable_issue_asset()->set_code(assetCode);
			ope->mutable_issue_asset()->set_amount(issueAmount);

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackPayAsset(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() < 4) {
				error_desc = "Parameter number error";
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, payAsset parameter 0 should be a string";
				break;
			}

			if (!args[1]->IsString()) {
				error_desc = "Contract execution error, payAsset parameter 1 should be a string";
				break;
			}

			if (!args[2]->IsString()) {
				error_desc = "Contract execution error, payAsset parameter 2 should be a string";
				break;
			}

			if (!args[3]->IsString()) {
				error_desc = "Contract execution error, payAsset parameter 3 should be a string";
				break;
			}


			std::string input;
			if (args.Length() > 4) {
				input = ToCString(v8::String::Utf8Value(args[4]));
			}

			std::string remark;
			if (args.Length() > 5 && CHECK_VERSION_GT_1002){
				remark = ToCString(v8::String::Utf8Value(args[5]));
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;

			std::string dest_address = std::string(ToCString(v8::String::Utf8Value(args[0])));
			std::string issuer = std::string(ToCString(v8::String::Utf8Value(args[1])));
			std::string assetCode = std::string(ToCString(v8::String::Utf8Value(args[2])));
			std::string amount = std::string(ToCString(v8::String::Utf8Value(args[3])));
			int64_t pay_amount = 0;
			if (!utils::String::SafeStoi64(amount, pay_amount) || pay_amount < 0) {
				error_desc = utils::String::Format("Failed to execute payAsset function in contract, dest_address:%s, amount:%s.", dest_address.c_str(), amount.c_str());
				break;
			}

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_PAY_ASSET);
			ope->set_metadata(remark);

			ope->mutable_pay_asset()->set_dest_address(dest_address);
			ope->mutable_pay_asset()->mutable_asset()->mutable_key()->set_issuer(issuer);
			ope->mutable_pay_asset()->mutable_asset()->mutable_key()->set_code(assetCode);
			ope->mutable_pay_asset()->mutable_asset()->set_amount(pay_amount);
			ope->mutable_pay_asset()->set_input(input);

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}


	// 	//Sends a message with arbitrary date to a given address path
	// 	void CallBackCall(const v8::FunctionCallbackInfo<v8::Value>& args);
	// 	//Sends a message with arbitrary date to a given address path
	void V8Contract::CallBackStorageStore(const v8::FunctionCallbackInfo<v8::Value>& args) {
		SetMetaData(args);
	}

	void V8Contract::CallBackStorageDel(const v8::FunctionCallbackInfo<v8::Value>& args) {
		SetMetaData(args, true);
	}

	void V8Contract::SetMetaData(const v8::FunctionCallbackInfo<v8::Value>& args, bool is_del) {
		std::string error_desc;
		do {
			uint8_t para_num = 2;
			bool para_type_valid = !args[0]->IsString();
			if (is_del) {
				para_num = 1;
			}
			else {
				para_type_valid = para_type_valid || !args[1]->IsString();
			}

			if (args.Length() != para_num) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (para_type_valid) {
				error_desc = "Storage operation parameter should be string";
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;
			std::string  key = ToCString(v8::String::Utf8Value(args[0]));
			std::string  value = "";
			if (!is_del) {
				value = ToCString(v8::String::Utf8Value(args[1]));
			}
			if (key.empty()) {
				error_desc = "Key is empty.";
				break;
			}

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_SET_METADATA);
			protocol::OperationSetMetadata *meta_data = ope->mutable_set_metadata();
			meta_data->set_key(key);
			meta_data->set_value(value);
			meta_data->set_delete_flag(is_del);

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}
}