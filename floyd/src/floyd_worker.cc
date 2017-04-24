#include "floyd/src/floyd_worker.h"

#include "floyd/include/floyd.h"
#include "floyd/src/logger.h"

namespace floyd {
using slash::Status;

FloydWorker::FloydWorker(const FloydWorkerEnv& env)
  : conn_factory_(env.floyd) {
    thread_ = pink::NewHolyThread(env.port,
                                  &conn_factory_, env.cron_interval, &handle_);
  }

FloydWorkerConn::FloydWorkerConn(int fd, const std::string& ip_port,
    pink::Thread* thread, Floyd* floyd)
  : PbConn(fd, ip_port, thread),
  floyd_(floyd){
  }

FloydWorkerConn::~FloydWorkerConn() {}

int FloydWorkerConn::DealMessage() {
  if (!command_.ParseFromArray(rbuf_ + 4, header_len_)) {
    LOG_DEBUG("DealMessage ParseFromArray failed");
    return -1;
  }
  command_res_.Clear();
  set_is_reply(true);

  switch (command_.type()) {
    case command::Command::Write:
    case command::Command::Delete:
    case command::Command::Read: {
      LOG_DEBUG("WorkerConn::DealMessage Write/Delete/Read");
      floyd_->DoCommand(command_, &command_res_);
      break;
    }
    case command::Command::DirtyWrite:
    case command::Command::SynRaftStage: {
      LOG_DEBUG("WorkerConn::DealMessage DirtyWrite/SyncRaftStage");
      floyd_->ExecuteDirtyCommand(command_, &command_res_);
      break;
    }
    case command::Command::RaftVote: {
      LOG_DEBUG("WorkerConn::DealMessage RaftVote");
      floyd_->DoRequestVote(command_, &command_res_);
      break;
    }
    case command::Command::RaftAppendEntries: {
      LOG_DEBUG("WorkerConn::DealMessage RaftAppendEntries");
      floyd_->DoAppendEntries(command_, &command_res_);
      break;
    }
    default:
      LOG_WARN("unknown cmd type");
      return -1;
  }

  res_ = &command_res_;
  return 0;
}

// Only connection from other node should be accepted
bool FloydWorkerHandle::AccessHandle(std::string& ip_port) const {
  LOG_DEBUG("WorkerThread::AccessHandle start check(%s)", ip_port.c_str());
  return true;
  //slash::MutexLock l(&Floyd::nodes_mutex);
  //for (auto it = Floyd::nodes_info.begin(); it != Floyd::nodes_info.end(); it++) {
  //  if (ip_port.find((*it)->ip) != std::string::npos) {
  //    LOG_DEBUG("WorkerThread::AccessHandle ok, with a node(%s:%d)",
  //              (*it)->ip.c_str(), (*it)->port);
  //    return true;
  //  }
  //}
  //LOG_DEBUG("WorkerThread::AccessHandle failed");
  //return false;
}

} // namespace floyd
