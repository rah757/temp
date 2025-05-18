// Standalone version without NS-3 dependencies
#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iomanip>

// Simple simulation time tracker
class SimulationTime {
private:
    std::chrono::steady_clock::time_point startTime;
    
public:
    SimulationTime() : startTime(std::chrono::steady_clock::now()) {}
    
    double GetSeconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now - startTime).count() / 1000.0;
    }
    
    static SimulationTime& Now() {
        static SimulationTime instance;
        return instance;
    }
};

// ZMQ receiving thread
void ReceiveMessages() {
    // Set up ZMQ context and sockets
    zmq::context_t context(1);
    
    // Subscribe to VM2 (haptic)
    zmq::socket_t hapticSub(context, zmq::socket_type::sub);
    try {
        hapticSub.connect("tcp://localhost:5556");
        hapticSub.set(zmq::sockopt::subscribe, "");
        std::cout << "Connected to haptic stream on port 5556" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect to haptic stream: " << e.what() << std::endl;
        return;
    }
    
    // Subscribe to VM3 (video)
    zmq::socket_t videoSub(context, zmq::socket_type::sub);
    try {
        videoSub.connect("tcp://localhost:5566");
        videoSub.set(zmq::sockopt::subscribe, "");
        std::cout << "Connected to video stream on port 5566" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect to video stream: " << e.what() << std::endl;
        return;
    }
    
    zmq::pollitem_t items[] = {
        { static_cast<void*>(hapticSub), 0, ZMQ_POLLIN, 0 },
        { static_cast<void*>(videoSub), 0, ZMQ_POLLIN, 0 }
    };
    
    // Stats tracking
    int hapticCount = 0;
    int videoCount = 0;
    double totalHapticLatency = 0;
    double totalVideoLatency = 0;
    
    std::cout << "Starting message reception..." << std::endl;
    std::cout << std::setw(10) << "Time (s)" 
              << std::setw(10) << "Source" 
              << std::setw(15) << "Timestamp" 
              << std::setw(15) << "Latency (ms)" 
              << std::endl;
    
    // Start polling
    while (true) {
        zmq::poll(items, 2, std::chrono::milliseconds(10));
        
        // Current simulation time
        double now = SimulationTime::Now().GetSeconds();
        
        // Check haptic socket
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            hapticSub.recv(msg);
            std::string data = msg.to_string();
            
            try {
                double timestamp = std::stod(data);
                double latency = (now - timestamp) * 1000;  // ms
                
                hapticCount++;
                totalHapticLatency += latency;
                
                std::cout << std::fixed << std::setprecision(3)
                          << std::setw(10) << now 
                          << std::setw(10) << "Haptic" 
                          << std::setw(15) << timestamp 
                          << std::setw(15) << latency 
                          << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error parsing haptic data: " << data << " - " << e.what() << std::endl;
            }
        }
        
        // Check video socket
        if (items[1].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            videoSub.recv(msg);
            std::string data = msg.to_string();
            
            try {
                // Parse timestamp from "timestamp,bitrate"
                size_t commaPos = data.find(',');
                if (commaPos != std::string::npos) {
                    double timestamp = std::stod(data.substr(0, commaPos));
                    double latency = (now - timestamp) * 1000;  // ms
                    
                    videoCount++;
                    totalVideoLatency += latency;
                    
                    std::cout << std::fixed << std::setprecision(3)
                              << std::setw(10) << now 
                              << std::setw(10) << "Video" 
                              << std::setw(15) << timestamp 
                              << std::setw(15) << latency 
                              << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing video data: " << data << " - " << e.what() << std::endl;
            }
        }
        
        // Print summary every 5 seconds
        if (((int)now) % 5 == 0 && ((int)(now * 10) % 10) == 0) {  // Every 5.0 seconds exactly
            std::cout << "\n--- Summary at " << now << "s ---" << std::endl;
            std::cout << "Haptic packets: " << hapticCount 
                      << ", Avg latency: " << (hapticCount > 0 ? totalHapticLatency / hapticCount : 0) << " ms" << std::endl;
            std::cout << "Video packets: " << videoCount 
                      << ", Avg latency: " << (videoCount > 0 ? totalVideoLatency / videoCount : 0) << " ms" << std::endl;
            std::cout << "------------------------\n" << std::endl;
        }
        
        // Check if we should exit (30 second simulation)
        if (now > 30.0) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Final summary
    std::cout << "\n=== Final Summary ====" << std::endl;
    std::cout << "Haptic packets: " << hapticCount 
              << ", Avg latency: " << (hapticCount > 0 ? totalHapticLatency / hapticCount : 0) << " ms" << std::endl;
    std::cout << "Video packets: " << videoCount 
              << ", Avg latency: " << (videoCount > 0 ? totalVideoLatency / videoCount : 0) << " ms" << std::endl;
    std::cout << "====================\n" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "Starting standalone ZMQ bridge for Tactile Internet..." << std::endl;
    
    // Run the message receiving function directly
    ReceiveMessages();
    
    std::cout << "Simulation complete" << std::endl;
    return 0;
}
