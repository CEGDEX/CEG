/*
Copyright CEG Technologies Co., Ltd. 2017 All Rights Reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <utils/headers.h>
#include <common/private_key.h>
#include <common/general.h>
#include <main/configure.h>
#include <overlay/peer_manager.h>
#include <glue/glue_manager.h>
#include <ledger/ledger_manager.h>
#include <contract/contract_manager.h>
#include <ledger/fee_calculate.h>
#include "web_server.h"

namespace CEG {

	void WebServer::SubmitTransaction(const http::server::request &request, std::string &reply) {

		Json::Value body;
		if (!body.fromString(request.body)) {
			LOG_ERROR("Failed to parse the json content of the request");
			Json::Value reply_json;
			reply_json["results"][Json::UInt(0)]["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
			reply_json["results"][Json::UInt(0)]["error_desc"] = "request must be in json format";
			reply_json["success_count"] = Json::UInt(0);
			reply = reply_json.toStyledString();
			return;
		}

		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &results = reply_json["results"];
		results = Json::Value(Json::arrayValue);
		uint32_t success_count = 0;

		int64_t begin_time = utils::Timestamp::HighResolution();
		const Json::Value &json_items = body["items"];
		for (size_t j = 0; j < json_items.size() && running; j++) {
			const Json::Value &json_item = json_items[j];
			Json::Value &result_item = results[results.size()];

			int64_t active_time = utils::Timestamp::HighResolution();
			Result result;
			result.set_code(protocol::ERRCODE_SUCCESS);
			result.set_desc("");

			protocol::TransactionEnv tran_env;
			do {
				if (json_item.isMember("transaction_blob")) {
					if (!json_item.isMember("signatures")) {
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc("'signatures' value is not existed");
						break;
					}

					std::string decodeblob;
					std::string decodesig;
					if (!utils::String::HexStringToBin(json_item["transaction_blob"].asString(), decodeblob)) {
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc("'transaction_blob' value must be Hex");
						break;
					}

					protocol::Transaction *tran = tran_env.mutable_transaction();
					if (!tran->ParseFromString(decodeblob)) {
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc("ParseFromString from 'sign_data' invalid");
						LOG_ERROR("Failed to parse the transaction, invalid decode blob");
						break;
					}

					const Json::Value &signatures = json_item["signatures"];
					for (uint32_t i = 0; i < signatures.size(); i++) {
						const Json::Value &signa = signatures[i];
						protocol::Signature *signpro = tran_env.add_signatures();

						//utils::decode_b16(signa["sign_data"].asString(), decodesig);
						decodesig = "";
						if (!utils::String::HexStringToBin(signa["sign_data"].asString(), decodesig)) {
							result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
							result.set_desc("'sign_data' value must be Hex");
							break;
						}

						PublicKey pubkey(signa["public_key"].asString());
						if (!pubkey.IsValid()) {
							result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
							result.set_desc("'public_key' value is not existed or parameter error");
							LOG_ERROR("Failed to verify the public key, invalid public key (%s)", signa["public_key"].asString().c_str());
							break;
						}

						signpro->set_sign_data(decodesig);
						signpro->set_public_key(pubkey.GetEncPublicKey());
					}

					std::string content = tran->SerializeAsString();
					result_item["hash"] = utils::String::BinToHexString(HashWrapper::Crypto(content));
				}
				else {
					protocol::Transaction *tran = tran_env.mutable_transaction();
					std::string error_msg;
					if (!Json2Proto(json_item["transaction_json"], *tran, error_msg)){
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc(error_msg);
						break;
					}
	

					std::string content = tran->SerializeAsString();
					const Json::Value &private_keys = json_item["private_keys"];
					for (uint32_t i = 0; i < private_keys.size(); i++) {
						const std::string &private_key = private_keys[i].asString();

						PrivateKey privateKey(private_key);
						if (!privateKey.IsValid()) {
							result.set_code(protocol::ERRCODE_INVALID_PRIKEY);
							result.set_desc("Private key format error");
							break;
						}

						std::string sign = privateKey.Sign(content);
						protocol::Signature *signpro = tran_env.add_signatures();
						signpro->set_sign_data(sign);
						signpro->set_public_key(privateKey.GetEncPublicKey());
					}

					result_item["hash"] = utils::String::BinToHexString(HashWrapper::Crypto(content));
				}

				TransactionFrm::pointer ptr = std::make_shared<TransactionFrm>(tran_env);
				GlueManager::Instance().OnTransaction(ptr, result);

				// do not broadcast if OnTransaction failed
				if (result.code() == protocol::ERRCODE_SUCCESS) {
					PeerManager::Instance().Broadcast(protocol::OVERLAY_MSGTYPE_TRANSACTION, ptr->GetFullData());
				}

			} while (false);

			//Force to exit successfully
			if (result.code() == protocol::ERRCODE_SUCCESS || result.code() == protocol::ERRCODE_ALREADY_EXIST) {
				result.set_code(protocol::ERRCODE_SUCCESS);
				success_count++;
			}

			result_item["error_code"] = result.code();
			result_item["error_desc"] = result.desc();
		}
		LOG_TRACE("Created %u transactions use " FMT_I64 "(ms)", json_items.size(),
			(utils::Timestamp::HighResolution() - begin_time) / utils::MICRO_UNITS_PER_MILLI);


		reply_json["success_count"] = success_count;
		reply = reply_json.toStyledString();
	}

	void WebServer::CreateKeyPair(const http::server::request &request, std::string &reply) {
		std::string error_desc;
		int32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);
		
		SignatureType sign_type = SIGNTYPE_CFCASM2;

		do {
			std::string sign_type_s = request.GetParamValue("sign_type");
			if (sign_type_s == ""){
				if (HashWrapper::GetLedgerHashType() == HashWrapper::HASH_TYPE_SHA256) {
					sign_type = SIGNTYPE_ED25519;
				}
				else {
					sign_type = SIGNTYPE_CFCASM2;
				}
			}
			else {
				sign_type = GetSignTypeByDesc(sign_type_s);
				if (sign_type == SIGNTYPE_NONE) {
					error_code = protocol::ERRCODE_INVALID_PARAMETER;
					break;
				} 
			}

			PrivateKey priv_key(sign_type);
            std::string public_key = priv_key.GetEncPublicKey();
            std::string private_key = priv_key.GetEncPrivateKey();
            std::string public_address = priv_key.GetEncAddress();

			LOG_TRACE("Creating account address:%s", public_address.c_str());

			Json::Value &result = reply_json["result"];
			result["public_key"] = public_key;
			result["private_key"] = private_key;
			result["private_key_aes"] = utils::Aes::CryptoHex(private_key, CEG::GetDataSecuretKey());
			result["address"] = public_address;
			result["public_key_raw"] = EncodePublicKey(priv_key.GetRawPublicKey());
			result["sign_type"] = GetSignTypeDesc(sign_type);

		} while (false);
		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}

	void WebServer::CallContract(const http::server::request &request, std::string &reply) {
		
		Json::Value body;
		if (!body.fromString(request.body)) {
			LOG_ERROR("Failed to parse the json content of the request");
			Json::Value reply_json;
			reply_json["results"][Json::UInt(0)]["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
			reply_json["results"][Json::UInt(0)]["error_desc"] = "request must be in json format";
			reply_json["success_count"] = Json::UInt(0);
			reply = reply_json.toStyledString();
			return;
		}

		ContractTestParameter test_parameter;
		test_parameter.code_ = body["code"].asString();
		test_parameter.input_ = body["input"].asString();
		test_parameter.opt_type_ = ContractTestParameter::OptType(body["opt_type"].asInt());
		test_parameter.contract_address_ = body["contract_address"].asString();
		test_parameter.source_address_ = body["source_address"].asString();
		test_parameter.fee_limit_ = body["fee_limit"].asInt64();
		int64_t sys_gas_price = LedgerManager::Instance().GetCurFeeConfig().gas_price();
		int64_t gas_price = body["gas_price"].asInt64() > sys_gas_price ? body["gas_price"].asInt64() : sys_gas_price;
		test_parameter.gas_price_ = gas_price;
		test_parameter.contract_balance_ = body["contract_balance"].asInt64();

		int32_t error_code = protocol::ERRCODE_SUCCESS;
		std::string error_desc;
		AccountFrm::pointer acc = NULL;

		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &result = reply_json["result"];

		do {
			if (!test_parameter.contract_address_.empty()) {
				if (!Environment::AccountFromDB(test_parameter.contract_address_, acc)) {
					error_code = protocol::ERRCODE_NOT_EXIST;
					error_desc = utils::String::Format("Account(%s) is not existed", test_parameter.contract_address_.c_str());
					LOG_ERROR("Failed to load the account from the database. %s", error_desc.c_str());
					break;
				}

				test_parameter.code_ = acc->GetProtoAccount().contract().payload();
			} 

			if (test_parameter.code_.empty()) {
				error_code = protocol::ERRCODE_NOT_EXIST;
				error_desc = utils::String::Format("Account(%s) has no contract code", test_parameter.contract_address_.c_str());
				LOG_ERROR("Failed to load test parameter. %s", error_desc.c_str());
				break;
			}

			Result exe_result;
			if (!LedgerManager::Instance().context_manager_.SyncTestProcess(LedgerContext::AT_TEST_V8,
				(TestParameter*)&test_parameter,
				utils::MICRO_UNITS_PER_SEC, 
				exe_result, result["logs"], result["txs"], result["query_rets"], result["stat"])) {
				error_code = exe_result.code();
				error_desc = exe_result.desc();
				LOG_ERROR("Failed to execute the test.%s", error_desc.c_str());
				break;
			}
		} while (false);

		if (error_code == protocol::ERRCODE_CONTRACT_SYNTAX_ERROR) {
			reply_json["error_desc_json"].fromString(error_desc);
		} 

		reply_json["error_code"] = error_code;
		reply_json["error_desc"] = error_desc;
		reply = reply_json.toStyledString();
	}

	void WebServer::TestTransaction(const http::server::request &request, std::string &reply) {
		Json::Value body;
		if (!body.fromString(request.body)) {
			LOG_ERROR("Failed to parse the json content of the request");
			Json::Value reply_json;
			reply_json["results"][Json::UInt(0)]["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
			reply_json["results"][Json::UInt(0)]["error_desc"] = "request must be in json format";
			reply_json["success_count"] = Json::UInt(0);
			reply = reply_json.toStyledString();
			return;
		}
		

		int64_t begin_time = utils::Timestamp::HighResolution();
		const Json::Value &json_items = body["items"];
		if (json_items.size() > 1) {			
			LOG_ERROR("Failed to test transaction, too many items (%d)", json_items.size());
			Json::Value reply_json;
			reply_json["results"][Json::UInt(0)]["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
			reply_json["results"][Json::UInt(0)]["error_desc"] = "Too many test transactions;you can test only one transaction";
			reply_json["success_count"] = Json::UInt(0);
			reply = reply_json.toStyledString();
			return;
		}

		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &result_json = reply_json["result"];
		//results = Json::Value(Json::arrayValue);
		//uint32_t success_count = 0;

		TransactionTestParameter test_parameter;
		protocol::TransactionEnvSet* tx_set = test_parameter.consensus_value_.mutable_txset();

		for (size_t j = 0; j < json_items.size() && running; j++) {
			const Json::Value &json_item = json_items[j];

			int32_t signature_number = 1;
			if (json_item["signature_number"].asInt())
				signature_number = json_item["signature_number"].asInt();

			int64_t active_time = utils::Timestamp::HighResolution();
			Result result;
			result.set_code(protocol::ERRCODE_SUCCESS);
			result.set_desc("");
			int64_t max = 0;
			int64_t min = 0;

			protocol::TransactionEnv tran_env;
			do {
				if (json_item.isMember("transaction_blob")) {
					std::string decodeblob;

					if (!utils::String::HexStringToBin(json_item["transaction_blob"].asString(), decodeblob)) {
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc("'transaction_blob' value must be Hex");
						break;
					}

					protocol::Transaction *tran = tran_env.mutable_transaction();
					if (!tran->ParseFromString(decodeblob)) {
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc("ParseFromString from 'sign_data' invalid");
						LOG_ERROR("Failed to parse the transaction, invalid decode blob");
						break;
					}
					
					int64_t sys_gas_price = LedgerManager::Instance().GetCurFeeConfig().gas_price();
					int64_t gas_price = tran->gas_price() > sys_gas_price ? tran->gas_price() : sys_gas_price;
					tran->set_gas_price(gas_price);
					tran->set_fee_limit(0);

					if (!EvaluateFee(tran_env, result, max, min))
						break;

					tran->set_fee_limit(max);

					std::string content = tran->SerializeAsString();
					result_json["hash"] = utils::String::BinToHexString(HashWrapper::Crypto(content));
				}
				else {
					protocol::Transaction *tran = tran_env.mutable_transaction();
					std::string error_msg;
					if (!Json2Proto(json_item["transaction_json"], *tran, error_msg)){
						result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result.set_desc(error_msg);
						break;
					}

					int64_t sys_gas_price = LedgerManager::Instance().GetCurFeeConfig().gas_price();
					int64_t gas_price = tran->gas_price() > sys_gas_price ? tran->gas_price() : sys_gas_price;

					tran->set_gas_price(gas_price);
					tran->set_fee_limit(0);

					if (!EvaluateFee(tran_env, result, max, min))
						break;

					tran->set_fee_limit(max);

					std::string content = tran->SerializeAsString();
					const Json::Value &private_keys = json_item["private_keys"];					
					result_json["hash"] = utils::String::BinToHexString(HashWrapper::Crypto(content));
				}

				int64_t nonce;
				TransactionFrm frm(tran_env);
				if (!frm.CheckValid(-1, false, nonce)){
					result = frm.result_;
					break;
				}

			} while (false);

			if (result.code() == protocol::ERRCODE_SUCCESS) {
				TransactionFrm::pointer ptr = std::make_shared<TransactionFrm>(tran_env);
				Json::Value logs;
				Json::Value txs;
				Json::Value query_rets;
				Json::Value stat;
				*tx_set->add_txs() = tran_env;
				Result exe_result;
				if (!LedgerManager::Instance().context_manager_.SyncTestProcess(LedgerContext::AT_TEST_TRANSACTION,
					(TestParameter*)&test_parameter,
					utils::MICRO_UNITS_PER_SEC,
					exe_result, logs, txs, query_rets, stat, signature_number)) {
					reply_json["error_code"] = exe_result.code();
					reply_json["error_desc"] = exe_result.desc();
					LOG_ERROR("Failed to test transaction.%s", exe_result.desc().c_str());
					break;
				}
				if (exe_result.code() == protocol::ERRCODE_SUCCESS){
					result_json["logs"] = logs;
					result_json["txs"] = txs;
					result_json["query_rets"] = query_rets;
					result_json["stat"] = stat;

					int i = 0;
					int64_t actual_fee1 = result_json["txs"][i]["actual_fee"].asInt64();
					int64_t actual_fee2 = 0;
					Result exe_result2;
					do{
						protocol::Transaction *tran = tran_env.mutable_transaction();
						tran->set_fee_limit(actual_fee1);
						tx_set->clear_txs();
						*tx_set->add_txs() = tran_env;
						logs.clear();
						txs.clear();;
						query_rets.clear();
						stat.clear();
						if (!LedgerManager::Instance().context_manager_.SyncTestProcess(LedgerContext::AT_TEST_TRANSACTION,
							(TestParameter*)&test_parameter,
							utils::MICRO_UNITS_PER_SEC,
							exe_result2, logs, txs, query_rets, stat, signature_number)) {
							break;
						}
						if (!txs[i].isMember("actual_fee")) txs[i]["actual_fee"] = 0;
						actual_fee2 = txs[i]["actual_fee"].asInt64();
						if (exe_result2.code() == protocol::ERRCODE_SUCCESS){
							result_json["logs"] = logs;
							result_json["txs"] = txs;
							result_json["query_rets"] = query_rets;
							result_json["stat"] = stat;
							if (actual_fee1 != actual_fee2){
								actual_fee1 = actual_fee2;
								continue;
							}
							else{
								break;
							}
						}
					} while (exe_result2.code() == protocol::ERRCODE_SUCCESS);
				}
				result = exe_result;
				
				//bool skip_flag = false;
				//int try_count = 0;
				//int64_t last_mid = min;
				//while (!skip_flag && min <= max){
				//	//...TODO
				//	result_json["txs"].clear();
				//	tx_set->clear_txs();
				//	*tx_set->add_txs() = tran_env;
				//	Result exe_result;
				//	if (!LedgerManager::Instance().context_manager_.SyncTestProcess(LedgerContext::AT_TEST_TRANSACTION,
				//		(TestParameter*)&test_parameter,
				//		utils::MICRO_UNITS_PER_SEC,
				//		exe_result, result_json["logs"], result_json["txs"], result_json["query_rets"], result_json["stat"], signature_number)) {
				//		reply_json["error_code"] = exe_result.code();
				//		reply_json["error_desc"] = exe_result.desc();
				//		LOG_ERROR("Failed to test transaction.%s", exe_result.desc().c_str());
				//		skip_flag = true;
				//		break;
				//	}
				//	else{
				//		int i = 0;
				//		result = exe_result;
				//		if (result_json["txs"][i].isMember("error_code")){
				//			if (result_json["txs"][i]["error_code"].asInt() == (int)protocol::ERRCODE_FEE_NOT_ENOUGH){
				//				int64_t actual_fee =result_json["txs"][i]["actual_fee"].asInt64();
				//				protocol::Transaction *tran = tran_env.mutable_transaction();
				//				int64_t mid = (actual_fee + max) / 2;
				//				tran->set_fee_limit(mid);
				//				min = last_mid+1;
				//				last_mid = mid;
				//				try_count++;
				//				LOG_INFO("reset actual_fee(" FMT_I64 ") %d", actual_fee, try_count);
				//				continue;
				//			}
				//			else if (result_json["txs"][i]["error_code"].asInt() == (int)protocol::ERRCODE_ACCOUNT_LOW_RESERVE){
				//				int64_t actual_fee = result_json["txs"][i]["actual_fee"].asInt64();
				//				protocol::Transaction *tran = tran_env.mutable_transaction();
				//				int64_t mid = (actual_fee + min) / 2;
				//				tran->set_fee_limit(mid);
				//				max = last_mid-1;
				//				last_mid = mid;
				//				try_count++;
				//				LOG_INFO("reset actual_fee(" FMT_I64 ") %d", actual_fee, try_count);
				//				continue;
				//			}
				//			else{
				//				skip_flag = true;
				//			}
				//		}
				//		else{
				//			skip_flag = true;
				//		}
				//	}
				//};
				
				
			}
			reply_json["error_code"] = result.code();
			reply_json["error_desc"] = result.desc();

		}//end for
		LOG_TRACE("Created %u transactions use " FMT_I64 "(ms)", json_items.size(),
			(utils::Timestamp::HighResolution() - begin_time) / utils::MICRO_UNITS_PER_MILLI);
		reply = reply_json.toStyledString();
	}

	bool WebServer::EvaluateFee(protocol::TransactionEnv &tran_env, Result& result, int64_t& max, int64_t& min){
		protocol::Transaction *tran = tran_env.mutable_transaction();
		int64_t pay_amount = 0;
		std::string tx_source_address = tran->source_address();
		AccountFrm::pointer source_account;
		if (!Environment::AccountFromDB(tx_source_address, source_account)) {
			result.set_code(protocol::ERRCODE_ACCOUNT_NOT_EXIST);
			result.set_desc(utils::String::Format("Source account(%s) not exist", tx_source_address.c_str()));
			LOG_ERROR("Failed to load the account from the database.%s", result.desc().c_str());
			return false;
		}

		int64_t total_opt_fee = 0;
		for (int i = 0; i < tran->operations_size(); i++) {
			const protocol::Operation &ope = tran->operations(i);
			std::string ope_source_address = ope.source_address();
			if (ope_source_address.size() == 0) 
				ope_source_address = tx_source_address;
			if (tx_source_address == ope_source_address){
				auto type = ope.type();
				int64_t opt_price;
                if (!utils::SafeIntMul(FeeCalculate::GetOperationTypeGas(ope), tran->gas_price(), opt_price)) {
					result.set_code(protocol::ERRCODE_MATH_OVERFLOW);
					result.set_desc(utils::String::Format("Source account(%s) math overflow, GetOperationTypeGas:(" FMT_I64 "), gas_price:(" FMT_I64 ")",
                        tx_source_address.c_str(), FeeCalculate::GetOperationTypeGas(ope), tran->gas_price()));
					LOG_ERROR("Failed to evaluate fee.%s", result.desc().c_str());
					return false;
				}

				if (!utils::SafeIntAdd(total_opt_fee, opt_price, total_opt_fee)){
					result.set_code(protocol::ERRCODE_MATH_OVERFLOW);
					result.set_desc(utils::String::Format("Source account(%s) math overflow, total_opt_fee:(" FMT_I64 "), opt_price:(" FMT_I64 ")",
						tx_source_address.c_str(), total_opt_fee, opt_price));
					LOG_ERROR("Failed to evaluate fee.%s", result.desc().c_str());
					return false;
				}

				if (type == protocol::Operation_Type_PAY_COIN) {
					if (!utils::SafeIntAdd(pay_amount, ope.pay_coin().amount(), pay_amount)) {
						result.set_code(protocol::ERRCODE_MATH_OVERFLOW);
						result.set_desc(utils::String::Format("Source account(%s) math overflow, pay_amount:(" FMT_I64 "), pay_coin().amount:(" FMT_I64 ")", 
							tx_source_address.c_str(), pay_amount, ope.pay_coin().amount()));
						LOG_ERROR("Failed to evaluate fee.%s", result.desc().c_str());
						return false;
					}
				}
				if (type == protocol::Operation_Type_CREATE_ACCOUNT) {
					if (!utils::SafeIntAdd(pay_amount, ope.create_account().init_balance(), pay_amount)) {
						result.set_code(protocol::ERRCODE_MATH_OVERFLOW);
						result.set_desc(utils::String::Format("Source account(%s) math overflow, pay_amount:(" FMT_I64 "), init_balance:(" FMT_I64 ")",
							tx_source_address.c_str(), pay_amount, ope.create_account().init_balance()));
						LOG_ERROR("Failed to evaluate fee.%s", result.desc().c_str());
						return false;
					}
				}
			}
		}

		int64_t balance = source_account->GetAccountBalance();
		int64_t fee;
		if (!utils::SafeIntSub(balance, LedgerManager::Instance().GetCurFeeConfig().base_reserve(), fee)) {
			result.set_code(protocol::ERRCODE_MATH_OVERFLOW);
			result.set_desc(utils::String::Format("Source account(%s) overflow for fee, balance:(" FMT_I64 "), base_reserve:(" FMT_I64 "), pay_amount:(" FMT_I64 ")",
				tx_source_address.c_str(), balance, LedgerManager::Instance().GetCurFeeConfig().base_reserve(), pay_amount));
			LOG_ERROR("Failed to evaluate fee.%s", result.desc().c_str());
			return false;
		}

		if (!utils::SafeIntSub(fee, pay_amount, fee)) {
			result.set_code(protocol::ERRCODE_MATH_OVERFLOW);
			result.set_desc(utils::String::Format("Source account(%s) overflow for fee, balance:(" FMT_I64 "), base_reserve:(" FMT_I64 "), pay_amount:(" FMT_I64 ")",
				tx_source_address.c_str(), balance, LedgerManager::Instance().GetCurFeeConfig().base_reserve(), pay_amount));
			LOG_ERROR("Failed to evaluate fee.%s", result.desc().c_str());
			return false;
		}
		tran->set_fee_limit(fee);

		int64_t bytes_fee = 0;
		if (tran->gas_price() > 0) {
			if (!utils::SafeIntMul(tran->gas_price(), (int64_t)tran_env.ByteSize(), bytes_fee)){
				result.set_code(protocol::ERRCODE_MATH_OVERFLOW);
				result.set_desc(utils::String::Format("Source account(%s) overflow for fee, gas_price:(" FMT_I64 "), ByteSize:%d",
					tx_source_address.c_str(), tran->gas_price(), tran->ByteSize()));
				LOG_ERROR("Failed to evaluate fee.%s", result.desc().c_str());
				return false;
			}
		}

		int64_t total_fee;
		if (!utils::SafeIntAdd(bytes_fee, total_opt_fee, total_fee)) {
			result.set_code(protocol::ERRCODE_MATH_OVERFLOW);
			result.set_desc(utils::String::Format("Source account(%s) overflow for fee, bytes_fee:(" FMT_I64 "), total_opt_fee:(" FMT_I64 ")", 
				tx_source_address.c_str(), bytes_fee, total_opt_fee));
			LOG_ERROR("Failed to evaluate fee.%s", result.desc().c_str());
			return false;
		}

		if (fee < total_fee) {
			result.set_code(protocol::ERRCODE_FEE_NOT_ENOUGH);
			result.set_desc(utils::String::Format("Source account(%s) not enough balance for fee", tx_source_address.c_str()));
			LOG_ERROR("Failed to evaluate fee.%s", result.desc().c_str());
			return false;
		}

		min = total_fee;
		max = fee;
		return true;
	}
}
