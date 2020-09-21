/*
	CEG is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	CEG is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with CEG.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <json/json.h>
#include <utils/headers.h>
#include <common/key_store.h>
#include <ledger/ledger_manager.h>
#include <glue/glue_manager.h>
#include <overlay/peer_manager.h>
#include "console.h"

namespace CEG {
	Console::Console() {
		thread_ptr_ = NULL;
		priv_key_ = NULL;
		funcs_["createWallet"] = std::bind(&Console::CreateWallet, this, std::placeholders::_1);
		funcs_["openWallet"] = std::bind(&Console::OpenWallet, this, std::placeholders::_1);
		funcs_["closeWallet"] = std::bind(&Console::CloseWallet, this, std::placeholders::_1);
		funcs_["restoreWallet"] = std::bind(&Console::RestoreWallet, this, std::placeholders::_1);
		funcs_["getBalance"] = std::bind(&Console::GetBalance, this, std::placeholders::_1);
		funcs_["getBlockNumber"] = std::bind(&Console::GetBlockNumber, this, std::placeholders::_1);
		funcs_["help"] = std::bind(&Console::Usage, this, std::placeholders::_1);
		funcs_["getAddress"] = std::bind(&Console::GetAddress, this, std::placeholders::_1);
		funcs_["payCoin"] = std::bind(&Console::PayCoin, this, std::placeholders::_1);
		funcs_["showKey"] = std::bind(&Console::ShowKey, this, std::placeholders::_1);
		funcs_["getState"] = std::bind(&Console::GetState, this, std::placeholders::_1);
		funcs_["exit"] = std::bind(&Console::CmdExit, this, std::placeholders::_1);
	}

	Console::~Console() {
		if (thread_ptr_) {
			delete thread_ptr_;
		}

		if (priv_key_){
			delete priv_key_;
		}
	}

	bool Console::Initialize() {
		thread_ptr_ = new utils::Thread(this);
		if (!thread_ptr_->Start("console")) {
			return false;
		}
		return true;
	}

	bool Console::Exit() {
		return true;
	}

	bool Console::ParseCmdline(const std::string &str, std::vector<std::string>& arr, std::string& errmsg) {
		std::string arg;
		bool in_double_quotes = false;
		bool in_single_quotes = false;
		for (std::string::size_type i = 0; i < str.length(); i++) {
			if (str[i] == '\"') {
				in_double_quotes = !in_double_quotes;
				continue;
			}
			if (str[i] == '\'') {
				in_single_quotes = !in_single_quotes;
				continue;
			}

			if (str[i] == ' ' && !in_double_quotes && !in_single_quotes) {
				arr.push_back(arg);
				arg.clear();
				continue;
			}

			if (str[i] == '\\') {
				i++;
				if (i > str.length()) {
					errmsg = "unexpected occurrence of '\\' at end of string";
					return false;
				}
			}
			arg += str[i];
		}
		if (in_single_quotes || in_double_quotes) {
			errmsg = "quote is not closed";
			return false;
		}
		if (arg.length() > 0) {
			arr.push_back(arg);
		}
		return true;
	}

	extern bool g_enable_;
	extern bool g_ready_;
	void Console::Run(utils::Thread *thread) {
		while (g_enable_ && !g_ready_) {
			utils::Sleep(100);
		}

		std::cout << "ready" << std::endl;
		while (g_enable_) {
			std::string input;
			std::cout << "> ";
			std::getline(std::cin, input);
			
			
			std::vector<std::string> args;
			std::string errmsg;
			if (!ParseCmdline(utils::String::Trim(input), args, errmsg)) {
				std::cout << "parse command line error, " << errmsg << std::endl;
				continue;
			}
			if (args.size() < 1) continue;

			ConsolePocMap::iterator iter = funcs_.find(args[0]);
			if (iter != funcs_.end()){
				iter->second(args);
			}
			else if (!args[0].empty()) {
				std::cout << "command not found" << std::endl;
			}
		}
	}

	void Console::OpenWallet(const utils::StringVector &args) {
		if (args.size() < 2) {
			std::cout << "error params" << std::endl;
			return;
		}

		std::string errmsg;
		PrivateKey *tmp_private = OpenKeystore(args[1], errmsg);
		if (tmp_private != NULL) {
			if (priv_key_) {
				delete priv_key_;
				priv_key_ = NULL;
			}

			priv_key_ = tmp_private;
			std::cout << "ok" << std::endl;
		}
		else {
			std::cout << "open wallet error, " << errmsg << std::endl;
		}
	}

	PrivateKey *Console::OpenKeystore(const std::string &path, std::string& errmsg) {
		std::string password;

		if (!utils::File::IsExist(path)) {
			errmsg = "path (" + path + ") not exist";
			return NULL;
		}

		password = utils::GetCinPassword("input the password:");
		std::cout << std::endl;
		if (password.empty()) {
			errmsg = "no password supplied";
			return NULL;
		}

		utils::File file_object;
		if (!file_object.Open(path, utils::File::FILE_M_READ)) {
			errmsg = utils::String::Format("open failed, error desc(%s)", STD_ERR_DESC);
			return NULL;
		}
		std::string serial_str;
		file_object.ReadData(serial_str, 1 * utils::BYTES_PER_MEGA);

		Json::Value key_store_json;
		if (!key_store_json.fromString(serial_str)) {
			errmsg = "parse string failed";
			return NULL;
		}

		KeyStore key_store;
		std::string restore_priv_key;
		bool ret = key_store.From(key_store_json, password, restore_priv_key);
		if (ret) {
			keystore_path_ = path;
			return new PrivateKey(restore_priv_key);
		}
		else {
			errmsg = "load private key with password failed";
			return NULL;
		}
	}

	void Console::CreateKestore(const utils::StringVector &args, std::string &private_key, std::string& errmsg) {
		if (utils::File::IsExist(args[1])) {
			errmsg = "path ("+ args[1] + ") exist";
			return;
		}

		std::string password;
		password = utils::GetCinPassword("input the password:");
		std::cout << std::endl;
		if (password.empty()) {
			errmsg = "no password supplied";
			return;
		}
		std::string password1 = utils::GetCinPassword("input the password again:");
		std::cout << std::endl;
		if (password != password1) {
			errmsg = "passwords do not match";
			return;
		}

		utils::File file_object;
		if (!file_object.Open(args[1], utils::File::FILE_M_WRITE)) {
			errmsg = utils::String::Format("error desc(%s)", STD_ERR_DESC);
			return;
		}

		KeyStore key_store;
		Json::Value key_store_json;
		bool ret = key_store.Generate(password, key_store_json, private_key);
		if (ret) {
			std::string serial_str = key_store_json.toFastString();
			std::cout << serial_str << std::endl;
			file_object.Write(serial_str.c_str(), 1, serial_str.size());
			if (priv_key_) {
				delete priv_key_;
				priv_key_ = NULL;
			}
			keystore_path_ = args[1];
			priv_key_ = new PrivateKey(private_key);
		}
		else {
			errmsg = "generate private key with password failed";
		}
	}

	void Console::CreateWallet(const utils::StringVector &args) {
		std::string password;
		if (args.size() > 1) {

			std::string private_key;
			std::string errmsg;
			CreateKestore(args, private_key, errmsg);
			if (!errmsg.empty())
			{
				std::cout << "create wallet error, " << errmsg << std::endl;
			}
		}
		else {
			std::cout << "error params" << std::endl;
			return;
		}
	}

	void Console::RestoreWallet(const utils::StringVector &args) {
		std::string password;
		if (args.size() > 2) {

			std::string private_key = args[2];
			PrivateKey priv_key(private_key);
			if (!priv_key.IsValid()) {
				std::cout << "error, private key not valid" << std::endl;
				return;
			}
			std::string errmsg;
			CreateKestore(args, private_key, errmsg);
			if (!errmsg.empty())
			{
				std::cout << "create wallet error, " << errmsg << std::endl;
			}
		}
		else {
			return;
		}
	}

	void Console::CloseWallet(const utils::StringVector &args) {
		if (priv_key_ != NULL) {
			delete priv_key_;
			priv_key_ = NULL;
		} 
		std::cout  << "ok" << std::endl;
	}

	void Console::GetBlockNumber(const utils::StringVector &args) {
		std::cout << LedgerManager::Instance().GetLastClosedLedger().seq() << std::endl;
	}

	void Console::GetBalance(const utils::StringVector &args) {
		std::string address;
		if (args.size() < 2){
			if (priv_key_ != NULL) {
				address = priv_key_->GetEncAddress();
			} 
			else {
				std::cout << "error, please input the address" << std::endl;
				return;
			}
		} 
		else {
			address = args[1];
		}

		AccountFrm::pointer account_ptr;
		if (!Environment::AccountFromDB(address, account_ptr)) {
			std::cout << "error " << address << " not exist" << std::endl;
		}
		else {
			std::cout << utils::String::FormatDecimal(account_ptr->GetAccountBalance(), General::CEG_DECIMALS ) << " CEG"<<std::endl;
		}
	}

	void Console::GetAddress(const utils::StringVector &args) {
		if (priv_key_ != NULL) {
			std::cout << priv_key_->GetEncAddress() << std::endl;
		} 
		else {
			std::cout << "error, wallet not opened" << std::endl;
		}
	}

	void Console::GetState(const utils::StringVector &args) {
		Json::Value reply_json;
		do {
			utils::ReadLockGuard guard(CEG::StatusModule::status_lock_);
			reply_json = *CEG::StatusModule::modules_status_;
		} while (false);

		std::cout << reply_json.toStyledString() << std::endl;
	}

	void Console::ShowKey(const utils::StringVector &args) {
		if (priv_key_ != NULL) {

			//check the password again;
			std::string errmsg;
			PrivateKey *tmp_private = OpenKeystore(keystore_path_, errmsg);
			if (tmp_private != NULL) {
				if (tmp_private->GetEncAddress() != priv_key_->GetEncAddress()) {
					std::cout << "error" << std::endl;
					return;
				}

				delete tmp_private;
			}
			else {
				std::cout << "open wallet error, " << errmsg << std::endl;
				return;
			}
			std::cout << priv_key_->GetEncPrivateKey() << std::endl;
		}
		else {
			std::cout << "error, wallet not opened" << std::endl;
		}
	}

	void Console::CmdExit(const utils::StringVector &args) {
		g_enable_ = false;
	}

	void Console::PayCoin(const utils::StringVector &args) {
		if (args.size() < 5) {
			std::cout << "error params" << std::endl;
			return;
		}

		if (priv_key_ != NULL) {

			//check the password again;
			std::string errmsg;
			PrivateKey *tmp_private = OpenKeystore(keystore_path_, errmsg);
			if (tmp_private != NULL ) {
				if (tmp_private->GetEncAddress() != priv_key_->GetEncAddress()) {
					std::cout << "error" << std::endl;
					return;
				}

				delete tmp_private;
			}
			else {
				std::cout << "open wallet error, " << errmsg << std::endl;
				return;
			}

			std::string source_address = priv_key_->GetEncAddress();
			std::string dest_address = args[1];

			if (!utils::String::IsDecNumber(args[2], General::CEG_DECIMALS)) {
				std::cout << "error, amount not valid" << std::endl;
				return;
			}

			if (!utils::String::IsDecNumber(args[3], General::CEG_DECIMALS)) {
				std::cout << "error, fee not valid" << std::endl;
				return;
			}

			int64_t coin_amount = utils::String::Stoi64(utils::String::MultiplyDecimal(args[2], General::CEG_DECIMALS));
			int64_t fee_limit = utils::String::Stoi64(utils::String::MultiplyDecimal(args[3], General::CEG_DECIMALS));
			int64_t price = utils::String::Stoi64(args[4]);
			
			std::string metadata, contract_input;
			if (args.size() > 5) metadata = args[5];
			if (args.size() > 6) contract_input = args[6];

			int64_t nonce = 0;
			do {
				AccountFrm::pointer account_ptr;
				if (!Environment::AccountFromDB(source_address, account_ptr)) {
					std::cout << "error " << source_address << " not exist" << std::endl;
					return;
				}
				else {
					nonce = account_ptr->GetAccountNonce() + 1;
				}
			} while (false);

			protocol::TransactionEnv tran_env;
			protocol::Transaction *tran = tran_env.mutable_transaction();
			tran->set_source_address(source_address);
			tran->set_metadata(metadata);
			tran->set_fee_limit(fee_limit);
			tran->set_gas_price(price);
			tran->set_nonce(nonce);
			protocol::Operation *ope = tran->add_operations();
			ope->set_type(protocol::Operation_Type_PAY_COIN);
			protocol::OperationPayCoin *pay_coin = ope->mutable_pay_coin();
			pay_coin->set_amount(coin_amount);
			pay_coin->set_dest_address(dest_address);
			pay_coin->set_input(contract_input);

			std::string content = tran->SerializeAsString();

			std::string sign = priv_key_->Sign(content);
			protocol::Signature *signpro = tran_env.add_signatures();
			signpro->set_sign_data(sign);
			signpro->set_public_key(priv_key_->GetEncPublicKey());

			std::string hash = utils::String::BinToHexString(HashWrapper::Crypto(content));

			Result result;
			TransactionFrm::pointer ptr = std::make_shared<TransactionFrm>(tran_env);
			GlueManager::Instance().OnTransaction(ptr, result);
			if (result.code() != 0) {
				std::cout << "error, desc(" << result.desc() << ")" << std::endl;
			}
			else {
				PeerManager::Instance().Broadcast(protocol::OVERLAY_MSGTYPE_TRANSACTION, tran_env.SerializeAsString());
				std::cout << "ok, tx hash(" << hash << ")" << std::endl;
			}
		}
		else {
			std::cout << "error, wallet not opened" << std::endl;
		}
	}

	void Console::Usage(const utils::StringVector &args) {
		printf(
			"OPTIONS:\n"
			"createWallet <path>                          create wallet\n"
			"openWallet <path>                            open a wallet\n"
			"closeWallet                                  close the current wallet\n"
			"payCoin <to-address> <CEG coin> <fee(CEG)> <gas price(baseuint)> [metatdata] [contract-input]\n"
			"                                             pay coin or invoke a contract\n"
			"getBalance [account]                         get the balance of CEG \n"
			"getBlockNumber                               get the lastest closed block number\n"
			"showKey                                      show the private key of wallet\n"
			"getState                                     get the current state of node\n"
			"exit                                         exit\n"
			);
	}
}
