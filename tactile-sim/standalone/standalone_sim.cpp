#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Starting ZMQ subscriber..." << std::endl;
    
    // Subscribe to VM2 (haptic)
    zmq::context_t context(1);
    zmq::socket_t hapticSub(context, zmq::socket_type::sub);
    
    try {
        hapticSub.connect("tcp://localhost:5556");
        hapticSub.set(zmq::sockopt::subscribe, "");
        std::cout << "Connected to haptic stream on port 5556" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect to haptic stream: " << e.what() << std::endl;
        return 1;
    }
    
    // Subscribe to VM3 (video)
    zmq::socket_t videoSub(context, zmq::socket_type::sub);
    
    try {
        videoSub.connect("tcp://localhost:5566");
        videoSub.set(zmq::sockopt::subscribe, "");
        std::cout << "Connected to video stream on port 5566" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect to video stream: " << e.what() << std::endl;
        return 1;
    }
    
    zmq::pollitem_t items[] = {
        { static_cast<void*>(hapticSub), 0, ZMQ_POLLIN, 0 },
        { static_cast<void*>(videoSub), 0, ZMQ_POLLIN, 0 }
    };
    
    const auto startTime = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 3000; i++) { // Run for ~30 seconds
        zmq::poll(items, 2, std::chrono::milliseconds(10));
        
        // Current time in seconds since start
        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count() / 1000.0;
        
        // Check haptic socket
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            hapticSub.recv(msg);
            std::string data = msg.to_string();
            
            try {
                double timestamp = std::stod(data);
                std::cout << "Standalone [" << now << "]: Received haptic timestamp " 
                          << timestamp << ", latency = " << (now - timestamp) * 1000 
                          << " ms" << std::endl;
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
                    std::cout << "Standalone [" << now << "]: Received video timestamp " 
                              << timestamp << ", latency = " << (now - timestamp) * 1000 
                              << " ms" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing video data: " << data << " - " << e.what() << std::endl;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return 0;
}
