/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Enhanced Miner Logging
 */

#ifndef INTCOIN_MINER_LOGGER_H
#define INTCOIN_MINER_LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace intcoin {
namespace mining {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class MinerLogger {
public:
    MinerLogger(const std::string& log_file = "", bool verbose = false)
        : log_file_(log_file), verbose_(verbose), console_enabled_(true) {
        if (!log_file_.empty()) {
            file_stream_.open(log_file_, std::ios::app);
        }
    }

    ~MinerLogger() {
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
    }

    void SetVerbose(bool verbose) { verbose_ = verbose; }
    void SetConsoleEnabled(bool enabled) { console_enabled_ = enabled; }

    void Log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Skip debug messages if not verbose
        if (level == LogLevel::DEBUG && !verbose_) {
            return;
        }

        std::string formatted = FormatMessage(level, message);

        // Console output
        if (console_enabled_) {
            std::cout << formatted << std::endl;
        }

        // File output
        if (file_stream_.is_open()) {
            file_stream_ << formatted << std::endl;
            file_stream_.flush();
        }
    }

    void Debug(const std::string& message) {
        Log(LogLevel::DEBUG, message);
    }

    void Info(const std::string& message) {
        Log(LogLevel::INFO, message);
    }

    void Warning(const std::string& message) {
        Log(LogLevel::WARNING, message);
    }

    void Error(const std::string& message) {
        Log(LogLevel::ERROR, message);
    }

    // Formatted output for mining stats
    void LogHashrate(uint64_t hashes, double hashrate, double avg_hashrate) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Hashrate: " << FormatHashrate(hashrate);
        oss << " | Avg: " << FormatHashrate(avg_hashrate);
        oss << " | Total: " << FormatLargeNumber(hashes) << " hashes";
        Info(oss.str());
    }

    void LogBlockFound(uint64_t height, const std::string& hash, uint32_t nonce) {
        std::ostringstream oss;
        oss << "*** BLOCK FOUND! ***";
        Info(oss.str());
        oss.str("");
        oss << "  Height: " << height;
        Info(oss.str());
        oss.str("");
        oss << "  Hash: " << hash.substr(0, 64);
        Info(oss.str());
        oss.str("");
        oss << "  Nonce: " << nonce;
        Info(oss.str());
    }

    void LogShareAccepted(uint64_t accepted, uint64_t rejected) {
        std::ostringstream oss;
        double accept_rate = (accepted + rejected > 0) ?
            (100.0 * accepted / (accepted + rejected)) : 0.0;
        oss << std::fixed << std::setprecision(1);
        oss << "Share accepted | Total: " << accepted << "/" << (accepted + rejected);
        oss << " (" << accept_rate << "%)";
        Info(oss.str());
    }

    void LogShareRejected(const std::string& reason, uint64_t accepted, uint64_t rejected) {
        std::ostringstream oss;
        oss << "Share rejected: " << reason;
        oss << " | Total: " << accepted << "/" << (accepted + rejected);
        Warning(oss.str());
    }

    void LogPoolConnected(const std::string& pool, const std::string& worker) {
        std::ostringstream oss;
        oss << "Connected to pool: " << pool;
        Info(oss.str());
        oss.str("");
        oss << "Worker: " << worker;
        Info(oss.str());
    }

    void LogNewJob(const std::string& job_id, uint64_t height, const std::string& target) {
        if (verbose_) {
            std::ostringstream oss;
            oss << "New job: " << job_id << " | Height: " << height;
            Debug(oss.str());
        }
    }

private:
    std::string FormatMessage(LogLevel level, const std::string& message) {
        std::ostringstream oss;

        // Timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        oss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";

        // Level
        switch (level) {
            case LogLevel::DEBUG:   oss << "[DEBUG]   "; break;
            case LogLevel::INFO:    oss << "[INFO]    "; break;
            case LogLevel::WARNING: oss << "[WARNING] "; break;
            case LogLevel::ERROR:   oss << "[ERROR]   "; break;
        }

        oss << message;
        return oss.str();
    }

    std::string FormatHashrate(double hashrate) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);

        if (hashrate >= 1e12) {
            oss << (hashrate / 1e12) << " TH/s";
        } else if (hashrate >= 1e9) {
            oss << (hashrate / 1e9) << " GH/s";
        } else if (hashrate >= 1e6) {
            oss << (hashrate / 1e6) << " MH/s";
        } else if (hashrate >= 1e3) {
            oss << (hashrate / 1e3) << " KH/s";
        } else {
            oss << hashrate << " H/s";
        }

        return oss.str();
    }

    std::string FormatLargeNumber(uint64_t num) {
        std::ostringstream oss;

        if (num >= 1e12) {
            oss << std::fixed << std::setprecision(2) << (num / 1e12) << "T";
        } else if (num >= 1e9) {
            oss << std::fixed << std::setprecision(2) << (num / 1e9) << "G";
        } else if (num >= 1e6) {
            oss << std::fixed << std::setprecision(2) << (num / 1e6) << "M";
        } else if (num >= 1e3) {
            oss << std::fixed << std::setprecision(2) << (num / 1e3) << "K";
        } else {
            oss << num;
        }

        return oss.str();
    }

    std::string log_file_;
    bool verbose_;
    bool console_enabled_;
    std::ofstream file_stream_;
    std::mutex mutex_;
};

} // namespace mining
} // namespace intcoin

#endif // INTCOIN_MINER_LOGGER_H
