#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>

int main() {
    std::cout << "Starting Performance Measurement" << std::endl;
    
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
    
    // Main measurement loop - run for 10 seconds
    const auto startTime = std::chrono::steady_clock::now();
    const auto endTime = startTime + std::chrono::seconds(10);
    
    std::cout << "\nStarting measurement for 10 seconds...\n";
    std::cout << std::setw(15) << "Time (s)" << std::setw(15) << "Haptic Msgs" << std::setw(15) << "Video Msgs" << std::endl;
    
    while (std::chrono::steady_clock::now() < endTime) {
        // Poll with a short timeout
        zmq::poll(items, 2, std::chrono::milliseconds(1));
        
        // Process haptic messages
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            hapticSub.recv(msg);
            hapticMsgCount++;
        }
        
        // Process video messages
        if (items[1].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            videoSub.recv(msg);
            videoMsgCount++;
        }
        
        // Print status every second
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        
        if (elapsed % 1000 < 10) { // Print approximately once per second
            double elapsedSec = elapsed / 1000.0;
            std::cout << std::fixed << std::setprecision(1);
            std::cout << std::setw(15) << elapsedSec 
                      << std::setw(15) << hapticMsgCount 
                      << std::setw(15) << videoMsgCount << std::endl;
        }
        
        // Short sleep to avoid high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    const auto measuredTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTime).count();
    
    // Print final summary
    std::cout << "\n========= Performance Summary =========\n";
    std::cout << "Measured over " << measuredTime << " seconds\n\n";
    
    std::cout << "Haptic Messages:\n";
    std::cout << "  Total Received: " << hapticMsgCount << " messages\n";
    std::cout << "  Rate: " << (double)hapticMsgCount / measuredTime << " msgs/sec\n\n";
    
    std::cout << "Video Messages:\n";
    std::cout << "  Total Received: " << videoMsgCount << " messages\n";
    std::cout << "  Rate: " << (double)videoMsgCount / measuredTime << " msgs/sec\n";
    
    return 0;
}
