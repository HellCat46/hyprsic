#include "stats.hpp"
#include "disk.hpp"
#include "network.hpp"
#include <cmath>
#include <string>

#define TAG "Stats"

Stats::Stats(LoggingManager *logMgr)
    : logger(logMgr), net(logMgr), disk("/", logMgr) {
  logger = nullptr;
  rx = 0;
  tx = 0;
  diskAvail = 0;
  diskTotal = 0;

  rx = net.GetTotRx();
  tx = net.GetTotTx();
  disk.GetDiskInfo(diskAvail, diskTotal);

  time = std::chrono::steady_clock::now();
}

void Stats::UpdateData() {
  tx = net.GetTotTx();
  rx = net.GetTotRx();
  disk.GetDiskInfo(diskAvail, diskTotal);
  time = std::chrono::steady_clock::now();
}

std::string Stats::GetNetRx() {
  double trx = net.GetTotRx();
  auto curTime = std::chrono::steady_clock::now();
  auto sec =
      std::chrono::duration_cast<std::chrono::seconds>(curTime - time).count();

  return ParseBytes((trx - rx) / sec, 1);
}

std::string Stats::GetNetTx() {
  double ttx = net.GetTotTx();
  auto curTime = std::chrono::steady_clock::now();
  auto sec =
      std::chrono::duration_cast<std::chrono::seconds>(curTime - time).count();

  return ParseBytes((ttx - tx) / sec, 1);
}

std::string Stats::GetDiskAvail() {
  return ParseBytes(diskTotal - diskAvail, 2);
}

std::string Stats::GetDiskTotal() { return ParseBytes(diskTotal, 2); }

std::string Stats::ParseBytes(double bytes, int precision) {
  std::string str;
  precision++;

  if (bytes > pow(1024, 3)) { // GigaBytes
    str = std::to_string(bytes / pow(1024, 3));
    str = str.substr(0, str.find('.') + precision);
    str += " GiB";
  } else if (bytes > pow(1024, 2)) { // MegaBytes
    str = std::to_string(bytes / pow(1024, 2));
    str = str.substr(0, str.find('.') + precision);
    str += " MiB";
  } else if (bytes > 1024) { // KiloBytes
    str = std::to_string(bytes / 1024);
    str = str.substr(0, str.find('.') + precision);
    str += " KiB";
  } else if (bytes > 0) { // Bytes
    str = std::to_string(bytes);
    str = str.substr(0, str.find('.'));
    str += " B";
  } else {
    str += "0 B";
  }
  return str;
}
