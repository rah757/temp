#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <deque>
#include <numeric>
#include <algorithm>
#include <cmath>

// Structure to hold interval statistics
struct IntervalStats {
    double min;
    double max;
    double avg;
    double stddev;
    int count;
};

// Calculate statistics for a vector of values
IntervalStats calculateStats(const std::deque<double>& values) {
    IntervalStats stats;
    stats.count = values.size();
    
    if (stats.count == 0) {
        stats.min = 0;
        stats.max = 0;
        stats.avg = 0;
        stats.stddev = 0;
        return stats;
    }
    
    stats.min = *std::min_element(values.begin(), values.end());
    stats.max = *std::max_element(values.begin(), values.end());
    stats.avg = std::accumulate(values.begin(), values.end(), 0.0) / stats.count;
    
    double varSum = 0;
    for (double val : values) {
        varSum += (val - stats.avg) * (val - stats.avg);
    }
    stats.stddev = std::sqrt(varSum / stats.count);
    
    return stats;
}

int main() {
    std::cout << "Starting Advanced Performance Monitoring" << std::endl;
    
    // Subscribe to haptic and video streams
    zmq::context_t context(1);
    
    // Haptic subscriber
    zmq::socket_t hapticSub(context, zmq::socket_type::sub);
    try {
        hapticSub.connect("tcp://localhost:5556");
        hapticSub.set(zmq::sockopt::subscribe, "");
        std::cout << "Connected to haptic stream on port 5556" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect to haptic stream: " << e.what() << std::endl;
        return 1;
    }
    
    // Video subscriber
    zmq::socket_t videoSub(context, zmq::socket_type::sub);
    try {
        videoSub.connect("tcp://localhost:5566");
        videoSub.set(zmq::sockopt::subscribe, "");
        std::cout << "Connected to video stream on port 5566" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect to video stream: " << e.what() << std::endl;
        return 1;
    }
    
    // Set up poll items
    zmq::pollitem_t items[] = {
        { static_cast<void*>(hapticSub), 0, ZMQ_POLLIN, 0 },
        { static_cast<void*>(videoSub), 0, ZMQ_POLLIN, 0 }
    };
    
    // Performance metrics
    int hapticMsgCount = 0;
    int videoMsgCount = 0;
    
    // Interval tracking (sliding window of inter-arrival times)
    std::deque<double> hapticIntervals;
    std::deque<double> videoIntervals;
    const int maxIntervals = 100; // Keep track of last 100 intervals
    
    // Last message timestamps
    auto lastHapticTime = std::chrono::steady_clock::now();
    auto lastVideoTime = std::chrono::steady_clock::now();
    bool firstHapticMsg = true;
    bool firstVideoMsg = true;
    
    // Main measurement loop - run for 20 seconds
    const auto startTime = std::chrono::steady_clock::now();
    const auto endTime = startTime + std::chrono::seconds(20);
    
    std::cout << "\nStarting measurement for 20 seconds...\n";
    std::cout << std::setw(8) << "Time" 
              << std::setw(8) << "H.Rate" 
              << std::setw(8) << "V.Rate" 
              << std::setw(8) << "H.Avg" 
              << std::setw(8) << "H.Std" 
              << std::setw(8) << "V.Avg" 
              << std::setw(8) << "V.Std" 
              << std::endl;
    
    while (std::chrono::steady_clock::now() < endTime) {
        // Poll with a short timeout
        zmq::poll(items, 2, std::chrono::milliseconds(1));
        
        // Process haptic messages
        if (items[0].revents & ZMQ_POLLIN) {
            auto now = std::chrono::steady_clock::now();
            zmq::message_t msg;
            hapticSub.recv(msg);
            hapticMsgCount++;
            
            // Calculate inter-arrival time
            if (!firstHapticMsg) {
                double interval = std::chrono::duration_cast<std::chrono::microseconds>(
                    now - lastHapticTime).count() / 1000.0; // ms
                
                hapticIntervals.push_back(interval);
                if (hapticIntervals.size() > maxIntervals) {
                    hapticIntervals.pop_front();
                }
            } else {
                firstHapticMsg = false;
            }
            
            lastHapticTime = now;
        }
        
        // Process video messages
        if (items[1].revents & ZMQ_POLLIN) {
            auto now = std::chrono::steady_clock::now();
            zmq::message_t msg;
            videoSub.recv(msg);
            videoMsgCount++;
            
            // Calculate inter-arrival time
            if (!firstVideoMsg) {
                double interval = std::chrono::duration_cast<std::chrono::microseconds>(
                    now - lastVideoTime).count() / 1000.0; // ms
                
                videoIntervals.push_back(interval);
                if (videoIntervals.size() > maxIntervals) {
                    videoIntervals.pop_front();
                }
            } else {
                firstVideoMsg = false;
            }
            
            lastVideoTime = now;
        }
        
        // Print status every second
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        
        if (elapsed % 1000 < 10) { // Print approximately once per second
            double elapsedSec = elapsed / 1000.0;
            
            // Calculate current rates
            double hapticRate = hapticMsgCount / elapsedSec;
            double videoRate = videoMsgCount / elapsedSec;
            
            // Calculate interval statistics
            IntervalStats hapticStats = calculateStats(hapticIntervals);
            IntervalStats videoStats = calculateStats(videoIntervals);
            
            std::cout << std::fixed << std::setprecision(1);
            std::cout << std::setw(8) << elapsedSec 
                      << std::setw(8) << hapticRate 
                      << std::setw(8) << videoRate
                      << std::setw(8) << hapticStats.avg
                      << std::setw(8) << hapticStats.stddev
                      << std::setw(8) << videoStats.avg
                      << std::setw(8) << videoStats.stddev
                      << std::endl;
        }
        
        // Short sleep to avoid high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    const auto measuredTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTime).count();
    
    // Calculate final statistics
    IntervalStats hapticStats = calculateStats(hapticIntervals);
    IntervalStats videoStats = calculateStats(videoIntervals);
    
    // Print final summary
    std::cout << "\n========= Performance Summary =========\n";
    std::cout << "Measured over " << measuredTime << " seconds\n\n";
    
    std::cout << "Haptic Messages:\n";
    std::cout << "  Total Received: " << hapticMsgCount << " messages\n";
    std::cout << "  Rate: " << (double)hapticMsgCount / measuredTime << " msgs/sec\n";
    std::cout << "  Inter-arrival Time:\n";
    std::cout << "    Min: " << hapticStats.min << " ms\n";
    std::cout << "    Max: " << hapticStats.max << " ms\n";
    std::cout << "    Avg: " << hapticStats.avg << " ms (expected ~10 ms for 100 Hz)\n";
    std::cout << "    StdDev: " << hapticStats.stddev << " ms\n\n";
    
    std::cout << "Video Messages:\n";
    std::cout << "  Total Received: " << videoMsgCount << " messages\n";
    std::cout << "  Rate: " << (double)videoMsgCount / measuredTime << " msgs/sec\n";
    std::cout << "  Inter-arrival Time:\n";
    std::cout << "    Min: " << videoStats.min << " ms\n";
    std::cout << "    Max: " << videoStats.max << " ms\n";
    std::cout << "    Avg: " << videoStats.avg << " ms (expected ~33.3 ms for 30 Hz)\n";
    std::cout << "    StdDev: " << videoStats.stddev << " ms\n";
    
    return 0;
}
