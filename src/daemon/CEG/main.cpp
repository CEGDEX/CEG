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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include<sys/mman.h>
#include <string>
#include <map>
#include <signal.h>

#define LOG(format, ...) \
	do { \
		time(&now); \
		strftime(tmp, 512, "%Y-%m-%d %H:%M:%S", localtime(&now)); \
		fprintf(fp, "[%s %s (%d)] " format "\n", \
			tmp, __FILE__, __LINE__, ##__VA_ARGS__); \
		printf("[%s %s(%d)] " format "\n",tmp, __FILE__,__LINE__,##__VA_ARGS__);\
		fflush(fp); \
	} while (false)
	
	FILE* fp;
	time_t   now;         //Instantiate the time_t structure
	char tmp[512]; 
	bool g_enable_ = false;

	bool loginit(const char* path) {
		fp = fopen(path, "at+");
		if (!fp) {
			printf("open %s failed", path);
			return false;
		}
		fseek(fp, 0, SEEK_END);
		return true;
	}
	bool log_close() {
		fclose(fp);
	}

	void SignalFunc(int32_t code) {
		LOG("Get quit signal(%d)", code);
		g_enable_ = false;
	}

	void InstallSignal() {
		signal(SIGHUP, SignalFunc);
		signal(SIGQUIT, SignalFunc);
		signal(SIGINT, SignalFunc);
		signal(SIGTERM, SignalFunc);
	}

	int main(int argc, char** argv) {
		if (argc < 2) {
			fprintf(stderr, "use: --exec your_process start --log your_log_file {default[/var/log/CEG-daemon.log]} \n");
			exit(EXIT_FAILURE);
		}

		std::string skey = "";
		std::map<std::string, std::string> commands;
		for (int i = 1; i < argc;i++)
		{
			std::string cmd(argv[i]);
			if (cmd.find("--") != std::string::npos)
			{
				skey = cmd;
			}
			else {
				if (commands[skey].empty())
					commands[skey] = cmd;
				else
					commands[skey] = commands[skey] + std::string(" ") + cmd;
			}
		}
		std::string str_exec;
		std::string str_log;

		if (commands.find("--exec") == commands.end())
		{
			fprintf(stderr, "use: --exec your_process start --log your_log_file {default[/var/log/CEG-daemon.log]} \n");
			exit(EXIT_FAILURE);
		}
		str_exec = commands["--exec"];

		if (commands.find("--log") == commands.end())
		{
			str_log = "/var/log/CEG-daemon.log";
		}
		else {
			str_log = commands["--log"];
		}
		
		
		if (!loginit(str_log.c_str())) {
			fprintf(stderr, "open log file (%s) fail", str_log.c_str());
			exit(EXIT_FAILURE);
		}
		
		LOG("the command is : [%s]", str_exec.c_str());

		do 
		{
			int fd;
			pthread_mutex_t *mptr = NULL;
			pthread_mutexattr_t mattr;
			fd = open("/dev/zero", O_RDWR, 0);
			mptr = (pthread_mutex_t*)mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			close(fd);
			pthread_mutexattr_init(&mattr);
			pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
			pthread_mutex_init(mptr, &mattr);

			void *shm = NULL;//The original initial address for shared memory allocated
			int64_t *shared;//Point to shm
			int shmid;//Identifier for shared memory
			//Create shared memory
			shmid = shmget((key_t)1234, sizeof(int64_t), 0666 | IPC_CREAT);
			if (shmid == -1) {
				LOG("shmget failed");
				break;
			}
			//Connect shared memory to the address space for the current progress
			shm = shmat(shmid, 0, 0);
			if (shm == (void*)-1) {
				break;
			}

			shared = (int64_t*)shm;
			int64_t last = 0;

			g_enable_ = true;
			InstallSignal();
			while (g_enable_)//Read data from shared memory
			{
				sleep(100);
				pthread_mutex_lock(mptr);
				LOG("last:%ld now:%ld ", last, *shared);
				if (last == *shared) {
					LOG("something wrong with process... do (%s)", str_exec.c_str());
					system(str_exec.c_str());
				}
				last = *shared;
				pthread_mutex_unlock(mptr);
			}
			//Separate shared memory from the current progress
			if (shmdt(shm) == -1) {
				LOG("shmdt failed");
				break;
			}
			//Delete shared memory
			if (shmctl(shmid, IPC_RMID, 0) == -1) {
				LOG("shmctl(IPC_RMID) failed");
				break;
			}
			log_close();
		} while (false);
		
		log_close();
		exit(EXIT_SUCCESS);
	}

